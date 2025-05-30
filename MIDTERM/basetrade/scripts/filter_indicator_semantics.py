#!/usr/bin/python


# RULES:
#     1  2  3
#  1  1 -1  1
#  2 -1  1 -1
#  3  1 -1  1
#
prod_cls_file = "../../../basetrade/files/IndicatorInfo/product_classes"
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


# print Rules[prod_data['FDAX']][prod_data['ZT']]
import sys
import os
prod = sys.argv[1]
for line in sys.stdin.readlines():
    line = line.strip().split()
    if line[0] != 'INDICATOR':
        continue
    corr = float(line[1])
    ind = line[2]
    p1 = line[3]
    p2 = ""
    if p1 != prod:
        p2 = p1
    else:
        try:
            float(line[4])
        except:
            p2 = line[4]

    if isOffline(ind) and corr < 0:
        continue
    if isNegative(ind):
        corr = -corr
    if p2 and corr * Rules[prod_data[prod] - 1][prod_data[p2] - 1] < 0:
        continue
    else:
        print(' '.join(line), '\n')
