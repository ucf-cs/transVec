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
NUM_TEST_CASES=19

# Start by extracting the commented out macros from the define.hpp file using
# grep, sed, and regex. It will be used to make sure all macros are correctly
# defined before running a testcase
NUM_PARAMETERS=$(grep "// #define" define.hpp | wc -l)
PARAMETERS=$(grep "// #define" define.hpp)
PARAMETERS=$(echo "$PARAMETERS" | sed -r 's/[a-z0-9/#\s]*//g')

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

# NUM_CORES will determine up to which value we go to for THREAD_COUNT
for i in `seq 1 $NUM_CORES`
do
    # Test for TRANSACTION_SIZE from 1 - 5
    for j in `seq 1 5`
    do
        echo "=====================================================" >> $REPORT
        echo "SGMT_SIZE        = 16    "                             >> $REPORT
        echo "NUM_TRANSACTIONS = 250000"                             >> $REPORT
        echo "THREAD_COUNT     = $i    "                             >> $REPORT
        echo "TRANSACTION_SIZE = $j    "                             >> $REPORT
        echo                                                         >> $REPORT

        # Preparing the the macros that are being passed this rouand of testing
        if [ "$NUM_PARAMETERS" != "0" ]; then
            TO_BE_PASSED=("THREAD_COUNT=$i")
            TO_BE_PASSED+=("TRANSACTION_SIZE=$j")
            TO_BE_PASSED+=$@

            # Make sure the macros there are the correct number of macros
            if [ "${#TO_BE_PASSED[@]}" != "$NUM_PARAMETERS" ]
            then
                # This will be the error message.
                for word in $PARAMETERS
                do
                    CORRECT+="$word=??? "
                done

                echo
                echo "Error: Wrong number of parameters are passed!"
                echo "       Expected number of macros: [$NUM_PARAMETERS]:"
                echo "       Currently, only [${#TO_BE_PASSED[@]}] are defined."
                echo
                echo "Proper syntax:"
                echo "  bash test-all.sh <macro1>=<value> <macro2>=<value> etc"
                echo -n " (or define them by editing the bash script and"
                echo " adding a for loop or something)"
                echo
                echo "=> Aborting test script. Byeeeeeeeee."
                echo
                rm $REPORT
                exit
            fi

            # Make sure parameters are spelt correcly
            for VAR in ${TO_BE_PASSED[@]}
            do
                TEMP=$(echo "$VAR" | sed -r 's/[a-z0-9/#\s=]*//g')
                MISMATCH=true

                for k in `seq 0 $NUM_PARAMETERS`
                do
                    if [ "$TEMP" = "${MACROS[k]}" ]
                    then
                        MISMATCH=false
                    fi
                done

                if $MISMATCH
                then
                    echo
                    echo "Error: There's no macro named $TEMP!"
                    echo "       Please fix this typo before continuing."
                    echo
                    echo "       (or it's already defined... Check define.hpp"
                    echo "        and make sure everything is correct.)"
                    echo 
                    echo "       Aborting test script. Byeeeeeeeee."
                    echo
                    rm $REPORT
                    exit
                fi
            done

            # Finally, replace spaces with "|" to be able to pass it to the
            # makefile (Makefiles are weird like that)
            TO_BE_PASSED="|$(echo "${TO_BE_PASSED[@]}" | sed -r 's/\s/|/g')"
        fi

        #######################################################################
        # Now we're actually going to start with the testcases.
        #######################################################################
        for k in `seq -f "%02g" 1 $NUM_TEST_CASES`;
        do
            echo -e -n "\e[96mStarting testcase$k:"
            echo -e -n " THRD_CNT = $i, TXN_SIZE = $j ... \e[0m"
            echo -n "Testcase $k ... "                               >> $REPORT

            # Make the executable file with a different main file everytime
            make MAIN=test_cases/testcase$k.cpp DEFINES=$TO_BE_PASSED -j8

            # Check if it compiled correctly or not
            compile_val=$?
            if [[ $compile_val != 0 ]]; then
                echo -e "\e[91m fail (failed to compile)\e[0m"                
                echo "fail (failed to compile)"                      >> $REPORT
                continue
            fi

            # Run the executable and suppress all ouptut
            ./transVec.out                              >> $REPORT 2> /dev/null

            # Check if the program crashed or not                 
            execution_val=$?
            if [[ $execution_val != 0 ]]; then
                echo -e "\e[91m fail (program crashed)\e[0m"                
                echo "fail (program crashed)"                        >> $REPORT
                continue
            fi

            echo -e "\e[94m Success!\e[0m"
        done
	
	# Clean all object files to prepare for the next round of testing
	make clean
	echo
    done
done

# Clean up after yo self
echo 
echo "====================================================="
echo "   End of test script. Cleaning up object files...   "
echo "====================================================="
make clean
echo 
