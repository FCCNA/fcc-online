import numpy as np
import sys
import matplotlib.pyplot as plt

with open(sys.argv[1],"r") as f:
    lines = f.readlines()
    lines = [line.replace("\n","") for line in lines]

currents = []
voltages = []
for line in lines:
    volt,curr = line.split(" ")
    currents.append(float(curr))
    voltages.append(float(volt))
    print("%.2f %.2f" % (float(curr), float(volt)))

fig, ax = plt.subplots()
ax.scatter(voltages,currents)
ax.xaxis.set_major_formatter(plt.FormatStrFormatter('%.2f'))
ax.yaxis.set_major_formatter(plt.FormatStrFormatter('%.2f'))
plt.yscale("log")

plt.show()
