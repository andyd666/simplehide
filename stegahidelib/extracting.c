// TODO: add license

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stegahide.h"


static int extractVerboseLevel = 0;
void set_extract_verbose_level(int level) {
    extractVerboseLevel = level;
}


uint64_t extract_hidden_data_size_location(uint8_t *rawData, uint64_t rawDataSize) {
    uint64_t hiddenDataSizeLocation = 0;

    if (!rawData || rawDataSize < BITS_uint64_t) {
        printf("rawData is NULL or too small\n");
        return 0;
    }

    for (uint64_t i = 0; i < BITS_uint64_t - 8; i++) {
        hiddenDataSizeLocation |= (rawData[i] & 0x01) << i;
        if (extractVerboseLevel >= 3) {
            printf("Raw data [%ld]: 0x%02x (%d)\n", i, rawData[i], (rawData[i] & 0x01));
        }
    }

    if (extractVerboseLevel >= 1) {
        printf("Extracted hidden data size location: %ld\n", hiddenDataSizeLocation);
    }

    return hiddenDataSizeLocation;
}

uint8_t extract_mask(uint8_t *rawData, uint64_t rawDataSize) {
    uint8_t mask = 0;
    if (!rawData || rawDataSize < BITS_uint64_t) {
        printf("rawData is NULL or too small\n");
        return 0;
    }

    for (uint64_t i = 0; i < 8; i++) {
        mask |= (rawData[BITS_uint64_t - 8 + i] & 0x01) << i;
    }

    if (extractVerboseLevel >= 1) {
        printf("Extracted mask: 0x%02X\n", mask);
    }

    return mask;
}


uint64_t extract_hidden_data_size(uint8_t *rawData, uint64_t rawDataSize, uint64_t hiddenDataSizeLocation) {
    uint64_t hiddenDataSize = 0;

    if (!rawData || rawDataSize < BITS_uint64_t) {
        printf("rawData is NULL or too small\n");
        return 0;
    }

    if (extractVerboseLevel >= 2) {
        printf("Exctracting data size\n");
    }

    if ((hiddenDataSizeLocation > rawDataSize) || (hiddenDataSizeLocation < BITS_uint64_t)) {
        printf("Error: hiddenDataSizeLocation %ld is out of bounds (rawDataSize %ld)\n", hiddenDataSizeLocation, rawDataSize);
        return 0;
    }

    for (uint64_t i = 0; i < BITS_uint64_t; i++) { // Skip last byte, as there can be mask
        hiddenDataSize |= (rawData[hiddenDataSizeLocation + i] & 0x01) << i;
        if (extractVerboseLevel >= 3) {
            printf("Raw data [%ld]: 0x%02x (%d)\n", hiddenDataSizeLocation + i, rawData[hiddenDataSizeLocation + i], (rawData[hiddenDataSizeLocation + i] & 0x01));
        }
    }

    if (extractVerboseLevel >= 1) {
        printf("Extracted hidden data size: %ld\n", hiddenDataSize);
    }

    if (hiddenDataSize > rawDataSize - BITS_uint64_t * 2) {
        printf("Error, hidden data size is out of bounds: %ld > %ld\n", hiddenDataSize, rawDataSize - BITS_uint64_t * 2);
        return 0;
    }

    return hiddenDataSize;
}
