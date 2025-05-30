#!/usr/bin/python


#----------------------------21 June, 2012
# This file is used for semantic filtering.
# 1 - equity, 2 - bnod, 3 - forex, defined in basetrade/files/Indicator/info/product_classes file
# Usage: pipe the output of make_indicator_list to it. Takes input from STDIN.
# prod=FGBL;~/basetrade_install/bin/make_indicator_list $prod ~/indicatorwork/$prod_0_2_na_e3_US_MORN_DAY_MktSizeWPrice_MktSizeWPrice/indicator_corr_record_file.txt MktSizeWPrice MktSizeWPrice TODAY-250 TODAY-100 0.85 2>/dev/null 1 a | head -n250 | ./filter.py $prod
# or ./filter.py FGBL < <Indicator list File>
#----------------------------


# RULES:
#     1  2  3
#  1  1 -1  1
#  2 -1  1 -1
#  3  1 -1  1
#


import sys
import os
import re
from subprocess import Popen, PIPE
from math import sqrt, fabs


usage = "%s <prod_code(without _0)> <end time(same as in make_indicator_list)> <EU/US> <corr_cutoff(from lrdb)> -llf ([optional]use lead-lag-filter)\n" % sys.argv[0]
if len(sys.argv) < 5:
    print('Sorry cannot run your code!!! \nUSAGE:', usage)
    exit(0)


Rules = [[1, -1, 1],
         [-1, 1, -1],
         [1, -1, 1]]


def RuleSays(p1, p2, corr):
    try:
        return corr * Rules[prod_data[p1] - 1][prod_data[p2] - 1] > 0
    except:
        return True


# LeadLag filter
BadLeader = {}

for l in open('/home/dvctrader/basetrade/files/IndicatorInfo/lead_lag_ind.txt').readlines():
    if l[0] == '#' or not l:
        continue
    l = l.strip().split()
    BadLeader[l[0]] = l[1:]


from getpass import getuser
home_dir_path = '/home/' + getuser()

prod_cls_file = home_dir_path + "/basetrade/files/IndicatorInfo/product_classes"
prod_data = {}
for line in open(prod_cls_file).readlines():
    line = line.strip()
    if not line:
        continue
    if line[0] == '#':
        # comments -  skip yaar!!!
        #if 'CATEGORIES' in line: cat = getCategories();
        continue
    p = line.split()[0]
    c = int(line.strip().split()[1])
    prod_data[p] = c


def isNegative(ind):
    return ind.find('Negative') >= 0


def isOffline(ind):
    return ind.find('Offline') >= 0


def isBadLeader(p, leader):
    if p not in BadLeader:
        return False
    if leader in BadLeader[p]:
        return True
    else:
        return False

# print Rules[prod_data['FDAX']][prod_data['ZT']]


def getBaseProd(prod):
    return re.sub(r'_[0-9]+', '', prod)


def getDate(d):
    import datetime
    try:
        return int(d)
    except:
        if d.find('TODAY') >= 0:
            td = datetime.date.today() - datetime.timedelta(days=int(d[6:]))
            return int(td.strftime("%Y%m%d"))


prod = getBaseProd(sys.argv[1])
end_date = getDate(sys.argv[2])

if sys.argv[3] == 'EU':
    time_zone = 14
elif sys.argv[3] == 'US':
    time_zone = 23
else:
    print("Wrong time Zone, EU or US.")
    exit(0)
corr_cutoff = float(sys.argv[4])
useLeadLagFilter = False
if '-llf' in sys.argv:
    useLeadLagFilter = True

lrdb_dir_path = "/spare/local/tradeinfo/LRDBBaseDir/"
#most_recent_file = int(Popen("ls | awk '{FS=\"_\"; if ( $1 != "" && $1 < 20120622 ) print $1}' | uniq | tail -n1", stdout=PIPE).stdout.read());
files = os.listdir(lrdb_dir_path)
most_recent = 0
for f in files:
    try:
        dt = int(f.split('_')[0])
    except:
        dt = 0
    if dt <= end_date and dt > most_recent:
        most_recent = dt
# print most_recent

most_recent_lrdb_file = lrdb_dir_path + "%d_120_PreGMT%d.txt" % (most_recent, time_zone)
lrdb_data = dict()
for l in open(most_recent_lrdb_file).readlines():
    l = l.strip().split()
    p1 = l[0].split('^')[0]
    if getBaseProd(p1) != prod:
        continue
    p2 = l[0].split('^')[1]
    l[2] = fabs(float(l[2]))
    lrdb_data[(p1, p2)] = l[2]
# print lrdb_data


def getMeanStddev():
    s = sum([fabs(lrdb_data[t]) for t in lrdb_data])
    sqsum = sum([lrdb_data[t] * lrdb_data[t] for t in lrdb_data])
    n = len(lrdb_data)
    mean = s / n
    std = sqrt(sqsum / n - mean * mean)
    # print n, mean, std
    return mean + std * 0.2


m = getMeanStddev()
# print "Automatic Corr Cutoff: %f" % m
# corr_cutoff=m
sys.stderr.write('lrdb correlation cutoff: %f\n' % corr_cutoff)


def lrdbPrunedItems():
    for l in lrdb_data:
        if l[0] == prod + '_0':
            if lrdb_data[l] < m:
                print(l[1], lrdb_data[l])


def lrdb_Filter(p1, p2, cut_off):
    try:
        return cut_off < lrdb_data[p1 + '_0', p2 + '_0']
    except:
        return True


for line in sys.stdin.readlines():
    line = line.strip().split()
    if line[0] != 'INDICATOR':
        print(' '.join(line))
        continue
    try:
        t = line.index('#')
        corr = float(line[t + 1])
    except:
        print(' '.join(line))
        continue  # Not applying filtering on lines if correlation value is absent.
#    if line[-2]=='#': corr = float(line[-1])
#    else corr = float(line[-2])
    ind = line[2]
    p1 = getBaseProd(line[3])
    p2 = ""
    if p1 != prod:
        p2 = p1
    else:
        try:
            float(line[4])
        except:
            p2 = getBaseProd(line[4])
            if p2 in ["Midprice", "MktSizeWPrice", "MktSinusoidal", "OrderWPrice", "OfflineMixMMS", "Max"]:
                p2 = ''

     # Filtering is done here.
    if isOffline(ind) and corr < 0:
        sys.stderr.write('rejected [ offline and still corr<0 ]       : ' + ' '.join(line) + '\n'); continue;
    if isNegative(ind):
        corr = -corr
    if p2 and RuleSays(prod, p2, corr) < 0:
        sys.stderr.write('rejected [ bond-equity corr sign mismatch ] : ' + ' '.join(line) + '\n'); continue
    if useLeadLagFilter and ind.lower().find('trend') >= 0 and isBadLeader(prod, p2):
        sys.stderr.write('rejected [ Bad leading product ]            : ' + ' '.join(line) + '\n'); continue
        # print ">"*10, ' '.join(line);
    elif lrdb_Filter(prod, p2, corr_cutoff):
        print(' '.join(line))
    else:
        sys.stderr.write('rejected [ corr < lrdb_corr ]               : ' + ' '.join(line) + ' --- %f\n' % lrdb_data[prod + '_0', p2 + '_0']); continue
