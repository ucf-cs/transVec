import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
import numpy as np 
import pandas as pd
import seaborn as sns
import sys

structures = ['SEGME', 'COMPA', 'BOOST', 'STMVE', 'STOVEC']
systems    = ['INTEL', 'AMD', 'ARM', 'NUMA']

# Took this off stack overflow, not really sure how it works, but it keeps the y-axis in scientific notation
class ScalarFormatterForceFormat(ScalarFormatter):
    def _set_format(self):  # Override function that finds format to use.
        self.format = "%1.1f"  # Give format here

# Read in the file as a Pandas DataFrame
col = ['DATA STRUCTURE', 'TESTCASE', 'SGMT_SIZE', 'NUM_TXN', 'TXN_SIZE', 'THRD_CNT', 'TIME', 'ABORTS', 'SYSTEM', 'THRUPUT', 'RELATIVE', 'JOINT']
df  = pd.read_csv('all_systems_final renamed.txt', sep='\t', names=col, skiprows=1)

# The plot settings (There are a few settings you can mess around with. They're all on Google. I'll be using the defaults)
sns.set(rc={'figure.figsize':(7.5,5)})

# Filter the data (Add or remove as many parameters as you want)
#fil = df.loc[ \
#    (df['SYSTEM']=='INTEL') & \
#    (df['TESTCASE']==4) & \
#    (df['TXN_SIZE']==5) \
#    #(df['DATA STRUCTURE']=='BOOST') \
#    ]

# print(fil)

# There's normally a grey-ish background for some reason. This makes it completely white
#sns.set_style(style='white')

# Plot the data. The seaborn.lineplot documentation (Google) gives descriptions of each of these paramters (and more)
#g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DATA STRUCTURE', style='DATA STRUCTURE', err_style=None, markers=["o","s","P","D","^"], dashes=False)

#Relative plot
#g = sns.lineplot(data=fil, x='THRD_CNT', y='RELATIVE', hue='SYSTEM', style='SYSTEM', markers=["o","s","P","D","^"], dashes=False)

# Label the axes
#plt.xlabel('Threads')
#plt.ylabel('Throughput (OP/s)')

##### From stack overflow. no idea how or why this block of code works #####
plt.gca()
yfmt = ScalarFormatterForceFormat()
yfmt.set_powerlimits((0,0))
plt.gca().yaxis.set_major_formatter(yfmt)
############################################################################

# Format and save, rather than display.
#sns.set_style(style='white')
#plt.xlabel('Threads')
#plt.ylabel('Relative Throughput')
#g.set(yscale='log')
#g.get_figure().savefig("BOOST_tc14txn5" + ".pdf")
#plt.clf()

# Finally, display the graph
#plt.show()


# Looping and outputting a buttload of graphs
# for s in systems:
#     for i in range(1, 20):
#         for j in range(1, 6):
#             # Filter the data
#             fil = df.loc[(df['SYSTEM']==s) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j)]
#             sns.set_style(style='white')
#             g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DATA STRUCTURE', style='DATA STRUCTURE', markers=["o","s","P","D","^"], dashes=False)
#             plt.xlabel('Threads')
#             plt.ylabel('Throughput (OP/s)')
#             g.set(yscale='log')
#             g.get_figure().savefig("output/" + s + "_tc" + str(i) + "txn" + str(j) + ".pdf")
#             plt.clf()

# Looping and outputting a buttload of graphs
# for d in structures:
#     for i in range(1, 20):
#         for j in range(1, 6):
#             # Filter the data
#             fil = df.loc[(df['DATA STRUCTURE']==d) & (df['TESTCASE']==i) & (df['TXN_SIZE']==j)]
#             sns.set_style(style='white')
#             g = sns.lineplot(data=fil, x='THRD_CNT', y='RELATIVE', hue='SYSTEM', style='SYSTEM', markers=["o","s","P","D","^"], dashes=False)
#             plt.xlabel('Threads')
#             plt.ylabel('Relative Throughput')
#             g.set(yscale='log')
#             g.get_figure().savefig("output/" + d + "_tc" + str(i) + "txn" + str(j) + ".pdf")
#             plt.clf()

# Start fresh
plt.clf()

# Intel, test case 4, txn size 5
fil = df.loc[ \
   (df['SYSTEM']=='INTEL') & \
   (df['TESTCASE']==4) & \
   (df['TXN_SIZE']==5) \
   ]
sns.set_style(style='white')
g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DATA STRUCTURE', style='DATA STRUCTURE', markers=["o","s","P","D","^"], dashes=False, ci=None)
plt.xlabel('Threads')
plt.ylabel('Throughput (OP/s)')
g.grid(False)
#g.set(yscale='log')
g.get_figure().savefig("output/" + 'INTEL' + "_tc" + str(4) + "txn" + str(5) + ".pdf")
plt.clf()

# Boost and compact, all arches, test case 7 (or 14), txn size 5
fil = df.loc[ \
   (df['TESTCASE']==7) & \
   (df['TXN_SIZE']==5) & \
   ((df['DATA STRUCTURE']=='TB') | (df['DATA STRUCTURE']=='Compact')) \
   #(df['DATA STRUCTURE']=='COMPA') \
   ]
sns.set_style(style='white')
g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='SYSTEM', style='DATA STRUCTURE', markers=["o","s","P","D","^"], dashes=False, ci=None)
plt.xlabel('Threads')
plt.ylabel('Throughput (OP/s)')
g.grid(False)
g.axes.ticklabel_format(style='sci', scilimits=(0,0), axis='y')
#g.set(yscale='log')
#g.get_figure().savefig("output/" + 'COMPACT_BOOSTED' + "_tc" + str(7) + "txn" + str(5) + ".pdf")
plt.show()
plt.clf()

# Intel, test case 5, txn size 5
fil = df.loc[ \
   (df['SYSTEM']=='INTEL') & \
   (df['TESTCASE']==5) & \
   (df['TXN_SIZE']==5) \
   ]
sns.set_style(style='white')
g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DATA STRUCTURE', style='DATA STRUCTURE', markers=["o","s","P","D","^"], dashes=False, ci=None)
plt.xlabel('Threads')
plt.ylabel('Throughput (OP/s)')
g.grid(False)
#g.set(yscale='log')
g.get_figure().savefig("output/" + 'INTEL' + "_tc" + str(5) + "txn" + str(5) + ".pdf")
plt.clf()

# Intel, test case 11, txn size 5
fil = df.loc[ \
   (df['SYSTEM']=='INTEL') & \
   (df['TESTCASE']==11) & \
   (df['TXN_SIZE']==5) \
   ]
sns.set_style(style='white')
g = sns.lineplot(data=fil, x='THRD_CNT', y='THRUPUT', hue='DATA STRUCTURE', style='DATA STRUCTURE', markers=["o","s","P","D","^"], dashes=False, ci=None)
plt.xlabel('Threads')
plt.ylabel('Throughput (OP/s)')
g.grid(False)
#g.set(yscale='log')
g.get_figure().savefig("output/" + 'INTEL' + "_tc" + str(11) + "txn" + str(5) + ".pdf")
plt.clf()