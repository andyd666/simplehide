#ifndef __STEGAHIDE_H__
#define __STEGAHIDE_H__

#include <string>
#include <stdint.h>
#include <opencv2/opencv.hpp>


#ifndef DEBUG
#define DEBUG false
#endif

#if DEBUG == true
#define trace() std::cout << __LINE__ << " " << __func__  << " trace"<< std::endl
#else
#define trace() do {} while (0)
#endif


typedef enum {
    STEGAHIDE_UNKNOWN_STATUS = -2,
    STEGAHIDE_CONTINUE       = -1,
    STEGAHIDE_SUCCESS        =  0,
    STEGAHIDE_USAGE          =  1,
    STEGAHIDE_USAGE_ERROR    =  2,
    STEGAHIDE_NO_FILE        =  3,
    STEGAHIDE_FILE_EMPTY     =  4,
    STEGAHIDE_READ_ERROR     =  5,
    STEGAHIDE_WRITE_ERROR    =  6,
    STEGAHIDE_INVALID_DATA   =  7,
    STEGAHIDE_INVALID_MASK   =  8,
} SteganotifyStatus;


class Steganotify {
public:
    bool decode                        = false;
    bool verboseOutput                 = false;
    bool fullVerboseOutput             = false;

private:
    std::string inputFilename          = "";
    std::string inputFileBaseName      = "";
    std::string inputFileFormat        = "";

    std::string outputFilename         = "";
    std::string outputFileBaseName     = "";
    std::string outputFileFormat       = "";

    std::string secretDataFilename     = "";

    uint32_t inputDataSize             = -1;
    uint8_t *inputData                 = nullptr;

    uint32_t secretDataSize            = -1;
    uint8_t *secretData                = nullptr;

    bool rawBinary                     = false;

    uint8_t mask                       = 0x00;
    uint8_t secretSizeMaskSequence[32] = {0};

    uint32_t embedDataMaskSequenceSize = 0;
    uint8_t *embedDataMaskSequence     = nullptr;

    const uint32_t embedSpacerSize     = 2; // Number of bytes to skip between each embedded data byte
    const uint32_t defaultOffset       = 32; // Do not put size at the beginning of the image
    uint32_t sizeBitsLocationOffset    = defaultOffset;

    cv::Mat image; // OpenCV image data


public:
    Steganotify() = default;
    ~Steganotify() {
        inputData = nullptr;
        delete[] secretData;
    };
    static void usage();

    int parseCommandLine(int argc, char* argv[]);
    void verboseParsePrint();
    int embedData();
    int extractSecretData();

private:
    int getMaskBits() {
        int bits = 0;
        for (int i = 0; i < 8; ++i) {
            bits += (mask & (1 << i));
        }
        return bits;
    };
    void parseInputFilename(std::string &inputFilename);
    void parseOutputFilename(std::string &outputFilename);
    bool inputFileIsImage();
    int extractImageData();
    int encodeImageData();

    int processInputData();

    void calculateMask();
    bool verifyMask();
    void embedDataSizeLocation();
    void genarateDataMaskSequence();
    void embedSecretData();

    int extractDataSize();
};





#endif // __STEGAHIDE_H__
