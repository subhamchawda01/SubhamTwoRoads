#!/usr/bin/python

import os
import sys
import re
from subprocess import Popen, PIPE
from datetime import datetime

r = re.compile('[ \n\r\t]+')


class TimeData:
    def __init__(self, line):
        line = r.split(line.strip())
        self.tStamp = int(line[0])
        self.l1events = int(line[1])
        self.price = [float(line[2]), float(line[3])]
        self.indData = [float(x) for x in line[4:]]

    def Time(self):
        return self.tStamp

    def L1Events(self):
        return self.l1events

    def str(self):
        return "%d %d %s" % (self.tStamp, self.l1events, ' '.join(['%f' % x for x in self.indData]))


if len(sys.argv) < 2:
    print("%s <timed_data_file> <algo>" % sys.argv[1])
    exit(0)


tData = open(sys.argv[1]).readlines()
n = len(tData)
# algo na_e3
TotNumL1Events = TimeData(tData[-1]).L1Events() - TimeData(tData[0]).L1Events()
TotTime = TimeData(tData[-1]).Time() - TimeData(tData[0]).Time()
t_gap = (TotTime / float(TotNumL1Events)) * 3.00
print(t_gap, TotNumL1Events, TotTime)
Weights = [0.333, 0.667, 1.00]
prediction_gaps = [t_gap * w for w in Weights]

from os import path
opfname = path.join(path.dirname(sys.argv[1]), 'reg_data_' + path.basename(sys.argv[1]))
opf = open(opfname, 'w')

if len(sys.argv) > 2:
    base_data = open(sys.argv[2]).readlines()
else:
    base_data = tData

j = i = 0
prev_i = i
while j < len(base_data):
    base_l = TimeData(base_data[j])
    j += 1
    reg_d = 0
    i = prev_i
    for tg, w in zip(prediction_gaps, Weights):
        while i < n and (TimeData(tData[i]).Time() - base_l.Time()) < tg:
            i += 1
        if i >= n:
            print('Gone out of range....!!!!')
            exit(0)
        pred_l = TimeData(tData[i])
        reg_d += (pred_l.price[1] - base_l.price[0]) * w
    prev_i = i - 10
    opf.write("%f %s\n" % (reg_d, ' '.join(["%.5f" % x for x in base_l.indData])))

opf.close()
