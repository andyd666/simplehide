#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include "stegahide.h"


void Steganotify::parseInputFilename(std::string &inputFilename) {
    this->inputFilename = inputFilename;
    this->inputFileBaseName = inputFilename.substr(0, inputFilename.find_last_of('.'));
    this->inputFileFormat = inputFilename.substr(inputFilename.find_last_of('.') + 1);
};

void Steganotify::parseOutputFilename(std::string &outputFilename) {
    this->outputFilename = outputFilename;
    this->outputFileBaseName = outputFilename.substr(0, outputFilename.find_last_of('.'));
    this->outputFileFormat = outputFilename.substr(outputFilename.find_last_of('.') + 1);
};


int main(int argc, char* argv[]) {
    int retval;
    Steganotify stegahide;

    if (argc == 1) {
        Steganotify::usage();
        return STEGAHIDE_USAGE;
    }

    retval = stegahide.parseCommandLine(argc, argv);
    if (retval == STEGAHIDE_USAGE) {
        return STEGAHIDE_USAGE;
    } else if (retval != STEGAHIDE_SUCCESS) {
        return retval;
    }

    stegahide.verboseParsePrint();
    if (!stegahide.decode) {
        retval = stegahide.embedData();
    } else {
        retval = stegahide.extractSecretData();
    }
    return retval;
}


void Steganotify::usage() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  -h, --help                          -   print help text" << std::endl;
    std::cout << "  -b, --binary                        -   perform raw binary injection (output file may be corrupted)" << std::endl;
    std::cout << "  -v, --verbose                       -   enable verbose output" << std::endl;
    std::cout << "  -V, --full-verbose                  -   enable full verbose output" << std::endl;
    std::cout << "  -m, --mask <mask>                   -   set mask for embedding data (default: 0x00)" << std::endl;
    std::cout << "  -d, --decode                        -   decode data from input file" << std::endl;
    std::cout << "  -i, --input <input_file_name>       -   set input file name" << std::endl;
    std::cout << "  -o, --output <output_file_name>     -   set output file name" << std::endl;
    std::cout << "  -s, --secret <secret_file_name>     -   set secret file name" << std::endl;
    std::cout << "  <input_file_name>                   -   input file name (if not specified with -i) must be specified first" << std::endl;
    std::cout << "  <output_file_name>                  -   output file name (if not specified with -o) must be specified second" << std::endl;
    std::cout << "  <secret_file_name>                  -   secret file name (if not specified with -s) must be specified third" << std::endl;
    std::cout << "Note: if output file name is not specified, it will be generated as <input_file_base_name>_output.<input_file_format>" << std::endl;
}


int Steganotify::parseCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (DEBUG) {
            std::string arg = argv[i];
            if (argv[i][0] == '-' && ((i + 1) < argc)) {
                arg += " " + std::string(argv[i + 1]);
            }
            std::cout << "Processing argument: " << arg << std::endl;
        }

        if (std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help") {
            usage();
            return STEGAHIDE_USAGE;
        } else if (std::string(argv[i]) == "-v" || std::string(argv[i]) == "--verbose") {
            verboseOutput = true;
            continue;
        } else if (std::string(argv[i]) == "-V" || std::string(argv[i]) == "--full-verbose") {
            fullVerboseOutput = true;
            verboseOutput = true;
            continue;
        } else if (std::string(argv[i]) == "-m" || std::string(argv[i]) == "--mask") {
            std::string maskStr = argv[++i];
            mask = std::stoi(maskStr, nullptr, 16);
            continue;
        } else if (std::string(argv[i]) == "-b" || std::string(argv[i]) == "--binary") {
            std::cout << "Warning: Raw binary injection is enabled. Output file may be corrupted." << std::endl;
            rawBinary = true;
            continue;
        } else if (std::string(argv[i]) == "-d" || std::string(argv[i]) == "--decode") {
            decode = true;
            continue;
        } else if (std::string(argv[i]) == "-i" || std::string(argv[i]) == "--input") {
            if (!inputFilename.empty()) {
                std::cout << "Error: Input file already specified." << std::endl;
                return STEGAHIDE_USAGE_ERROR;
            }
            inputFilename = std::string(argv[++i]);
            parseInputFilename(inputFilename);
            continue;
        } else if (std::string(argv[i]) == "-o" || std::string(argv[i]) == "--output") {
            if (!outputFilename.empty()) {
                std::cout << "Error: Output file already specified." << std::endl;
                return STEGAHIDE_USAGE_ERROR;
            }
            outputFilename = std::string(argv[++i]);
            parseOutputFilename(outputFilename);
            continue;
        } else if (std::string(argv[i]) == "-s" || std::string(argv[i]) == "--secret") {
            if (!secretDataFilename.empty()) {
                std::cout << "Error: Secret file already specified." << std::endl;
                return STEGAHIDE_USAGE_ERROR;
            }
            secretDataFilename = std::string(argv[++i]);
            continue;
        }

        if (inputFilename.empty()) {
            inputFilename = std::string(argv[i]);
            parseInputFilename(inputFilename);
        } else if (outputFilename.empty()) {
            outputFilename = std::string(argv[i]);
            parseOutputFilename(outputFilename);
        } else if (secretDataFilename.empty()) {
            secretDataFilename = std::string(argv[i]);
        } else {
            std::cout << "Error: Unknown argument: " << argv[i] << std::endl;
            return STEGAHIDE_USAGE_ERROR;
        }
    }

    if (inputFilename.empty()) {
        std::cout << "Error: Input file not specified." << std::endl;
        return STEGAHIDE_USAGE_ERROR;
    }

    if (secretDataFilename.empty()) {
        std::cout << "Error: Secret file not specified." << std::endl;
        return STEGAHIDE_USAGE_ERROR;
    }

    if (outputFilename.empty()) {
        outputFilename = inputFileBaseName + "_output." + inputFileFormat;
        parseOutputFilename(outputFilename);
    }

    return STEGAHIDE_SUCCESS;
}


void Steganotify::verboseParsePrint() {
    if (!verboseOutput)
        return;

    std::cout << "Input file            -> " << inputFilename << std::endl;
    std::cout << "Output file           -> " << outputFilename << std::endl;
    std::cout << "Input file base name  -> " << inputFileBaseName << std::endl;
    std::cout << "Input file format     -> " << inputFileFormat << std::endl;
    std::cout << "Output file base name -> " << outputFileBaseName << std::endl;
    std::cout << "Output file format    -> " << outputFileFormat << std::endl;
    std::cout << "Secret data file      -> " << secretDataFilename << std::endl;
    std::cout << "Raw binary injection  -> " << (rawBinary ? "enabled" : "disabled") << std::endl;
}


int Steganotify::embedData() {
    int retval = STEGAHIDE_SUCCESS;

    std::ifstream infile(inputFilename);
    if (!infile.good()) {
        std::cout << "Error: Input file does not exist: " << inputFilename << std::endl;
        return STEGAHIDE_NO_FILE;
    }
    infile.close();

    std::ifstream secretfile(secretDataFilename);
    if (!secretfile.good()) {
        std::cout << "Error: Secret file does not exist: " << secretDataFilename << std::endl;
        return STEGAHIDE_NO_FILE;
    }

    secretDataSize = static_cast<uint32_t>(std::filesystem::file_size(secretDataFilename));
    secretData = new uint8_t[secretDataSize];
    secretfile.read(reinterpret_cast<char*>(secretData), secretDataSize);
    if (!secretfile) {
        std::cout << "Error: Failed to read secret data from file: " << secretDataFilename << std::endl;
        return STEGAHIDE_READ_ERROR;
    }
    secretfile.close();

    if (rawBinary && false) {
        // TODO: Implement raw binary injection logic
    } if (inputFileIsImage() && extractImageData() == STEGAHIDE_SUCCESS) {
        retval = encodeImageData();
    }
    /* TODO: Implement other file types: audio, video, raw binary, etc.
        else if () ...
    */

    return retval;
}
