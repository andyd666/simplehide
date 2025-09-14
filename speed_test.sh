#!/bin/bash

# run this script:
# sudo ./speed_test.sh | tee speed_test_results.txt ; sync

EXECUTABLE=$(find . -name "run_test")

PROGRAM_THREADS_MAX=$(nproc --all)

if [ -z "$EXECUTABLE" ]; then
    echo "Executable 'run_test' not found. Run 'make' first"
    exit 1
fi

VERBOSE=false
HUMAN_READABLE=false

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
            VERBOSE=true
        ;;
        -r)
            HUMAN_READABLE=true
        ;;
        *)
            echo "Unknown argument: ${!i}"
            echo "Usage: $0 [-j <threads>] [-t <repeats>] [-v] [-r]"
            echo "  -j <threads>   Number of threads to use (default 1, max $PROGRAM_THREADS_MAX, 0 for all)"
            echo "  -t <repeats>   Number of times to run the test (default 1000, 0 for infinite)"
            echo "  -v             Verbose output"
            echo "  -r             Human readable verbose output"
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
echo "Running speed $REPEATS times"
echo "Using $PROGRAM_THREADS threads"

RUN_NUMBER=1

RUN_TIMES_US=""

while [ $RUN_NUMBER -le $REPEATS ]; do
    START_TIME_US=$(date -u +%s%6N)
    $EXECUTABLE -j $PROGRAM_THREADS > /dev/null
    STOP_TIME_US=$(date -u +%s%6N)

    DIFF_TIME_S=$((STOP_TIME_US-START_TIME_US))

    RUN_TIMES_US="$RUN_TIMES_US $DIFF_TIME_S"

    DIFF_TIME_S=$(printf "%d.%06d" $((DIFF_TIME_S/1000000)) $((DIFF_TIME_S%1000000)))

    if [ "$VERBOSE" = "true" ] && [ "$HUMAN_READABLE" = "false" ]; then
        echo "$RUN_NUMBER $DIFF_TIME_S"
    elif [ "$VERBOSE" = "true" ] && [ "$HUMAN_READABLE" = "true" ]; then
        echo "Run #$RUN_NUMBER finished in $DIFF_TIME_S s"
    fi
    RUN_NUMBER=$((RUN_NUMBER+1))
done

SUM_TIME_US=0
for TIME in $RUN_TIMES_US; do
    SUM_TIME_US=$((SUM_TIME_US+TIME))
done

SUM_TIME_S=$(printf "%d.%06d" $((SUM_TIME_US/1000000)) $((SUM_TIME_US%1000000)))
AVG_TIME_S=$(printf "%d" $((SUM_TIME_US/REPEATS)))
AVG_TIME_S=$(printf "%d.%06d" $((AVG_TIME_S/1000000)) $((AVG_TIME_S%1000000)))
echo "Total run time: $SUM_TIME_S s"
echo "Average run time: $AVG_TIME_S s"
