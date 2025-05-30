#!/usr/bin/python

import os
import sys
import re
from subprocess import Popen, PIPE
from datetime import datetime

if len(sys.argv) < 2:
    print("%s <reg_data> <model file name> date" % sys.argv[0])
    exit(0)
rg_file = sys.argv[1]
mdl_file = sys.argv[2]
date = sys.argv[3]
Ilist = []


def parseIlistFile(mdl_file):
    global Ilist
    il = open(mdl_file).readlines()
    Ilist = [0] * (len(il) - 4)
    i = 0
    dep_corr = Popen(["/home/rahul/basetrade_install/bin/get_dep_corr", rg_file],
                     stdout=PIPE).stdout.readlines()[0].split()
    for ind in il:
        ind = ind.strip().split()
        if ind[0] != 'INDICATOR':
            continue
        indname = ' '.join(ind[2:])
        print("%s INDICATOR %s %s" % (date, dep_corr[i], indname))
        i += 1


parseIlistFile(mdl_file)
