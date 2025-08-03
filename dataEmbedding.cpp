#include <string>
#include "stegahide.h"

int Steganotify::processInputData() {
    if (verboseOutput) {
        std::cout << "Processing input data..." << std::endl;
        std::cout << "Input data size:  " << inputDataSize << " bytes" << std::endl;
        std::cout << "Secret data size: " << secretDataSize << " bytes" << std::endl;
    }

    if (secretDataSize > inputDataSize) {
        std::cout << "Error: Secret data size (" << secretDataSize << " bytes) exceeds input data size (" << inputDataSize << " bytes)." << std::endl;
        return STEGAHIDE_INVALID_DATA;
    }

    if (mask == 0x00) {
        if (verboseOutput) {
            std::cout << "Mask not set. Calculating default mask" << std::endl;
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

    for (uint32_t i = 0; i < 32; ++i) {
        secretSizeMaskSequence[i] = (secretDataSize >> i) & 1;
    }

    if (verboseOutput) {
        std::cout << "Using mask: 0x" << std::hex << static_cast<int>(mask) << std::dec << std::endl;
        std::cout << "Secret data size bits: ";
        for (int i = 31; i >= 0; --i) {
            std::cout << static_cast<int>(secretSizeMaskSequence[i]);
        }
        std::cout << std::endl;
    }

    embedDataSizeLocation();
    genarateDataMaskSequence();
    embedSecretData();

    return STEGAHIDE_SUCCESS;
}


void Steganotify::calculateMask() {
    int dataRatio = inputDataSize / secretDataSize;
    int bits = 0;

    if (dataRatio > 8) {
        bits = 1;
    } else if (dataRatio > 4) {
        bits = 2;
    } else if (dataRatio > 2) {
        bits = 4;
    } else {
        mask = 0x00;
    }

    if (verboseOutput) {
        std::cout << "Data ratio: " << dataRatio << std::endl;
        std::cout << "Optimal bits to use: " << bits << std::endl;
    }

    int input1s[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int input0s[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int secret1s = 0;
    int secret0s = 0;

    for (uint32_t i = 0; i < inputDataSize; ++i) {
        for (int j = 0; j < 8; ++j) {
            (inputData[i] & (1 << j)) != 0 ? input1s[j]++ : input0s[j]++;
        }
    }

    for (uint32_t i = 0; i < secretDataSize; ++i) {
        for (int j = 0; j < 8; ++j) {
            (secretData[i] & (1 << j)) != 0 ? secret1s++ : secret0s++;
        }
    }


    if (verboseOutput) {
        std::cout << "Input data 1s:  ";
        for (int i = 7; i >= 0; --i) {
            std::cout << std::setw(10) << input1s[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "Input data 0s:  ";
        for (int i = 7; i >= 0; --i) {
            std::cout << std::setw(10) << input0s[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "Secret data 1s: " << std::setw(10) << secret1s << std::endl;
        std::cout << "Secret data 0s: " << std::setw(10) << secret0s << std::endl;
    }


    double appealFactor[8] = {0.0};
    double secretRatio = static_cast<double>(secret1s) / (secret1s + secret0s);

    for (int i = 0; i < 8; ++i) {
        double inputRatio = static_cast<double>(input1s[i]) / (input1s[i] + input0s[i]);
        appealFactor[i] = abs(inputRatio - secretRatio) * static_cast<double>(1 << i);

        if (verboseOutput) {
            std::cout << "Appeal factor for bit " << i << ": " << appealFactor[i] << std::endl;
        }
    }

    int minIndex = 0;
    double minAppeal = appealFactor[minIndex];
    for (int bit = 0; bit < bits; ++bit) {
        minAppeal = appealFactor[minIndex + 1];
        for (int i = minIndex + 1; i < 8; ++i) {
            if (appealFactor[i] < minAppeal) {
                minAppeal = appealFactor[i];
                minIndex = i;
            }
        }
        mask |= (1 << minIndex);
    }
    if (verboseOutput) {
        std::cout << "Calculated mask: 0x" << std::hex << static_cast<int>(mask) << std::dec << std::endl;
    }
}


bool Steganotify::verifyMask() {
    if (mask == 0x00) {
        std::cout << "Error: Mask is not set." << std::endl;
        return false;
    }

    int setBits = 0;
    int dataRatio = inputDataSize / secretDataSize;
    int minbits = 0;

    setBits = getMaskBits();

    if (dataRatio > 8) {
        minbits = 1;
    } else if (dataRatio > 4) {
        minbits = 2;
    } else if (dataRatio > 2) {
        minbits = 4;
    } else {
        std::cout << "Error: Data-to-Secret ratio is too low for embedding." << std::endl;
        mask = 0x00;
        return false;
    }

    if (verboseOutput) {
        std::cout << "Mask is set to use " << setBits << " bit." << std::endl;
        std::cout << "Data-to-Secret ratio: " << dataRatio << std::endl;
        std::cout << "Minimum bits required: " << minbits << std::endl;
    }

    return setBits >= minbits;
}


void Steganotify::embedDataSizeLocation() {
    bool perfectFitFound = false;
    int maxCorrectBits = 0;
    for (uint32_t i = defaultOffset; i < inputDataSize - 32; ++i) {
        int correctBits = 0;
        for (uint32_t j = 0; j < 32; ++j) {
            correctBits += secretSizeMaskSequence[j] == (1 & inputData[i + j]);
        }
        if (correctBits > maxCorrectBits) {
            maxCorrectBits = correctBits;
            sizeBitsLocationOffset = i;
        }
        if (correctBits == 32) {
            perfectFitFound = true;
            break; // Found a perfect match
        }
    }
    if (verboseOutput) {
        std::cout << "Best location for embedding found at offset: " << sizeBitsLocationOffset << std::endl;
        std::cout << "Max correct bits: " << maxCorrectBits << std::endl;
    }

    int bestLocationOffsetMask[32] = {0};
    for (uint32_t i = 0; i < 32; ++i) {
        bestLocationOffsetMask[i] = (sizeBitsLocationOffset >> i) & 1;
    }

    for (uint32_t i = 0; i < 32; ++i) {
        if (fullVerboseOutput)
            std::cout << "Mask applied to input data at offset " << std::setw(10) << i << ": 0x" << std::hex << static_cast<int>(inputData[i]) << std::dec;
        inputData[i] &= ~0x01;
        inputData[i] |= bestLocationOffsetMask[i];
        if (fullVerboseOutput)
            std::cout << " -> 0x" << std::hex << static_cast<int>(inputData[i]) << std::dec << " with " << bestLocationOffsetMask[i] << std::endl;
    }

    if (perfectFitFound)
        return;

    for (uint32_t i = sizeBitsLocationOffset; i < sizeBitsLocationOffset + 32; ++i) {
        if (fullVerboseOutput)
            std::cout << "Mask applied to input data at offset " << std::setw(10) <<  i << ": 0x" << std::hex << static_cast<int>(inputData[i]) << std::dec;
        inputData[i] &= ~0x01;
        inputData[i] |= secretSizeMaskSequence[i - sizeBitsLocationOffset];
        if (fullVerboseOutput)
            std::cout << " -> 0x" << std::hex << static_cast<int>(inputData[i]) << std::dec << " with " << static_cast<int>(secretSizeMaskSequence[i - sizeBitsLocationOffset]) << std::endl;
    }

    if (verboseOutput) {
        std::cout << "Secret size mask sequence applied to input data." << std::endl;
    }
}


void Steganotify::genarateDataMaskSequence() {
    int bitsInMask = getMaskBits();

    if (bitsInMask == 0) {
        std::cout << "Error: Mask is not set. Cannot generate data mask sequence." << std::endl;
        return;
    }

    embedDataMaskSequenceSize = secretDataSize * 8 / bitsInMask;
    embedDataMaskSequence = new uint8_t[embedDataMaskSequenceSize];
    memset(embedDataMaskSequence, 0, embedDataMaskSequenceSize);

    int splitSize = 8 / bitsInMask;
    for (uint32_t i = 0; i < secretDataSize; ++i) {
        std::unique_ptr<uint8_t[]> splitData(new uint8_t[splitSize]);
        for (int j = 0; j < splitSize; ++j) {
            embedDataMaskSequence[i * splitSize + j] = (secretData[i] >> (bitsInMask * j)) & (bitsInMask * 2 - 1);
        }

        if (fullVerboseOutput) {
            std::cout << "secret data 0x" << std::setw(2) << std::hex << static_cast<int>(secretData[i]) << std::dec << " split into: ";
            for (int j = splitSize - 1; j >= 0; --j) {
                std::cout << std::setw(2) << static_cast<int>(embedDataMaskSequence[i * splitSize + j]) << " ";
            }
            std::cout << std::endl;
        }
    }

    for (uint32_t i = 0; i < embedDataMaskSequenceSize; ++i) {
        int bitCount = 0;
        uint8_t byte = 0x00;
        for (int j = 0; j < 8; j++) {
            if (mask & (1 << j)) {
                byte |= ((embedDataMaskSequence[i] >> bitCount) & 0x01) << j;
                bitCount++;
            }
        }
        if (fullVerboseOutput) {
            std::cout << "Byte " << std::setw(3) << i << ": 0x" << std::hex << static_cast<int>(byte) << std::dec << std::endl;
        }

        embedDataMaskSequence[i] = byte;
    }
}


void Steganotify::embedSecretData() {
    uint32_t embeddingInputDataSize = inputDataSize - 32;
    uint8_t *embeddinginputData = inputData + 32;
    int stepSize = embeddingInputDataSize / (embedDataMaskSequenceSize + embedSpacerSize);

    if (verboseOutput) {
        std::cout << "Embedding secret data into input data..." << std::endl;
        std::cout << "Embedding input data size:     " << embeddingInputDataSize << " bytes" << std::endl;
        std::cout << "Embed data mask sequence size: " << embedDataMaskSequenceSize << " bytes" << std::endl;
        std::cout << "Step size for embedding:       " << stepSize << " bytes" << std::endl;
    }

    for (uint32_t i = 0; i < embedDataMaskSequenceSize; ++i) {
        uint32_t location = (i + 1) * stepSize;
        if ((location >= sizeBitsLocationOffset) && (location < (sizeBitsLocationOffset + 32))) {
            location = sizeBitsLocationOffset + 32; // Skip the size bits location
        }
        embeddinginputData[location] &= ~mask; // Clear the bits specified by the mask
        embeddinginputData[location] |= (embedDataMaskSequence[i] & mask);
        if (fullVerboseOutput) {
            std::cout << "Set byte " << std::setw(3) << i << ": 0x" << std::hex << static_cast<int>(embeddinginputData[location]) << std::dec << "; location " << location << std::endl;
        }
    }

    if (verboseOutput) {
        std::cout << "Secret data embedded successfully." << std::endl;
    }
    delete[] embedDataMaskSequence;
    embedDataMaskSequence = nullptr;
    embedDataMaskSequenceSize = 0;
}
