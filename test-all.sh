#!/bin/bash

# Soliman Alnaizy
# May 2019
# Insired from Dr. Szumlanski's program testing scripts
# Test Script for AREA 67's transactional vector data structure

# ADDING YOUR OWN TESTCASES: Just create a file named testcase0X.cpp and place
# it in the test_cases/ directory. Update the variable below to the new number
# of testcases
NUM_TEST_CASES=1

# Check if parameters are correct:
# Make sure all macros are defined, start by extracting the parameters from
# the define.hpp file using grep, sed, and regex.
NUM_PARAMETERS=$(grep "// #define" define.hpp | wc -l)
PARAMETERS=$(grep "// #define" define.hpp)
PARAMETERS=$(echo "$PARAMETERS" | sed -r 's/[a-z0-9/#\s]*//g')

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

    echo
    echo -e "test-all.sh: \e[31moopsie woopsie you made a fucky wucky (˘ω˘) \e[0m"
    echo    "test-all.sh: Not enough parameters are passed!"
    echo    "             Expected macros to be passed are [$NUM_PARAMETERS]:"
    echo    "             ${TOKENS[@]}"
    echo
    echo    "test-all.sh: Proper syntax:"
    echo    "             bash test-all.sh $CORRECT"
    echo    "             (Replace the \"???\" with your desired values. Duh.)"
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
        echo -e "test-all.sh: \e[31moopsie woopsie you made a fucky wucky (˘ω˘) \e[0m"
        echo -e "test-all.sh: There's no macro named \e[34m$TEMP\e[0m!"
        echo    "             Please fix this typo before continuing."
        echo 
        echo -e "test-all.sh: Aborting test script. Byeeeeeeeee."
        echo
        exit
    fi
done


# Replace spaces with '|' to be able to pass it as one variable, we'll then put
# back the spaces in the makefile
if [ "$NUM_PARAMETERS" != "0" ]
then
    TO_BE_PASSED="|$(echo "$@" | sed -r 's/\s/|/g')"
fi

# For loop to go through the test cases
for (( i=1; i<=$NUM_TEST_CASES; i++ ))
do
    # Make the executable file with a different main file everytime
    make MAIN=test_cases/main.cpp DEFINES=$TO_BE_PASSED -j8
done

# Clean up after yo self
echo 
echo "====================================================="
echo "Cleaning up and restoring old values of define.hpp..."
echo "====================================================="
make clean
echo 