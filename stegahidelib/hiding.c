// TODO: add license

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stegahide.h"


static int hideVerboseLevel = 0;
void set_hide_verbose_level(int level) {
    hideVerboseLevel = level;
}


static uint64_t find_hidden_data_size_position(uint8_t *rawData, uint64_t rawDataSize, uint8_t hiddenRawDataSize[BITS_uint64_t]);
static uint64_t get_correct_data_bits_in_sequence(uint8_t rawData[BITS_uint64_t], uint8_t hiddenRawDataSize[BITS_uint64_t]);

static int get_bits_in_mask(uint8_t mask);
static StegahideMaskType get_mask_type(uint8_t mask);
static uint8_t complex_remask_byte(uint8_t inputByte, uint8_t mask);


StegahideStatus embed_hidden_data_size(uint8_t *rawData, uint64_t *sizePosition, uint64_t rawDataSize, uint64_t hiddenRawDataSize) {
    uint8_t hiddenDataSizeBytes[BITS_uint64_t];
    uint8_t verboseDataSizeBytesBefore[BITS_uint64_t];
    uint8_t verboseDataSizeBytesAfter[BITS_uint64_t];
    uint8_t verboseDataPositionBytesBefore[BITS_uint64_t];
    uint8_t verboseDataPositionBytesAfter[BITS_uint64_t];

    if (rawData == NULL || rawDataSize < BITS_uint64_t || hiddenRawDataSize == 0) {
        return STEGAHIDE_INVALID_DATA;
    }

    for (uint64_t i = 0; i < BITS_uint64_t; i++) {
        hiddenDataSizeBytes[i] = (hiddenRawDataSize >> i) & 0x01;
    }

    if (hideVerboseLevel >= 2) {
        printf("Embedding hidden rawData size bits: ");
        for (uint64_t i = BITS_uint64_t - 1; i < BITS_uint64_t; i--) {
            printf("%d", hiddenDataSizeBytes[i]);
        }
        printf("\n");
    }

    *sizePosition = find_hidden_data_size_position(rawData, rawDataSize, hiddenDataSizeBytes);
    if (*sizePosition + BITS_uint64_t > rawDataSize) {
        printf("Data size %ld, sizePosition %ld is out of bounds\n", rawDataSize, *sizePosition);
        return STEGAHIDE_SIZE_EMBED_ERROR;
    }

    if (hideVerboseLevel >= 1) {
        printf("Found best hidden rawData size sizePosition: %ld\n", *sizePosition);
    }

    if (hideVerboseLevel >= 3) {
        memcpy(verboseDataSizeBytesBefore, &rawData[*sizePosition], BITS_uint64_t);
        memcpy(verboseDataPositionBytesBefore, &rawData[0], BITS_uint64_t);
    }

    for (uint64_t i = 0; i < BITS_uint64_t; i++) {
        rawData[*sizePosition + i] = (rawData[*sizePosition + i] & ~0x01) | (hiddenDataSizeBytes[i] & 0x01);
        rawData[i] = (rawData[i] & ~0x01) | ((*sizePosition >> i) & 0x01);
    }

    if (hideVerboseLevel >= 3) {
        printf("Size embedding result:\n");
        memcpy(verboseDataSizeBytesAfter, &rawData[*sizePosition], BITS_uint64_t);
        memcpy(verboseDataPositionBytesAfter, &rawData[0], BITS_uint64_t);
        for (uint64_t i = 0; i < BITS_uint64_t; i++) {
            printf("[%10ld]: 0x%02x -> 0x%02x (%d)            [%2ld]: 0x%02x -> 0x%02x (%d)\n",
                    *sizePosition + i,
                    verboseDataSizeBytesBefore[i],
                    verboseDataSizeBytesAfter[i],
                    hiddenDataSizeBytes[i],
                    i,
                    verboseDataPositionBytesBefore[i],
                    verboseDataPositionBytesAfter[i],
                    (uint8_t)((*sizePosition >> i) & 0x01));
        }
    }

    return STEGAHIDE_SUCCESS;
}


static uint64_t find_hidden_data_size_position(uint8_t *rawData, uint64_t rawDataSize, uint8_t hiddenRawDataSize[BITS_uint64_t]) {
    uint64_t currentHiddenDataSizePosition = BITS_uint64_t; // cannot start at 0
    uint64_t currentHiddenDataSizeBits = 0;

    // TODO: use pthread to increase search speed
    for (uint64_t i = BITS_uint64_t; i < rawDataSize - BITS_uint64_t; i++) {
        uint64_t correctBits = get_correct_data_bits_in_sequence(&rawData[i], hiddenRawDataSize);
        if (correctBits == BITS_uint64_t) {
            if (hideVerboseLevel >= 3) {
                printf("New Correct bits sizePosition: %ld\n", i);
                printf("New Correct bits:          %ld\n", correctBits);
            }
            return i;
        } else if (correctBits >= currentHiddenDataSizeBits) { // Take the furthest best match
            currentHiddenDataSizeBits = correctBits;
            currentHiddenDataSizePosition = i;
            if (hideVerboseLevel >= 3) {
                printf("New Correct bits sizePosition: %ld\n", currentHiddenDataSizePosition);
                printf("New Correct bits:          %ld\n", correctBits);
            }
        }
    }
    return currentHiddenDataSizePosition;
}


static uint64_t get_correct_data_bits_in_sequence(uint8_t rawData[BITS_uint64_t], uint8_t hiddenRawDataSize[BITS_uint64_t]) {
    uint64_t correctBits = 0;
    for (uint64_t i = 0; i < BITS_uint64_t; i++) {
        if ((rawData[i] & 0x01) == (hiddenRawDataSize[i] & 0x01)) {
            correctBits++;
        }
    }
    return correctBits;
}


StegahideStatus hide_mask(uint8_t *rawData, uint8_t mask) {
    if (!rawData) {
        printf("Error: rawData is NULL\n");
        return STEGAHIDE_INVALID_DATA;
    }

    for (uint64_t i = 0; i < 8; i++) {
        rawData[i + BITS_uint64_t - 8] = (rawData[i + BITS_uint64_t - 8] & ~0x01) | ((mask >> i) & 0x01);
    }

    return STEGAHIDE_SUCCESS;
}


StegahideStatus get_hidden_data_masked_size(uint8_t mask, uint64_t hiddenRawDataSize, uint64_t *maskedHiddenDataSize) {
    int bitsInMask;

    if (mask == 0) {
        printf("Error: Mask not set\n");
        return STEGAHIDE_INVALID_MASK;
    }

    bitsInMask = get_bits_in_mask(mask);

    // TODO: make function for mask verification
    switch (bitsInMask) {
    case 1:
        *maskedHiddenDataSize = hiddenRawDataSize * 8;
        break;
    case 2:
        *maskedHiddenDataSize = hiddenRawDataSize * 4;
        break;
    case 4:
        *maskedHiddenDataSize = hiddenRawDataSize * 2;
        break;
    case 8:
        *maskedHiddenDataSize = hiddenRawDataSize;
        printf("Warning: Mask with 8 bits set is not recommended\n");
        break;
    default:
        *maskedHiddenDataSize = 0;
        printf("Error: Mask must have power of 2 bits set (1, 2, 4, 8(not recommended))\n");
        return STEGAHIDE_INVALID_MASK;
    }

    if (hideVerboseLevel >= 1) {
        printf("Masked raw data size:    %ld\n", hiddenRawDataSize);
        printf("Masked hidden data size: %ld\n", *maskedHiddenDataSize);
    }

    return STEGAHIDE_SUCCESS;
}

static int get_bits_in_mask(uint8_t mask) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            count++;
        }
    }

    if (hideVerboseLevel >= 1) {
        printf("Bits in mask: %d\n", count);
    }

    return count;
}


StegahideStatus generate_masked_hidden_data(uint8_t *hiddenRawData, uint64_t hiddenRawDataSize, uint8_t *hiddenMaskedData, uint64_t hiddenMaskedDataSize, uint8_t mask) {
    StegahideMaskType maskType;
    int shiftSize = 1;
    uint64_t split;

    if (!hiddenRawData || !hiddenMaskedData || !get_bits_in_mask(mask)) {
        printf("Error: Invalid input data\n");
        if (hideVerboseLevel >= 1) {
            printf("Raw hidden data:    0x%08lX\n", (uint64_t)hiddenRawData);
            printf("Masked hidden data: 0x%08lX\n", (uint64_t)hiddenMaskedData);
            printf("Mask:               0x%02X\n", mask);
        }
        return STEGAHIDE_INVALID_DATA;
    }

    maskType = get_mask_type(mask);
    if (maskType == STEGAHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
        for (shiftSize = 1; shiftSize < 8; shiftSize++) {
            if ((mask & (1 << shiftSize)) != 0) {
                break;
            }
        }
    }

    split = hiddenMaskedDataSize / hiddenRawDataSize;
    if (((split != 1) && (split != 2) && (split != 4) && (split != 8)) || ((hiddenMaskedDataSize % hiddenRawDataSize) != 0)) {
        printf("Error: Invalid split factor\n");
        return STEGAHIDE_INVALID_MASK;
    }

    // TODO: use pthread to increase splitting speed
    for (uint64_t i = 0; i < hiddenRawDataSize; i++) {
        for (uint64_t j = split - 1; j < split; j--) {
            hiddenMaskedData[i * split + j] = (hiddenRawData[i] >> (8 / split * j)) & (0xFF >> (8 - 8 / split));
            if (hideVerboseLevel >= 4) {
                if (j == split - 1) {
                    printf("Hidden data [%ld]: 0x%02x -> ", i, hiddenRawData[i]);
                }
                printf("%02x ", hiddenMaskedData[i * split + j]);
            }

            if (maskType == STEGAHIDE_MASK_TYPE_COMPLEX) {
                hiddenMaskedData[i * split + j] = complex_remask_byte(hiddenMaskedData[i * split + j], mask);
            } else if (maskType == STEGAHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
                hiddenMaskedData[i * split + j] <<= shiftSize;
            }
        }
        if (hideVerboseLevel >= 4) {
            printf("-> ");
            for (uint64_t j = split - 1; j < split; j--) {
                printf("%02x ", hiddenMaskedData[i * split + j]);
            }
            printf("\n");
        }
    }

    return STEGAHIDE_SUCCESS;
}


static StegahideMaskType get_mask_type(uint8_t mask) {
    switch (mask) {
        case 0x01:
        case 0x03:
        case 0x0F:
        case 0xFF:
        // Currently unsupported:
        case 0x07:
        case 0x1F:
        case 0x3F:
        case 0x7F:
            return STEGAHIDE_MASK_TYPE_SIMPLE_UNIFORM;
        case 0x02: // 1 bit
        case 0x04:
        case 0x08:
        case 0x10:
        case 0x20:
        case 0x40:
        case 0x80:
        case 0x06: // 2 bits
        case 0x0C:
        case 0x18:
        case 0x30:
        case 0x60:
        case 0xC0:
        case 0x1E: // 4 bits
        case 0x3C:
        case 0x78:
        case 0xF0:
        // Currently unsupported:
        case 0x0E: // 3 bits
        case 0x1C:
        case 0x38:
        case 0x70:
        case 0xE0:
        case 0x3E: // 5 bits
        case 0x7C:
        case 0xF8:
        case 0x7E: // 6 bits
        case 0xFC:
        case 0xFE: // 7 bits
            return STEGAHIDE_MASK_TYPE_SHIFTED_UNIFORM;
        default:
            return STEGAHIDE_MASK_TYPE_COMPLEX;
    }
}


static uint8_t complex_remask_byte(uint8_t inputByte, uint8_t mask) {
    uint8_t outputByte = 0;
    int inputBitIndex = 0;
    for (int i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            outputByte |= ((inputByte >> inputBitIndex) & 1) << i;
            inputBitIndex++;
        }
    }

    return outputByte;
}


StegahideStatus hide_data(uint8_t *rawData,
                          uint64_t rawDataSize,
                          uint8_t *hiddenMaskedData,
                          uint64_t hiddenMaskedDataSize,
                          uint8_t mask,
                          uint64_t dataSizeLocation)
{
    uint64_t stepSize;
    // data sizes and dataSizeLocation must be verified by user of this function
    if (!rawData || !hiddenMaskedData) {
        printf("rawData          = 0x%08lX\n", (uint64_t)rawData);
        printf("hiddenMaskedData = 0x%08lX\n", (uint64_t)hiddenMaskedData);
        return STEGAHIDE_INVALID_DATA;
    }

    // Skip data size position
    rawData += BITS_uint64_t;
    rawDataSize -= BITS_uint64_t;

    stepSize = (rawDataSize - BITS_uint64_t) / (hiddenMaskedDataSize + 1);
    if (stepSize == 0) {
        printf("Error: Invalid step size\n");
        return STEGAHIDE_INVALID_DATA;
    }
    if (stepSize == 1) {
        printf("Warning: Step size is 1. Consider decreasing hidden data size\n");
    }

    if (hideVerboseLevel >= 1) {
        printf("Hiding data with step size: %ld\n", stepSize);
    }

    // TODO: use pthread to increase hiding speeds
    for (uint64_t i = 0; i < hiddenMaskedDataSize; i++) {
        uint64_t currentPosition = (i + 1) * stepSize;
        currentPosition = (currentPosition >= dataSizeLocation) ? currentPosition + BITS_uint64_t : currentPosition;

        if (hideVerboseLevel >= 4) {
            printf("Raw data [%ld]: 0x%02x x 0x%02x -> ", currentPosition + BITS_uint64_t, rawData[currentPosition], hiddenMaskedData[i]);
        }

        rawData[currentPosition] &= ~mask;
        rawData[currentPosition] |= hiddenMaskedData[i];

        if (hideVerboseLevel >= 4) {
            printf("0x%02x\n", rawData[currentPosition]);
        }
    }

    return STEGAHIDE_SUCCESS;
}
