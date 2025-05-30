#!/usr/bin/python

import os
import sys
import re
import time
from random import random
from subprocess import Popen, PIPE
from datetime import datetime
# my own library...:)
from .pythonUtils import getNextDate, logDB, MultSimResults, meanStdev, getFullPath
from .find_ind_performance_lib import getIndPerf
Usage = "%s strategy_path [startDate=20120901] [endDate=20121025]" % sys.argv[0]
IndList = dict()
if len(sys.argv) < 2:
    print(Usage)
    exit(0)
startDate, endDate = 20120901, 20121025
if len(sys.argv) > 2:
    startDate = int(sys.argv[2])
if len(sys.argv) > 3:
    endDate = int(sys.argv[3])
if startDate < 2011101 or endDate > 20141231:
    sys.stderr.write("Dates [%d, %d] are beyond limit - [20110101 to 20141231]\n" % (startDate, endDate))
    exit(0)


def analyse_Model(stratFile, Inds):
    global IndList
    sum_res = getIndPerf(stratFile, startDate, endDate)
    # print "==="
    # print '\n'.join(Inds)
    # print '\n'.join(["%d: %s \t--->\t%s"%(x, Inds[i], str(sum_res[x])) for i, x in enumerate(sum_res)])
    if len(sum_res) > len(Inds):
        return
    for i, x in enumerate(sum_res.keys()):
        if Inds[i] in IndList:
            mergeResults(Inds[i], sum_res[x])
        else:
            IndList[Inds[i]] = sum_res[x]
            # print "$$$\tAdded:", i, Inds[i], len(IndList.keys())


def mergeResults(ind, res):
    global IndList
    s = IndList[ind]
    n1, n2, n = s[2], res[2], s[2] + res[2]
    print(s, res)
    if n != 0:
        IndList[ind][0] = (s[0] * n1 + res[0] * n2) / n
        IndList[ind][1] = (n1 * s[1] + n2 * res[1]) / n
        IndList[ind][2] = n

    n1, n2, n = s[5], res[5], s[5] + res[5]
    if n != 0:
        IndList[ind][3] = (s[3] * n1 + res[3] * n2) / n
        IndList[ind][4] = (n1 * s[4] + n2 * res[4]) / n
        IndList[ind][5] = n


def pick_from_stats(stat):
    return False


def getInds(st_file):
    Inds = []
    Ilistdata = open(open(st_file).read().split()[3]).readlines()
    for l in Ilistdata[3:-1]:
        if("INDICATOR" not in l):
            continue
        lind = l.strip().split('#', 1)
        if not lind:
            sys.stderr.write('ERR: %s\n' % l); continue;
        lind[0] = ' '.join(lind[0].split()[2:])
        Inds.append(' #'.join(lind))
    return Inds


def score(t):
    return (t[0] * t[2] + t[3] * t[5]) / (t[2] + t[5]) - (t[1] + t[4]) / 10


def printDic(Indlist):
    for k in Indlist:
        print(k, ' : ', Indlist[k], ' : %.2f' % score(Indlist[k]))


for st in [sys.argv[1]]:
    sys.stderr.write("\n======\nChecking for: %s\n" % st);
    fl_st = getFullPath(st)
    if not fl_st:
        continue
    analyse_Model(fl_st, getInds(fl_st))

printDic(IndList)
