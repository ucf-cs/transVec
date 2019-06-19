# A bash script that's meant to write up a report that tests the transactional
# vector for AREA 67

# Written by: Soliman Alnaizy - June 2019

# Get some data before starting anything
DATE=$(date)
REPORT_NAME="final_report$(date +%m%d%H%M%S).txt"
NUM_CORES=$(nproc)
MODEL="Processor $(lscpu | grep GHz)"
MODEL=$(echo "$MODEL" | sed -r 's/\s\s\s\s\s\s\s\s\s//g')

# Start by generating the report
touch reports/$REPORT_NAME

# Output a bunch of housekeeping information
echo "=================================================" >> reports/$REPORT_NAME
echo "T R A N S A C T I O N A L   V E C T O R   T E S T" >> reports/$REPORT_NAME
echo "=================================================" >> reports/$REPORT_NAME
echo                                                     >> reports/$REPORT_NAME
echo "Test preformed on $DATE."                          >> reports/$REPORT_NAME
echo "Tested on a computer with $NUM_CORES cores."       >> reports/$REPORT_NAME
echo "$MODEL"                                            >> reports/$REPORT_NAME
echo                                                     >> reports/$REPORT_NAME

