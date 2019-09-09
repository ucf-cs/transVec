import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import sys

# A couple of lists that we'll be using
init_cols  = ['DATA STRUCTURE', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS']
final_cols = ['DATA STRUCTURE', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS', 'SYSTEM', 'THRUPUT', 'RELATIVE', 'JOINT']
structures = ['SEGME', 'COMPA', 'BOOST', 'STMVE', 'STOVEC']
systems    = ['INTEL', 'AMD', 'ARM', 'NUMA']

# Read in the file as a Pandas DataFrame
# MAKE SURE TO PUT THEM IN THE SAME ORDER AS THE SYSTEMS LIST ABOVE
df1 = pd.read_csv('Intelfinal_report0906232313.txt', sep='\t', names=init_cols, skiprows=1)
df2 = pd.read_csv('AMDfinal_report0906232126.txt',   sep='\t', names=init_cols, skiprows=1)
df3 = pd.read_csv('ARMfinal_report0906232238.txt',   sep='\t', names=init_cols, skiprows=1)
df4 = pd.read_csv('NUMAfinal_report0906232125.txt',   sep='\t', names=init_cols, skiprows=1)

# Add the 'SYSTEM' column
df1['SYSTEM'] = systems[0]
df2['SYSTEM'] = systems[1]
df3['SYSTEM'] = systems[2]
df4['SYSTEM'] = systems[3]

# Concatenate all the dataframes to make 1 large dataframe
frames = [df1, df2, df3, df4]
df = pd.concat(frames)

# Add the 'THRUPUT' column (measured in Ops/Second)
df['THRUPUT'] = (df['NUM_TXN'] - df['ABORTS']) * df['TXN_SIZE'] * 1e9 / df['TIME']

# Add the 'JOINT' column
df['JOINT'] = df['SYSTEM'] + "-" + df['DATA STRUCTURE']

# Calculate the RELATIVE column and add it for every case
# THRUPUT = (NUM_TXN - ABORTS) / TIME * 1e9
df.loc[(df['TESTCASE']==1) | (df['TESTCASE']==2) | (df['TESTCASE']==20), 'THRUPUT'] = \
    (df.loc[(df['TESTCASE']==1) | (df['TESTCASE']==2) | (df['TESTCASE']==20), 'NUM_TXN'] - \
    df.loc[(df['TESTCASE']==1) | (df['TESTCASE']==2) | (df['TESTCASE']==20), 'NUM_TXN']) / \
    df.loc[(df['TESTCASE']==1) | (df['TESTCASE']==2) | (df['TESTCASE']==20), 'TIME'] * 1e9

# Initialize the RELATIVE column that we're going to fill
df['RELATIVE'] = np.nan

# Add the 'RELATIVE' column (The relative performance compared to 1 Thread of the same tescase and txn_size)
for d in structures:
    for s in systems:
        for i in range(1, 20):
            for j in range(1, 6):
                # null check
                if df.loc[(df['DATA STRUCTURE']==d) & (df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j)].empty:
                    continue
                
                # Extract the time it took for 1 thread to complete
                rel = df.loc[(df['DATA STRUCTURE']==d) & (df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j)].iloc[0, 9]

                # Calculate the RELATIVE column and add it for every case
                df.loc[(df['DATA STRUCTURE']==d) & (df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j), 'RELATIVE'] = \
                    df.loc[(df['DATA STRUCTURE']==d) & (df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j), 'THRUPUT'] / rel

# Export the new dataframe
df.to_csv(path_or_buf="all_systems_final.txt", index=False, sep='\t', columns=final_cols)