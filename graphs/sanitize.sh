# Delete all the lines with the word 'Pool'
sed -i 's/Pool.*//' $1

# Delete lines corresponding to failed tests
sed -i '/^-1$/d' $1

# Get rid of all the empty lines
sed -i '/^$/d' $1

# Combine any lines that may have been split up
sed -i ':a;N;$!ba;s/\t\n/\t/g' $1