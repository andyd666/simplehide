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
#include <pthread.h>
#include "simplehide-lib.h"


static int hideVerboseLevel = 0;
static int hidingThreadNumber = 1;

void set_hide_verbose_level(int level) {
    hideVerboseLevel = level;
}

void set_hide_thread_number(int num) {
    if (num < 1) {
        printf("Hide Thread number cannot be set to %d\n", num);
        return;
    }

    if (hideVerboseLevel >= 1) {
        printf("Setting hiding threads to %d\n", num);
    }

    hidingThreadNumber = num;
}


static void *find_hidden_data_size_position_thread(void *data__);
static size_t get_correct_data_bits_in_sequence(const uint8_t rawData[BITS_SIZE_T], size_t rawDataSize);

static void *generate_masked_hidden_data_thread(void *data__);

static uint8_t lookup_table_generator(uint8_t inputByte, uint8_t mask);
static uint8_t complex_hide_remask_byte(uint8_t inputByte, uint8_t mask);

static void *hide_data_thread(void *data__);


StegahideStatus embed_hidden_data_size(uint8_t *rawData, size_t rawDataSize, size_t hiddenRawDataSize) {
    uint8_t verboseDataSizeBytesBefore[BITS_SIZE_T];
    uint8_t verboseDataSizeBytesAfter[BITS_SIZE_T];

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

    if (hideVerboseLevel >= 3) {
        memcpy(verboseDataSizeBytesBefore, &rawData[0], BITS_SIZE_T);
    }

    for (size_t i = 0; i < BITS_SIZE_T; i++) {
        rawData[i] = (rawData[i] & ~0x01) | ((hiddenRawDataSize & (0x01 << i)) >> i);
    }

    if (hideVerboseLevel >= 3) {
        printf("Size embedding result:\n");
        memcpy(verboseDataSizeBytesAfter, &rawData[0], BITS_SIZE_T);
        for (size_t i = 0; i < BITS_SIZE_T; i++) {
            printf("[%10ld]: 0x%02x -> 0x%02x (%ld)\n",
                    i,
                    verboseDataSizeBytesBefore[i],
                    verboseDataSizeBytesAfter[i],
                    (hiddenRawDataSize & (0x01 << i)) >> i);
        }
    }

    return SIMPLEHIDE_SUCCESS;
}


size_t find_hidden_data_size_position(const uint8_t *rawData, size_t rawDataSize, size_t hiddenRawDataSize) {
    size_t currentHiddenDataSizePosition = BITS_SIZE_T; // cannot start at 0
    size_t currentHiddenDataSizeBits = 0;
    pthread_t *threadVector = (pthread_t *)malloc(sizeof(pthread_t) * hidingThreadNumber);
    HideDataSizeThreadData *threadData = (HideDataSizeThreadData *)malloc(sizeof(HideDataSizeThreadData) * hidingThreadNumber);
    size_t totalBytes;
    size_t bytesPerThread;
    size_t bytesPerThreadRemainder;
    size_t threadStartBytePosition;
    size_t threadStopBytePosition;
    int storedThreads = hidingThreadNumber;

    if (threadVector == NULL || threadData == NULL) {
        printf("Error: cannot allocate threadData:\n\tthreadVector = 0x%08lx\n\tdata         = 0x%08lx\n", (size_t)threadVector, (size_t)threadData);
        if (threadData)
            free(threadData);

        if (threadVector)
            free(threadVector);

        return -1;
    }

    if (DISABLE_MULTITHREADING) {
        hidingThreadNumber = 1;
    }

    totalBytes = rawDataSize - (2 * BITS_SIZE_T);
    bytesPerThread = totalBytes / hidingThreadNumber;
    bytesPerThreadRemainder = totalBytes % hidingThreadNumber;
    threadStartBytePosition = BITS_SIZE_T;
    threadStopBytePosition = threadStartBytePosition + bytesPerThread;

    if (bytesPerThreadRemainder > 0) {
        threadStopBytePosition += 1;
        bytesPerThreadRemainder--;
    }

    for (int i = 0; i < hidingThreadNumber; i++) {
        threadData[i].rawData           = rawData;
        threadData[i].hiddenRawDataSize = hiddenRawDataSize;
        threadData[i].startPosition     = threadStartBytePosition;
        threadData[i].stopPosition      = threadStopBytePosition;
        threadData[i].correctBits       = 0;
        threadData[i].position          = threadStartBytePosition;

        pthread_create(&threadVector[i], NULL, &find_hidden_data_size_position_thread, (void *)(&threadData[i]));

        threadStartBytePosition = threadStopBytePosition;
        threadStopBytePosition += bytesPerThread;
        if (bytesPerThreadRemainder > 0) {
            threadStopBytePosition += 1;
            bytesPerThreadRemainder--;
        }
    }

    for (int i = 0; i < hidingThreadNumber; i++) {
        pthread_join(threadVector[i], NULL);
        if (hideVerboseLevel >= 3) {
            printf("Thread %d found best match at %ld with %ld bits\n", i, threadData[i].position, threadData[i].correctBits);
        }
    }

    for (int i = 0; i < hidingThreadNumber; i++) {
        if (threadData[i].correctBits >= currentHiddenDataSizeBits) {
            currentHiddenDataSizeBits = threadData[i].correctBits;
            currentHiddenDataSizePosition = threadData[i].position;
        }
    }

    if (DISABLE_MULTITHREADING) {
        hidingThreadNumber = storedThreads;
    }

    free(threadData);
    free(threadVector);

    return currentHiddenDataSizePosition;
}

static void *find_hidden_data_size_position_thread(void *data__) {
    HideDataSizeThreadData *threadData = (HideDataSizeThreadData *)(data__);
    const uint8_t *rawData   = threadData->rawData;
    size_t hiddenRawDataSize = threadData->hiddenRawDataSize;
    size_t startPosition     = threadData->startPosition;
    size_t stopPosition      = threadData->stopPosition;
    size_t correctBits       = threadData->correctBits;
    size_t position          = threadData->position;

    size_t currentHiddenDataSizeBits = 0;

    for (size_t i = startPosition; i < stopPosition; i++) {
        correctBits = get_correct_data_bits_in_sequence(&rawData[i], hiddenRawDataSize);
        if (correctBits == BITS_SIZE_T) {
            if (hideVerboseLevel >= 3 && hidingThreadNumber == 1) {
                printf("New Correct bits sizePosition: %ld\n", i);
                printf("New Correct bits:              %ld\n", correctBits);
            }
            break;
        } else if (correctBits >= currentHiddenDataSizeBits) { // Take the furthest best match
            currentHiddenDataSizeBits = correctBits;
            position = i;
            if (hideVerboseLevel >= 3 && hidingThreadNumber == 1) {
                printf("New Correct bits sizePosition: %ld\n", position);
                printf("New Correct bits:              %ld\n", correctBits);
            }
        }
    }
    threadData->position = position;
    return NULL;
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

int get_bits_in_mask(uint8_t mask) {
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
    pthread_t *threadVector = (pthread_t *)malloc(sizeof(pthread_t) * hidingThreadNumber);
    GenerateMaskedHiddenDataThreadData *threadData = (GenerateMaskedHiddenDataThreadData *)malloc(sizeof(GenerateMaskedHiddenDataThreadData) * hidingThreadNumber);
    size_t totalBytes;
    size_t bytesPerThread;
    size_t bytesPerThreadRemainder;
    size_t threadStartBytePosition;
    size_t threadStopBytePosition;
    int storedThreads = hidingThreadNumber;

    if (threadVector == NULL || threadData == NULL) {
        printf("Error: cannot allocate threadData:\n\tthreadVector = 0x%08lx\n\tdata         = 0x%08lx\n", (size_t)threadVector, (size_t)threadData);
        if (threadData)
            free(threadData);

        if (threadVector)
            free(threadVector);

        return SIMPLEHIDE_MEMORY_ALLOCATION_ERROR;
    }

    if (!hiddenRawData || !hiddenMaskedData || !get_bits_in_mask(mask)) {
        printf("Error: Invalid input data\n");
        if (hideVerboseLevel >= 1) {
            printf("Raw hidden data:    0x%08lX\n", (size_t)hiddenRawData);
            printf("Masked hidden data: 0x%08lX\n", (size_t)hiddenMaskedData);
            printf("Mask:               0x%02X\n", mask);
        }
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_INVALID_DATA;
    }

    maskType = get_mask_type(mask);
    if (maskType == SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
        shiftSize = get_uniform_mask_shift(mask);
        if (shiftSize < 1) {
            printf("Error: Invalid uniform mask\n");
            free(threadData);
            free(threadVector);
            return SIMPLEHIDE_INVALID_MASK;
        }
    }

    split = hiddenMaskedDataSize / hiddenRawDataSize;
    if (((split != 1) && (split != 2) && (split != 4) && (split != 8)) || ((hiddenMaskedDataSize % hiddenRawDataSize) != 0)) {
        printf("Error: Invalid split factor\n");
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_INVALID_MASK;
    }

    if (DISABLE_MULTITHREADING) {
        hidingThreadNumber = 1;
    }

    totalBytes = hiddenRawDataSize;
    bytesPerThread = totalBytes / hidingThreadNumber;
    bytesPerThreadRemainder = totalBytes % hidingThreadNumber;
    threadStartBytePosition = 0;
    threadStopBytePosition = threadStartBytePosition + bytesPerThread;

    if (bytesPerThreadRemainder > 0) {
        threadStopBytePosition += 1;
        bytesPerThreadRemainder--;
    }

    for (int i = 0; i < hidingThreadNumber; i++) {
        threadData[i].rawData          = hiddenRawData;
        threadData[i].startPosition    = threadStartBytePosition;
        threadData[i].stopPosition     = threadStopBytePosition;
        threadData[i].hiddenMaskedData = hiddenMaskedData;
        threadData[i].mask             = mask;
        threadData[i].maskType         = maskType;
        threadData[i].split            = split;
        threadData[i].shiftSize        = shiftSize;

        pthread_create(&threadVector[i], NULL, &generate_masked_hidden_data_thread, (void *)(&threadData[i]));

        threadStartBytePosition = threadStopBytePosition;
        threadStopBytePosition += bytesPerThread;
        if (bytesPerThreadRemainder > 0) {
            threadStopBytePosition += 1;
            bytesPerThreadRemainder--;
        }
    }

    for (int i = 0; i < hidingThreadNumber; i++) {
        pthread_join(threadVector[i], NULL);
        if (hideVerboseLevel >= 3) {
            printf("Thread %d finished generating masked data\n", i);
        }
    }

    if (DISABLE_MULTITHREADING) {
        hidingThreadNumber = storedThreads;
    }

    free(threadData);
    free(threadVector);

    return SIMPLEHIDE_SUCCESS;
}


static void *generate_masked_hidden_data_thread(void *data__) {
    GenerateMaskedHiddenDataThreadData *threadData = (GenerateMaskedHiddenDataThreadData *)data__;
    const uint8_t *hiddenRawData = threadData->rawData;
    size_t startPosition         = threadData->startPosition;
    size_t stopPosition          = threadData->stopPosition;
    uint8_t *hiddenMaskedData    = threadData->hiddenMaskedData;
    uint8_t mask                 = threadData->mask;
    StegahideMaskType maskType   = threadData->maskType;
    size_t split                 = threadData->split;
    int shiftSize                = threadData->shiftSize;

    for (size_t i = startPosition; i < stopPosition; i++) {
        for (size_t j = split - 1; j < split; j--) {
            hiddenMaskedData[i * split + j] = (hiddenRawData[i] >> (8 / split * j)) & (0xFF >> (8 - 8 / split));
            if (hideVerboseLevel >= 4 && hidingThreadNumber == 1) {
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
        if (hideVerboseLevel >= 4 && hidingThreadNumber == 1) {
            printf("-> ");
            for (size_t j = split - 1; j < split; j--) {
                printf("%02x ", hiddenMaskedData[i * split + j]);
            }
            printf("\n");
        }
    }
    return NULL;
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


static uint8_t lookupTableInitialized = 0;
static uint8_t lookupTableHide[256] = {0};
static uint8_t lookupTableExtract[256] = {0};

void get_lookup_tables(uint8_t mask, uint8_t *lookupTableInitialized__, uint8_t **lookupTableHide__, uint8_t **lookupTableExtract__) {
    if (lookupTableInitialized__) *lookupTableInitialized__ = lookupTableInitialized;
    if (lookupTableHide__)        *lookupTableHide__        = lookupTableHide;
    if (lookupTableExtract__)     *lookupTableExtract__     = lookupTableExtract;

    if (!lookupTableInitialized) {
        int setBits = get_bits_in_mask(mask);
        if (hideVerboseLevel >= 2) {
            printf("Initializing complex remask lookup table for mask 0x%02X with %d bits set\n", mask, setBits);
        }
        if (hideVerboseLevel >= 3) {
            printf("                   Hide index         Extract index\n");
        }

        for (int i = 0; i < (1 << setBits); i++) {
            if (hideVerboseLevel >= 3) {
                printf("Lookup table init: 0x%02x     <->     0x%02x\n", (uint8_t)i, lookup_table_generator((uint8_t)i, mask));
            }
            lookupTableHide[i] = lookup_table_generator((uint8_t)i, mask);
            lookupTableExtract[lookupTableHide[i]] = (uint8_t)i;
        }
        lookupTableInitialized = 1;
    }
}


static uint8_t lookup_table_generator(uint8_t inputByte, uint8_t mask) {
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


static uint8_t complex_hide_remask_byte(uint8_t inputByte, uint8_t mask) {
    static uint8_t lookupTableInitialized = 0;
    static uint8_t *lookupTableHide = NULL;

    if (!lookupTableInitialized) {
        get_lookup_tables(mask, &lookupTableInitialized, &lookupTableHide, NULL);
    }

    return lookupTableHide[inputByte];
}


StegahideStatus hide_data(uint8_t *rawData,
                          size_t rawDataSize,
                          const uint8_t *hiddenMaskedData,
                          size_t hiddenMaskedDataSize,
                          uint8_t mask)
{
    size_t stepSize;
    pthread_t *threadVector = (pthread_t *)malloc(sizeof(pthread_t) * hidingThreadNumber);
    HideDataThreadData *threadData = (HideDataThreadData *)malloc(sizeof(HideDataThreadData) * hidingThreadNumber);
    size_t totalBytes;
    size_t bytesPerThread;
    size_t bytesPerThreadRemainder;
    size_t threadStartBytePosition;
    size_t threadStopBytePosition;
    int storedThreads = hidingThreadNumber;

    if (threadVector == NULL || threadData == NULL) {
        printf("Error: cannot allocate threadData:\n\tthreadVector = 0x%08lx\n\tdata         = 0x%08lx\n", (size_t)threadVector, (size_t)threadData);
        if (threadData)
            free(threadData);

        if (threadVector)
            free(threadVector);

        return SIMPLEHIDE_MEMORY_ALLOCATION_ERROR;
    }

    // data sizes and dataSizePosition must be verified by user of this function
    if (!rawData || !hiddenMaskedData) {
        printf("rawData          = 0x%08lX\n", (size_t)rawData);
        printf("hiddenMaskedData = 0x%08lX\n", (size_t)hiddenMaskedData);
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_INVALID_DATA;
    }

    // Skip data size position
    rawData += BITS_SIZE_T;
    rawDataSize -= BITS_SIZE_T;

    stepSize = (rawDataSize - BITS_SIZE_T) / (hiddenMaskedDataSize + 1);

    if (hideVerboseLevel >= 1) {
        printf("Hiding data with step size: %ld\n", stepSize);
        printf("rawData:          0x%08lX\n", (size_t)rawData);
        printf("rawDataSize:      %ld\n", rawDataSize);
    }

    if (stepSize == 0) {
        printf("Error: Invalid step size\n");
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_INVALID_DATA;
    } else if (stepSize == 1) {
        printf("Warning: Step size is 1. Consider decreasing hidden data size\n");
    }

    if (DISABLE_MULTITHREADING) {
        hidingThreadNumber = 1;
    }

    totalBytes = hiddenMaskedDataSize;
    bytesPerThread = totalBytes / hidingThreadNumber;
    bytesPerThreadRemainder = totalBytes % hidingThreadNumber;
    threadStartBytePosition = 0;
    threadStopBytePosition = threadStartBytePosition + bytesPerThread;

    if (bytesPerThreadRemainder > 0) {
        threadStopBytePosition += 1;
        bytesPerThreadRemainder--;
    }

    for (int i = 0; i < hidingThreadNumber; i++) {
        threadData[i].rawData          = rawData;
        threadData[i].startPosition    = threadStartBytePosition;
        threadData[i].stopPosition     = threadStopBytePosition;
        threadData[i].hiddenMaskedData = hiddenMaskedData;
        threadData[i].mask             = mask;
        threadData[i].stepSize         = stepSize;

        pthread_create(&threadVector[i], NULL, &hide_data_thread, (void *)(&threadData[i]));

        threadStartBytePosition = threadStopBytePosition;
        threadStopBytePosition += bytesPerThread;
        if (bytesPerThreadRemainder > 0) {
            threadStopBytePosition += 1;
            bytesPerThreadRemainder--;
        }
    }

    for (int i = 0; i < hidingThreadNumber; i++) {
        pthread_join(threadVector[i], NULL);
        if (hideVerboseLevel >= 3) {
            printf("Thread %d finished hiding data\n", i);
        }
    }

    if (DISABLE_MULTITHREADING) {
        hidingThreadNumber = storedThreads;
    }

    free(threadData);
    free(threadVector);

    return SIMPLEHIDE_SUCCESS;
}


static void *hide_data_thread(void *data__) {
    HideDataThreadData *threadData = (HideDataThreadData *)data__;
    uint8_t *rawData                = threadData->rawData;
    size_t startPosition            = threadData->startPosition;
    size_t stopPosition             = threadData->stopPosition;
    const uint8_t *hiddenMaskedData = threadData->hiddenMaskedData;
    uint8_t mask                    = threadData->mask;
    size_t stepSize                 = threadData->stepSize;

    for (size_t i = startPosition; i < stopPosition; i++) {
        size_t currentPosition = (i + 1) * stepSize;

        if (hideVerboseLevel >= 4 && hidingThreadNumber == 1) {
            printf("Raw data [%ld]: 0x%02x x 0x%02x -> ", currentPosition + BITS_SIZE_T, rawData[currentPosition], hiddenMaskedData[i]);
        }

        rawData[currentPosition] &= ~mask;
        rawData[currentPosition] |= hiddenMaskedData[i];

        if (hideVerboseLevel >= 4 && hidingThreadNumber == 1) {
            printf("0x%02x\n", rawData[currentPosition]);
        }
    }
    return NULL;
}
