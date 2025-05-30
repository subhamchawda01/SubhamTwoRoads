#!/usr/bin/python

import os
import sys
import re
from subprocess import Popen, PIPE
from datetime import datetime

if len(sys.argv) < 2:
    print "%s <timed_data_file_name> [<output file name>]" % sys.argv[0]
    exit(0)

time_data = open(sys.argv[1])
prod = sys.argv[2]
date = sys.argv[3]
os.system("prod=% s_0
          dt=% s
          ~ / basetrade_install / bin / mds_log_l1_trade $prod $dt |
          grep L1 | awk '{ if (prevb!=$5 || preva!=$6) {print $2; prevb=$5; preva=$6}}}' > .tmp__"% (prod, date))
l1_eventData = open(".tmp__")


line = time_data.readline()
r = re.compile('[ \t\n\r]+')

from os import path
output_flname = path.join(path.dirname(sys.argv[1]), 'pc_filtered_' + path.basename(sys.argv[1]))
if len(sys.argv) > 2:
    opf = open(output_flname, 'w')
else:
    opf = sys.stdout
# na_t3


class Timedata:
    def __init__(line):
        dt = r.split(line.strip())
        self.tstamp = int(dt[0])
        self.l1events = int(dt[1])
        self.rest_info = dt[2:]

    def str():
        return "%d %d %s" % (self.tstamp, self.l1events, self.rest_info)

    def ttime():
        return self.tstamp


if len(sys.argv) == 3:
    output_flname = sys.argv[2]


def getNextL1eventTime():
    ll = l1_eventData.readline().strip()
    if not ll:
        return False
    tm = datetime.fromtimestamp(float(ll))
    return (tm.hour * 3600 + tm.minute * 60 + tm.second) * 1000 + tm.microsecond / 1000


while line:
    stTime = int(r.split(line)[0])
    l1tm = getNextL1eventTime()
    while l1tm < stTime:
        l1tm = getNextL1eventTime()
        if not l1tm:
            print 'L1 event file ended...sorry!!!'
            exit(0)
    while line and int(r.split(line)[0]) < l1tm:
        prevLine = line
        line = time_data.readline()
        #opf.write('0 '+prevLine);
    if line:
        opf.write(prevLine)
    else:
        break


time_data.close()
l1_eventData.close()
os.remove(".tmp__")
opf.close()
