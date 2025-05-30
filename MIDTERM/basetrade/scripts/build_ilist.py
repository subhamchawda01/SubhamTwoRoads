#!/usr/bin/python

import os
import sys
import re
import time
from random import random
from subprocess import Popen, PIPE
from datetime import datetime
# my own library...:)
from .pythonUtils import *
from .find_ind_performance_lib import getIndPerf

Usage = "%s short_code cutoff[0.00] [EU/US] list of ilsittype[1]\n" % sys.argv[0]
Usage += "1 Mid_Mid\t2 Mid_Mkt\t3 Mkt_Mkt\n4 OMix_OMix\t5 Sin_Sin\t6 Owp_Owp\n"

PriceTypePair = {
    1: ["Midprice Midprice", "Mid_Mid"],
    2: ["Midprice MktSizeWPrice", "Mid_Mkt"],
    3: ["MktSizeWPrice MktSizeWPrice", "Mkt_Mkt"],
    4: ["OfflineMixMMS OfflineMixMMS", "OMix_OMix"],
    5: ["MktSinusoidal MktSinusoidal", "Sin_Sin"],
    6: ["OrderWPrice OrderWPrice", "Owp_Owp"]
}
cutoff = 0.0
time_period = "US"
list_of_files = [0]
IndList = dict()
IndList_t = dict()
if len(sys.argv) < 2:
    print(Usage)
    exit(0)

shortcode_ = sys.argv[1]
if len(sys.argv) > 2:
    cutoff = float(sys.argv[2])
if len(sys.argv) > 3:
    time_period = sys.argv[3]
if len(sys.argv) > 4:
    list_of_files = [int(x) for x in sys.argv[4:]]


use_given_strats_ = False


if len(sys.argv) < 2:
    print(Usage)
    exit(0)


def getStrats(shortcode):
    if use_given_strats_:
        return sys.argv[2:];
    cmd = "~/basetrade_install/scripts/rank_hist_queries.pl %s %s TODAY-90 TODAY-1 2>/dev/null" % (
        time_period, shortcode)
    tt = RunCommand(cmd)[0].strip() . split('\n')
    strats = [x.strip().split()[2] for x in tt]    # Changed for test;
    if len(strats) < 10:
        cmd = "~/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo kCNAPnlAdjAverage 20 20 -1.0 20 10000 10000000 /NAS1/ec2_globalresults/%s | grep STRATEGYFILEBASE" % (
            shortcode)
        tt = RunCommand(cmd)[0] . strip() . split('\n')
        strats += [x.strip().split()[1] for x in tt]
    print("Total number of strats:",  len(strats))
    return strats


def analyse_Model(stratFile, Inds):
    global IndList
    sum_res = getIndPerf(stratFile, 20121101, 20121230)
    # print "==="
    # print '\n'.join(Inds)
    # print '\n'.join(["%d: %s \t--->\t%s"%(x, Inds[i], str(sum_res[x])) for i, x in enumerate(sum_res)])
    if not sum_res or (len(sum_res) > len(Inds)):
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
    global IndList_t
    Ilistdata = open(open(st_file).read().split()[3]).readlines()
    for l in Ilistdata[3:-1]:
        if l.find("INDICATOR ") == -1:
            continue
        lind = l.strip().split('#')
        if not lind:
            sys.stderr.write('ERR: %s\n' % l); continue;
        lind[0] = ' '.join(lind[0].split()[2:])
        if len(lind) < 2:
            lind.append("")
        IndList_t[lind[0]] = lind[1]
        Inds.append(lind[0])
    return Inds


def score(t):
    return (t[0] * t[2] + t[3] * t[5]) / (t[2] + t[5]) - (t[1] + t[4]) / 10


def printDic(Indlist):
    for k in Indlist:
        print(k, '#', IndList_t[k], ' : ', Indlist[k], ' : ', score(Indlist[k]))


def CreateIlistFiles(Indlist):
    IndicatorPart = '\n'.join(["INDICATOR 1.00 %s # %s" % (k, IndList_t[k])
                               for k in Indlist if score(Indlist[k]) > cutoff])
    for l in list_of_files:
        heading = "MODELINIT DEPBASE %s %s\nMODELMATH LINEAR CHANGE\nINDICATORSTART" % (shortcode_, PriceTypePair[l][0])
        filename_ = "ilist_%s_%s_%s_best_ind.rhl" % (shortcode_, time_period, PriceTypePair[l][1])
        with open(filename_, 'w') as fl:
            fl.write("%s\n%s\nINDICATOREND\n" % (heading, IndicatorPart))
        print("Created ilist_file: %s" % filename_);


for st in getStrats(shortcode_):
    sys.stderr.write("\n======\nChecking for: %s\n" % st);
    fl_st = getFullPath(st)
    if not fl_st:
        continue
    analyse_Model(fl_st, getInds(fl_st))

printDic(IndList)
CreateIlistFiles(IndList)
