#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stegahide.h"

#define BITS_SIZE_T (sizeof(size_t) * 8)

static int embeddingVerboseLevel = 0;
void set_embedding_verbose_level(int level) {
    embeddingVerboseLevel = level;
}


// Embedding data:
static size_t find_hidden_data_size_position(uint8_t *data, size_t dataSize, uint8_t hiddenDataSize[BITS_SIZE_T]);
static size_t get_correct_data_bits_in_sequence(uint8_t data[BITS_SIZE_T], uint8_t hiddenDataSize[BITS_SIZE_T]);


StegahideStatus embed_hidden_data_size(uint8_t *data, size_t *sizePosition, size_t dataSize, size_t hiddenDataSize) {
    uint8_t hiddenDataSizeBytes[BITS_SIZE_T];
    uint8_t verboseDataSizeBytesBefore[BITS_SIZE_T];
    uint8_t verboseDataSizeBytesAfter[BITS_SIZE_T];
    uint8_t verboseDataPositionBytesBefore[BITS_SIZE_T];
    uint8_t verboseDataPositionBytesAfter[BITS_SIZE_T];

    if (data == NULL || dataSize < BITS_SIZE_T || hiddenDataSize == 0) {
        return STEGAHIDE_INVALID_DATA;
    }

    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        hiddenDataSizeBytes[i] = (hiddenDataSize >> i) & 0x01;
    }

    if (embeddingVerboseLevel >= 2) {
        printf("Embedding hidden data size bits: ");
        for (size_t i = BITS_SIZE_T - 1; i < BITS_SIZE_T; i--) {
            printf("%d", hiddenDataSizeBytes[i]);
        }
        printf("\n");
    }

    *sizePosition = find_hidden_data_size_position(data, dataSize, hiddenDataSizeBytes);
    if (*sizePosition + BITS_SIZE_T > dataSize) {
        printf("Data size %ld, sizePosition %ld is out of bounds\n", dataSize, *sizePosition);
        return STEGAHIDE_SIZE_EMBED_ERROR;
    }

    if (embeddingVerboseLevel >= 1) {
        printf("Found best hidden data size sizePosition: %ld\n", *sizePosition);
    }

    if (embeddingVerboseLevel >= 3) {
        memcpy(verboseDataSizeBytesBefore, &data[*sizePosition], BITS_SIZE_T);
        memcpy(verboseDataPositionBytesBefore, &data[0], BITS_SIZE_T);
    }

    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        data[*sizePosition + i] = (data[*sizePosition + i] & ~0x01) | (hiddenDataSizeBytes[i] & 0x01);
        data[i] = (data[i] & ~0x01) | ((*sizePosition >> i) & 0x01);
    }

    if (embeddingVerboseLevel >= 3) {
        printf("Size embedding result:\n");
        memcpy(verboseDataSizeBytesAfter, &data[*sizePosition], BITS_SIZE_T);
        memcpy(verboseDataPositionBytesAfter, &data[0], BITS_SIZE_T);
        for (size_t i = 0; i < BITS_SIZE_T; i++) {
            printf("[%2ld]: 0x%02x -> 0x%02x (%d)            [%10ld]: 0x%02x -> 0x%02x (%d)\n",
                    i,
                    verboseDataSizeBytesBefore[i],
                    verboseDataSizeBytesAfter[i],
                    hiddenDataSizeBytes[i],
                    *sizePosition + i,
                    verboseDataPositionBytesBefore[i],
                    verboseDataPositionBytesAfter[i],
                    (uint8_t)((*sizePosition >> i) & 0x01));
        }
    }

    return STEGAHIDE_SUCCESS;
}


static size_t find_hidden_data_size_position(uint8_t *data, size_t dataSize, uint8_t hiddenDataSize[BITS_SIZE_T]) {
    size_t currentHiddenDataSizePosition = BITS_SIZE_T; // cannot start at 0
    size_t currentHiddenDataSizeBits = 0;

    for (size_t i = BITS_SIZE_T; i < dataSize - BITS_SIZE_T; i++) {
        size_t correctBits = get_correct_data_bits_in_sequence(&data[i], hiddenDataSize);
        if (correctBits == BITS_SIZE_T) {
            if (embeddingVerboseLevel >= 3) {
                printf("New Correct bits sizePosition: %ld\n", i);
                printf("New Correct bits:          %ld\n", correctBits);
            }
            return i;
        } else if (correctBits >= currentHiddenDataSizeBits) { // Take the furthest best match
            currentHiddenDataSizeBits = correctBits;
            currentHiddenDataSizePosition = i;
            if (embeddingVerboseLevel >= 3) {
                printf("New Correct bits sizePosition: %ld\n", currentHiddenDataSizePosition);
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
