#!/usr/bin/python


# 21st June, 2012
# Used for analyzing Indicators.
# It runs make_indicator_list on varying product with different time periods.
# then for each ilist file group the indiacators based on indicator classes.
#
#
# ./IndAnalysis.py y  // will run the make_indicator_list and store the result in tmp_<prod_code>_<time_period>
# ./IndAnalysis.py n  // will do the rest of classification and tabulation.
######################


import sys
import os
import operator
from subprocess import Popen, PIPE


products = ['FGBL']  # , 'FGBM', 'FGBS', 'ZN', 'ZB', 'ZF', 'BR_IND', 'BR_DOL', 'BR_WIN']
times = ['2', '32', '96']
#IndClass = ['trend', 'trade', 'relative', 'book', 'orderflow'];
IndClass = ['trend', 'trade', 'book', 'orderflow', 'negative', 'online_nneg', 'offline']  # indicator calsses


def parse_line(line):
    l = line.split()
    r = int(l[0].split(":")[0])
    corr = float(l[1])
    ind = l[2]
    p1 = l[3]
    p2 = ''
    try:
        t = float(l[4])
    except:
        p2 = l[4]
    return r, corr, p1, p2


from getpass import getuser
home_dir_path = '/home/' + getuser()
tmp_folder_loc = home_dir_path + '/basetrade/testbed'


def __main__():
    for prod in products:
        for time in times:
            if(sys.argv[1] == 'y'):
                make_indicator_list_exec = "~/basetrade_install/bin/make_indicator_list %s /home/dvctrader/indicatorwork/%s_0_%s_na_e3_US_MORN_DAY_MktSizeWPrice_MktSizeWPrice/indicator_corr_record_file.txt MktSizeWPrice MktSizeWPrice TODAY-250 TODAY-100 0.85 2>/dev/null 1 a | head -n250 | ~/basetrade/scripts/filterIndicator.py %s > %s/tmp_%s_%s" % (
                    prod, prod, time, prod, tmp_folder_loc, prod, time)
                #                make_indicator_list_exec = "~/basetrade_install/bin/make_indicator_list %s ~/indicatorwork/%s_0_%s_na_e3_US_MORN_DAY_MktSizeWPrice_MktSizeWPrice/indicator_corr_record_file.txt MktSizeWPrice MktSizeWPrice TODAY-250 TODAY-100 0.85 2>/dev/null 1 a | head -n250 > %s/tmp_%s_%s" %(prod, prod, time, tmp_folder_loc, prod, time)
                print(make_indicator_list_exec)
                os.system(make_indicator_list_exec)
                continue
            print("=*=" * 10)
            for t in IndClass:
                cmd = "grep -wnf %s/basetrade/files/IndicatorInfo/ind_class/%s %s/tmp_%s_%s" % (
                    home_dir_path, t, tmp_folder_loc, prod, time)

                x = Popen(cmd.split(), stdout=PIPE).stdout.readlines()
                Products = {}
                # print len(x);
                # continue;
                avg_corr = 0.0
                for l in x:
                    ln = parse_line(l)
                    avg_corr += ln[1]
                    try:
                        Products[ln[2]][0] += 1
                        Products[ln[2]][1] += ln[1]
                    except:
                        Products[ln[2]] = [1, ln[1]]
                    try:
                        Products[ln[3]][0] += 1
                        Products[ln[3]][1] += ln[1]
                    except:
                        Products[ln[3]] = [1, ln[1]]

                if not x:
                    continue
                avg_corr /= float(len(x))
                if '' in Products:
                    Products.pop('')
                for p in Products:
                    Products[p][1] /= Products[p][0]
                Products_sorted = sorted(list(Products.items()), key=lambda k_v: k_v[1][1], reverse=True);
                print("\n", prod, time, t.upper(), '\n', '-' * 8, end=' ')
                # print t, len (x), avg_rank
                print("Average Corr: ", avg_corr, len(x), "\n\t%s" % '\n\t'.join(
                    ["%s\t\t:\t%d\t%f" % (p[0], p[1][0], p[1][1]) for p in Products_sorted]))


def get_ind_classes():
    ind_data = open(home_dir_path + "/basetrade/files/IndicatorInfo/indicator_categories.txt").readlines()
    IndClass = {}
    for line in ind_data:
        line = line.strip().split(" ")
        if line[0] in IndClass:
            IndClass[line[0]] += [line[1]]
        else:
            IndClass[line[0]] = [line[1]]

        for t in IndClass:
            f = open(home_dir_path + '/basetrade/files/IndicatorInfo/ind_class/%s' % t, 'w')
            f.write('\n'.join(IndClass[t]))
        f.close()


# get_ind_classes();
__main__()
