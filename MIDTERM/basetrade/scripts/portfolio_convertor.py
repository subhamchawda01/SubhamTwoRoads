#!/usr/bin/python

import sys
import os

date_ = sys.argv[1]
if not date_:
    date_ = "DEFAULT"
port_consts_file = "/spare/local/tradeinfo/PCAInfo/portfolio_inputs"
port_const_weight_file = "/spare/local/tradeinfo/PCAInfo/pca_portfolio_stdev_eigen_" + date_ + ".txt"

port = dict()
for p in open(port_consts_file).readlines():
    p = p.strip().split()
    port[p[1]] = [p[2:]];
for p in open(port_const_weight_file).readlines():
    p = p.strip().split()
    if len(p) < 2:
        continue
    if p[1] not in port:
        continue
    if p[0] == "PORTFOLIO_EIGEN" and p[2] == '1':
        port[p[1]] += [p[4:]]
#        print port[p[1]]
# for p in  port:
#     if len(port[p])>1: print p, '|', port[p][0], '|', port[p][1]
print('\n'.join(["%s %s" % (p, ' '.join(["%s %s" % (port[p][0][i], port[p][1][i])
                                         for i in range(len(port[p][0]))])) for p in port if len(port[p]) > 1]))
