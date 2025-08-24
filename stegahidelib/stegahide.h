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

void set_embedding_verbose_level(int level);

// Embedding data:
StegahideStatus embed_hidden_data_size(uint8_t *data, size_t *sizePosition, size_t dataSize, size_t hiddenDataSize);


#endif // __STEGAHIDE_H__
