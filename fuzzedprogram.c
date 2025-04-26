#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char type;
    unsigned int len;
    unsigned char val[0];
} __attribute__((packed)) ldata;

// Modified process_data function
void process_data(unsigned char *data, unsigned int size) {
    unsigned char stackBuffer[64];  // [NEW] Added small stack buffer
    unsigned int numToRead = 0;      // [NEW] Number of bytes to copy if type == 0xAA

    unsigned char *curr = data;
    unsigned char *end = data + size;

    while (curr < end) {
        ldata *ld = (ldata *)curr;

        if ((curr + sizeof(ldata)) > end) {
            break;
        }

        // [COMMENTED OUT - original code]
        /*
        printf("Type: %02x Length: %u\n", ld->type, ld->len);
        for (unsigned int i = 0; i < ld->len && (curr + sizeof(ldata) + i) < end; i++) {
            printf("%02x ", ld->val[i]);
        }
        printf("\n");
        */

        // [NEW] Added special processing for types
        if (ld->type == 0xAA && ld->len == 4) {
            // Little Endian reading (like VLC's GetDWBE but flipped)
            numToRead = (ld->val[0]) | (ld->val[1] << 8) | (ld->val[2] << 16) | (ld->val[3] << 24);
        }

        if (ld->type == 0xBB && numToRead != 0) {
            // [NEW] Vulnerable memcpy into small stack buffer
            memcpy(stackBuffer, ld->val, numToRead);
        }

        curr += sizeof(ldata) + ld->len;
    }
}

int main(int argc, char **argv) {
    FILE *fp;
    unsigned char *data;
    unsigned int size;

    if (argc != 2) {
        printf("Usage: %s <inputfile>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    data = malloc(size);
    if (!data) {
        fclose(fp);
        printf("malloc failed\n");
        return 1;
    }

    fread(data, 1, size, fp);
    fclose(fp);

    process_data(data, size);

    free(data);
    return 0;
}
