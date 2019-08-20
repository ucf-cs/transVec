import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
import numpy as np
import pandas as pd
import seaborn as sns
import sys

# Took this off stack overflow, not really sure how it works, but it keeps the y-axis in scientific notation
class ScalarFormatterForceFormat(ScalarFormatter):
    def _set_format(self):  # Override function that finds format to use.
        self.format = "%1.1f"  # Give format here

# Read in the file as a Pandas DataFrame
col = ['DS', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS', 'SYSTEM', 'THRUPUT', 'RELATIVE']
df  = pd.read_csv('all_systems_final.txt', sep='\t', names=col, skiprows=1)

# The plot settings (There are a few settings you can mess around with. They're all on Google. I'll be using the defaults)
sns.set()

# Filter the data (Add or remove as many parameters as you want)
fil = df.loc[ \
    (df['SYSTEM']=='INTEL') & \
    (df['TESTCASE']==16) & \
    (df['TXN_SIZE']==5) \
    #(df['DS']=='SEGME') \
    ]

# Filtering for comparing help-free ranged reads.
# fil = df.loc[
#     (df['SYSTEM']=='INTEL') & \
#     (((df['TESTCASE']==1) & (df['DS']!='SEGHF')) | ((df['TESTCASE']==20) & (df['DS']=='SEGHF'))) & \
#     (df['TXN_SIZE']==2)\
#     ]

# Filtering for comparing help-free random reads.
# fil = df.loc[
#     (df['SYSTEM']=='INTEL') & \
#     (((df['TESTCASE']==4) & (df['DS']!='SEGHF')) | ((df['TESTCASE']==21) & (df['DS']=='SEGHF'))) & \
#     (df['TXN_SIZE']==5)\
#     ]

# print(fil)

# There's normally a grey-ish background for some reason. This makes it completely white
sns.set_style(style='white')

# Plot the data. The seaborn.lineplot documentation (Google) gives descriptions of each of these paramters (and more)
g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DS', style='DS', err_style=None, markers=True, dashes=False)

# Label the axes
plt.xlabel('Threads')
plt.ylabel('Throughput (OP/s)')
plt.title("Test")

##### From stack overflow. no idea how or why this block of code works #####
plt.gca()
yfmt = ScalarFormatterForceFormat()
yfmt.set_powerlimits((0,0))
plt.gca().yaxis.set_major_formatter(yfmt)
############################################################################

# Finally, display the graph
plt.show()