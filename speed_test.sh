#!/bin/bash

# run this script:
# sudo ./speed_test.sh | tee speed_test_results.txt ; sync

cd $(find . -name "speed_test" | cut -d "/" -f 1-3)

EXECUTABLE=$(find . -name "speed_test")

PROGRAM_THREADS_MAX=$(nproc --all)

if [ -z "$EXECUTABLE" ]; then
    echo "Executable 'speed_test' not found. Run 'make' first"
    exit 1
fi



VERBOSE=0

for (( i=1; i<=$#; i++ ));
do
    next_arg=$((i+1))
    case ${!i} in
        -j*)
            PROGRAM_THREADS=$(echo ${!i} | tr -d -c 0-9)
            if [ -z $PROGRAM_THREADS ]; then
                PROGRAM_THREADS=${!next_arg}
                i=$((i+1))
            fi
            if [ -z $PROGRAM_THREADS ]; then
                echo "Thread number was not given when using \"-j\" flag"
                #exit 22
            fi
            if [ "$PROGRAM_THREADS" = "0" ]; then
                PROGRAM_THREADS=$PROGRAM_THREADS_MAX
            fi
        ;;
        -t*)
            REPEATS=$(echo ${!i} | tr -d -c 0-9)
            if [ -z $REPEATS ]; then
                REPEATS=${!next_arg}
                i=$((i+1))
            fi
            if [ -z $REPEATS ]; then
                echo "Repeats number was not given when using \"-t\" flag"
                #exit 22
            fi
            if [ "$REPEATS" = "0" ]; then
                REPEATS=""
            fi
        ;;
        -v)
            VERBOSE=1
        ;;
        *)
            echo "Unknown argument: ${!i}"
            echo "Usage: $0 [-j <threads>] [-t <repeats>] [-v] [-r]"
            echo "  -j <threads>   Number of threads to use (default 1, max $PROGRAM_THREADS_MAX, 0 for all)"
            echo "  -t <repeats>   Number of times to run the test (default 1000, 0 for infinite)"
            echo "  -v             Verbose output"
            exit 22
        ;;
    esac
done

if [ -z "$REPEATS" ]; then
    REPEATS=1000
fi

if [ -z "$PROGRAM_THREADS" ]; then
    PROGRAM_THREADS=1
fi

if [ "$PROGRAM_THREADS" -gt "$PROGRAM_THREADS_MAX" ]; then
    PROGRAM_THREADS=$PROGRAM_THREADS_MAX
fi

lscpu
dmidecode --type 17
echo "################################"
echo ""
echo ""
echo "Running speed test $REPEATS times"
echo "Using $PROGRAM_THREADS threads"

RUN_NUMBER=1

$EXECUTABLE -j $PROGRAM_THREADS -t $REPEATS -V $VERBOSE
