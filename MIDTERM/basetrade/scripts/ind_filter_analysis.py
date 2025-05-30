#!/usr/bin/python

import os
import sys
from math import sqrt
import numpy as np
from io import StringIO


def getMeanSD(a):
    mn = a.mean()
    sd = sqrt(a.var())
    return mn, sd


def corr(a, b):
    n = a.size()
    ma, sa = getMeanSD(a)
    mb, sb = getMeanSD(b)
    cov = np.correlate(a, b)
    return (cov - ma * mb) / (n * sa * sb)


#st = int(sys.argv[2])
#en = int(sys.argv[3])
# ind_r=int(sys.argv[2])

data = np.genfromtxt(open(sys.argv[1]))

filters = [(0, 10),
           (0, 1),
           (1, 10),
           (0, 2),
           (2, 10),
           (0, 3),
           (3, 10),
           (2, 5)]

dep = data[:, 0]


def Applyfilter(a, b, gt, lt):
    mn, sd = getMeanSD(b)
    gt = gt * sd
    lt = lt * sd
    a = [x for x in zip(a, b) if abs(x[1] - mn) > gt and abs(x[1] - mn) < lt]
    x, y = list(zip(*a))
    return np.corrcoef(x, y)[0, 1]


# filters f0, fsl
IndicatorName = []


def getIndicatorsName():
    indF = sys.argv[2]  # Name of the ilist_file
#    indF = 'ilist_file_1.txt'
    for l in open(indF).readlines():
        l = l.strip().split()
        if l[0] != 'INDICATOR':
            continue
        IndicatorName.append(l[2:])


getIndicatorsName()

print('---', '\t'.join(["(%3d,%-3d)" % (f[0], f[1]) for f in filters]))
for i in range(1, len(IndicatorName)):
    cr = []
    mf = 0
    for fl in filters:
        #        try:
        cr.append(Applyfilter(data[:, 0], data[:, i], fl[0], fl[1]))
        # except: cr.append(0.00)
    if max(cr) > .08:
        mf = filters[cr.index(max(cr))]
    elif min(cr) < -0.08:
        mf = filters[cr.index(min(cr))]
    else:
        continue
    print("%3d" % i, '\t'.join(["%-0.6f" % x for x in cr]), '\t', mf, ' '.join(IndicatorName[i - 1]))

exit(0)


# not required any further-----END-----
Sum1 = [float(x) for x in data[0].strip().split()]
Sum2 = [float(x)**2 for x in data[0].strip().split()]
Sum3 = [float(x)**3 for x in data[0].strip().split()]
for l in data[1:]:
    l = l.strip().split()
    for i in range(NumInd):
        t = float(l[i])
        n += 1
        Sum1[i] += t
        Sum2[i] += t * t
        Sum3[i] += t**3


for i in range(NumInd):
    print(Sum1[i] / n, Sum2[i] / n, Sum3[i] / n)
