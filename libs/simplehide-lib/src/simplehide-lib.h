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

#ifndef __SIMPLEHIDE_LIB_H__
#define __SIMPLEHIDE_LIB_H__


#include <stdint.h>


#define SIMPLEHIDE_LIB_VERSION_MAJOR 0
#define SIMPLEHIDE_LIB_VERSION_MINOR 3
#define SIMPLEHIDE_LIB_VERSION_PATCH 0


#define BITS_SIZE_T (sizeof(size_t) * 8)

#define DISABLE_MULTITHREADING 0


typedef enum {
    SIMPLEHIDE_UNKNOWN_STATUS          = -1,
    SIMPLEHIDE_SUCCESS                 =  0,
    SIMPLEHIDE_INVALID_DATA            =  1,
    SIMPLEHIDE_INVALID_MASK            =  2,
    SIMPLEHIDE_MASK_EMBED_ERROR        =  3,
    SIMPLEHIDE_SIZE_EMBED_ERROR        =  4,
    SIMPLEHIDE_MEMORY_ALLOCATION_ERROR =  5,
} StegahideStatus;


typedef enum {
    SIMPLEHIDE_MASK_TYPE_UNKNOWN         = 0,
    SIMPLEHIDE_MASK_TYPE_SIMPLE_UNIFORM  = 1, // e.g. 0x01, 0x03, 0x0F, 0xFF
    SIMPLEHIDE_MASK_TYPE_SHIFTED_UNIFORM = 2, // e.g. 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                                              //      0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0,
                                              //      0x1E, 0x3C, 0x78, 0xF0
    SIMPLEHIDE_MASK_TYPE_COMPLEX         = 3, // e.g. every else that is allowed
} StegahideMaskType;

void set_hide_verbose_level(int level);
void set_hide_thread_number(int num);
void set_extract_verbose_level(int level);
void set_extract_thread_number(int num);

// Hiding hidden data:
StegahideStatus embed_hidden_data_size(uint8_t *rawData, size_t rawDataSize, size_t hiddenRawDataSize);
size_t find_hidden_data_size_position(const uint8_t *rawData, size_t rawDataSize, size_t hiddenRawDataSize);
StegahideStatus get_hidden_data_masked_size(uint8_t mask, size_t hiddenRawDataSize, size_t *maskedHiddenDataSize);
StegahideStatus verify_mask(uint8_t mask, size_t hiddenRawDataSize, size_t *maskedHiddenDataSize);
StegahideStatus generate_masked_hidden_data(const uint8_t *hiddenRawData, size_t hiddenRawDataSize, uint8_t *hiddenMaskedData, size_t hiddenMaskedDataSize, uint8_t mask);
int get_uniform_mask_shift(uint8_t mask);
StegahideMaskType get_mask_type(uint8_t mask);
void get_lookup_tables(uint8_t mask, uint8_t *lookupTableInitialized__, uint8_t **lookupTableHide__, uint8_t **lookupTableExtract__);
StegahideStatus hide_mask(uint8_t *rawData, uint8_t mask);
StegahideStatus hide_data(uint8_t *rawData,
                          size_t rawDataSize,
                          const uint8_t *hiddenMaskedData,
                          size_t hiddenMaskedDataSize,
                          uint8_t mask);


// Extracting hidden data:
uint8_t extract_mask(const uint8_t *rawData, size_t rawDataSize);
size_t extract_hidden_data_size(const uint8_t *rawData, size_t rawDataSize);
StegahideStatus extract_hidden_data(const uint8_t *rawData,
                                    size_t rawDataSize,
                                    uint8_t *hiddenMaskedData,
                                    size_t hiddenDataSize,
                                    size_t hiddenMaskedDataSize,
                                    uint8_t mask);

typedef struct HideDataSizeThreadData {
    const uint8_t *rawData;
    size_t hiddenRawDataSize;
    size_t startPosition;
    size_t stopPosition;
    size_t correctBits;
    size_t position;
} HideDataSizeThreadData;

typedef struct GenerateMaskedHiddenDataThreadData {
    const uint8_t *rawData;
    size_t startPosition;
    size_t stopPosition;
    uint8_t *hiddenMaskedData;
    uint8_t mask;
    StegahideMaskType maskType;
    size_t split;
    int shiftSize;
} GenerateMaskedHiddenDataThreadData;

typedef struct HideDataThreadData {
    uint8_t *rawData;
    size_t startPosition;
    size_t stopPosition;
    const uint8_t *hiddenMaskedData;
    uint8_t mask;
    size_t stepSize;
} HideDataThreadData;


typedef struct ExtractMaskedDataThreadData {
    const uint8_t *rawData;
    size_t startPosition;
    size_t stopPosition;
    uint8_t *hiddenMaskedData;
    uint8_t mask;
    StegahideMaskType maskType;
    int uniformMaskShift;
    size_t stepSize;
} ExtractMaskedDataThreadData;

typedef struct CollectMaskedDataThreadData {
    size_t startPosition;
    size_t stopPosition;
    const uint8_t *hiddenMaskedData;
    uint8_t *hiddenData;
    int bitsInMask;
} CollectMaskedDataThreadData;


#endif // __SIMPLEHIDE_H__
