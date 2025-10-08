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


static int extractVerboseLevel = 0;
static int extractingThreadNumber = 1;

void set_extract_verbose_level(int level) {
    extractVerboseLevel = level;
}

void set_extract_thread_number(int num) {
    if (num < 1) {
        printf("Extract Thread number cannot be set to %d\n", num);
        return;
    }

    if (extractVerboseLevel >= 1) {
        printf("Setting extract threads to %d\n", num);
    }

    extractingThreadNumber = num;
}


static void *extract_hidden_data_extract_masked_data_thread(void *data__);
static void *extract_hidden_data_collect_hidden_bytes_thread(void *data__);
static uint8_t complex_extract_remask_byte(uint8_t inputByte, uint8_t mask);
static uint8_t collect_hidden_byte(const uint8_t *bytes, int bitsInMask);


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


size_t extract_hidden_data_size(const uint8_t *rawData, size_t rawDataSize) {
    size_t hiddenRawDataSize = 0;

    if (!rawData || rawDataSize < BITS_SIZE_T) {
        printf("rawData is NULL or too small\n");
        return 0;
    }

    if (extractVerboseLevel >= 2) {
        printf("Exctracting data size\n");
    }

    for (size_t i = 0; i < BITS_SIZE_T; i++) { // Skip last byte, as there can be mask
        hiddenRawDataSize |= (rawData[i] & 0x01) << i;
        if (extractVerboseLevel >= 3) {
            printf("Raw data [%ld]: 0x%02x (%d)\n", i, rawData[i], (rawData[i] & 0x01));
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
                                    uint8_t mask)
{
    StegahideMaskType maskType;
    size_t stepSize;
    int uniformMaskShift = 0;
    int bitsInMask = 8 / (hiddenMaskedDataSize / hiddenDataSize);
    pthread_t *threadVector = (pthread_t *)malloc(sizeof(pthread_t) * extractingThreadNumber);
    void *threadData = malloc(sizeof(ExtractMaskedDataThreadData) * extractingThreadNumber);
    uint8_t *hiddenData = NULL;
    size_t totalBytes;
    size_t bytesPerThread;
    size_t bytesPerThreadRemainder;
    size_t threadStartBytePosition;
    size_t threadStopBytePosition;
    int storedThreads = extractingThreadNumber;

    if (threadVector == NULL || threadData == NULL) {
        printf("Error: cannot allocate threadData:\n\tthreadVector = 0x%08lx\n\tdata         = 0x%08lx\n", (size_t)threadVector, (size_t)threadData);
        if (threadData)
            free(threadData);

        if (threadVector)
            free(threadVector);

        return SIMPLEHIDE_MEMORY_ALLOCATION_ERROR;
    }

    if (hiddenDataSize == 0) {
        printf("Error: hiddenDataSize is 0\n");
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_INVALID_DATA;
    }

    if (!rawData || !hiddenMaskedData)  {
        printf("Error: rawData or hiddenMaskedData is NULL\n");
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_INVALID_DATA;
    }

    if (hiddenMaskedDataSize > rawDataSize - BITS_SIZE_T * 2) {
        printf("Error: hiddenMaskedDataSize %ld is larger than rawDataSize %ld\n", hiddenMaskedDataSize, rawDataSize);
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_INVALID_DATA;
    }

    rawData += BITS_SIZE_T;
    rawDataSize -= BITS_SIZE_T;

    stepSize = (rawDataSize - BITS_SIZE_T) / (hiddenMaskedDataSize + 1);

    if (extractVerboseLevel >= 1) {
        printf("Extracting data with step size: %ld\n", stepSize);
    }

    if (stepSize == 0) {
        printf("Error: Invalid step size\n");
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_INVALID_DATA;
    }

    maskType = get_mask_type(mask);
    if (maskType == SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
        uniformMaskShift = get_uniform_mask_shift(mask);
        if (uniformMaskShift < 1) {
            printf("Error: Invalid uniform mask\n");
            free(threadData);
            free(threadVector);
            return SIMPLEHIDE_INVALID_MASK;
        }
    }

    if (DISABLE_MULTITHREADING) {
        extractingThreadNumber = 1;
    }

    totalBytes = hiddenMaskedDataSize;
    bytesPerThread = totalBytes / extractingThreadNumber;
    bytesPerThreadRemainder = totalBytes % extractingThreadNumber;
    threadStartBytePosition = 0;
    threadStopBytePosition = threadStartBytePosition + bytesPerThread;

    if (bytesPerThreadRemainder > 0) {
        threadStopBytePosition += 1;
        bytesPerThreadRemainder--;
    }

    for (int i = 0; i < extractingThreadNumber; i++) {
        ((ExtractMaskedDataThreadData *)threadData)[i].rawData                = rawData;
        ((ExtractMaskedDataThreadData *)threadData)[i].startPosition          = threadStartBytePosition;
        ((ExtractMaskedDataThreadData *)threadData)[i].stopPosition           = threadStopBytePosition;
        ((ExtractMaskedDataThreadData *)threadData)[i].hiddenMaskedData       = hiddenMaskedData;
        ((ExtractMaskedDataThreadData *)threadData)[i].mask                   = mask;
        ((ExtractMaskedDataThreadData *)threadData)[i].maskType               = maskType;
        ((ExtractMaskedDataThreadData *)threadData)[i].uniformMaskShift       = uniformMaskShift;
        ((ExtractMaskedDataThreadData *)threadData)[i].stepSize               = stepSize;

        pthread_create(&threadVector[i], NULL, &extract_hidden_data_extract_masked_data_thread, &((ExtractMaskedDataThreadData *)threadData)[i]);

        threadStartBytePosition = threadStopBytePosition;
        threadStopBytePosition += bytesPerThread;
        if (bytesPerThreadRemainder > 0) {
            threadStopBytePosition += 1;
            bytesPerThreadRemainder--;
        }
    }

    for (int i = 0; i < extractingThreadNumber; i++) {
        pthread_join(threadVector[i], NULL);
        if (extractVerboseLevel >= 3) {
            printf("Thread %d finished extracting masked data\n", i);
        }
    }

    if (DISABLE_MULTITHREADING) {
        extractingThreadNumber = storedThreads;
    }

    if (mask == 0xff) {
        free(threadData);
        free(threadVector);
        return SIMPLEHIDE_SUCCESS;
    }

    free(threadData);

    if (DISABLE_MULTITHREADING) {
        extractingThreadNumber = 1;
    }

    threadData = malloc(sizeof(CollectMaskedDataThreadData) * extractingThreadNumber);
    hiddenData = (uint8_t *)malloc(hiddenDataSize);

    totalBytes = hiddenDataSize;
    bytesPerThread = totalBytes / extractingThreadNumber;
    bytesPerThreadRemainder = totalBytes % extractingThreadNumber;
    threadStartBytePosition = 0;
    threadStopBytePosition = threadStartBytePosition + bytesPerThread;

    if (bytesPerThreadRemainder > 0) {
        threadStopBytePosition += 1;
        bytesPerThreadRemainder--;
    }

    for (int i = 0; i < extractingThreadNumber; i++) {
        ((CollectMaskedDataThreadData *)threadData)[i].startPosition    = threadStartBytePosition;
        ((CollectMaskedDataThreadData *)threadData)[i].stopPosition     = threadStopBytePosition;
        ((CollectMaskedDataThreadData *)threadData)[i].hiddenMaskedData = hiddenMaskedData;
        ((CollectMaskedDataThreadData *)threadData)[i].hiddenData       = hiddenData;
        ((CollectMaskedDataThreadData *)threadData)[i].bitsInMask       = bitsInMask;

        pthread_create(&threadVector[i], NULL, &extract_hidden_data_collect_hidden_bytes_thread, &((CollectMaskedDataThreadData *)threadData)[i]);

        threadStartBytePosition = threadStopBytePosition;
        threadStopBytePosition += bytesPerThread;
        if (bytesPerThreadRemainder > 0) {
            threadStopBytePosition += 1;
            bytesPerThreadRemainder--;
        }
    }

    for (int i = 0; i < extractingThreadNumber; i++) {
        pthread_join(threadVector[i], NULL);
        if (extractVerboseLevel >= 3) {
            printf("Thread %d finished collecting masked data\n", i);
        }
    }

    memcpy(hiddenMaskedData, hiddenData, hiddenDataSize);

    if (DISABLE_MULTITHREADING) {
        extractingThreadNumber = storedThreads;
    }

    free(hiddenData);
    free(threadData);
    free(threadVector);

    return SIMPLEHIDE_SUCCESS;
}


static void *extract_hidden_data_extract_masked_data_thread(void *data__) {
    ExtractMaskedDataThreadData *threadData = (ExtractMaskedDataThreadData *)data__;
    const uint8_t *rawData        = threadData->rawData;
    size_t startPosition          = threadData->startPosition;
    size_t stopPosition           = threadData->stopPosition;
    uint8_t *hiddenMaskedData     = threadData->hiddenMaskedData;
    uint8_t mask                  = threadData->mask;
    StegahideMaskType maskType    = threadData->maskType;
    int uniformMaskShift          = threadData->uniformMaskShift;
    size_t stepSize               = threadData->stepSize;

    for (size_t i = startPosition; i < stopPosition; i++) {
        size_t currentPosition = (i + 1) * stepSize;

        hiddenMaskedData[i] = rawData[currentPosition] & mask;

        if (extractVerboseLevel >= 4 && extractingThreadNumber == 1) {
            printf("Extracted masked data [%ld] at [%ld]: 0x%02x\n", i, currentPosition + BITS_SIZE_T, hiddenMaskedData[i]);
        }
    }

    if (mask == 0xff) {
        return NULL;
    }

    if (maskType == SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM) {
        for (size_t i = startPosition; i < stopPosition; i++) {
            uint8_t maskedByteBefore = hiddenMaskedData[i];
            hiddenMaskedData[i] >>= uniformMaskShift;
            if (extractVerboseLevel >= 4 && extractingThreadNumber == 1) {
                printf("Masked data [%ld]: 0x%02x -> 0x%02x after shift\n", i, maskedByteBefore, hiddenMaskedData[i]);
            }
        }
    } else if (maskType == SIMPLEHIDE_MASK_TYPE_COMPLEX) {
        for (size_t i = startPosition; i < stopPosition; i++) {
            uint8_t maskedByteBefore = hiddenMaskedData[i];
            hiddenMaskedData[i] = complex_extract_remask_byte(hiddenMaskedData[i], mask);
            if (extractVerboseLevel >= 4 && extractingThreadNumber == 1) {
                printf("Masked data [%ld]: 0x%02x -> 0x%02x after remasking\n", i, maskedByteBefore, hiddenMaskedData[i]);
            }
        }
    }

    return NULL;
}


static void *extract_hidden_data_collect_hidden_bytes_thread(void *data__) {
    CollectMaskedDataThreadData *threadData = (CollectMaskedDataThreadData *)data__;
    size_t startPosition            = threadData->startPosition;
    size_t stopPosition             = threadData->stopPosition;
    const uint8_t *hiddenMaskedData = threadData->hiddenMaskedData;
    uint8_t *hiddenData             = threadData->hiddenData;
    int bitsInMask                  = threadData->bitsInMask;

    for (size_t i = startPosition; i < stopPosition; i++) {
        if (extractVerboseLevel >= 4 && extractingThreadNumber == 1) {
            printf("Collecting byte [%ld]:", i);
            for (int j = 0; j < (8 / bitsInMask); j++) {
                if (extractVerboseLevel >= 4) {
                    printf(" 0x%02x", hiddenMaskedData[i * bitsInMask + j]);
                }
            }
        }

        hiddenData[i] = collect_hidden_byte(&hiddenMaskedData[i * (8 / bitsInMask)], bitsInMask);

        if (extractVerboseLevel >= 4 && extractingThreadNumber == 1) {
            printf(" -> 0x%02x\n", hiddenMaskedData[i]);
        }
    }
    return NULL;
}


static uint8_t complex_extract_remask_byte(uint8_t inputByte, uint8_t mask) {
    static uint8_t lookupTableInitialized = 0;
    static uint8_t *lookupTableExtract = NULL;

    if (!lookupTableInitialized) {
        get_lookup_tables(mask, &lookupTableInitialized, NULL, &lookupTableExtract);
    }

    return lookupTableExtract[inputByte];
}


static uint8_t collect_hidden_byte(const uint8_t *bytes, int bitsInMask) {
    uint8_t retval = bytes[0];
    for (int i = 0; i < (8 / bitsInMask); i++) {
        retval |= bytes[i] << (i * bitsInMask);
    }
    return retval;
}
