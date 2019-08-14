import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import sys

# Read in the file as a Pandas DataFrame
col = ['DS', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS', 'SYSTEM', 'THRUPUT', 'RELATIVE']
df  = pd.read_csv('all_systems_final.txt', sep='\t', names=col, skiprows=1)

# The plot settings
sns.set()

# Showing a single graph to screen
# Filter the data
fil = df.loc[(df['SYSTEM']=='ARMS') & (df['TESTCASE']==1) & (df['TXN_SIZE']==5)]
sns.set_style(style='white')
g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DS', style='SYSTEM', err_style=None, markers=True, dashes=False)
plt.xlabel('Threads')
plt.ylabel('Throughput (OP/s)')
plt.show()

# Outputting a bunch of graphs to a directory
# for i in range(1, 1):
#     for j in range(1, 1):
#         # Filter the data
#         fil = df.loc[(df['TESTCASE'] == i) & (df['TXN_SIZE'] == j)]
#         sns.set_style(style='white')
#         sns_plot = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DS', style='DS', markers=True, dashes=False)
#         plt.xlabel('Threads')
#         plt.ylabel('Throughput (OP/s)')
#         fig = sns_plot.get_figure()
#         fig.savefig("tc" + str(i) + "txn" + str(j) + "output.png")
#         plt.clf()
