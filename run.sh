make clean && make

for i in {1..24}; do
    for j in {1}; do
        ./transVec.out ${i} 500000 >> output.txt
    done
done