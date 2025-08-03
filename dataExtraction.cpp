#include <fstream>
#include <string>
#include "stegahide.h"


int Steganotify::extractSecretData() {
    if (verboseOutput) {
        std::cout << "Extracting data from input file..." << std::endl;
    }

    if (rawBinary && false) {
        // TODO: Implement raw binary injection logic
    } else if (inputFileIsImage() && extractImageData() == STEGAHIDE_SUCCESS) {
        if (verboseOutput) {
            std::cout << "Image data extracted successfully." << std::endl;
        }
    } else {
        std::cout << "Error: Input file is not a valid image or could not be read." << std::endl;
        return STEGAHIDE_READ_ERROR;
    }

    if (extractDataSize() == STEGAHIDE_INVALID_DATA) {
        std::cout << "Error extracting secret data size." << std::endl;
        return STEGAHIDE_INVALID_DATA;
    }

    if (mask == 0x00) {
        if (verboseOutput) {
            std::cout << "Mask not set. Trying to calculate mask" << std::endl;
        }

        calculateMask();
        if (!verifyMask()) {
            std::cout << "Error: Invalid mask." << std::endl;
            return STEGAHIDE_INVALID_MASK;
        }
    } else if (!verifyMask()) {
        std::cout << "Error: Invalid mask." << std::endl;
        return STEGAHIDE_INVALID_MASK;
    }

    int bitsInMask = getMaskBits();
    embedDataMaskSequenceSize = secretDataSize * 8 / bitsInMask;

    uint32_t embeddingInputDataSize = inputDataSize - 32;
    uint8_t *embeddinginputData = inputData + 32;
    int stepSize = embeddingInputDataSize / (embedDataMaskSequenceSize + embedSpacerSize);

    if (verboseOutput) {
        std::cout << "Extracting secret data from input data..." << std::endl;
        std::cout << "Step size: " << stepSize << std::endl;
    }

    secretData = new uint8_t[secretDataSize];
    memset(secretData, 0, secretDataSize);

    std::unique_ptr<uint8_t[]> splittedData(new uint8_t[embedDataMaskSequenceSize]);

    for (uint32_t i = 0; i < embedDataMaskSequenceSize; ++i) {
        uint32_t location = (i + 1) * stepSize;
        if ((location >= sizeBitsLocationOffset) && (location < (sizeBitsLocationOffset + 32))) {
            location = sizeBitsLocationOffset + 32; // Skip the size bits location
        }
        splittedData[i] = embeddinginputData[location];
        if (fullVerboseOutput) {
            std::cout << "Extracted byte " << std::setw(3) << i << ": 0x" << std::hex << static_cast<int>(splittedData[i]) << std::dec << "; location " << location << std::endl;
        }
    }

    int embeddedparsedIndex = 0;
    for (uint32_t i = 0; i < secretDataSize; ++i) {
        int bitsParsed = 0;
        uint8_t byte = 0x00;
        for (int j = 0; j < 8 / bitsInMask; ++j) {
            for (int k = 0; k < 8; ++k) {
                if (mask & (1 << k)) {
                    byte |= ((splittedData[embeddedparsedIndex + j] >> k) & 0x01) << bitsParsed++;
                }
            }
        }
        secretData[i] = byte;
        if (fullVerboseOutput) {
            std::cout << "Extracted secret data byte " << std::setw(3) << i << ": 0x" << std::hex << static_cast<int>(secretData[i]) << std::dec << std::endl;
        }
        embeddedparsedIndex += 8 / bitsInMask;
    }

    
    std::ofstream secretFile(secretDataFilename, std::ios::binary);
    if (!secretFile) {
        std::cout << "Error: Could not open secret file for writing." << std::endl;
        return STEGAHIDE_WRITE_ERROR;
    }
    secretFile.write(reinterpret_cast<const char*>(secretData), secretDataSize);
    if (!secretFile) {
        std::cout << "Error: Failed to write secret data to file." << std::endl;
        return STEGAHIDE_WRITE_ERROR;
    }
    secretFile.close();
    if (verboseOutput) {
        std::cout << "Secret data written to file: " << secretDataFilename << std::endl;
    }
    return STEGAHIDE_SUCCESS;
}


int Steganotify::extractDataSize() {
    sizeBitsLocationOffset = 0;
    for (int i = 31; i >= 0; --i) {
        sizeBitsLocationOffset |= (inputData[i] & 0x01) << i;
        if (fullVerboseOutput) {
            std::cout << "Extracted data size location bit " << i << ": " << (inputData[i] & 0x01) << "; sizeBitsLocationOffset = 0x";
            std::cout << std::hex << std::setfill('0') << std::setw(8) << sizeBitsLocationOffset << std::dec << std::endl;
        }
    }

    if (verboseOutput) {
        std::cout << "Extracting secret data size..." << std::endl;
        std::cout << "Secret data size offset: " << sizeBitsLocationOffset << " bytes" << std::endl;
    }

    if (sizeBitsLocationOffset > inputDataSize) {
        std::cout << "Error: Size bits location offset exceeds input data size." << std::endl;
        return STEGAHIDE_INVALID_DATA;
    }

    secretDataSize = 0;
    for (int i = 31; i >= 0 ; --i) {
        secretDataSize |= ((inputData[sizeBitsLocationOffset + i] & 0x01) << i);
        if (fullVerboseOutput) {
            std::cout << "Secret data size bit " << sizeBitsLocationOffset + i << ": " << (inputData[sizeBitsLocationOffset + i] & 0x01) << std::endl;
        }
    }

    if (verboseOutput) {
        std::cout << "Secret data size: " << secretDataSize << " bytes" << std::endl;
    }

    return STEGAHIDE_SUCCESS;
}
