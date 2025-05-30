#!/usr/bin/python


#######################
# To check whether all the indicators in a model are +vely contributive or not.
#
# Runs Sim_strategy for the given <strat file> for the last few datys given in arg[2].
# Strating from month of arg[2] to 20120730
# Runs sim parallaly over time. (12 at a time)
#
# TODO: to add better sorting algrithm
########################

import os
import sys
import re
import time
import math
import random
from shutil import copy2
from random import random
from subprocess import Popen, PIPE
from datetime import datetime

# my own library...:)
from .pythonUtils import *

tmp_dir_path = os.path.expanduser('~/temp_pnl/')
current_dir_path = os.getcwd()
pid = random.randint(0, 2544452)
res = dict()
keep = dict()
Ilistdata = []


def score(t):
    return (t[0] * t[2] + t[3] * t[5]) / (t[2] + t[5]) - (t[1] + t[4]) / 10


def getIndPerf(stfile, fromDate=20120801, toDate=20121030, flag_prune_model=0):
    stratfile = createNewIlist(stfile)
    runParallalSimAndgetResult(stratfile, fromDate, toDate)
    # do whatever you want
    sum_res = SummarizeResult(res)
    if not sum_res:
        sys.stderr.write(stfile + " file is bad. Please look into it")
        cleanUp(stratfile)
        return None
    if not flag_prune_model:
        cleanUp(stratfile)
        return sum_res

    style = 1
    if style == 1:
        # style 1: ... remove the indicators those are not performing well.
        scores_ = [score(sum_res[i]) for i in sum_res]
        sc_mean_stdev = meanStdev(scores_)
        cutoff_ = sc_mean_stdev[0] - 0.2 * sc_mean_stdev[1]
        mdl_data = ''.join(Ilistdata[:3]);
        rej = 0
        for i in range(len(scores_)):
            if scores_[i] > cutoff_ or scores_[i] > 0:
                mdl_data += Inds[i]
            else:
                rej += 1
        if rej == 0:
            print("Good Strat: nothing to remove"); cleanUp(stratfile); return None;
        mdl_data += "INDICATOREND"
        prod_code = open(stfile).read().split()[1]
        if not os.path.exists("/home/rahul/myModels/%s/" % prod_code):
            os.makedirs("/home/rahul/myModels/%s/" % prod_code)
        n_mdlfl = "/home/rahul/myModels/%s/pruned_%s" % (prod_code, os.path.basename(open(stfile).read().split()[3]))
        n_stfl = "/home/rahul/myModels/%s/pruned_%s" % (prod_code, os.path.basename(stfile))
        copy2(stfile, n_stfl)
        stfl = open(stfile).read().split()
        stfl[3] = n_mdlfl
        open(n_stfl, 'w').write(' '.join(stfl) + '\n')
        open(n_mdlfl, 'w').write(''.join(mdl_data) + '\n')

    if style == 2:
        # style 2: ... findinig the top x indicators where pnl is maximized...
        i = max(sum_res, key=lambda x: sum_res[x][0])
        print(i)
        if i <= 3:
            print("look what have you done!!!! :-/")
        if i < len(list(sum_res.keys())) - 1:
            print("Seems got a good model.. copying.... ")
            n_mdlfl = "/home/rahul/myModels/%s/pruned_%s" % (prod_code,
                                                             os.path.basename(open(stfile).read().split()[3]))
            copy2(tmp_dir_path + 'model_%d.mdl_0' % i, n_mdlfl)
            stfl = open(n_stfl).read().split()
            stfl[3] = n_mdlfl
            open(n_stfl, 'w').write(' '.join(stfl) + '\n')
        else:
            print("Nothing to move. Good model!!")
    cleanUp(stratfile)
    return None  # sum_res


def SummarizeResult(log):
    # log[date] -> [I1 I2 I3 .. ] performance
    if not log:
        return None
    k = list(log.keys())[0]
    numDays = len(list(log.keys()))
    numInd = len(log[k])
    sys.stderr.write("Total Num Days: %d" % numDays);
    sumRes = dict()
    keep = dict()
    for i in range(1, numInd):
        try:
            # for creatign ilist...
            sumRes[i] = pnlMean([(float(log[k][i].pnl_[0]) / log[k][i].Vol - float(log[k]
                                                                                   [i - 1].pnl_[0]) / log[k][i - 1].Vol) * i for k in log])
            # for creating new strat files....
            #sumRes[i] = sampledMeanStdev([log[k][i].pnl_[0]*math.sqrt(log[k][i].Vol) for k in log])
            #keep[i] = stepAnalyse([log[k][i].pnl_[0] - log[k][i-1].pnl_[0] for k in log]);
        except:
            sys.stderr.write("i: %d, k: %s\n" % (i, str(k)));
            continue
#        sumRes[i].append(meanStdev([ log[k][i].Vol - log[k][i-1].Vol for k in log]))
#        sumRes[i].append(meanStdev([ log[k][i].pnlMINmaxDD[2] - log[k][i-1].pnlMINmaxDD[2] for k in log]))
    return sumRes


def printSummarizedRes():

    return False


Inds = []


def createNewIlist(stfile):
    # preprocessing
    global Inds, tmp_dir_path, Ilistdata
    StratFileData = open(stfile).read().split()
    Ilistdata = open(StratFileData[3]).readlines()
    Inds = [l for l in Ilistdata if "INDICATOR " in l]

    head = ''.join(Ilistdata[:3])
    tail = "INDICATOREND"

    numInds = len(Inds) + 1
    for i in range(10):
        t_path = tmp_dir_path + "/%d/" % i
        if not os.path.exists(t_path):
            os.makedirs(t_path)
            break
        else:
            sys.stderr.write(t_path + " already exist. trying next one!!\n")
    tmp_dir_path = t_path
    os.chdir(tmp_dir_path)

    stfile = os.path.basename(stfile)
    strat_fl_name = "%s_rhl.pfi" % (stfile)
    stratfl = open(strat_fl_name, 'w')

    sys.stderr.write('numInds: %d\n' % numInds);
    for i in range(numInds):
        model_fl_name = "./model_%d.mdl_0" % (i)
    # create model file
        fl = open(model_fl_name, "w")
        fl.write(head + ''.join(Inds[:i]) + tail)
        fl.close()
    # append strategy file.
        StratFileData[3] = model_fl_name
        StratFileData[-1] = str(pid + i)
        stratfl.write(' '.join(StratFileData) + '\n')

    stratfl.close()
    return strat_fl_name


st = ''


def RunSim(dt):
    global st
    uid = random.randint(10000, 99999)
    res1 = RunCommand("~/basetrade_install/bin/sim_strategy SIM %s %d %s ADD_DBG_CODE -1" % (st, uid, dt))
    if "UORI" not in res1[1]:
        print(res1[1])
    cmd = "~/basetrade_install/ModelScripts/get_pnl_stats_2.pl /spare/local/logs/tradelogs/trades.%s.%d | sort -k1 -n" % (
        dt, uid)
    res2 = RunCommand(cmd)
    RunCommand(" rm -rf /spare/local/logs/tradelogs/*.%s.%d" % (dt, uid))
    # return '~'.join(res[0].strip().split('\n'));
    # print res1[0], '\n------\n', res2[0]
    return MultSimResults(res1[0].strip().split('\n'), res2[0].strip().split('\n'))


def runParallalSimAndgetResult(strat_fl_name, fromDate, toDate):
    global st
    st = strat_fl_name
    dates = [dt for dt in itertools.islice(date_generator(datetime.today()), 20)]
    from multiprocessing import Pool
    pool = Pool(7)
    simresults = pool.map(RunSim, dates)
    global res
    for d, sres in zip(dates, simresults):
        res[d] = sres

# score PNL*sqrt(Vol)
# def score(res, DD):
#    pnl = res[0]
#    vol = res[1]
#    adjpnl = pnl+300
#    sc = adjpnl * vol**0.5 / DD  # kCNADDSqrtVolPNL
#    return sc


def cleanUp(stratfile):
    # cleaning the mess!!!!
    global tmp_dir_path
    os.chdir(current_dir_path)
    # print current_dir_path
    os.system("rm -rf %s" % tmp_dir_path)
    tmp_dir_path = os.path.expanduser('~/temp_pnl/')


if sys.argv[0] == __file__:
    import hashlib
    pid = pid + int(hashlib.sha256(sys.argv[1]).hexdigest()[:3], base=16)
    if os.path.exists('/home/rahul/myModels/pruned_' + os.path.basename(sys.argv[1])):
        sys.stderr.write("Already pruned!!\n")
        exit(0)
    p = getIndPerf(getFullPath(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), 1)
    if p:
        print('\n'.join([str(x) + ' => ' + ' '.join(Ind.strip().split()[2:-2]) + "\t:\t%d %f" %
                         (p[x][0], float(p[x][0]) / p[x][1]) for (Ind, x) in zip(Inds, p)]))
