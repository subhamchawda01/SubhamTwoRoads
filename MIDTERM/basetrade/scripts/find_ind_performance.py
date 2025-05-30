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
from random import random
from subprocess import Popen, PIPE
from datetime import datetime

# my own library...:)
from .pythonUtils import getNextDate, logDB, MultSimResults, meanStdev, getFullPath

Usage = "%s <start file/ strategy name> startDate [endDate]" % sys.argv[0]
if len(sys.argv) < 4:
    print(Usage)
    exit(0)
stfile = sys.argv[1]


cmd = "~/basetrade/scripts/get_iso_date_from_str_min1.pl %s" % (sys.argv[2])
fromDate = int(Popen(cmd, shell=True, stdout=PIPE) . stdout . readlines()[0])

cmd = "~/basetrade/scripts/get_iso_date_from_str_min1.pl %s" % (sys.argv[3])
toDate = int(Popen(cmd, shell=True, stdout=PIPE) . stdout . readlines()[0])

#fromDate = int(sys.argv[2])
#toDate = int(sys.argv[3])

# preprocessing
stfile_path = getFullPath(stfile)
if stfile_path:
    StratFileData = open(stfile_path).read().split()
else:
    print("No such strat file!! Sorry.")
    exit(0)
Ilistdata = open(StratFileData[3]).readlines()

shortcode_ = StratFileData[1]

Inds = Ilistdata[3:-1]
head = ''.join(Ilistdata[:3])
tail = Ilistdata[-1]

pid = 25452  # int(random()*10000)
numInds = len(Inds) + 1
tmp_dir_path = '~/temp_pnl/'
tmp_dir_path = os.path.expanduser(tmp_dir_path)
print(tmp_dir_path)

if not os.path.exists(tmp_dir_path):
    os.makedirs(tmp_dir_path)
os.chdir(tmp_dir_path)

stfile = os.path.basename(stfile)
strat_fl_name = "%s_rhl.pfi" % (stfile)
stratfl = open(strat_fl_name, 'w')


log_fl_name = os.path.join(logDB, 'res_strat_%s_%d_%d.txt' % (stfile, fromDate, toDate))
# Dont run if previous data is available
# if os.path.exists(log_fl_name): print "You have run it previously. The data is in %s.." % log_fl_name; exit(0);

for i in range(len(Inds) + 1):
    model_fl_name = "./model_%d.mdl_0" % (i)
    # create model file
    fl = open(model_fl_name, "w")
    fl.write(head)
    fl.write(''.join(Inds[:i]))
    fl.write(tail)
    fl.close()

    # append strategy file.
    StratFileData[3] = model_fl_name
    StratFileData[-1] = str(pid + i)
    stratfl.write(' '.join(StratFileData) + '\n')
stratfl.close()

dt = dict()
results = [[0, 0, 0, 0, 0, 0] for i in range(numInds)]
# last one is score

# run sim strategy in parallal for each day and for the strategt file.
num_sim_count = 0
date_ = fromDate

while (date_ < toDate):
    #    print date_
    cmd = "~/basetrade/scripts/get_market_model_for_shortcode.pl %s" % (shortcode_)
    mkt_model_ = int(Popen(cmd, shell=True, stdout=PIPE) . stdout . readlines()[0])

    cmd = "~/basetrade_install/bin/sim_strategy  SIM %s %d %d %d ADD_DBG_CODE -1 2>/dev/null | grep SIMRESULT &" % (
        strat_fl_name, pid + date_, date_, mkt_model_)

#    if os.path.isfile('/spare/local/logs/tradelogs/trades.%d.%d' % (date_, pid+date_)
    dt[date_] = Popen(cmd, shell=True, stdout=PIPE)
    num_sim_count += 1
    if num_sim_count >= 8:
        time.sleep(2)
        num_sim_count -= 2   # TO control the spwing. Resource Crunch!!!
    date_ = getNextDate(date_)


cnt = 0
time.sleep(30)  # Wait for running sims to terminate.
cnt = 0


# score PNL*sqrt(Vol)
def score(res, DD):
    pnl = res[0]
    vol = res[1]
    DD = DD
    adjpnl = pnl + 300
    sc = adjpnl * vol**0.5 / DD  # kCNADDSqrtVolPNL
    return sc


log = dict()
# read the resutls and analysize
lines = ''
for k in dt:
    lines = dt[k].stdout.readlines()

    if not lines or lines[0] == 'SIMRESULT 0 0 0 0 0 0\n':
        continue

    cnt += 1
    cmd = "~/basetrade_install/ModelScripts/get_pnl_stats_2.pl /spare/local/logs/tradelogs/trades.%d.%d" % (k, k + pid)
    DD = Popen(cmd, shell=True, stdout=PIPE).stdout.readlines()
    # Remove log and trades file.
    for f in ["/spare/local/logs/tradelogs/trades.%d.%d /spare/local/logs/tradelogs/log.%d.%d" % (k, k + pid, k, k + pid)]:
        try:
            os.remove(f)
        except:
            continue
    Popen(cmd, shell=True, stdout=PIPE)

    log[k] = MultSimResults(lines, DD)
    for i, l in enumerate(lines):
        l = l.strip().split()[1:]
        dd = DD[i].strip().split()[-1]
        if l[1] == 0:
            continue
        results[i][0] += int(l[0])
        results[i][1] += int(l[1])
        results[i][2] += float(l[2]) * int(l[1]) / 100.0
        results[i][3] += float(l[3]) * int(l[1]) / 100.0
        results[i][4] += float(l[4]) * int(l[1]) / 100.0
        results[i][5] += score(results[i], float(dd))


if cnt == 0:
    print("No days to show...", date_, toDate)
    exit(0)

# have all the data in array 'log', log[<date>][i] is the sim data for the day <date> and strategy_i
# logging
if not os.path.exists(logDB):
    os.system('mkdir -p %s' % logDB)

f = open(log_fl_name, 'w')
f.write("%s %d %d\n" % (stfile, fromDate, toDate))
f.write("\n# Vol [finalPNL median avg stdev sharpe fracpos min max DD] avg_absolute_pos [medianTTC avgTTC] [bestlvlTrades nonBestLvlTrades aggrTrades]")
for i, l in enumerate(lines):
    f.write("\n\n#With only top %d indicators..\n" % i)
    f.write('\n'.join(['%d %s' % (k, log[k][i].str()) for k in log]))
f.close()

# Result print.
print("Total Number of Days: %d ( %d to %d )" % (cnt, fromDate, toDate))
print("NumIndicatros -- ProfitNLoss -- TradedVol -- NonBestLevel -- BestLevel ---- Aggressive ---- score")
for t, l in enumerate(results):
    print('%4d\t' % t, '\t'.join(["%10d" % int(i / cnt) for i in l]), end=' ')
    print(' '.join(["%.2f" % x for x in meanStdev([log[k][t].pnl_[0] for k in log])]))
print('\n Detailed results are in %s' % log_fl_name)


# cleaning the mess!!!!
os.system("rm ./model_*.mdl_0")
os.system("rm ./%s_rhl.pfi" % stfile)
for k in dt:
    os.system("rm -f /spare/local/logs/tradelogs/*.%d.%d" % (k, k + pid))  # Removing Trade and log files.
