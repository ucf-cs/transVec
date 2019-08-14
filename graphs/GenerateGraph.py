import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import sys

# Read in the file as a Pandas DataFrame
col = ['DS', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS', 'SYSTEM', 'THRUPUT', 'RELATIVE']
df  = pd.read_csv('all_systems_final.txt', sep='\t', names=col, skiprows=1)

# The plot settings (There are a few settings you can mess around with. They're all on Google)
sns.set()

# Filter the data
fil = df.loc[(df['SYSTEM']=='ARMS') & (df['TESTCASE']==1) & (df['TXN_SIZE']==5)]

# There's normally a grey-ish background for some reason. This makes it completely white
sns.set_style(style='white')

# Plot the data
g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DS', style='SYSTEM', err_style=None, markers=True, dashes=False)

# Label the axes
plt.xlabel('Threads')
plt.ylabel('Throughput (OP/s)')

# Finally, display the graph
plt.show()