#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stegahide.h"

#define BITS_SIZE_T (sizeof(size_t) * 8)

#define ENABLE_DEBUG 1



// Embedding data:
static size_t find_hidden_data_size_position(uint8_t *data, size_t dataSize, uint8_t hiddenDataSize[BITS_SIZE_T]);
static size_t get_correct_data_bits_in_sequence(uint8_t data[BITS_SIZE_T], uint8_t hiddenDataSize[BITS_SIZE_T]);


size_t embed_hidden_data_size(uint8_t *data, size_t dataSize, size_t hiddenDataSize) {
    if (data == NULL || dataSize < BITS_SIZE_T || hiddenDataSize == 0) {
        return STEGAHIDE_INVALID_DATA;
    }

    uint8_t hiddenDataSizeBytes[BITS_SIZE_T];

    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        hiddenDataSizeBytes[i] = (hiddenDataSize >> i) & 0x01; // Extract each bit
    }

    size_t position = find_hidden_data_size_position(data, dataSize, hiddenDataSizeBytes);
    if (position + BITS_SIZE_T > dataSize) {
        return STEGAHIDE_SIZE_EMBED_ERROR;
    }

    if (ENABLE_DEBUG) {
        printf("Found best hidden data size position: %ld\n", position);
    }

    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        data[position + i] = (data[position + i] & ~0x01) | (hiddenDataSizeBytes[i] & 0x01);
        data[i] = (data[i] & ~0x01) | ((position >> i) & 0x01);
    }



    return position;
}


static size_t find_hidden_data_size_position(uint8_t *data, size_t dataSize, uint8_t hiddenDataSize[BITS_SIZE_T]) {
    size_t currentHiddenDataSizePosition = BITS_SIZE_T; // cannot start at 0
    size_t currentHiddenDataSizeBits = 0;

    for (size_t i = BITS_SIZE_T; i < dataSize - BITS_SIZE_T; i++) {
        size_t correctBits = get_correct_data_bits_in_sequence(&data[i], hiddenDataSize);
        if (correctBits == BITS_SIZE_T) {
            if (ENABLE_DEBUG) {
                printf("New Correct bits position: %ld\n", i);
                printf("New Correct bits:          %ld\n", correctBits);
            }
            return i;
        } else if (correctBits > currentHiddenDataSizeBits) {
            currentHiddenDataSizeBits = correctBits;
            currentHiddenDataSizePosition = i;
            if (ENABLE_DEBUG) {
                printf("New Correct bits position: %ld\n", currentHiddenDataSizePosition);
                printf("New Correct bits:          %ld\n", correctBits);
            }
        }
    }
    return currentHiddenDataSizePosition;
}


static size_t get_correct_data_bits_in_sequence(uint8_t data[BITS_SIZE_T], uint8_t hiddenDataSize[BITS_SIZE_T]) {
    size_t correctBits = 0;
    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        if ((data[i] & 0x01) == (hiddenDataSize[i] & 0x01)) {
            correctBits++;
        }
    }
    return correctBits;
}


// Extracting data:
