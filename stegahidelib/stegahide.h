#ifndef __STEGAHIDE_H__
#define __STEGAHIDE_H__

//#include <string>
//#include <stdint.h>


typedef enum {
    STEGAHIDE_UNKNOWN_STATUS   = -2,
    STEGAHIDE_CONTINUE         = -1,
    STEGAHIDE_SUCCESS          =  0,
    STEGAHIDE_USAGE            =  1,
    STEGAHIDE_USAGE_ERROR      =  2,
    STEGAHIDE_NO_FILE          =  3,
    STEGAHIDE_FILE_EMPTY       =  4,
    STEGAHIDE_READ_ERROR       =  5,
    STEGAHIDE_WRITE_ERROR      =  6,
    STEGAHIDE_INVALID_DATA     =  7,
    STEGAHIDE_INVALID_MASK     =  8,
    STEGAHIDE_SIZE_EMBED_ERROR =  9,
} StegahideStatus;


typedef enum {
    STEGAHIDE_MASK_TYPE_UNKNOWN         = 0,
    STEGAHIDE_MASK_TYPE_SIMPLE_UNIFORM  = 1, // e.g. 0x01, 0x03, 0x0F, 0xFF
    STEGAHIDE_MASK_TYPE_SHIFTED_UNIFORM = 2, // e.g. 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                                             //      0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0,
                                             //      0x1E, 0x3C, 0x78, 0xF0
    STEGAHIDE_MASK_TYPE_COMPLEX         = 3, // e.g. every else that is allowed
} StegahideMaskType;

void set_hide_verbose_level(int level);
void set_extract_verbose_level(int level);

// Hiding hidden data:
StegahideStatus embed_hidden_data_size(uint8_t *rawData, size_t *sizePosition, size_t rawDataSize, size_t hiddenRawDataSize);
StegahideStatus get_hidden_data_masked_size(uint8_t mask, size_t hiddenRawDataSize, size_t *maskedHiddenDataSize);
StegahideStatus hide_data(uint8_t *rawData,
                          size_t rawDataSize,
                          uint8_t *hiddenMaskedData,
                          size_t hiddenMaskedDataSize,
                          uint8_t mask,
                          size_t dataSizeLocation);


// Extracting hidden data:
size_t extract_hidden_data_size_location(uint8_t *rawData, size_t rawDataSize);
size_t extract_hidden_data_size(uint8_t *rawData, size_t rawDataSize, size_t hiddenDataSizeLocation);


#endif // __STEGAHIDE_H__
