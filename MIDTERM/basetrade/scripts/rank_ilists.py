#!/usr/bin/python

from .pythonUtils import *
import os
import sys
import re
from math import sqrt
from copy import deepcopy
import json

USAGE = "%s <shortcode> <time-period> <rank-method[0=real,1=sim(till not implemented)]> <check-stdev=0>" % sys.argv[0]
if len(sys.argv) < 4:
    print(USAGE)
    exit()


def getTimePeriodString(tm):
    if tm == "US":
        return "US_MORN_DAY"
    if tm == "EU":
        return "EU_MORN_DAY"
    if tm == "EUS":
        return "EU_US_MORN_DAY"
    return tm


vol_cutoff = 0
ttc_cutoff = 3000
ppc_cutoff = -5.0


def setVolTtcPpcCutoffs(prune_strat_config_line):
    if not prune_strat_config_line:
        return
    global vol_cutoff_, ppc_cutoff, ttc_cutoff
    t = prune_strat_config_line.strip().split()
    vol_cutoff, ttc_cutoff, ppc_cutoff = int(t[2]), int(t[3]), float(t[1])


def getStdevOfAModel(strat):
    t = open(getFullPath(strat)) .readlines()[0].strip().split()
    stdev_model = float(RunCommand(
        "%s/get_stdev_model.pl %s TODAY-5 TODAY-1 %s %s | head -n1 | awk '{print $1}'" % (modelscript_dir, t[3], t[5], t[6]))[0].strip())
    return stdev_model


def getModelParamExecfromStrat(strat):
    dt = open(getFullPath(strat)).readlines()[0].strip().split()
    return [dt[2], os.path.basename(dt[3]), os.path.basename(dt[4])]


def getGenstratInfoFromModelName(model):
    ret = re.sub(
        r'.*(ilist_.*|config.*)_([\.0-9]*)_([na][ac]_[te][0-9])_(.*)_([0-9]*_[0-9]*_[0-9]*)_(f[^A-Z]*)_([A-Z]*).*', r'\1 \2 \3 \5 \6 \7', model) . split()
    return ret


def get_stats(line):
    x = line.strip().split()
    try:
        st = x[3]
    except:
        print(x)
    # pnl * sqrt( vol ) /( ttc * dd )
    pnl, vol, ttc, dd = int(x[0]), int(x[1]), int(x[6]), float(x[4])
    if vol < vol_cutoff or ttc > ttc_cutoff or pnl < ppc_cutoff * vol:
        return None
    score = 0.0
    try:
        score = sqrt(vol / (ttc * dd))
        if pnl > 0:
            score = pnl * score
        else:
            score = pnl / score
    except:
        print(line)  # shoul dnot happen but still a check
        return None

    return (st, [pnl, vol, dd, ttc, 1])


def score3(sc):  # arrray of [pnl, vol, ttc, dd ]
    a = [0, 0, 0, 0]
    for s in sc:
        for i in range(len(s)):
            a[i] += s[i]
    for i in range(len(s)):
        a[i] /= len(sc)  # may median ttc would be better but lets see
    try:
        score = sqrt(a[1] / (a[2] * a[3]))
    except:
        score = 1.0
    if a[0] > 0:
        return a[0] * score
    else:
        return a[0] / score


def score2(a):
    n = len(a)
#    for i in xrange( n-1 ):
#        a[i]/=float(a[-1])
    try:
        score = sqrt(a[1] / (a[2] * a[3]))
        if a[0] > 0:
            score = a[0] * score
        else:
            score = a[0] / score
    except:
        score = 1.0
    return score * sqrt(a[-1])


def InsertScore(dictionary_, key, score):
    try:
        for i in range(len(score)):
            dictionary_[key][i] += score[i]
    except:
        dictionary_[key] = deepcopy(score)


def collectStrats(shc, time_period, rankm):
    # strats=[];
    # if rankm==0:
    #     strats = RunCommand("~/basetrade/scripts/rank_hist_queries.pl %s %s TODAY-30 TODAY-1 | awk '{print $3}'" % ( time_period, shc) )[0]
    # else:
    #     strats = RunCommand ( "ls ~/modelling/strats/%s/*/* 2>/dev/null | sed 's/\/.*\///g'" % shc )[0];

    prune_strat_config = RunCommand("perl ~/basetrade/GenPerlLib/get_prune_strats_config.pl %s %s" %
                                    (shc, time_period))[0]  # . strip() . split(); # Not used currently
    setVolTtcPpcCutoffs(prune_strat_config)
    # print "SumLocalParams: ", sum_local_params
    # with open( "%s/rank_ilist_tmp" % home_dir, 'w' ) as f: f.write ( ''.join(strats) );
    # st_end_date="%d %d" % ( getPrevDate(40), getPrevDate(3))
    cmd = "%s/ss_noc.sh %s %s 30 TODAY-1 1 INVALIDFILE" % (script_dir, shc, time_period)
    stats_t = [x for x in (RunCommand(cmd)[0]).split('\n') if x]
    eligible_strats = [get_stats(x) for x in stats_t]
    return [x for x in eligible_strats if x]


shortcode_list = ['FGBS_0', 'FGBM_0', 'FGBL_0', 'FESX_0', 'FDAX_0', 'FBTS_0', 'FBTP_0', 'FOAT_0',  # EUREX
                  'ZF_0', 'ZB_0', 'ZN_0', 'UB_0',  # CME
                  'LFZ_0', 'LFR_0', 'KFFTI_0', 'JFFCE_0', 'LFI_6', 'LFI_5',  # LIFFE
                  'BR_DOL_0', 'BR_WIN_0', 'BR_IND_0', 'DI1F16', 'DI1F17', 'DI1F18',  # BMF
                  'CGB_0',  # MX
                  ]

#shortcode_list = [ 'FGBS_0', 'FGBM_0', 'FGBL_0' ]

shc = sys.argv[1]
time_period = getTimePeriodString(sys.argv[2])
rankm = sys.argv[3]
see_stdev = False
send_mail = True
write_to_db = False
if len(sys.argv) > 4:
    see_stdev = int(sys.argv[4]) > 0

if shc == 'ALL':
    send_mail = True  # False
    write_to_db = True
else:
    shortcode_list = [shc]

# Things to consider
# 1) paramfile
# 2) TradingLogic  --  Not that important
# 3) ilist_name
# 4) regression lookg ahead( 2, 32, 96) and algo( na_t3, na_e3 etc.)
# 5) datagen params
# 6) filter
# 7) Regress Exec

paramfile = dict()
trading_logic = dict()
ilist = dict()
reg_la = dict()
reg_algo = dict()
regress_exec = dict()
datagen_param = dict()
filters = dict()
stdevs = []  # not sure how to interpret it

FinalOutput = dict()  # to dump in the database

out_fl = open('/home/%s/email_file' % user, 'w')
back_stdout = sys.stdout
if send_mail:
    sys.stdout = out_fl

for shc in shortcode_list:
    FinalOutput[shc] = {}
    print("\n\n========================\nRunning for ", shc)
    r = 0
    for (st, score) in collectStrats(shc, time_period, rankm):
        #score = 1
        # ['PriceBasedAggressiveTrading', 'w_model_ilist_Sin_Sin_2_na_t3_bla_bla', 'param_LFI_4_xyz_0']
        p = getModelParamExecfromStrat(st)
        InsertScore(trading_logic, p[0], score)
        InsertScore(paramfile, p[2], score)
        r += 1
        q = getGenstratInfoFromModelName(p[1])  # ilist, reg_la, reg_algo, datagen_param, filter, regress_exec
        if len(q) < 6:
            sys.stderr.write('Could not find all Info for model: %s stra: %s\n' % (p[1], st)); continue
        InsertScore(ilist, q[0], score)
        InsertScore(reg_la, q[1], score)
        InsertScore(reg_algo, q[2], score)
        InsertScore(datagen_param, q[3], score)
        InsertScore(filters, q[4], score)
        InsertScore(regress_exec, q[5], score)
        if see_stdev and r <= 20:
            stdevs.append(getStdevOfAModel(st))  # report top 20 strats' stdevs

        things = [("paramfile", paramfile), ("trading_logic", trading_logic), ("ilist", ilist), ("reg_la", reg_la),
                  ("reg_algo", reg_algo), ("regress_exec", regress_exec), ("datagen_param", datagen_param), ("filters", filters)]

    for t in things:
        print("\n#", t[0], "#")
        FinalOutput[shc][t[0]] = {}
        st_t = sorted(list(t[1].items()), key=lambda k_v: score2(k_v[1]),  reverse=True)

        for v in st_t:
            print(v[0], ' : %.2f %d' % (score2(v[1]), v[1][-1]))
            FinalOutput[shc][t[0]][v[0]] = [score2(v[1]), v[1][-1]]
        print("------------")
    for t in things:
        t[1].clear()

# STDEV calcualtion commmented for time being ( correct code )
    if see_stdev:
        print("# Stdevs: # ")
        print('\n'.join(["%.3f" % x for x in stdevs]))

sys.stdout = back_stdout
out_fl.close()

if write_to_db:
    JSON_DB_File = "/spare/local/ModellingElements/modelling_element_db_%s.json" % time_period
    with open(JSON_DB_File, 'wb') as jdb_file:
        json.dump(FinalOutput, jdb_file, sort_keys=True,
                  indent=2, separators=(',', ':'));

if not send_mail:
    exit()
# Import smtplib for the actual sending function
import smtplib
# Import the email modules we'll need
from email.mime.text import MIMEText
# for email support
out_fl.close()
msg = MIMEText(open('/home/%s/email_file' % user, 'rb'). read())
msg['Subject'] = "Rankings of Modeling Elements"
msg['From'] = 'dvcap@%s' % machine_name
msg['To'] = my_email()

if not msg['To']:
    print(msg.as_string())
else:
    s = smtplib.SMTP('localhost')
    s.sendmail(msg['From'], msg['To'], msg.as_string())
s.quit()
