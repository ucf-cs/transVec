# !/bin/bash

# Soliman Alnaizy - May 2019
# Insired from Dr. Szumlanski's program testing scripts.
# Test Script for AREA 67's transactional vector data structure.

# This script is made with the intention of providing a convinient way to run
# multiple testcases and generate a cute report at the end. If you want to run
# any individual testcase, just change the MAIN variable in the makefile and
# simply run "make" from a commandline

# ADDING YOUR OWN TESTCASES: Just create a file named testcaseX.cpp and place
# it in the test_cases/ directory. Update the variable below to the new number
# of testcases
NUM_TEST_CASES=21

# Start by extracting the commented out macros from the define.hpp file using
# grep, sed, and regex. Depending on what values are defined, certain values
# will be passed.
NUM_PARAMETERS=$(grep "// #define" define.hpp | wc -l)
PARAMETERS=$(grep "// #define" define.hpp)
PARAMETERS=$(echo "$PARAMETERS" | sed -r 's/[a-z0-9/#\s]*//g')
DATA_STRUCTURES=(SEGMENTVEC COMPACTVEC STMVEC)

# Redirect all local error messages to /dev/null (ie "process aborted" errors).
exec 2> /dev/null

# Store all the macros in an array
for word in $PARAMETERS
do
    MACROS+=($word)
done

###############################################################################
# Gather some system data before starting anything
###############################################################################
DATE=$(date)
REPORT="reports/final_report$(date +%m%d%H%M%S).txt"
NUM_CORES=$(nproc)
MODEL="Processor $(lscpu | grep GHz)"
MODEL=$(echo "$MODEL" | sed -r 's/\s\s\s\s\s\s\s\s\s//g')
RAM="$(free -h --si | grep -o -m 1 "\w*G\b" | head -1)B"

# Create the file
touch $REPORT

# Make sure there aren't any pre-existing object files
make clean

# Output a bunch of housekeeping information
echo "====================================================="         >> $REPORT
echo "  T R A N S A C T I O N A L   V E C T O R   T E S T  "         >> $REPORT
echo "====================================================="         >> $REPORT
echo                                                                 >> $REPORT
echo "Test preformed on $DATE."                                      >> $REPORT
echo "Tested on a computer with $NUM_CORES cores."                   >> $REPORT
echo "$MODEL"                                                        >> $REPORT
echo "Total RAM on system: $RAM"                                     >> $REPORT
echo                                                                 >> $REPORT

# More grepping to get data that will be useful to print out
NUM_TXN=$(grep "NUM_TRANSACTIONS" define.hpp | grep "[0-9]*" -o)
SGMT_SIZE=$(grep "SGMT_SIZE" define.hpp | grep "(.*)" -o)

# Each of the data structures will go through all these testcases
for ds in "${DATA_STRUCTURES[@]}"
do
    # NUM_CORES will determine up to which value we go to for THREAD_COUNT
    for i in `seq 1 $NUM_CORES`
    do
        # Test for TRANSACTION_SIZE from 1 - 5
        for j in `seq 1 5`
        do
            echo "=====================================================" >> $REPORT
            echo "DATA STRUCTURE   = $ds       "                         >> $REPORT
            echo "SGMT_SIZE        = $SGMT_SIZE"                         >> $REPORT
            echo "NUM_TRANSACTIONS = $NUM_TXN  "                         >> $REPORT
            echo "THREAD_COUNT     = $i        "                         >> $REPORT
            echo "TRANSACTION_SIZE = $j        "                         >> $REPORT
            echo                                                         >> $REPORT

            # Parsing the values that will be changed during compile time
            TO_BE_PASSED="|THREAD_COUNT=$i|TRANSACTION_SIZE=$j"

            #######################################################################
            # Now we're actually going to start with the testcases.
            #######################################################################
            for k in `seq -f "%02g" 1 $NUM_TEST_CASES`
            do
                echo -e -n "\e[93mStarting testcase$k: "
                echo -e -n "\e[96mDS = \e[35m$ds\e[96m, THRD_CNT = \e[35m$i\e[96m,"
                echo -e -n " TXN_SIZE = \e[35m$j\e[96m ... \e[0m"
                echo -n "Testcase $k ... "                               >> $REPORT

                # Make the executable file with a different main file everytime
                make DATA_STRUCTURE=$ds MAIN=test_cases/testcase$k.cpp DEFINES=$TO_BE_PASSED -j$NUM_CORES

                # Check if it compiled correctly or not
                compile_val=$?
                if [[ $compile_val != 0 ]]; then
                    echo -e "\e[91m fail (failed to compile)\e[0m"                
                    echo "fail (failed to compile)"                      >> $REPORT
                    continue
                fi

                # Run the executable and suppress all ouptut
                ./transVec.out                                           >> $REPORT

                # Check if the program crashed or not                 
                execution_val=$?
                if [[ $execution_val != 0 ]]; then
                    echo -e "\e[91m fail (program crashed)\e[0m"                
                    echo "fail (program crashed)"                        >> $REPORT
                    continue
                fi

                echo -e "\e[92m Success!\e[0m"
            done

        # Clean all object files to prepare for the next round of testing
        make clean
            echo "====================================================="
            echo                                                         >> $REPORT
        done
    done
done

# Clean up after yo self
echo 
echo "====================================================="
echo "   End of test script. Cleaning up object files...   "
echo "====================================================="
echo "====================================================="         >> $REPORT
echo "         E N D   O F   T E S T   S C R I P T         "         >> $REPORT
echo "====================================================="         >> $REPORT
make clean
echo