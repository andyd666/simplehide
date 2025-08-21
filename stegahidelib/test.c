#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "stegahide.c"

void get_size_in_bits(char *sizeBits, size_t size);
void sprintf_bits(char *bits, size_t num, size_t numBits);

#define KB (1024)
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

int main() {
    size_t fileDataSize = 128 * MB; // Example size, adjust as needed
    size_t hiddenDataSize = 1 * KB;
    uint8_t *fileData = NULL;
    uint8_t *hiddenData = NULL;

    size_t hiddenDataSizePosition;
    char *hiddenDataSizeBits = NULL;
    char *byteBits = NULL;

    printf("Running test for stegohide-lib\n");

    printf("Allocating data.\n\tFile:   %ld bytes\n\tHidden: %ld bytes\n", fileDataSize, hiddenDataSize);
    fileData = malloc(fileDataSize);
    hiddenData = malloc(hiddenDataSize);
    hiddenDataSizeBits = malloc(BITS_SIZE_T + 1);
    byteBits = malloc(8 + 1);

    if (!fileData || !hiddenData) {
        printf("Error allocating data. Consider decreasing data size\n");
        printf("fileData           = 0x%08lX\n", (size_t)fileData);
        printf("hiddenData         = 0x%08lX\n", (size_t)hiddenData);
        printf("hiddenDataSizeBits = 0x%08lX\n", (size_t)hiddenDataSizeBits);
        printf("byteBits           = 0x%08lX\n", (size_t)byteBits);
        goto teardown;
    }

    hiddenDataSizeBits[BITS_SIZE_T] = 0;
    byteBits[8] = 0;

    srand(time(NULL));

    for (size_t i = 0; i < fileDataSize; i++) {
        fileData[i] = rand();
    }

    for (size_t i = 0; i < hiddenDataSize; i++) {
        hiddenData[i] = rand();
    }

    hiddenDataSizePosition = embed_hidden_data_size(fileData, fileDataSize, hiddenDataSize);
    printf("Expected embedded size position:      %ld\n", hiddenDataSizePosition);

    get_size_in_bits(hiddenDataSizeBits, hiddenDataSizePosition);
    printf("Expected embedded size location bits: %s\n", hiddenDataSizeBits);

    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        sprintf_bits(byteBits, fileData[i], 8);
        printf("pos [%2ld] [%10ld] =\t0x%02x\t%s\n",
                i,
                i,
                fileData[i],
                byteBits);
    }

    get_size_in_bits(hiddenDataSizeBits, hiddenDataSize);
    printf("Expected embedded size bits:          %s\n", hiddenDataSizeBits);
    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        sprintf_bits(byteBits, fileData[hiddenDataSizePosition + i], 8);
        printf("pos [%2ld] [%10ld] =\t0x%02x\t%s\n",
                i,
                hiddenDataSizePosition + i,
                fileData[hiddenDataSizePosition + i],
                byteBits);
    }

teardown:
    if (fileData)
        free(fileData);

    if (hiddenData)
        free(hiddenData);

    if (hiddenDataSizeBits)
        free(hiddenDataSizeBits);

    printf("Test finished\n");
    return 0;
}


void get_size_in_bits(char *sizeBits, size_t size) {
    sprintf_bits(sizeBits, size, BITS_SIZE_T);
}

void sprintf_bits(char *bits, size_t num, size_t numBits) {
    for (size_t i = 0; i < numBits; i++) {
        bits[numBits - i - 1] = ((num >> i) & 0x01) ? '1' : '0';
    }
}
