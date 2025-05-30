#!/usr/bin/env python
import os
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import time
from matplotlib.font_manager import FontProperties

min_order_size = '1'
if len(sys.argv) < 3:
    print('Usage: ./threshold_visualization.py PARAM_FILE SHORTCODE [TRADINGDATE = Today]')
    sys.exit(0)

param_file = os.path.abspath(sys.argv[1])
shortcode = sys.argv[2]

if len(sys.argv) == 4:
    date = sys.argv[3]
else:
    date = time.strftime("%Y%m%d")

os.system('cat ~/basetrade/CDefCode/security_definitions.cpp >tmp_sec_def')
f = open('tmp_sec_def', 'r')
for l in f:
    if 'contract_specification_map_' in l:
        if ('"' + shortcode + '"') in l:
            min_order_size = l.split('//')[0].strip().split(',')[-1].split(')')[0].strip()
f.close()


plot_vars = ['BP', 'BK', 'AP', 'AK', 'BI', 'BIK', 'AI', 'AIK', 'AA', 'bidsz', 'asksz']

if len(sys.argv) > 5:
    plot_vars = argv[5:]

os.system('~/basetrade_install/bin/get_min_price_increment ' + shortcode + ' ' + date + ' >/tmp/thresh_min_price')
f = open('/tmp/thresh_min_price')
price = f.readline().strip()
f.close()
os.system('~/basetrade_install/bin/print_paramset ' + param_file + ' ' +
          date + ' ' + min_order_size + ' ' + shortcode + ' >/tmp/tmp_params')


f = open('/tmp/tmp_params')


mp_thresh = {}

count = 0
flag = 0
positions = []
for l in f:
    tks = l.split()
    pos = int(tks[1])
    positions.append(pos)
    for vars in plot_vars:
        i = tks.index(vars + ':')
        if count == 0:
            mp_thresh[vars] = [float(tks[i + 1])]
            flag = 1
        else:
            mp_thresh[vars].append(float(tks[i + 1]))
    if flag == 1:
        count = 1


rev_index = 0
for i in range(0, len(positions)):
    if positions[i] < 0:
        rev_index = i
        break


positions = positions[rev_index:][::-1] + positions[:rev_index]
for vars in plot_vars:
    mp_thresh[vars] = mp_thresh[vars][rev_index:][::-1] + mp_thresh[vars][:rev_index]


fontP = FontProperties()
fontP.set_size('small')

pos_bp = []
pos_bk = []
pos_ap = []
pos_ak = []

for i in range(0, len(positions)):
    if mp_thresh['BP'][i] < 90:
        pos_bp.append(i)

for i in range(0, len(positions)):
    if mp_thresh['BK'][i] < 90:
        pos_bk.append(i)

for i in range(0, len(positions)):
    if mp_thresh['AP'][i] < 90:
        pos_ap.append(i)

for i in range(0, len(positions)):
    if mp_thresh['AK'][i] < 90:
        pos_ak.append(i)


l1, = plt.plot([positions[x] for x in pos_bp], [mp_thresh['BP'][x] for x in pos_bp], '.-', color='r')
l2, = plt.plot([positions[x] for x in pos_bk], [mp_thresh['BK'][x] for x in pos_bk], '.-', color='g')
l3, = plt.plot([positions[x] for x in pos_ap], [mp_thresh['AP'][x] for x in pos_ap], '.-', color='b')
l4, = plt.plot([positions[x] for x in pos_ak], [mp_thresh['AK'][x] for x in pos_ak], '.-', color='m')

plt.legend([l1, l2, l3, l4], ['BP', 'BK', 'AP', 'AK'], prop=fontP)
plt.xlabel('Position')
plt.grid()
plt.savefig('plot1.png')
plt.clf()


pos_ai = []
pos_aik = []
pos_bi = []
pos_bik = []
pos_aa = []

for i in range(0, len(positions)):
    if mp_thresh['AI'][i] < 90:
        pos_ai.append(i)

for i in range(0, len(positions)):
    if mp_thresh['AIK'][i] < 90:
        pos_aik.append(i)

for i in range(0, len(positions)):
    if mp_thresh['BI'][i] < 90:
        pos_bi.append(i)

for i in range(0, len(positions)):
    if mp_thresh['BIK'][i] < 90:
        pos_bik.append(i)

for i in range(0, len(positions)):
    if mp_thresh['AA'][i] < 90:
        pos_aa.append(i)


l1, = plt.plot([positions[x] for x in pos_ai], [mp_thresh['AI'][x] for x in pos_ai], '.-', color='r')
l2, = plt.plot([positions[x] for x in pos_aik], [mp_thresh['AIK'][x] for x in pos_aik], '.-', color='g')
l3, = plt.plot([positions[x] for x in pos_bi], [mp_thresh['BI'][x] for x in pos_bi], '.-', color='b')
l4, = plt.plot([positions[x] for x in pos_bik], [mp_thresh['BIK'][x] for x in pos_bik], '.-', color='m')
l5, = plt.plot([positions[x] for x in pos_aa], [mp_thresh['AA'][x] for x in pos_aa], '.-', color='k')

plt.legend([l1, l2, l3, l4, l5], ['AI', 'AIK', 'BI', 'BIK', 'AA'], prop=fontP)
plt.xlabel('Position')
plt.grid()
plt.savefig('plot2.png')
plt.clf()

l1, = plt.plot(positions, mp_thresh['bidsz'], '.-', color='r')
l2, = plt.plot(positions, mp_thresh['asksz'], '.-', color='g')

plt.legend([l1, l2], ['bidsz', 'asksz'], prop=fontP)
plt.xlabel('Position')
plt.grid()
plt.savefig('plot3.png')
plt.clf()

print('Plots saved as:')
print(os.path.abspath('plot1.png'))
print(os.path.abspath('plot2.png'))
print(os.path.abspath('plot2.png'))
print('Use: "display $image_path" to view')
