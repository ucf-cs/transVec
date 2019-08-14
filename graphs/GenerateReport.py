import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import sys

# A couple of lists that we'll be using
init_cols  = ['DS', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS']
final_cols = ['DS', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS', 'SYSTEM', 'THRUPUT', 'RELATIVE']
structures = ['SEGME', 'COMPA', 'BOOST', 'STMVE']
systems    = ['INTEL', 'AMD', 'ARM']

# Read in the file as a Pandas DataFrame
df1 = pd.read_csv('sanitizedIntelfinal_report0729202547.txt', sep='\t', names=init_cols, skiprows=1)
df2 = pd.read_csv('SanitizedAMDfinal_report0730132519.txt',   sep='\t', names=init_cols, skiprows=1)
df3 = pd.read_csv('SanitizedARMfinal_report0729202242.txt',   sep='\t', names=init_cols, skiprows=1)

# Add the 'SYSTEM' column
df1['SYSTEM'] = systems[0]
df2['SYSTEM'] = systems[1]
df3['SYSTEM'] = systems[2]

# Concatenate all the dataframes to make 1 large dataframe
frames = [df1, df2, df3]
df = pd.concat(frames)

# Add the 'THRUPUT' column (measured in Ops/Second)
df['THRUPUT'] = df['NUM_TXN'] * df['TXN_SIZE'] * 1e9 / df['TIME']

# Initialize the RELATIVE column that we're going to fill
df['RELATIVE'] = np.nan

# Add the 'RELATIVE' column (The relative performance compared to 1 Thread of the same tescase and txn_size)
for d in structures:
    for s in systems:
        for i in range(1, 22):
            for j in range(1, 6):
                # null check
                if df.loc[(df['DS']==d) & (df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j)].empty:
                    continue
                
                # Extract the time it took for 1 thread to complete
                rel = df.loc[(df['DS']==d) & (df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j)].iloc[0, 9]

                # Calculate the RELATIVE column and add it for every case
                df.loc[(df['DS']==d) & (df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j), 'RELATIVE'] = \
                    df.loc[(df['DS']==d) & (df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j), 'THRUPUT'] / rel

# Export the new dataframe
df.to_csv(path_or_buf="all_systems_final.txt", index=False, sep='\t', columns=final_cols)