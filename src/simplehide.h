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

#ifndef __SIMPLEHIDE_H__
#define __SIMPLEHIDE_H__

#include "../libs/simplehide-lib/src/simplehide-lib.h"
#include "../libs/lodepng/lodepng.h"

#define UNUSED(x) (void)(x)


#define SIMPLEHIDE_VERSION_MAJOR 0
#define SIMPLEHIDE_VERSION_MINOR 2
#define SIMPLEHIDE_VERSION_PATCH 0

#if defined(DEBUG)
#define DEBUG_PRINT_LINE __LINE__ << ": " << __func__ << ": "
#define TRACE() std::cout << DEBUG_PRINT_LINE << "trace" << std::endl
#else
#define DEBUG_PRINT_LINE ""
#define TRACE()
#endif

class SimpleHide {
public:
    class FileInfo {
    public:
        std::string fullName = "";
        std::string baseName = "";
        std::string formatName = "";
    };

    typedef enum {
        xx_FORMAT_UNKNOWN        = 0,     //
        RW_FORMAT_IMAGE_PNG      = 1,     // [  SUPPORTED  ]
        Rx_FORMAT_IMAGE_JPEG     = 2,     //
        RW_FORMAT_IMAGE_BMP      = 3,     //
        RW_FORMAT_IMAGE_TIFF     = 4,     //
        RW_FORMAT_IMAGE_RAW      = 5,     //

        RW_FORMAT_AUDIO_WAV      = 20,    //
        Rx_FORMAT_AUDIO_MP3      = 21,    //
        RW_FORMAT_AUDIO_FLAC     = 22,    //
        RW_FORMAT_AUDIO_ALAC     = 23,    //
        RW_FORMAT_AUDIO_AAC      = 24,    //

        RW_FORMAT_TEXT_TXT       = 40,    // [  SUPPORTED  ]

        RW_FORMAT_BINARY         = 100,   // [  SUPPORTED  ]
    } ReadWriteFormats;

public:
    SimpleHide() {};
    ~SimpleHide() {};

    void set_verbose_level(int level) { set_extract_verbose_level(level); set_hide_verbose_level(level); verboseOutput = (level > 0); }
    void set_thread_num(int num) { set_extract_thread_number(num); set_hide_thread_number(num); }

    void parse_file_full_name(std::string inputFileFullName, FileInfo &fileInfo);
    static bool is_read_file_format(std::string format);
    static bool is_read_file_format(ReadWriteFormats format);
    static bool is_write_file_format(std::string format);
    static bool is_write_file_format(ReadWriteFormats format);
    static ReadWriteFormats get_format(std::string format);

    bool readInputFile();
    bool readSecretFile();
    bool writeOutputFile();

public:
    FileInfo inputFile;
    FileInfo outputFile;
    FileInfo secretFile;

    std::vector<uint8_t> inputFileData;
    std::vector<uint8_t> secretFileData;
    std::vector<uint8_t> secretRemaskedData;

    uint8_t mask = 0x00;

    bool embedSize = true;
    bool embedMask = true;
    bool extractData = false;
    bool binaryEmbedding = true;
    size_t hiddenDataSizePosition = (size_t)(-1);

private:
    bool verboseOutput = false;

    // For image files:
    unsigned int width = 0;
    unsigned int height = 0;
};



#endif /* __SIMPLEHIDE_H__ */
