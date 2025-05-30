#!/usr/bin/env python

import sys
import numpy as np

if len(sys.argv) < 3:
    print "Usage:<script><ilist file><tree file>"
    exit(0)

tree_file_ = sys.argv[2]
ilist_file_ = sys.argv[1]

with open(ilist_file_) as file:
    indicators_ = file.readlines()
file.close()

indicator_depths_ = [[] for i in range(len(indicators_))]

depths = {}
line_number = 0
with open(tree_file_) as file:
    for line in file:
        line = line.strip().split()
        if line[0] == "TREELINE" and line[1] != '-1' and line[2] != '-1':  # ignore lfs
            _indicator_ = int(line[3])
            _lc_ = int(line[1])
            _rc_ = int(line[2])
            if len(depths) == 0:
                depths[_lc_] = 1
                depths[_rc_] = 1
                indicator_depths_[_indicator_].append(0)
            else:
                current_depth_ = depths[line_number]
                depths[_lc_] = current_depth_ + 1
                depths[_rc_] = current_depth_ + 1
                indicator_depths_[_indicator_].append(current_depth_)
        if line[0] == "TREELINE":
            line_number = line_number + 1

print "Printing the average depths of all indicators"
means = [np.mean(indicator_depths_[i]) for i in range(len(indicator_depths_))]
sort_idx_ = np.argsort(means)
for i in range(len(indicators_)):
    print ' '.join(indicators_[sort_idx_[i]].strip().split()[2:]), means[sort_idx_[i]]
