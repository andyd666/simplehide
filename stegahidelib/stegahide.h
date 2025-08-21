#ifndef __STEGAHIDE_H__
#define __STEGAHIDE_H__

//#include <string>
//#include <stdint.h>


#ifndef DEBUG
#define DEBUG false
#endif

#if DEBUG == true
#define trace() std::cout << __LINE__ << " " << __func__  << " trace"<< std::endl
#else
#define trace() do {} while (0)
#endif


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

// Embedding data:
size_t embed_hidden_data_size(uint8_t *data, size_t dataSize, size_t hiddenDataSize);


#endif // __STEGAHIDE_H__
