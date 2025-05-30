#!/usr/bin/python

#---------------21 June, 2012
# Classify portfolios into bonds(2) or equities(1). The polarity is determimed by weighted sum of polarities of its constituents.
# No input param, just run ./portfolio_class.py
#----------------------------------

# RULES:
#     1  2  3
#  1  1 -1  1
#  2 -1  1 -1
#  3  1 -1  1
#
import re


def getBaseProd(prod):
    return re.sub(r'_[0-9]+', '', prod)


def getProdType(p_w, p_t):
    s = 0
    for i, w in enumerate(p_w):
        s += p_t[i] * float(w)
    if s == 0:
        return -1
    if s > 0:
        return 2
    else:
        return 1


prod_cls_file = "/home/dvctrader/basetrade/files/IndicatorInfo/product_classes"
prod_data = {}
for line in open(prod_cls_file).readlines():
    line = line.strip()
    if not line:
        continue
    if line[0] == '#':
        # comment
        #if 'CATEGORIES' in line: cat = getCategories();
        continue
    p = line.split()[0]
    c = int(line.strip().split()[1])
    prod_data[p] = c

Rules = [[1, -1, 1],
         [-1, 1, -1],
         [1, -1, 1]]


def isNegative(ind):
    return ind.find('Negative') > 0


def isOffline(ind):
    return ind.find('Offline') > 0


def isBond(p):
    if p not in prod_data:
        print('ERROR:', p)
        return 0
    if prod_data[p] == 2:
        return 1
    else:
        return -1


portfolio_dir = "/spare/local/tradeinfo/PCAInfo/"
Portfolio_DB = {}
for line in open(portfolio_dir + 'portfolio_inputs'):
    line = line.strip().split()
    p = getBaseProd(line[1])
    Portfolio_DB[p] = [getBaseProd(t) for t in line[2:]]


Port_type = {}
for line in open(portfolio_dir + 'pca_portfolio_stdev_eigen_DEFAULT.txt'):
    line = line.strip().split()
    p = line[1]
    if line[2] == '1':
        Port_type[p] = getProdType(line[4:], [isBond(t) for t in Portfolio_DB[p]]);


print('\n'.join(["%s %d" % (p, Port_type[p]) for p in Port_type]))
