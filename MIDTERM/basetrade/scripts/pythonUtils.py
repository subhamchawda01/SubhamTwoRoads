#!/usr/bin/python


import pwd
import os
import sys
import glob
import itertools
import gzip
from subprocess import Popen, PIPE
from datetime import datetime, timedelta

M = ['00', '01', '02', '03', '04', '05', '06', '07', '08', '09', '10', '11', '12']
D = ['00', '01', '02', '03', '04', '05', '06', '07', '08', '09', '10',
     '11', '12', '13', '14', '15', '16', '17', '18', '19', '20',
     '21', '22', '23', '24', '25', '26', '27', '28', '29', '30', '31']

office_email = ["tworoads.co.in", "circulumvite.com"]
office_staff = [['archit', 'rakesh', 'ravi', 'anshul', 'piyush', 'rkumar'],
                ['ankit', 'gchak', 'sourav', 'kputta', 'msinghal', 'sghosh']]

user = pwd.getpwuid(os.getuid())[0]
logDB = '/spare/local/' + user + '/simlogs/'
home_dir = pwd.getpwuid(os.getuid())[5]
bin_path = home_dir + '/basetrade_install/bin/'
script_dir = home_dir + '/basetrade_install/scripts/'
modelscript_dir = home_dir + '/basetrade_install/ModelScript/'
machine_name = os.getenv("HOSTNAME")
today = datetime.today()

#          000 Ja  Fe  Ma  Ap  Ma  Ju Jul
MonthDays = [00, 31, 28, 31, 30, 31, 30, 31,
             31, 30, 31, 30, 31]
#          Au  Se  Oct Nov Dec


def parseDate(date_string):
    yr = int(date_string[:4])
    mn = int(date_string[4:6])
    day = int(date_string[6:])
    return yr, mn, day


def getNextDate(dt):
    dt = int(dt) + 1
    d = dt % 100
    m = (dt / 100) % 100
    y = dt / 10000
    if d > MonthDays[m]:
        d = 1
        m += 1
    if m > 12:
        m = 1
        y += 1
    return y * 10000 + m * 100 + d


def date_generator(st):
    from_date = st
    while True:
        from_date = from_date - timedelta(days=1)
        if from_date.weekday() in range(1, 5):
            yield from_date.strftime("%Y%m%d")


def email(_user_):
    for i in range(len(office_staff)):
        if _user_ in office_staff[i]:
            return '%s@%s' % (_user_, office_email[i])
    # could not find in the list,...
    print(_user_,  '-- not found in ', office_staff)
    return None


def my_email():
    return email(user)


def RunCommand(cmd):
    l = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    out, err = l.communicate()
    # if ( err ) : print cmd, '\n', out, '\n', err
    return [out, err]


def getModelsFromStrat(strat):
    with open(getFullPath(strat)) as f:
        lines = f.readlines()
        if "STRATEGYLINE" == lines[0].strip().split()[0]:
            return [s.strip().split()[3] for s in lines if len(s) > 10]
    return [getFullPath(strat)]


def getIndicatorsFromModel(modelfl):
    with open(modelfl) as mfl:
        return [x for x in mfl.readlines() if x.strip().split()[0] == "INDICATOR"]


def getFullPath(strat):
    if os.path.exists(strat):
        return strat
    l = Popen("find ~/modelling/ -name %s" % strat, shell=True, stdout=PIPE, stderr=PIPE)
    err = l.stderr.readlines()
    l = l.stdout.readlines()
    if not l:
        return None
    if not err:
        return l[0].strip()
    else:
        sys.stderr.write(''.join(err) + '\n')
        return None


def getFullPathInModel(strat, prod='*'):
    if os.path.exists(strat):
        return strat
    path = glob.glob("%s/modelling/strats/%s/*/%s" % (home_dir, prod, strat))
    if path:
        return path[0]
    else:
        return None


def isgz(fname):
    if fname[-3:] == ".gz":
        return True
    return False


def opengz(fname, way='rb'):
    if not os.path.exists(fname):
        fname += ".gz"
    print("opengz::::", fname);
    if isgz(fname):
        # print fname, " is a Gzip file, opening with python gzip library"
        return gzip.open(fname, way)
    else:
        return open(fname, way)


def compDatagen(st1, st2):
    try:
        f1 = opengz(st1).readlines()
        f2 = opengz(st2).readlines()
    except:
        print(st1, 'or', st2, 'does not exist')
        return [-1]
    t = set()
    for l1, l2 in zip(f1, f2):
        l1 = l1.strip().split()
        l2 = l2.strip().split()
        if len(l1) != len(l2):
            print("What is this, num columns are different..:P")
            return [-1]
        i = 0
        for x1, x2 in zip(l1, l2):
            if float(x1) != float(x2) and i > 3:
                t.add(i - 4)
        if (len(t) > 0):
            return [x for x in t]
    return t


def getDatePrev(days_past):
    return RunCmd("date +%Y%m%d -d '-%s days' " % days_past)


def getPrevDate(dt):
    dt = int(dt)
    d = dt % 100
    m = (dt / 100) % 100
    y = dt / 10000
    if d > 1:
        d -= 1
    elif m > 1:
        m -= 1
        d = MonthDays[m]
    else:
        m = 12
        y -= 1
    return y * 10000 + m * 100 + d


class simResult:
    def getPNL(finalPNL, line, minMaxDD):
        line = line.split(':')
        pnl = int(summary_res[6].split(':')[1])
        median = int(line[2].strip()[0])
        avg = int(line[3].strip()[0])
        stdev = int(line[4].strip()[0])
        sharpe = float(line[5].strip()[0])
        fracpos = float(line[6].strip()[0])
        return (pnl, median, avg, stdev, sharpe, fracpos)

    def __inti__(self, summary_line):
        sim_res = sim_res_line.strip().split()
        summary_res = summary_line.strip().split('\n')
        self.avg_absolute_pos_ = float(summary_res[0].split(':')[1])  # "Average Abs position: %.1f\n",
        self.TTC_ = [int(summary_res[1].split(':')[1]), summary_res[6].split(':')[1]]
        # "Median trade-close-time ( secs ): %d\n",
        # "Average trade-close-time ( secs ): %d\n",
        # "TradePNL stats: Median: %d Avg: %d Stdev: %d Sharpe: %.2f Fracpos: %.2f\n",
        self.pnl_ = getPNL(summary_res[6], summary_res[3])
        # printf "FinalPNL: %d\n", $final_pnl_ ;
        # "PNL-min-max-draw: %d %d %d\n", $min_pnl_, $max_pnl_, $max_drawdown_ ;
        self.pnlMINmaxDD = [int(x) for x in summary_res[4].split(':')[1].split()]
#        self.numTradeLines = int(summary_res[5].split(':')[1])# "NumTradeLines : %d\n", $num_trades_ ;
        self.Vol = int(summary_res[6].split(':')[1])  # "FinalVolume: %d\n", $final_volume_ ;

    def __init__(self, sim_res, summary_res):
        sim = sim_res.strip().split()
        summ = summary_res.strip().split()[1:]
        self.trades = [int(sim[3]), int(sim[4]), int(sim[5])]
        self.avg_absolute_pos_ = float(summ[0])
        self.TTC_ = [int(summ[1]),  int(summ[2])]
        self.pnl_ = [int(sim[1]), int(summ[3]), int(summ[4]),
                     # finalPNL, $median_closed_trade_pnls_, $average_closed_trade_pnls_,
                     int(summ[5]), float(summ[6]), float(summ[7])]
        # $stdev_closed_trade_pnls_, $sharpe_closed_trade_pnls_, $fracpos_closed_trade_pnls_,
        self.pnlMINmaxDD = [int(x) for x in summ[8:]]
        self.Vol = int(sim[2])

# avg_absolute_pos ttc<median average> pnl_<pnl median avg stdev shrape fracpos> pnl<Min Max DD> Vol trades<best nonbest aggressive>

    def str(self):
        return "%d %s %s %.2f [%s] [%s]" % (self.Vol, ' '.join(["%.2f" % k for k in self.pnl_]), ' '.join(['%d' % k for k in self.pnlMINmaxDD]), self.avg_absolute_pos_, ' '.join(["%d" % k for k in self.TTC_]),  ' '.join(['%d' % k for k in self.trades]))


class MultSimResults:
    resDatabase = []

    def __init__(self, simlines, results):

        if len(simlines) != len(results):
            sys.stderr.write("Mismatch in sim result and pnl_stat_results.\n%s\n%s\n" %
                             (''.join(simlines), ''.join(results)))
        self.resDatabase = [simResult(sim, sumres) for (sim, sumres) in zip(simlines, results)]
        # print '\t', len(self.resDatabase)

    def __getitem__(self, i):
        if i < len(self.resDatabase):
            return self.resDatabase[i]
        else:
            print("%d is not in resDatabase" % i, len(self.resDatabase))

    def __len__(self):
        return len(self.resDatabase)

    def str(self):
        print("# Vol [finalPNL median avg stdev sharpe fracpos min max DD] avg_absolute_pos [medianTTC avgTTC] [bestlvlTrades nonBestLvlTrades aggrTrades]")
        return '\n'.join([i.str() for i in self.resDatabase])

    def __str__(self):
        return str(self)


from math import sqrt


def nearInt(flt):
    return int(flt + 0.5)


def meanStdev(arr):
    #    print arr
    if len(arr) <= 0:
        return [0, 1]
    m = sum(arr) / len(arr)
    sqm = sum([x * x for x in arr]) / len(arr)
    return [m, nearInt(sqrt(sqm - m * m))]


def pnlMean(arr):
    if not arr:
        return []
    arr1 = [x for x in arr if x > 0]
    arr2 = [x for x in arr if x <= 0]
    res = meanStdev(arr1) + [len(arr1)]
    res += meanStdev(arr2) + [len(arr2)]
    return res


import random
random.seed()


def stepAnalyse(arr):
    a = [[] for i in range(3)]
    for i in arr:
        a[random.randint(0, 2)] += [i]
    s = [meanStdev(r)[0] for r in a]
    print(a)
    if s[0] * s[1] > 0:
        if s[0] * s[2] > 0:
            return s[0] > 0
        else:
            print("test failed")
            return True
    else:
        print("not consistent")
        return True


def sampledMeanStdev(arr):
    a = [x for x in arr if random.randint(0, 1) == 0]
    if len(a) < len(arr) / 3:
        print('Error ', len(a), len(arr))
        return sampledMeanStdev(arr)
    return meanStdev(a)
