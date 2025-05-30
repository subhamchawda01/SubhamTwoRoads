#!/usr/bin/env python

# author: diwakar

import sys
import math


def __main__():

    if len(sys.argv) < 3:
        print("Usage: modelpath1 modelpath2 [fractional_change_significant]")
        sys.exit()

    file1 = open(sys.argv[1]).readlines()
    file2 = open(sys.argv[2]).readlines()
    frac_change_ = 0.02
    if len(sys.argv) >= 4:
        frac_change_ = float(sys.argv[3])

    if len(file1) != len(file2):
        print("Unequal lengths of files")
        sys.exit()

    modeltype = 0  # linear
    for i in range(1, min(len(file1) - 1, len(file2) - 1)):
        file1[i] = file1[i].strip().split()
        file2[i] = file2[i].strip().split()
        if (((len(file1[i]) >= 2) and (len(file2[i]) >= 2)) and
            ((file1[i][0] == "MODELMATH") and (file2[i][0] == "MODELMATH")) and
                ((file1[i][1] == "SIGLR") and (file2[i][1] == "SIGLR"))):
            modeltype = 1  # SIGLR

        if (((len(file1[i]) >= 2) and (len(file2[i]) >= 2)) and
                ((file1[i][0] == "INDICATOR") and (file2[i][0] == "INDICATOR"))):
            if modeltype == 0:
                orig_weight = float(file1[i][1])
                opt_weight = float(file2[i][1])
            else:
                weightstrvec = file1[i][1].split(':')
                if len(weightstrvec) >= 2:
                    orig_weight = float(weightstrvec[1])
                weightstrvec = file2[i][1].split(':')
                if len(weightstrvec) >= 2:
                    opt_weight = float(weightstrvec[1])
            change_wt_ = math.fabs(opt_weight - orig_weight)
            if ((math.fabs(orig_weight) + math.fabs(opt_weight)) <= 0) or (change_wt_ / (math.fabs(orig_weight) + math.fabs(opt_weight)) < frac_change_):
                continue
            file1[i].insert(2, file2[i][1])
            file1[i].insert(2, '->')
            file1str = ' '.join(file1[i])
            print(file1str)


__main__()
