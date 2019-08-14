DATA_STRUCTURES=(SEGMENTVEC COMPACTVEC BOOSTEDVEC STMVEC)

for k in `seq -f "%02g" 1 21`; do
    for ds in "${DATA_STRUCTURES[@]}"; do
        for j in `seq 1 5`; do
            FILE=$ds"_testcase"$k"_txn"$j".txt"
            echo -e "THREAD_COUNT\tTIME\tABORTS" >> $FILE
            for i in `seq 1 48`; do
                TIME=$(grep -A 27 $ds final_report0719183133.txt | grep -A 23 "THREAD_COUNT     = $i " | grep -A 23 "TRANSACTION_SIZE = $j" | grep "Testcase $k" | grep -o "[0-9]*ns" | grep -o "[0-9]*" )
                ABORT=$(grep -A 27 $ds final_report0719183133.txt | grep -A 23 "THREAD_COUNT     = $i " | grep -A 23 "TRANSACTION_SIZE = $j" | grep "Testcase $k" | grep -o "[0-9]* abort(s)" | grep -o "[0-9]*")
                echo -e "$i\t$TIME\t$ABORT" >> $FILE
            done
        done
    done
done