#!/bin/bash

# Soliman Alnaizy - May 2019
# Insired from Dr. Szumlanski's program testing scripts.
# Test Script for AREA 67's transactional vector data structure.

# ADDING YOUR OWN TESTCASES: Just create a file named testcase0X.cpp and place
# it in the test_cases/ directory. Update the variable below to the new number
# of testcases
NUM_TEST_CASES=5

# Before doing anything, make sure if passed in parameters are correct.
# Make sure all macros are defined, start by extracting the macros from the
# define.hpp file using grep, sed, and regex.
NUM_PARAMETERS=$(grep "// #define" define.hpp | wc -l)
PARAMETERS=$(grep "// #define" define.hpp)
PARAMETERS=$(echo "$PARAMETERS" | sed -r 's/[a-z0-9/#\s]*//g')
MAIN=testcase01.cpp

# Store tokens in an array
for word in $PARAMETERS
do
    TOKENS+=($word)
done

# If not enough command line arguements are passed
if [ "$NUM_PARAMETERS" != "$#" ]
then
    # This will be the error message.
    for word in $PARAMETERS
    do
        CORRECT+="$word=??? "
    done

    # All these if-statements are just to make sure that the error message is
    # [somewhat] gramatically correct.
    echo
    echo -e "test-all.sh: \e[31m(⌐■_■) You've been pulled over by the C++ police! (⌐■_■) \e[0m"
    echo    "test-all.sh: Wrong number of parameters are passed!"
    echo    "             Expected macros to be passed are [$NUM_PARAMETERS]:"
    if [ "$NUM_PARAMETERS" != "0" ]
    then
        echo    "             ${TOKENS[@]}"
    fi
    echo
    echo    "test-all.sh: Proper syntax:"
    echo    "             bash test-all.sh $CORRECT"
    if [ "$NUM_PARAMETERS" != "0" ]
    then
        echo    "             (Replace the \"???\" with your desired values. Duh.)"
    fi
    echo
    echo    "test-all.sh: Aborting test script. Byeeeeeeeee."
    echo 
    exit
fi

# Make sure that the passed arguments are spelled correctly. Go through each
# aguement that's passed in and compare with tokenized parameters.
for VAR in "$@"
do
    TEMP=$(echo "$VAR" | sed -r 's/[a-z0-9/#\s=]*//g')
    MISMATCH=true

    for (( j=0; j<$NUM_PARAMETERS; j++ ))
    do
        if [ "$TEMP" = "${TOKENS[j]}" ]
        then
            MISMATCH=false
        fi
    done

    if $MISMATCH
    then
        echo
        echo -e "test-all.sh: \e[31m(⌐■_■) You've been pulled over by the C++ police! (⌐■_■) \e[0m"
        echo -e "test-all.sh: There's no macro named \e[34m$TEMP\e[0m!"
        echo    "             Please fix this typo before continuing."
        echo 
        echo -e "test-all.sh: Aborting test script. Byeeeeeeeee."
        echo
        exit
    fi
done

# If we reached this part, then that means that all the passed in parameters
# should be correct. Start generating the report.

# Get some data before starting anything
DATE=$(date)
REPORT_NAME="final_report$(date +%m%d%H%M%S).txt"
NUM_CORES=$(nproc)
MODEL="Processor $(lscpu | grep GHz)"
MODEL=$(echo "$MODEL" | sed -r 's/\s\s\s\s\s\s\s\s\s//g')

# Create the file
touch reports/$REPORT_NAME

# Output a bunch of housekeeping information
echo "====================================================="         >> reports/$REPORT_NAME
echo "  T R A N S A C T I O N A L   V E C T O R   T E S T  "         >> reports/$REPORT_NAME
echo "====================================================="         >> reports/$REPORT_NAME
echo                                                                 >> reports/$REPORT_NAME
echo "Test preformed on $DATE."                                      >> reports/$REPORT_NAME
echo "Tested on a computer with $NUM_CORES cores."                   >> reports/$REPORT_NAME
echo "$MODEL"                                                        >> reports/$REPORT_NAME
echo                                                                 >> reports/$REPORT_NAME

# Replace spaces with '|' to be able to pass it as one variable, we'll then put
# back the spaces in the makefile
if [ "$NUM_PARAMETERS" != "0" ]
then
    TO_BE_PASSED="|$(echo "$@" | sed -r 's/\s/|/g')"
fi

# If we want to test for one specific case instead of all testcases
if [ -z "$MAIN" ]
then
    # Test for THREAD_COUNT up to NUM_CORES*2
    for (( i=1; i<=$NUM_CORES*2; i++ ))
    do
        # Test for TRANSACTION_SIZE from 1 - 5
        for (( j=1; j<=5; j++))
        do
            echo "=====================================================" >> reports/$REPORT_NAME
            echo "Currently testing for THREAD_COUNT = $i and          " >> reports/$REPORT_NAME
            echo "NUM_TRANSACTIONS = $j."                                >> reports/$REPORT_NAME
            echo "=====================================================" >> reports/$REPORT_NAME

            # For loop to go through the test cases
            for (( k=1; k<=$NUM_TEST_CASES; k++ ))
            do
                echo -e "~~ RUNNING TESTCASE #$k ~~"                     >> reports/$REPORT_NAME
                echo                                                     >> reports/$REPORT_NAME
                echo -e "\e[33m~~ RUNNING TESTCASE #$k ~~\e[0m"
                echo "-- Building executable file."
                echo 

                # Make the executable file with a different main file everytime
                make MAIN=test_cases/testcase0$k.cpp DEFINES=$TO_BE_PASSED -j8

                echo
                echo "-- Running the testcase."
                echo 
                ./transVec.out                                           >> reports/$REPORT_NAME

                # remove the old main to make space for the new main
                echo                                                     >> reports/$REPORT_NAME
                echo -e "~~ END OF TESTCASE #$k ~~"                      >> reports/$REPORT_NAME
                echo
                echo -e "\e[33m~~ END OF TESTCASE #$k ~~\e[0m"
                echo "-- Cleaning up old object files to prepare for the next testcase."
                make clean
                echo
            done
        done
    done
else
    make MAIN=test_cases/$MAIN DEFINES=$TO_BE_PASSED -j8
fi

# Clean up after yo self
# echo 
# echo "===================================="
# echo "Cleaning up the mess that we made..."
# echo "===================================="
# make clean
# echo 