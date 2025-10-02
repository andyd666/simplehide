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

#include <iostream>
#include <string>
#include "simplehide.h"


static SimpleHide simpleHide;

static int parse_args(int argc, const char *argv[]);
static void usage();
static void print_warranty_header();
static void print_versions();
static const char *get_version();
static const char *get_lib_version();


int main(int argc, const char *argv[]) {
    print_warranty_header();
    int retArgc = parse_args(argc, argv);
    StegahideStatus status = SIMPLEHIDE_UNKNOWN_STATUS;

    if (argc != retArgc) {
        return -1;
    }
    TRACE();

    if (simpleHide.inputFile.fullName.length() == 0) {
        std::cout << DEBUG_PRINT_LINE << "Error: No input file specified" << std::endl;
        return -1;
    }
    TRACE();

    if (!simpleHide.extractData && simpleHide.secretFile.fullName.length() == 0) {
        std::cout << DEBUG_PRINT_LINE << "Error: No secret file specified" << std::endl;
        return -1;
    }
    TRACE();

    if (simpleHide.outputFile.fullName.length() == 0) {
        simpleHide.outputFile.baseName = simpleHide.inputFile.baseName + "_output";
        simpleHide.outputFile.formatName = simpleHide.extractData ? "bin" :simpleHide.inputFile.formatName;
        simpleHide.outputFile.fullName = simpleHide.outputFile.baseName + "." + simpleHide.outputFile.formatName;
        std::cout << DEBUG_PRINT_LINE << "No output file specified, " << simpleHide.outputFile.fullName << " will be used." << std::endl;
    }


    if (!simpleHide.is_read_file_format(simpleHide.inputFile.formatName)) {
        std::cout << DEBUG_PRINT_LINE << "Error: Input file format '" << simpleHide.inputFile.formatName << "' is not supported for reading" << std::endl;
        return -1;
    }
    TRACE();

    if (!simpleHide.readInputFile()) {
        std::cout << DEBUG_PRINT_LINE << "Error: Could not read input file '" << simpleHide.inputFile.fullName << "'" << std::endl;
        return -1;
    }
    TRACE();


    if (!simpleHide.extractData) {
        if (simpleHide.mask == 0x00) {
            simpleHide.mask = 0x01;
            std::cout << DEBUG_PRINT_LINE << "No mask specified, 0x01 will be used." << std::endl;
        }

        if (!simpleHide.is_write_file_format(simpleHide.outputFile.formatName)) {
            std::cout << DEBUG_PRINT_LINE << "Error: Output file format '" << simpleHide.outputFile.formatName << "' is not supported for writing" << std::endl;
            return -1;
        }
        TRACE();

        if (!simpleHide.readSecretFile()) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not read secret file '" << simpleHide.secretFile.fullName << "'" << std::endl;
            return -1;
        }
        TRACE();

        if (simpleHide.embedSize) {
            status = embed_hidden_data_size(simpleHide.inputFileData.data(),
                                            &simpleHide.hiddenDataSizePosition,
                                            simpleHide.inputFileData.size(),
                                            simpleHide.secretFileData.size());
        } else {
            simpleHide.hiddenDataSizePosition = BITS_SIZE_T;
        }

        if ((status != SIMPLEHIDE_SUCCESS) || (simpleHide.hiddenDataSizePosition > simpleHide.inputFileData.size())) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not embed hidden data size" << std::endl;
            return -1;
        }
        TRACE();

        if (simpleHide.embedMask) {
            status = hide_mask(simpleHide.inputFileData.data(), simpleHide.mask);
        }

        if (status != SIMPLEHIDE_SUCCESS) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not embed mask" << std::endl;
            return -1;
        }
        TRACE();

        size_t maskedHiddenDataSize = 0;
        status = get_hidden_data_masked_size(simpleHide.mask, simpleHide.secretFileData.size(), &maskedHiddenDataSize);

        if (status != SIMPLEHIDE_SUCCESS) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not get masked hidden data size" << std::endl;
            return -1;
        }
        TRACE();

        simpleHide.secretRemaskedData.resize(maskedHiddenDataSize);

        status = generate_masked_hidden_data(simpleHide.secretFileData.data(),
                                             simpleHide.secretFileData.size(),
                                             simpleHide.secretRemaskedData.data(),
                                             simpleHide.secretRemaskedData.size(),
                                             simpleHide.mask);

        if (status != SIMPLEHIDE_SUCCESS) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not generate masked hidden data" << std::endl;
            return -1;
        }
        TRACE();

        status = hide_data(simpleHide.inputFileData.data(),
                           simpleHide.inputFileData.size(),
                           simpleHide.secretRemaskedData.data(),
                           simpleHide.secretRemaskedData.size(),
                           simpleHide.mask,
                           simpleHide.hiddenDataSizePosition);

        if (status != SIMPLEHIDE_SUCCESS) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not hide data" << std::endl;
            return -1;
        }

        if (!simpleHide.writeOutputFile()) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not write output file '" << simpleHide.outputFile.fullName << "'" << std::endl;
            return -1;
        }
        TRACE();

    } else {
        size_t extractedHiddenMaskedDataSize = 0;
        size_t extractedHiddenDataSizePosition = 0;

        if (simpleHide.mask == 0x00) {
            simpleHide.mask = extract_mask(simpleHide.inputFileData.data(), simpleHide.inputFileData.size());
        }

        if (simpleHide.secretFileData.size() == 0) {
            size_t extractedHiddenRawDataSize = 0;

            extractedHiddenDataSizePosition = extract_hidden_data_size_position(simpleHide.inputFileData.data(), simpleHide.inputFileData.size());

            if ((extractedHiddenDataSizePosition < BITS_SIZE_T) || (extractedHiddenDataSizePosition > simpleHide.inputFileData.size())) {
                std::cout << DEBUG_PRINT_LINE << "Error: Invalid extracted hidden data size position: " << extractedHiddenDataSizePosition << std::endl;
                return -1;
            }
            TRACE();

            extractedHiddenRawDataSize = extract_hidden_data_size(simpleHide.inputFileData.data(),
                                                                  simpleHide.inputFileData.size(),
                                                                  extractedHiddenDataSizePosition);
            if ((extractedHiddenRawDataSize == 0) || (extractedHiddenRawDataSize > simpleHide.inputFileData.size() - BITS_SIZE_T * 2)) {
                std::cout << DEBUG_PRINT_LINE << "Error: Invalid extracted hidden data size: " << extractedHiddenRawDataSize << std::endl;
                return -1;
            }
            TRACE();

            simpleHide.secretFileData.resize(extractedHiddenRawDataSize);
        } else {
            // TODO: add function to get possible file size position from input data
        }

        status = verify_mask(simpleHide.mask, simpleHide.secretFileData.size(), &extractedHiddenMaskedDataSize);
        if (status != SIMPLEHIDE_SUCCESS) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not verify mask" << std::endl;
            return -1;
        }
        TRACE();

        simpleHide.secretRemaskedData.resize(extractedHiddenMaskedDataSize);
        status = extract_hidden_data(simpleHide.inputFileData.data(),
                                     simpleHide.inputFileData.size(),
                                     simpleHide.secretRemaskedData.data(),
                                     simpleHide.secretFileData.size(),
                                     simpleHide.secretRemaskedData.size(),
                                     extractedHiddenDataSizePosition,
                                     simpleHide.mask);

        if (status != SIMPLEHIDE_SUCCESS) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not extract hidden data" << std::endl;
            return -1;
        }
        TRACE();

        simpleHide.inputFileData.clear();
        simpleHide.inputFileData.shrink_to_fit();
        simpleHide.secretRemaskedData.resize(simpleHide.secretFileData.size());
        simpleHide.secretRemaskedData.shrink_to_fit();

        simpleHide.inputFileData = simpleHide.secretRemaskedData;
        simpleHide.inputFile.fullName = simpleHide.secretFile.fullName;

        if (!simpleHide.writeOutputFile()) {
            std::cout << DEBUG_PRINT_LINE << "Error: Could not write output file '" << simpleHide.outputFile.fullName << "'" << std::endl;
            return -1;
        }
        TRACE();
    }

    return 0;
}


static int parse_args(int argc, const char *argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }
    TRACE();

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
            usage();
            return 1;
        } else if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "--version") == 0)) {
            print_versions();
            return 1;
        } else if ((strcmp(argv[i], "-V") == 0) || (strcmp(argv[i], "--verbose") == 0)) {
            i++;
            if (i < argc) {
                simpleHide.set_verbose_level(atoi(argv[i]));
            } else {
                simpleHide.set_verbose_level(1);
            }
        }
    }
    TRACE();

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-V") == 0) || (strcmp(argv[i], "--verbose") == 0)) {
            i++;
            // already parsed
        } else if (strcmp(argv[i], "-j") == 0) {
            i++;
            if (i < argc) {
                simpleHide.set_thread_num(atoi(argv[i]));
            } else {
                std::cout << DEBUG_PRINT_LINE << "Error: No thread number specified. Using 1 thread" << std::endl;
                simpleHide.set_thread_num(1);
            }
            TRACE();
        } else if ((strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "--input") == 0)) {
            i++;
            if (i < argc) {
                simpleHide.parse_file_full_name(argv[i], simpleHide.inputFile);
            } else {
                std::cout << DEBUG_PRINT_LINE << "Error: No input file specified" << std::endl;
                return 1;
            }
            TRACE();
        } else if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "--output") == 0)) {
            i++;
            if (i < argc) {
                simpleHide.parse_file_full_name(argv[i], simpleHide.outputFile);
            } else {
                std::cout << DEBUG_PRINT_LINE << "Error: No output file specified" << std::endl;
                return 1;
            }
            TRACE();
        } else if ((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "--secret") == 0)) {
            i++;
            if (i < argc) {
                simpleHide.parse_file_full_name(argv[i], simpleHide.secretFile);
            } else {
                std::cout << DEBUG_PRINT_LINE << "Error: No secret file specified" << std::endl;
                return 1;
            }
            TRACE();
        } else if ((strcmp(argv[i], "-m") == 0) || (strcmp(argv[i], "--mask") == 0)) {
            i++;
            if (i < argc) {
                simpleHide.mask = (uint8_t)strtol(argv[i], nullptr, 16);
            } else {
                std::cout << DEBUG_PRINT_LINE << "Error: No mask specified" << std::endl;
                return 1;
            }
            TRACE();
        } else if ((strcmp(argv[i], "-e") == 0) || (strcmp(argv[i], "--extract") == 0)) {
            simpleHide.extractData = true;
            TRACE();
        } else if ((strcmp(argv[i], "--no-mask") == 0)) {
            simpleHide.embedMask = false;
            TRACE();
        } else if ((strcmp(argv[i], "--no-size") == 0)) {
            simpleHide.embedSize = false;
            TRACE();
        } else if (strcmp(argv[i], "--secret-size") == 0) {
            i++;
            if (i < argc) {
                size_t hiddenRawDataSize = (size_t)strtoull(argv[i], nullptr, 10);
                simpleHide.secretFileData.resize(hiddenRawDataSize);
                if (hiddenRawDataSize != simpleHide.secretFileData.size()) {
                    std::cout << DEBUG_PRINT_LINE << "Error: Invalid secret size specified: " << hiddenRawDataSize << std::endl;
                    return 1;
                }
                TRACE();
            } else {
                std::cout << DEBUG_PRINT_LINE << "Error: No secret size specified" << std::endl;
                return 1;
            }
            TRACE();
        } else {
            std::cout << DEBUG_PRINT_LINE << "Error: Unknown argument: " << argv[i] << std::endl;
            return 1;
        }
    }
    TRACE();

    return argc;
}


static void usage() {
    std::cout << std::endl;
    std::cout << "Usage: simplehide <options>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "    -h, --help               Show this help message" << std::endl;
    std::cout << "    -v, --version            Show version information" << std::endl;
    std::cout << "    -V, --verbose            Enable verbose output with levels 0-4 (0 is default)." << std::endl;
    std::cout << "                             If this argument is last - level 1 will be chosen" << std::endl;
    std::cout << "    -j                       Set thread number" << std::endl;
    std::cout << "    -e, --extract            Extract hidden data" << std::endl;
    std::cout << "    -i, --input              Input file" << std::endl;
    std::cout << "    -o, --output             Output file" << std::endl;
    std::cout << "    -s, --secret             Secret file. Only for hiding data" << std::endl;
    std::cout << "    -m, --mask               Mask (hexadecimal)" << std::endl;
    std::cout << "        --no-mask            Do not embed mask" << std::endl;
    std::cout << "        --no-size            Do not embed size" << std::endl;
    std::cout << "        --secret-size        Size of secret data in bytes. Only for extracting data" << std::endl;
    std::cout << std::endl;
}


static void print_warranty_header() {
    std::cout << "Copyright (c) 2025 andyd666" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "This software is provided 'as-is', without any express or implied" << std::endl;
    std::cout << "warranty. In no event will the authors be held liable for any damages" << std::endl;
    std::cout << "arising from the use of this software." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Permission is granted to anyone to use this software for any purpose," << std::endl;
    std::cout << "including commercial applications, and to alter it and redistribute it" << std::endl;
    std::cout << "freely, subject to the following restrictions:" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "    1. The origin of this software must not be misrepresented; you must not" << std::endl;
    std::cout << "    claim that you wrote the original software. If you use this software" << std::endl;
    std::cout << "    in a product, an acknowledgment in the product documentation would be" << std::endl;
    std::cout << "    appreciated but is not required." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "    2. Altered source versions must be plainly marked as such, and must not be" << std::endl;
    std::cout << "    misrepresented as being the original software." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "    3. This notice may not be removed or altered from any source" << std::endl;
    std::cout << "    distribution." << std::endl;
}


static void print_versions() {
    std::cout << "Simplehide ................... v" << get_version() << std::endl;
    std::cout << "Simplehide-lib ............... v" << get_lib_version() << std::endl;
    std::cout << "LodePNG ...................... v" << LODEPNG_VERSION_STRING << std::endl;
}


static const char *get_version() {
    static std::string version = "";

    if (version.length() == 0) {
        version = std::to_string(SIMPLEHIDE_VERSION_MAJOR) + "." +
                  std::to_string(SIMPLEHIDE_VERSION_MINOR) + "." +
                  std::to_string(SIMPLEHIDE_VERSION_PATCH);
    }

    return version.c_str();
}


static const char *get_lib_version() {
    static std::string version = "";

    if (version.length() == 0) {
        version = std::to_string(SIMPLEHIDE_LIB_VERSION_MAJOR) + "." +
                  std::to_string(SIMPLEHIDE_LIB_VERSION_MINOR) + "." +
                  std::to_string(SIMPLEHIDE_LIB_VERSION_PATCH);
    }

    return version.c_str();
}


void SimpleHide::parse_file_full_name(std::string fileFullName, FileInfo &fileInfo) {
    size_t dotPosition = fileFullName.find_last_of(".");

    fileInfo.fullName = fileFullName;

    if (dotPosition != std::string::npos) {
        fileInfo.baseName = fileFullName.substr(0, dotPosition);
        fileInfo.formatName = fileFullName.substr(dotPosition + 1);
    } else {
        fileInfo.baseName = fileFullName;
        fileInfo.formatName = "";
    }
}


bool SimpleHide::is_read_file_format(std::string format) {
    return is_read_file_format(get_format(format));
}


bool SimpleHide::is_read_file_format(SimpleHide::ReadWriteFormats format) {
    switch (format) {
        case SimpleHide::RW_FORMAT_IMAGE_PNG:
        case SimpleHide::RW_FORMAT_BINARY:
        case SimpleHide::RW_FORMAT_TEXT_TXT:
            return true;
        default:
            // Else are false because they are not supported yet
            return false;
    }
}


bool SimpleHide::is_write_file_format(std::string format) {
    return is_write_file_format(get_format(format));
}


bool SimpleHide::is_write_file_format(SimpleHide::ReadWriteFormats format) {
    switch (format) {
        case SimpleHide::RW_FORMAT_IMAGE_PNG:
        case SimpleHide::RW_FORMAT_BINARY:
        case SimpleHide::RW_FORMAT_TEXT_TXT:
            return true;
        default:
            // Else are false because they are not supported yet
            return false;
    }
}


SimpleHide::ReadWriteFormats SimpleHide::get_format(std::string format) {
    if (format == "png")
        return SimpleHide::RW_FORMAT_IMAGE_PNG;
    else if (format == "jpeg" || format == "jpg")
        return SimpleHide::Rx_FORMAT_IMAGE_JPEG;
    else if (format == "bmp")
        return SimpleHide::RW_FORMAT_IMAGE_BMP;
    else if (format == "tiff" || format == "tif")
        return SimpleHide::RW_FORMAT_IMAGE_TIFF;
    else if (format == "raw")
        return SimpleHide::RW_FORMAT_IMAGE_RAW;
    else if (format == "wav")
        return SimpleHide::RW_FORMAT_AUDIO_WAV;
    else if (format == "mp3")
        return SimpleHide::Rx_FORMAT_AUDIO_MP3;
    else if (format == "flac")
        return SimpleHide::RW_FORMAT_AUDIO_FLAC;
    else if (format == "alac")
        return SimpleHide::RW_FORMAT_AUDIO_ALAC;
    else if (format == "bin" || format == "")
        return SimpleHide::RW_FORMAT_BINARY;
    else if (format == "txt")
        return SimpleHide::RW_FORMAT_TEXT_TXT;

    return SimpleHide::xx_FORMAT_UNKNOWN;
}


bool SimpleHide::readInputFile() {
    ReadWriteFormats format = get_format(inputFile.formatName);
    switch (format) {
        case RW_FORMAT_IMAGE_PNG:
        {
            std::vector<uint8_t> inputFileBuffer;
            lodepng::State state;
            unsigned int retval;

            retval = lodepng::load_file(inputFileBuffer, inputFile.fullName);
            if (retval) {
                std::cout << DEBUG_PRINT_LINE << "Error: Could not load file '" << inputFile.fullName << "': " << lodepng_error_text(retval) << std::endl;
                return false;
            }
            TRACE();

            retval = lodepng::decode(inputFileData, width, height, state, inputFileBuffer);
            if (retval) {
                std::cout << DEBUG_PRINT_LINE << "Error: Could not decode PNG file '" << inputFile.fullName << "': " << lodepng_error_text(retval) << std::endl;
                return false;
            }
            TRACE();

            if (verboseOutput) {
                std::cout << "Loaded " << inputFileBuffer.size() << " bytes from " << inputFile.fullName << std::endl;
                std::cout << "Decoded to " << inputFileData.size() << " bytes" << std::endl;
            }
            return true;
        }
        case SimpleHide::RW_FORMAT_BINARY:
        case SimpleHide::RW_FORMAT_TEXT_TXT:
        {
            unsigned int retval;

            retval = lodepng::load_file(inputFileData, inputFile.fullName);
            if (retval) {
                std::cout << DEBUG_PRINT_LINE << "Error: Could not load file '" << inputFile.fullName << "': " << lodepng_error_text(retval) << std::endl;
                return false;
            }
            TRACE();

            if (verboseOutput) {
                std::cout << "Loaded " << inputFileData.size() << " bytes from binary file " << inputFile.fullName << std::endl;
            }
            return true;
        }
        default:
            break;
    }

    std::cout << DEBUG_PRINT_LINE << "Error: Input file format '" << inputFile.formatName << "' is not supported for reading" << std::endl;

    return false;
}


bool SimpleHide::readSecretFile() {
    FILE *file = fopen(secretFile.fullName.c_str(), "rb");
    if (!file) {
        std::cout << DEBUG_PRINT_LINE << "Error: Could not open secret file '" << secretFile.fullName << "'" << std::endl;
        return false;
    }
    TRACE();

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize == 0) {
        std::cout << DEBUG_PRINT_LINE << "Error: Secret file '" << secretFile.fullName << "' is empty" << std::endl;
        fclose(file);
        return false;
    }
    TRACE();

    secretFileData.resize(fileSize);
    size_t bytesRead = fread(secretFileData.data(), 1, fileSize, file);
    fclose(file);

    if (bytesRead != fileSize) {
        std::cout << DEBUG_PRINT_LINE << "Error: Could not read secret file '" << secretFile.fullName << "'" << std::endl;
        return false;
    }
    TRACE();

    if (verboseOutput) {
        std::cout << "Loaded " << bytesRead << " bytes from " << secretFile.fullName << std::endl;
    }

    return true;
}


bool SimpleHide::writeOutputFile() {
    switch (get_format(outputFile.formatName)) {
        case RW_FORMAT_IMAGE_PNG:
        {
            std::vector<uint8_t> outputFileBuffer;
            lodepng::State state;
            unsigned int retval;

            retval = lodepng::encode(outputFileBuffer, inputFileData, width, height, state);
            if (retval) {
                std::cout << DEBUG_PRINT_LINE << "Error: Could not encode PNG file '" << outputFile.fullName << "': " << lodepng_error_text(retval) << std::endl;
                return false;
            }
            TRACE();

            retval = lodepng::save_file(outputFileBuffer, outputFile.fullName);
            if (retval) {
                std::cout << DEBUG_PRINT_LINE << "Error: Could not save file '" << outputFile.fullName << "': " << lodepng_error_text(retval) << std::endl;
                return false;
            }
            TRACE();

            break;
        }
        case SimpleHide::RW_FORMAT_BINARY:
        case SimpleHide::RW_FORMAT_TEXT_TXT:
        {
            unsigned int retval;

            retval = lodepng::save_file(inputFileData, outputFile.fullName);
            if (retval) {
                std::cout << DEBUG_PRINT_LINE << "Error: Could not save file '" << outputFile.fullName << "': " << lodepng_error_text(retval) << std::endl;
                return false;
            }
            TRACE();

            if (verboseOutput) {
                std::cout << "Saved " << inputFileData.size() << " bytes to binary file " << outputFile.fullName << std::endl;
            }

            break;
        }
        default:
            std::cout << DEBUG_PRINT_LINE << "Error: Output file format '" << outputFile.formatName << "' is not supported for writing" << std::endl;
            return false;
    }

    return true;
}
