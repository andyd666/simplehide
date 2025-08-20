
#include <stdio.h>
#include "stegahide.h"

#define KB (1024)
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

int main() {
    int fileDataSize = 10 * MB; // Example size, adjust as needed
    int emdeddedDataSize = 1 * KB;
    printf("Running test for stegohide-lib\n");



    printf("Test finished\n");
    return 0;
}
