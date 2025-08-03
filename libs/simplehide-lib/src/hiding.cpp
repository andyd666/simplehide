/*
Copyright (c) 2025 andyd666

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "simplehide-lib.h"


static int hideVerboseLevel = 0;
void set_hide_verbose_level(int level) {
    hideVerboseLevel = level;
}


static size_t get_correct_data_bits_in_sequence(const uint8_t rawData[BITS_SIZE_T], size_t rawDataSize);

static int get_bits_in_mask(uint8_t mask);
static uint8_t complex_hide_remask_byte(uint8_t inputByte, uint8_t mask);


StegahideStatus embed_hidden_data_size(uint8_t *rawData, size_t *sizePosition, size_t rawDataSize, size_t hiddenRawDataSize) {
    uint8_t verboseDataSizeBytesBefore[BITS_SIZE_T];
    uint8_t verboseDataSizeBytesAfter[BITS_SIZE_T];
    uint8_t verboseDataPositionBytesBefore[BITS_SIZE_T];
    uint8_t verboseDataPositionBytesAfter[BITS_SIZE_T];

    if (rawData == NULL || rawDataSize < BITS_SIZE_T || hiddenRawDataSize == 0) {
        return SIMPLEHIDE_INVALID_DATA;
    }

    if (hideVerboseLevel >= 2) {
        printf("Embedding hidden rawData size bits: ");
        for (size_t i = BITS_SIZE_T - 1; i < BITS_SIZE_T; i--) {
            printf("%ld", (hiddenRawDataSize >> i) & (size_t)0x01);
        }
        printf("\n");
    }

    *sizePosition = find_hidden_data_size_position(rawData, rawDataSize, hiddenRawDataSize);
    if (*sizePosition + BITS_SIZE_T > rawDataSize) {
        printf("Data size %ld, sizePosition %ld is out of bounds\n", rawDataSize, *sizePosition);
        return SIMPLEHIDE_SIZE_EMBED_ERROR;
    }

    if (hideVerboseLevel >= 1) {
        printf("Found best hidden rawData size sizePosition: %ld\n", *sizePosition);
    }

    if (hideVerboseLevel >= 3) {
        memcpy(verboseDataSizeBytesBefore, &rawData[*sizePosition], BITS_SIZE_T);
        memcpy(verboseDataPositionBytesBefore, &rawData[0], BITS_SIZE_T);
    }

    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        rawData[*sizePosition + i] = (rawData[*sizePosition + i] & ~0x01) | ((hiddenRawDataSize & (0x01 << i)) >> i);
        rawData[i] = (rawData[i] & ~0x01) | ((*sizePosition >> i) & 0x01);
    }

    if (hideVerboseLevel >= 3) {
        printf("Size embedding result:\n");
        memcpy(verboseDataSizeBytesAfter, &rawData[*sizePosition], BITS_SIZE_T);
        memcpy(verboseDataPositionBytesAfter, &rawData[0], BITS_SIZE_T);
        for (size_t i = 0; i < BITS_SIZE_T; i++) {
            printf("[%10ld]: 0x%02x -> 0x%02x (%ld)            [%2ld]: 0x%02x -> 0x%02x (%d)\n",
                    *sizePosition + i,
                    verboseDataSizeBytesBefore[i],
                    verboseDataSizeBytesAfter[i],
                    (hiddenRawDataSize & (0x01 << i)) >> i,
                    i,
                    verboseDataPositionBytesBefore[i],
                    verboseDataPositionBytesAfter[i],
                    (uint8_t)((*sizePosition >> i) & 0x01));
        }
    }

    return SIMPLEHIDE_SUCCESS;
}


size_t find_hidden_data_size_position(const uint8_t *rawData, size_t rawDataSize, size_t hiddenRawDataSize) {
    size_t currentHiddenDataSizePosition = BITS_SIZE_T; // cannot start at 0
    size_t currentHiddenDataSizeBits = 0;

    // TODO: use pthread to increase search speed  ->  https://github.com/andyd666/simplehide/issues/4
    for (size_t i = BITS_SIZE_T; i < rawDataSize - BITS_SIZE_T; i++) {
        size_t correctBits = get_correct_data_bits_in_sequence(&rawData[i], hiddenRawDataSize);
        if (correctBits == BITS_SIZE_T) {
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


static size_t get_correct_data_bits_in_sequence(const uint8_t rawData[BITS_SIZE_T], size_t rawDataSize) {
    size_t correctBits = 0;
    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        if ((rawData[i] & 0x01) == ((rawDataSize & (0x01 << i)) >> i)) {
            correctBits++;
        }
    }
    return correctBits;
}


StegahideStatus hide_mask(uint8_t *rawData, uint8_t mask) {
    if (!rawData) {
        printf("Error: rawData is NULL\n");
        return SIMPLEHIDE_MASK_EMBED_ERROR;
    }

    for (size_t i = 0; i < 8; i++) {
        rawData[i] = (rawData[i] & ~0x02) | (((mask >> i) & 0x01) << 1);
    }

    return SIMPLEHIDE_SUCCESS;
}


StegahideStatus get_hidden_data_masked_size(uint8_t mask, size_t hiddenRawDataSize, size_t *maskedHiddenDataSize) {
    if (mask == 0) {
        printf("Error: Mask not set\n");
        return SIMPLEHIDE_INVALID_MASK;
    }

    if (verify_mask(mask, hiddenRawDataSize, maskedHiddenDataSize) != SIMPLEHIDE_SUCCESS) {
        printf("Cannot verify mask = 0x%02X\n", mask);
        return SIMPLEHIDE_INVALID_MASK;
    }

    if (hideVerboseLevel >= 1) {
        printf("Masked raw data size:    %ld\n", hiddenRawDataSize);
        printf("Masked hidden data size: %ld\n", *maskedHiddenDataSize);
    }

    return SIMPLEHIDE_SUCCESS;
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


StegahideStatus verify_mask(uint8_t mask, size_t hiddenRawDataSize, size_t *maskedHiddenDataSize) {
    int bitsInMask = get_bits_in_mask(mask);

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
        if (hideVerboseLevel >= 1) {
            printf("Warning: Mask with 8 bits set is not recommended\n");
        }
        break;
    default:
        *maskedHiddenDataSize = 0;
        printf("Error: Mask must have power of 2 bits set (1, 2, 4, 8(not recommended))\n");
        return SIMPLEHIDE_INVALID_MASK;
    }
    return SIMPLEHIDE_SUCCESS;
}


StegahideStatus generate_masked_hidden_data(const uint8_t *hiddenRawData, size_t hiddenRawDataSize, uint8_t *hiddenMaskedData, size_t hiddenMaskedDataSize, uint8_t mask) {
    StegahideMaskType maskType;
    int shiftSize = 1;
    size_t split;

    if (!hiddenRawData || !hiddenMaskedData || !get_bits_in_mask(mask)) {
        printf("Error: Invalid input data\n");
        if (hideVerboseLevel >= 1) {
            printf("Raw hidden data:    0x%08lX\n", (size_t)hiddenRawData);
            printf("Masked hidden data: 0x%08lX\n", (size_t)hiddenMaskedData);
            printf("Mask:               0x%02X\n", mask);
        }
        return SIMPLEHIDE_INVALID_DATA;
    }

    maskType = get_mask_type(mask);
    if (maskType == SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
        shiftSize = get_uniform_mask_shift(mask);
        if (shiftSize < 1) {
            printf("Error: Invalid uniform mask\n");
            return SIMPLEHIDE_INVALID_MASK;
        }
    }

    split = hiddenMaskedDataSize / hiddenRawDataSize;
    if (((split != 1) && (split != 2) && (split != 4) && (split != 8)) || ((hiddenMaskedDataSize % hiddenRawDataSize) != 0)) {
        printf("Error: Invalid split factor\n");
        return SIMPLEHIDE_INVALID_MASK;
    }

    // TODO: use pthread to increase splitting speed  ->  https://github.com/andyd666/simplehide/issues/4
    for (size_t i = 0; i < hiddenRawDataSize; i++) {
        for (size_t j = split - 1; j < split; j--) {
            hiddenMaskedData[i * split + j] = (hiddenRawData[i] >> (8 / split * j)) & (0xFF >> (8 - 8 / split));
            if (hideVerboseLevel >= 4) {
                if (j == split - 1) {
                    printf("Hidden data [%ld]: 0x%02x -> ", i, hiddenRawData[i]);
                }
                printf("%02x ", hiddenMaskedData[i * split + j]);
            }

            if (maskType == SIMPLEHIDE_MASK_TYPE_COMPLEX) {
                hiddenMaskedData[i * split + j] = complex_hide_remask_byte(hiddenMaskedData[i * split + j], mask);
            } else if (maskType == SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
                hiddenMaskedData[i * split + j] <<= shiftSize;
            }
        }
        if (hideVerboseLevel >= 4) {
            printf("-> ");
            for (size_t j = split - 1; j < split; j--) {
                printf("%02x ", hiddenMaskedData[i * split + j]);
            }
            printf("\n");
        }
    }

    return SIMPLEHIDE_SUCCESS;
}


int get_uniform_mask_shift(uint8_t mask) {
    for (int shiftSize = 1; shiftSize < 8; shiftSize++) {
        if ((mask & (1 << shiftSize)) != 0) {
            return shiftSize;
        }
    }
    return -1;
}


StegahideMaskType get_mask_type(uint8_t mask) {
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
            return SIMPLEHIDE_MASK_TYPE_SIMPLE_UNIFORM;
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
            return SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM;
        default:
            return SIMPLEHIDE_MASK_TYPE_COMPLEX;
    }
}


static uint8_t complex_hide_remask_byte(uint8_t inputByte, uint8_t mask) {
    // TODO: make this faster with a lookup table
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
                          size_t rawDataSize,
                          const uint8_t *hiddenMaskedData,
                          size_t hiddenMaskedDataSize,
                          uint8_t mask,
                          size_t dataSizePosition)
{
    size_t stepSize;
    // data sizes and dataSizePosition must be verified by user of this function
    if (!rawData || !hiddenMaskedData) {
        printf("rawData          = 0x%08lX\n", (size_t)rawData);
        printf("hiddenMaskedData = 0x%08lX\n", (size_t)hiddenMaskedData);
        return SIMPLEHIDE_INVALID_DATA;
    }

    // Skip data size position
    rawData += BITS_SIZE_T;
    rawDataSize -= BITS_SIZE_T;
    dataSizePosition -= BITS_SIZE_T;

    stepSize = (rawDataSize - BITS_SIZE_T) / (hiddenMaskedDataSize + 1);

    if (hideVerboseLevel >= 1) {
        printf("Hiding data with step size: %ld\n", stepSize);
        printf("rawData:          0x%08lX\n", (size_t)rawData);
        printf("rawDataSize:      %ld\n", rawDataSize);
        printf("dataSizePosition: %ld\n", dataSizePosition);
    }

    if (stepSize == 0) {
        printf("Error: Invalid step size\n");
        return SIMPLEHIDE_INVALID_DATA;
    }
    if (stepSize == 1) {
        printf("Warning: Step size is 1. Consider decreasing hidden data size\n");
    }

    // TODO: use pthread to increase hiding speeds  ->  https://github.com/andyd666/simplehide/issues/4
    for (size_t i = 0; i < hiddenMaskedDataSize; i++) {
        size_t currentPosition = (i + 1) * stepSize;
        currentPosition = (currentPosition >= dataSizePosition) ? currentPosition + BITS_SIZE_T : currentPosition;

        if (hideVerboseLevel >= 4) {
            printf("Raw data [%ld]: 0x%02x x 0x%02x -> ", currentPosition + BITS_SIZE_T, rawData[currentPosition], hiddenMaskedData[i]);
        }

        rawData[currentPosition] &= ~mask;
        rawData[currentPosition] |= hiddenMaskedData[i];

        if (hideVerboseLevel >= 4) {
            printf("0x%02x\n", rawData[currentPosition]);
        }
    }

    return SIMPLEHIDE_SUCCESS;
}
