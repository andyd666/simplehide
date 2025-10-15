/*
* simplehide  Copyright (C) 2025  andyd666
* This program comes with ABSOLUTELY NO WARRANTY; for details type `--license-warranty'.
* This is free software, and you are welcome to redistribute it
* under certain conditions; type `--license-conditions' for details.
*/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <string>
#include <bitset>
#include <vector>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <csignal>

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>


std::string execute_cmd(const char* cmd);
void parse_argv(int argc, char **argv, int *threads, int *verboseLevel, int *runTimes);
void signalHandler(int sig);
template<typename T> void drawDistribution(const std::vector<T> &data);
void getTerminalSize(int *width, int *height);


bool sigingCaught = false;


int main(int argc, char **argv) {
    int threads = 1;
    int verboseLevel = 0;
    int runTimesNumber = 1000;
    std::string shellCommandReturn;
    std::string testCommand = "./run_test";
    std::vector<uint64_t> runTimesUs;
    std::chrono::time_point<std::chrono::system_clock> startTime;
    uint64_t totalRunTime = 0;
    int runNumber = 0;

    parse_argv(argc, argv, &threads, &verboseLevel, &runTimesNumber);
    signal(SIGINT, signalHandler);

    runTimesUs.reserve(runTimesNumber);


    if (threads > 1)
        testCommand += " -j " + std::to_string(threads);

    if (verboseLevel > 0)
        testCommand += " -V " + std::to_string(verboseLevel);

    testCommand += " > /dev/null";

    for (runNumber = 0; runNumber < runTimesNumber; runNumber++) {
        startTime = std::chrono::system_clock::now();
        shellCommandReturn = execute_cmd(testCommand.c_str());
        runTimesUs.push_back((uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - startTime).count());
        totalRunTime += runTimesUs[runNumber];
        if (verboseLevel > 0) {
            std::cout << runNumber + 1 << " " << runTimesUs[runNumber] << std::endl;
        }
        if (sigingCaught)
            runTimesNumber = runNumber + 1;
    }

    double meanRunTime = (double)totalRunTime / runNumber;
    double rootMeanSquare = 0;
    double standardError = 0;
    double coefficientOfVariation;

    for (int i = 0; i < runNumber; i++) {
        rootMeanSquare += (runTimesUs[i] - meanRunTime) * (runTimesUs[i] - meanRunTime);
    }

    rootMeanSquare = std::sqrt(rootMeanSquare / (runNumber - 1));

    standardError = rootMeanSquare / std::sqrt(runNumber);

    coefficientOfVariation = rootMeanSquare / meanRunTime * 100.0;

    std::cout << "Total run time:           " << std::setw(16) << (double)totalRunTime / 1000.0 << " ms" << std::endl;
    std::cout << "Mean run time:            " << std::setw(16) << (double)meanRunTime / 1000.0 << " ms" << std::endl;
    std::cout << "RMS:                      " << std::setw(16) << (double)rootMeanSquare / 1000.0 << " ms" << std::endl;
    std::cout << "Standard error:           " << std::setw(16) << (double)standardError / 1000.0 << " ms";
    std::cout << " (" << standardError / meanRunTime * 100.0 << " %)" << std::endl;

    std::cout << "Coefficient Of Variation: " << std::setw(16) << (double)coefficientOfVariation << " %" << std::endl;

    drawDistribution(runTimesUs);

    return 0;
}


std::string execute_cmd(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    }
    catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}


void parse_argv(int argc, char **argv, int *threads, int *verboseLevel, int *runTimes) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-j") == 0) {
            i++;
            if (i < argc) {
                *threads = atoi(argv[i]);
            } else {
                *threads = 1;
            }
        } else if (strcmp(argv[i], "-V") == 0) {
            i++;
            if (i < argc) {
                *verboseLevel = atoi(argv[i]);
            } else {
                *verboseLevel = 0;
            }
        } else if (strcmp(argv[i], "-t") == 0) {
            i++;
            if (i < argc) {
                *runTimes = atoi(argv[i]);
            } else {
                *runTimes = 0;
            }
        }
    }
}


void signalHandler(int sig) {
    if (sig == SIGINT)
        sigingCaught = true;
}


template<typename T> void drawDistribution(const std::vector<T> &data) {
    constexpr int maxBinNumber = 1920;
    constexpr int binHeightValuesNumber = 5;
    constexpr int binWidthValuesStep = 10;

    int terminalWidth = 0;
    int terminalHeight = 0;
    int binsNum;
    int maxBinHeight;
    double binHeightPercentPrintStep;
    double maxPercent = 0;
    T minDataValue = data[0];
    T maxDataValue = data[0];
    T binWidthValue;
    std::vector<int> bins;

    std::vector<std::string> specialCharacters;

    specialCharacters.push_back(" ");
    specialCharacters.push_back("▁");
    specialCharacters.push_back("▂");
    specialCharacters.push_back("▃");
    specialCharacters.push_back("▄");
    specialCharacters.push_back("▅");
    specialCharacters.push_back("▆");
    specialCharacters.push_back("▇");
    specialCharacters.push_back("█");

    getTerminalSize(&terminalWidth, &terminalHeight);

    binsNum = terminalWidth - 8;
    if (binsNum > (int)data.size())
        binsNum = data.size();

    if (binsNum > maxBinNumber)
        binsNum = maxBinNumber;

    maxBinHeight = terminalHeight - 3;

    bins.reserve(binsNum);
    for (int i = 0; i < binsNum; i++)
        bins.push_back(0);

    for (size_t i = 1; i < data.size(); i++) {
        if (data[i] < minDataValue)
            minDataValue = data[i];
        if (data[i] > maxDataValue)
            maxDataValue = data[i];
    }

    binWidthValue = (maxDataValue - minDataValue) / binsNum;
    binHeightPercentPrintStep = maxBinHeight / binHeightValuesNumber;
    maxBinHeight = binHeightPercentPrintStep * binHeightValuesNumber;

    if ((maxDataValue - minDataValue) % binsNum) {
        binWidthValue++;
        maxDataValue = minDataValue + binsNum * binWidthValue;
    }

    binWidthValue = (maxDataValue - minDataValue) / binsNum;

    for (size_t i = 0; i < data.size(); i++) {
        bins[(data[i] - minDataValue) / binWidthValue]++;
    }

    for(size_t i = 0; i < bins.size(); i++) {
        if (((double)bins[i] * 100.0 / (int)data.size()) > maxPercent)
            maxPercent = (double)bins[i] * 100.0 / data.size();
    }

    std::cout << "    % ↑" << std::endl;
    for (int i = maxBinHeight; i > 0; i--) {
        std::string line = "";
        double upperPercent = (double)(i * 100.0 / maxBinHeight) / 100.0 * (double)maxPercent;
        double lowerPercent = (double)((i - 1.0) * 100.0 / maxBinHeight) / 100.0 * (double)maxPercent;

        if ((i % (int)binHeightPercentPrintStep) == 0) {
            int precision;

            if (upperPercent >= 100.0)
                precision = 1;
            else if (upperPercent >= 10.0)
                precision = 2;
            else
                precision = 3;

            std::cout << std::setw(6) << std::setprecision(precision) << upperPercent << "│";
        } else {
            std::cout << "      │";
        }

        for (size_t j = 0; j < bins.size(); j++) {
            if ((double)((double)bins[j] * 100.0 / data.size()) >= upperPercent) {
                line += specialCharacters[specialCharacters.size() - 1];
            } else if ((double)((double)bins[j] * 100.0 / data.size()) <= lowerPercent) {
                line += " ";
            } else {
                double overshoot = bins[j] * 100.0 / data.size() - lowerPercent;
                double step = (upperPercent - lowerPercent) / 8.0;
                line += specialCharacters[(int)(overshoot / step)];
            }
        }

        std::cout << line << std::endl;
    }

    int highestBin = 0;
    for (size_t i = 0; i < bins.size(); i++) {
        if (bins[i] > highestBin)
            highestBin = bins[i];
    }

    std::string lastLine = "   0.0└";

    for (int i = 1; i <= binsNum; i++) {
        if (i % binWidthValuesStep == 0) {
            lastLine += "┼";
        } else {
            lastLine += "─";
        }
    }

    std::cout << lastLine << ">" << std::endl;

    lastLine = "    ";
    for (int i = 0; i < binsNum; i++) {
        if (i % binWidthValuesStep == 0) {
            std::string tmp = std::to_string((T)(minDataValue + i * binWidthValue));
            lastLine += tmp;
            for (size_t j = 0; j < (binWidthValuesStep - tmp.length()); j++)
                lastLine += " ";
        }
    }

    lastLine = lastLine.substr(0, binsNum + 6);

    std::cout << lastLine << std::endl;
}


void getTerminalSize(int *width, int *height) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    *width = w.ws_col;
    *height = w.ws_row;
}


