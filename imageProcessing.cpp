#include <string>
#include <opencv2/opencv.hpp>
#include "stegahide.h"


bool Steganotify::inputFileIsImage() {
    return (inputFileFormat == "jpg" || inputFileFormat == "jpeg" || inputFileFormat == "png" || inputFileFormat == "bmp" || inputFileFormat == "tiff");
}


int Steganotify::extractImageData() {
    image = cv::imread(inputFilename.c_str(), cv::IMREAD_UNCHANGED);

    if (image.empty()) {
        std::cout << " --- Input file empty" << std::endl;
        return STEGAHIDE_FILE_EMPTY;
    }

    inputDataSize = static_cast<uint32_t>(image.total() * image.elemSize());
    inputData = reinterpret_cast<uint8_t *>(image.data);

    if (verboseOutput) {
        std::cout << "Image data size: " << inputDataSize << " bytes" << std::endl;
    }

    return STEGAHIDE_SUCCESS;
}


int Steganotify::encodeImageData() {
    int channels = image.channels();
    int dataType = image.type();
    int rows     = image.rows;
    int cols     = image.cols;

    if (verboseOutput) {
        std::cout << "Image data type: " << dataType << std::endl;
        std::cout << "Image rows: " << rows << ", cols: " << cols << ", channels: " << channels << std::endl;
    }

    int retval = processInputData();

    if (retval != STEGAHIDE_SUCCESS) {
        std::cout << "Error processing input data." << std::endl;
        return retval;
    }

    // Save the modified image
    if (image.empty()) {
        std::cout << "Error: Image data is empty after processing." << std::endl;
        return STEGAHIDE_WRITE_ERROR;
    }

    std::vector<int> compression_params;

    if (outputFileFormat == "jpg" || outputFileFormat == "jpeg" || outputFileFormat == "png") {
        outputFileFormat = "png"; // jpeg is lossy, force to png for lossless
        outputFilename = outputFileBaseName + "." + outputFileFormat;
        compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(9); // Maximum PNG compression
    } else if (outputFileFormat == "bmp") {
        // BMP does not support compression parameters
    } else if (outputFileFormat == "tiff") {
        compression_params.push_back(cv::IMWRITE_TIFF_COMPRESSION);
        compression_params.push_back(cv::IMWRITE_TIFF_COMPRESSION_LZW); // Default TIFF compression
    } else {
        std::cout << "Error: Unsupported output file format: " << outputFileFormat << std::endl;
        return STEGAHIDE_USAGE_ERROR;
    }

    if (!cv::imwrite(outputFilename, image, compression_params)) {
        std::cout << "Error: Failed to write output image to file: " << outputFilename << std::endl;
        return STEGAHIDE_WRITE_ERROR;
    }

    return retval;
}