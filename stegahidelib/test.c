#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "extracting.c"
#include "hiding.c"

void get_size_in_bits(char *sizeBits, uint64_t size);
void sprintf_bits(char *bits, uint64_t num, uint64_t numBits);

#define KB (1024)
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

int main() {
    uint64_t fileDataSize = 128 * MB; // Example size, adjust as needed
    uint64_t hiddenRawDataSize = 1 * KB;
    uint64_t hiddenMaskedDataSize;
    uint8_t *fileData = NULL;
    uint8_t *hiddenData = NULL;
    uint8_t *hiddenMaskedData = NULL;

    uint64_t hiddenDataSizeLocation;
    char *hiddenDataSizeBits = NULL;
    char *byteBits = NULL;

    uint8_t mask = 0x18;

    printf("Running test for stegohide-lib\n");


    set_hide_verbose_level(3);
    set_extract_verbose_level(3);

    printf("Allocating rawData.\n\tFile:   %ld bytes\n\tHidden: %ld bytes\n", fileDataSize, hiddenRawDataSize);
    fileData = malloc(fileDataSize);
    hiddenData = malloc(hiddenRawDataSize);
    hiddenDataSizeBits = malloc(BITS_uint64_t + 1);
    byteBits = malloc(8 + 1);

    if (!fileData || !hiddenData) {
        printf("Error allocating rawData. Consider decreasing rawData size\n");
        printf("fileData           = 0x%08lX\n", (uint64_t)fileData);
        printf("hiddenData         = 0x%08lX\n", (uint64_t)hiddenData);
        printf("hiddenDataSizeBits = 0x%08lX\n", (uint64_t)hiddenDataSizeBits);
        printf("byteBits           = 0x%08lX\n", (uint64_t)byteBits);
        goto teardown;
    }

    hiddenDataSizeBits[BITS_uint64_t] = 0;
    byteBits[8] = 0;

    srand(time(NULL));

    for (uint64_t i = 0; i < fileDataSize; i++) {
        fileData[i] = rand();
    }

    for (uint64_t i = 0; i < hiddenRawDataSize; i++) {
        hiddenData[i] = rand();
    }

    embed_hidden_data_size(fileData, &hiddenDataSizeLocation, fileDataSize, hiddenRawDataSize);

    hide_mask(fileData, mask);

    get_hidden_data_masked_size(mask, hiddenRawDataSize, &hiddenMaskedDataSize);

    hiddenMaskedData = malloc(hiddenMaskedDataSize);
    if (!hiddenMaskedData) {
        printf("Error allocating hiddenMaskedData\n");
        goto teardown;
    }

    generate_masked_hidden_data(hiddenData, hiddenRawDataSize, hiddenMaskedData, hiddenMaskedDataSize, mask);

    hide_data(fileData, fileDataSize, hiddenMaskedData, hiddenMaskedDataSize, mask, hiddenDataSizeLocation);

    // Save file here...

    uint64_t extractedHiddenDataSizeLocation = extract_hidden_data_size_location(fileData, fileDataSize);
    uint64_t extractedHiddenDataSize = extract_hidden_data_size(fileData, fileDataSize, extractedHiddenDataSizeLocation);
    uint8_t extractedMask = extract_mask(fileData, fileDataSize);

    printf("Extracted hidden data size location: %ld, expected: %ld\n", extractedHiddenDataSizeLocation, hiddenDataSizeLocation);
    printf("Extracted hidden data size: %ld, expected: %ld\n", extractedHiddenDataSize, hiddenRawDataSize);
    printf("Extracted mask: 0x%02X, expected: 0x%02X\n", extractedMask, mask);

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


void get_size_in_bits(char *sizeBits, uint64_t size) {
    sprintf_bits(sizeBits, size, BITS_uint64_t);
}

void sprintf_bits(char *bits, uint64_t num, uint64_t numBits) {
    for (uint64_t i = 0; i < numBits; i++) {
        bits[numBits - i - 1] = ((num >> i) & 0x01) ? '1' : '0';
    }
}
