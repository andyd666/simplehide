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


static int extractVerboseLevel = 0;
void set_extract_verbose_level(int level) {
    extractVerboseLevel = level;
}


static uint8_t complex_extract_remask_byte(uint8_t inputByte, uint8_t mask);
static uint8_t collect_hidden_byte(uint8_t *bytes, int bitsInMask);


size_t extract_hidden_data_size_position(const uint8_t *rawData, size_t rawDataSize) {
    size_t hiddenDataSizePosition = 0;

    if (!rawData || rawDataSize < BITS_SIZE_T) {
        printf("rawData is NULL or too small\n");
        return 0;
    }

    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        hiddenDataSizePosition |= (rawData[i] & 0x01) << i;
        if (extractVerboseLevel >= 3) {
            printf("Raw data [%ld]: 0x%02x (%d)\n", i, rawData[i], (rawData[i] & 0x01));
        }
    }

    if (extractVerboseLevel >= 1) {
        printf("Extracted hidden data size position: %ld\n", hiddenDataSizePosition);
    }

    return hiddenDataSizePosition;
}

uint8_t extract_mask(const uint8_t *rawData, size_t rawDataSize) {
    uint8_t mask = 0;
    if (!rawData || rawDataSize < BITS_SIZE_T) {
        printf("rawData is NULL or too small\n");
        return 0;
    }

    for (size_t i = 0; i < 8; i++) {
        mask |= ((rawData[i] & 0x02) >> 1) << i;
    }

    if (extractVerboseLevel >= 1) {
        printf("Extracted mask: 0x%02X\n", mask);
    }

    return mask;
}


size_t extract_hidden_data_size(const uint8_t *rawData, size_t rawDataSize, size_t hiddenDataSizePosition) {
    size_t hiddenRawDataSize = 0;

    if (!rawData || rawDataSize < BITS_SIZE_T) {
        printf("rawData is NULL or too small\n");
        return 0;
    }

    if (extractVerboseLevel >= 2) {
        printf("Exctracting data size\n");
    }

    if ((hiddenDataSizePosition > rawDataSize) || (hiddenDataSizePosition < BITS_SIZE_T)) {
        printf("Error: hiddenDataSizePosition %ld is out of bounds (rawDataSize %ld)\n", hiddenDataSizePosition, rawDataSize);
        return 0;
    }

    for (size_t i = 0; i < BITS_SIZE_T; i++) { // Skip last byte, as there can be mask
        hiddenRawDataSize |= (rawData[hiddenDataSizePosition + i] & 0x01) << i;
        if (extractVerboseLevel >= 3) {
            printf("Raw data [%ld]: 0x%02x (%d)\n", hiddenDataSizePosition + i, rawData[hiddenDataSizePosition + i], (rawData[hiddenDataSizePosition + i] & 0x01));
        }
    }

    if (extractVerboseLevel >= 1) {
        printf("Extracted hidden data size: %ld\n", hiddenRawDataSize);
    }

    if (hiddenRawDataSize > rawDataSize - BITS_SIZE_T * 2) {
        printf("Error, hidden data size is out of bounds: %ld > %ld\n", hiddenRawDataSize, rawDataSize - BITS_SIZE_T * 2);
        return 0;
    }

    return hiddenRawDataSize;
}


StegahideStatus extract_hidden_data(const uint8_t *rawData,
                                    size_t rawDataSize,
                                    uint8_t *hiddenMaskedData,
                                    size_t hiddenDataSize,
                                    size_t hiddenMaskedDataSize,
                                    size_t hiddenDataSizePosition,
                                    uint8_t mask)
{
    StegahideMaskType maskType;
    size_t stepSize;
    int uniformMaskShift = 0;
    int bitsInMask = 8 / (hiddenMaskedDataSize / hiddenDataSize);

    if (hiddenDataSize == 0) {
        printf("Error: hiddenDataSize is 0\n");
        return SIMPLEHIDE_INVALID_DATA;
    }

    if (!rawData || !hiddenMaskedData)  {
        printf("Error: rawData or hiddenMaskedData is NULL\n");
        return SIMPLEHIDE_INVALID_DATA;
    }

    if (hiddenMaskedDataSize > rawDataSize - BITS_SIZE_T * 2) {
        printf("Error: hiddenMaskedDataSize %ld is larger than rawDataSize %ld\n", hiddenMaskedDataSize, rawDataSize);
        return SIMPLEHIDE_INVALID_DATA;
    }

    rawData += BITS_SIZE_T;
    rawDataSize -= BITS_SIZE_T;
    hiddenDataSizePosition -= BITS_SIZE_T;

    stepSize = (rawDataSize - BITS_SIZE_T) / (hiddenMaskedDataSize + 1);

    if (extractVerboseLevel >= 1) {
        printf("Extracting data with step size: %ld\n", stepSize);
    }

    if (stepSize == 0) {
        printf("Error: Invalid step size\n");
        return SIMPLEHIDE_INVALID_DATA;
    }

    maskType = get_mask_type(mask);
    if (maskType == SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
        uniformMaskShift = get_uniform_mask_shift(mask);
        if (uniformMaskShift < 1) {
            printf("Error: Invalid uniform mask\n");
            return SIMPLEHIDE_INVALID_MASK;
        }
    }

    // TODO: use pthread to increase performance  ->  https://github.com/andyd666/simplehide/issues/4
    for (size_t i = 0; i < hiddenMaskedDataSize; i++) {
        size_t currentPosition = (i + 1) * stepSize;
        currentPosition = (currentPosition >= hiddenDataSizePosition) ? currentPosition + BITS_SIZE_T : currentPosition;

        hiddenMaskedData[i] = rawData[currentPosition] & mask;

        if (extractVerboseLevel >= 4) {
            printf("Extracted masked data [%ld] at [%ld]: 0x%02x\n", i, currentPosition + BITS_SIZE_T, hiddenMaskedData[i]);
        }
    }

    if (mask == 0xff) {
        return SIMPLEHIDE_SUCCESS;
    }

    if (maskType == SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
        for (size_t i = 0; i < hiddenMaskedDataSize; i++) {
            uint8_t maskedByteBefore = hiddenMaskedData[i];
            hiddenMaskedData[i] >>= uniformMaskShift;
            if (extractVerboseLevel >= 4) {
                printf("Masked data [%ld]: 0x%02x -> 0x%02x after shift\n", i, maskedByteBefore, hiddenMaskedData[i]);
            }
        }
    } else if (maskType == SIMPLEHIDE_MASK_TYPE_COMPLEX) {
        for (size_t i = 0; i < hiddenMaskedDataSize; i++) {
            uint8_t maskedByteBefore = hiddenMaskedData[i];
            hiddenMaskedData[i] = complex_extract_remask_byte(hiddenMaskedData[i], mask);
            if (extractVerboseLevel >= 4) {
                printf("Masked data [%ld]: 0x%02x -> 0x%02x after remasking\n", i, maskedByteBefore, hiddenMaskedData[i]);
            }
        }
    }

    for (size_t i = 0; i < hiddenDataSize; i++) {
        if (extractVerboseLevel >= 4) {
            printf("Collecting byte [%ld]:", i);
            for (int j = 0; j < (8 / bitsInMask); j++) {
                if (extractVerboseLevel >= 4) {
                    printf(" 0x%02x", hiddenMaskedData[i * bitsInMask + j]);
                }
            }
        }

        hiddenMaskedData[i] = collect_hidden_byte(&hiddenMaskedData[i * (8 / bitsInMask)], bitsInMask);

        if (extractVerboseLevel >= 4) {
            printf(" -> 0x%02x\n", hiddenMaskedData[i]);
        }
    }

    return SIMPLEHIDE_SUCCESS;
}


static uint8_t complex_extract_remask_byte(uint8_t inputByte, uint8_t mask) {
    static uint8_t lookupTableInitialized = 0;
    static uint8_t *lookupTableExtract = NULL;

    if (!lookupTableInitialized) {
        get_lookup_tables(mask, &lookupTableInitialized, NULL, &lookupTableExtract);
    }

    return lookupTableExtract[inputByte];
}


static uint8_t collect_hidden_byte(uint8_t *bytes, int bitsInMask) {
    for (int i = 0; i < (8 / bitsInMask); i++) {
        bytes[0] |= bytes[i] << (i * bitsInMask);
    }
    return bytes[0];
}
