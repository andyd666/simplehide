/*
* simplehide  Copyright (C) 2025  andyd666
* This program comes with ABSOLUTELY NO WARRANTY; for details type `--license-warranty'.
* This is free software, and you are welcome to redistribute it
* under certain conditions; type `--license-conditions' for details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "../src/extracting.cpp"
#include "../src/hiding.cpp"

void get_size_in_bits(char *sizeBits, size_t size);
void sprintf_bits(char *bits, size_t num, size_t numBits);

#define B  (1)
#define KB (1024 * B)
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)


void parse_argv(int argc, char **argv, int *threads, int *verboseLevel);


int main(int argc, char **argv) {
    size_t fileDataSize = 1 * MB; // Example size, adjust as needed
    size_t hiddenRawDataSize = 128 * KB;
    size_t hiddenMaskedDataSize;
    uint8_t *fileData = NULL;
    uint8_t *hiddenData = NULL;
    uint8_t *hiddenMaskedData = NULL;
    uint8_t *extractedHiddenMaskedData = NULL;

    size_t hiddenDataSizePosition;
    char *hiddenDataSizeBits = NULL;
    char *byteBits = NULL;

    uint8_t mask = 0x1B;

    int threads = 1;
    int verboseLevel = 0;

    parse_argv(argc, argv, &threads, &verboseLevel);

    printf("Running test for simplehide-lib\n");


    set_hide_verbose_level(verboseLevel);
    set_extract_verbose_level(verboseLevel);

    set_hide_thread_number(threads);
    set_extract_thread_number(threads);


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

    embed_hidden_data_size(fileData, &hiddenDataSizePosition, fileDataSize, hiddenRawDataSize);

    hide_mask(fileData, mask);

    get_hidden_data_masked_size(mask, hiddenRawDataSize, &hiddenMaskedDataSize);

    hiddenMaskedData = malloc(hiddenMaskedDataSize);
    if (!hiddenMaskedData) {
        printf("Error allocating hiddenMaskedData\n");
        goto teardown;
    }

    generate_masked_hidden_data(hiddenData, hiddenRawDataSize, hiddenMaskedData, hiddenMaskedDataSize, mask);

    hide_data(fileData, fileDataSize, hiddenMaskedData, hiddenMaskedDataSize, mask, hiddenDataSizePosition);

    // Save file here...

    size_t extractedHiddendataSizePosition = extract_hidden_data_size_position(fileData, fileDataSize);
    size_t extractedHiddenDataSize = extract_hidden_data_size(fileData, fileDataSize, extractedHiddendataSizePosition);
    uint8_t extractedMask = extract_mask(fileData, fileDataSize);
    size_t extractedHiddenMaskedDataSize;
    verify_mask(mask, hiddenRawDataSize, &extractedHiddenMaskedDataSize);

    printf("Extracted hidden data size position: %ld, expected: %ld\n", extractedHiddendataSizePosition, hiddenDataSizePosition);
    printf("Extracted hidden data size: %ld, expected: %ld\n", extractedHiddenDataSize, hiddenRawDataSize);
    printf("Extracted mask: 0x%02X, expected: 0x%02X\n", extractedMask, mask);


    extractedHiddenMaskedData = malloc(extractedHiddenMaskedDataSize);

    if (!extractedHiddenMaskedData) {
        printf("Error allocating extractedHiddenMaskedData\n");
        goto teardown;
    }

    if (extract_hidden_data(fileData, fileDataSize, extractedHiddenMaskedData, hiddenRawDataSize, extractedHiddenMaskedDataSize, hiddenDataSizePosition, mask) != SIMPLEHIDE_SUCCESS) {
        printf("Error extracting hidden data\n");
    }

    int mismatchBytes = 0;
    for (size_t i = 0; i < hiddenRawDataSize; i++) {
        if (hiddenData[i] != extractedHiddenMaskedData[i]) {
            printf("Mismatch at byte %ld: expected 0x%02X, got 0x%02X\n", i, hiddenData[i], extractedHiddenMaskedData[i]);
            mismatchBytes++;
        }
    }

    if (mismatchBytes == 0) {
        printf("Test successful, all %ld bytes match\n", hiddenRawDataSize);
    } else {
        printf("Test failed, %d / %ld bytes mismatched\n", mismatchBytes, hiddenRawDataSize);
    }

teardown:
    if (fileData)
        free(fileData);

    if (hiddenData)
        free(hiddenData);

    if (hiddenDataSizeBits)
        free(hiddenDataSizeBits);

    if (hiddenMaskedData)
        free(hiddenMaskedData);

    if (extractedHiddenMaskedData)
        free(extractedHiddenMaskedData);

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


void parse_argv(int argc, char **argv, int *threads, int *verboseLevel) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-j") == 0) {
            i++;
            if (i < argc) {
                *threads = atoi(argv[i]);
            } else {
                *threads = 1;
            }
        } else if (strcmp(argv[i], "-V") == 0) {
            i++;
            if (i < argc) {
                *verboseLevel = atoi(argv[i]);
            } else {
                *verboseLevel = 0;
            }
        }
    }
}
