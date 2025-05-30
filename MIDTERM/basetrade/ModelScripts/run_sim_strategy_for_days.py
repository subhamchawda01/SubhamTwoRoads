from datetime import datetime, timedelta
import commands

import sys
import numpy as np

if (len(sys.argv) < 3):
    print 'USAGE: ' + sys.argv[0] + ' stratfile num_working_days'
    exit(0)

time_delta = timedelta(days=1)
cur_date = datetime.today() - time_delta
stratfile = sys.argv[1]
num_working_days = int(sys.argv[2])

outputs = []

while num_working_days > 0:
    if cur_date.weekday() >= 5:
        cur_date -= time_delta
        continue
    str_date = str(cur_date).split(' ')[0].replace('-', '')
    output = commands.getoutput('~/basetrade_install/bin/sim_strategy SIM ' + stratfile + ' 3999123 ' + str_date)
    if (not output.strip()[9:] == '0 0 0 0 0 0'):
        print str_date + ': ' + output
        outputs.append(output)
        num_working_days -= 1
    cur_date -= time_delta

pnls = []
sum_pnl = 0

for output in outputs:
    pnl = int(output.split()[1])
    pnls.append(pnl)
    sum_pnl += pnl

print 'PNLAverage:\t', sum_pnl / len(pnls)
print 'PNLStdev:\t', np.std(pnls)
print 'PNLSharpe:\t', sum_pnl * 1.0 / (len(pnls) * np.std(pnls))
