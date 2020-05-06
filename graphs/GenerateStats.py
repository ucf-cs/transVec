import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
import numpy as np 
import pandas as pd
import seaborn as sns
import sys

structures = ['SEGME', 'COMPA', 'BOOST', 'STMVE', 'STOVEC']
systems    = ['INTEL', 'AMD', 'ARM', 'NUMA']

# Read in the file as a Pandas DataFrame
col = ['DATA STRUCTURE', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS', 'SYSTEM', 'THRUPUT', 'RELATIVE', 'JOINT']
df  = pd.read_csv('all_systems_final.txt', sep='\t', names=col, skiprows=1)

# Filter the data (Add or remove as many parameters as you want)
#fil = df.loc[ \
#    (df['SYSTEM']=='INTEL') & \
#    (df['TESTCASE']==4) & \
#    (df['TXN_SIZE']==5) \
#    #(df['DATA STRUCTURE']=='BOOST') \
#    ]

bestSpeedup = -10000
bestThread = 0
avgSpeedupAdded = 0
total = 0

# Intel, test case 4, txn size 5
# base = df.loc[ \
#     (df['SYSTEM']=='INTEL') & \
#     (df['TESTCASE']==7) & \
#     (df['TXN_SIZE']==5)]

# read-only
base = df.loc[ \
   (df['SYSTEM']=='INTEL') & \
   (df['TESTCASE']==4) & \
   (df['TXN_SIZE']==5) \
   ]

# # write-only
# base = df.loc[ \
#    (df['SYSTEM']=='INTEL') & \
#    (df['TESTCASE']==5) & \
#    (df['TXN_SIZE']==5) \
#    ]

for i in base.THRD_CNT.unique():
    # Filter by thread count.
    fil = base.loc[(base['THRD_CNT']==i)]

    # Average out the throughput accross multiple runs for each data structures of interest.

    tempThrough = 0
    tempCount = 0
    compactThrough = 0
    compactFil = fil.loc[(fil['DATA STRUCTURE']=='COMPA')]
    for index, row in compactFil.iterrows():
        tempThrough += row['THRUPUT']
        tempCount += 1
    compactThrough = tempThrough / tempCount
    
    tempThrough = 0
    tempCount = 0
    segmentedThrough = 0
    segmentedFil = fil.loc[(fil['DATA STRUCTURE']=='SEGME')]
    for index, row in segmentedFil.iterrows():
        tempThrough += row['THRUPUT']
        tempCount += 1
    segmentedThrough = tempThrough / tempCount

    tempThrough = 0
    tempCount = 0
    boostedThrough = 0
    boostedFil = fil.loc[(fil['DATA STRUCTURE']=='BOOST')]
    for index, row in boostedFil.iterrows():
        tempThrough += row['THRUPUT']
        tempCount += 1
    boostedThrough = tempThrough / tempCount

    tempThrough = 0
    tempCount = 0
    STMThrough = 0
    STMFil = fil.loc[(fil['DATA STRUCTURE']=='STMVE')]
    for index, row in STMFil.iterrows():
        tempThrough += row['THRUPUT']
        tempCount += 1
    STMThrough = tempThrough / tempCount

    # # Comparing compact to boosted vectors
    # total += 1
    # avgSpeedup = compactThrough / boostedThrough
    # avgSpeedupAdded += avgSpeedup
    # if bestSpeedup < avgSpeedup:
    #     bestSpeedup = avgSpeedup
    #     bestThread = i
    
    # Comparing segmented to GCC STM vectors
    total += 1
    avgSpeedup = segmentedThrough / STMThrough
    avgSpeedupAdded += avgSpeedup
    if bestSpeedup < avgSpeedup:
        bestSpeedup = avgSpeedup
        bestThread = i
avgSpeedup = avgSpeedupAdded / total

print('bestSpeedup: ', bestSpeedup, 'on thread ', bestThread)
print('total: ', total)
print('avgSpeedup: ', avgSpeedup)