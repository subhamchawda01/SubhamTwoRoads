#!/usr/bin/env python
import os
import sys
from datetime import datetime, timedelta, date
import subprocess
import math

fname = '/home/dvcinfra/pnls.txt'

if len(sys.argv) < 2:
    print('Usage: ./get_pc1_exposure.py CONTRACT_NAME[FVS/VX/GE/LFI/DI]')
    sys.exit(0)

contract = sys.argv[1]

ls = contract.split('_')
if ls[0] == 'SP':
    try:
        index0 = int(ls[1][-2:])
    except ValueError:
        index0 = int(ls[1][-1])
    try:
        index1 = int(ls[2][-2:])
    except ValueError:
        index1 = int(ls[2][-1])
    base_contract = ls[1][0:len(ls[1]) - len(str(index0))] + "_0";
    contract0 = ls[1][:len(ls[1]) - len(str(index1))] + "_" + str(index0);
    contract1 = ls[1][:len(ls[1]) - len(str(index1))] + "_" + str(index1);
else:
    if 'DI1' in contract:
        base_contract = 'DI1F15'
    else:
        base_contract = contract.split('_')[0] + '_0'


today = datetime.today()
tomorrow = today + timedelta(days=1)

while tomorrow.weekday() >= 5:
    tomorrow = tomorrow + timedelta(days=1)

str_today = str(today).split(' ')[0].replace('-', '')
str_tomorrow = str(tomorrow).split(' ')[0].replace('-', '')

spread_def = 1


if 'VX' in contract:
    spread_def = -1

stdev_con = 0
stdev_basecon = 0
lookback_stdev_num = 5
i = 0
cur_date = today
while i < lookback_stdev_num:
    cur_date = cur_date - timedelta(days=1)
    if cur_date.weekday() >= 5:
        continue
    str_cur_date = str(cur_date).split(' ')[0].replace('-', '')
    try:
        f = open('/spare/local/L1Norms/' + str_cur_date + '/' + base_contract + '_l1norm_value')
        k = f.readline()
        if not k.strip():
            continue
        this_std = float(k.split()[1])
        if this_std != -1:
            stdev_basecon += this_std / lookback_stdev_num
            i += 1
        f.close()
    except IOError:
        print('file_not_found')
        continue
i = 0
cur_date = today

while i < lookback_stdev_num:
    cur_date = cur_date - timedelta(days=1)
    if cur_date.weekday() >= 5:
        continue
    str_cur_date = str(cur_date).split(' ')[0].replace('-', '')
    try:
        f = open('/spare/local/L1Norms/' + str_cur_date + '/' + contract + '_l1norm_value')
        k = f.readline()
        if not k.strip():
            continue
        this_std = float(k.split()[1])
        if this_std != -1:
            stdev_con += this_std / lookback_stdev_num
            i += 1
        f.close()
    except IOError:
        print('file_not_found')
        continue
i = 0
cur_date = today
stdev0 = 0
stdev1 = 0
if 'SP' in contract:
    while i < lookback_stdev_num:
        cur_date = cur_date - timedelta(days=1)
        if cur_date.weekday() >= 5:
            continue
        str_cur_date = str(cur_date).split(' ')[0].replace('-', '')
        try:
            f = open('/spare/local/L1Norms/' + str_cur_date + '/' + contract0 + '_l1norm_value')
            k = f.readline()
            if not k.strip():
                continue
            this_std = float(k.split()[1])
            if this_std != -1:
                stdev0 += this_std / lookback_stdev_num
                i += 1
            f.close()
        except IOError:
            print('file_not_found')
            continue
i = 0
cur_date = today

if 'SP' in contract:
    while i < lookback_stdev_num:
        cur_date = cur_date - timedelta(days=1)
        if cur_date.weekday() >= 5:
            continue
        str_cur_date = str(cur_date).split(' ')[0].replace('-', '')
        try:
            f = open('/spare/local/L1Norms/' + str_cur_date + '/' + contract1 + '_l1norm_value')
            k = f.readline()
            if not k.strip():
                continue
            this_std = float(k.split()[1])
            if this_std != -1:
                stdev1 += this_std / lookback_stdev_num
                i += 1
            f.close()
        except IOError:
            print('file_not_found')
            continue

stdev0 = stdev0 / lookback_stdev_num
stdev1 = stdev1 / lookback_stdev_num
if ls[0] != 'SP':
    print(str(stdev_con / stdev_basecon))
else:
    if stdev0 > stdev1:
        print(str(spread_def * stdev_con / stdev_basecon))
    else:
        print(str(-1 * spread_def * stdev_con / stdev_basecon))
