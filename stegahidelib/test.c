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
    size_t hiddenRawDataSize = 1 * KB;
    size_t hiddenMaskedDataSize;
    uint8_t *fileData = NULL;
    uint8_t *hiddenData = NULL;
    uint8_t *hiddenMaskedData = NULL;

    size_t hiddenDataSizeLocation;
    char *hiddenDataSizeBits = NULL;
    char *byteBits = NULL;

    uint8_t mask = 0x18;

    printf("Running test for stegohide-lib\n");


    set_hide_verbose_level(3);
    set_extract_verbose_level(3);

    printf("Allocating rawData.\n\tFile:   %ld bytes\n\tHidden: %ld bytes\n", fileDataSize, hiddenRawDataSize);
    fileData = malloc(fileDataSize);
    hiddenData = malloc(hiddenRawDataSize);
    hiddenDataSizeBits = malloc(BITS_SIZE_T + 1);
    byteBits = malloc(8 + 1);

    if (!fileData || !hiddenData) {
        printf("Error allocating rawData. Consider decreasing rawData size\n");
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

    for (size_t i = 0; i < hiddenRawDataSize; i++) {
        hiddenData[i] = rand();
    }

    embed_hidden_data_size(fileData, &hiddenDataSizeLocation, fileDataSize, hiddenRawDataSize);

    get_hidden_data_masked_size(mask, hiddenRawDataSize, &hiddenMaskedDataSize);

    hiddenMaskedData = malloc(hiddenMaskedDataSize);
    if (!hiddenMaskedData) {
        printf("Error allocating hiddenMaskedData\n");
        goto teardown;
    }

    generate_masked_hidden_data(hiddenData, hiddenRawDataSize, hiddenMaskedData, hiddenMaskedDataSize, mask);

    hide_data(fileData, fileDataSize, hiddenMaskedData, hiddenMaskedDataSize, mask, hiddenDataSizeLocation);

    // Save file here...

    size_t extractedHiddenDataSizeLocation = extract_hidden_data_size_location(fileData, fileDataSize);
    size_t extractedHiddenDataSize = extract_hidden_data_size(fileData, fileDataSize, extractedHiddenDataSizeLocation);

    printf("Extracted hidden data size location: %ld, expected: %ld\n", extractedHiddenDataSizeLocation, hiddenDataSizeLocation);
    printf("Extracted hidden data size: %ld, expected: %ld\n", extractedHiddenDataSize, hiddenRawDataSize);

teardown:
    if (fileData)
        free(fileData);

    if (hiddenData)
        free(hiddenData);

    if (hiddenDataSizeBits)
        free(hiddenDataSizeBits);

    if (hiddenMaskedData)
        free(hiddenMaskedData);

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
