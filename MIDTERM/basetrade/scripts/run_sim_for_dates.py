#!/usr/bin/python

from datetime import datetime, timedelta
from .pythonUtils import *
import itertools

USAGE = "%s strat last_num_days [start_day]"
if len(sys.argv) < 2:
    print(USAGE)
    exit()


def date_generator(st):
    from_date = st
    while True:
        from_date = from_date - timedelta(days=1)
        if from_date.weekday() in range(1, 5):
            yield from_date.strftime("%Y%m%d")


st = getFullPathInModel(sys.argv[1])


def RunSim(dt):
    global st
    uid = random.randint(10000, 99999)
    res = RunCommand("~/basetrade_install/bin/sim_strategy SIM %s %d %s ADD_DBG_CODE -1" % (st, uid, dt))
    RunCommand(" rm -rf /spare/local/logs/tradelogs/*.%s.%d" % (dt, uid))
    if "UORI" not in res[1]:
        print(res[1])
    return '~'.join(res[0].strip().split('\n'))


if len(sys.argv) < 4:
    dates = [dt for dt in itertools.islice(date_generator(datetime.today()), int(sys.argv[2]))]
else:
    dates = [dt for dt in itertools.islice(date_generator(datetime.strptime(sys.argv[3], "%Y%m%d")), int(sys.argv[2]))]

pnl = []
vol = []


def avg(vec):
    return float(sum(vec)) / len(vec)


from multiprocessing import Pool
pool = Pool(10)
simresults = pool.map(RunSim, dates)

for dt, s in zip(dates, simresults):
    for i, res in enumerate(s.split('~')):
        res = res.split()
        p, v = int(res[1]), int(res[2])
        if len(pnl) <= i:
            pnl.append(p)
        else:
            pnl[i] += p
        if len(vol) <= i:
            vol.append(v)
        else:
            vol[i] += v
    print(dt, s)

n = len(simresults)
print('STAT  :  ', ' ~~ '.join(["Average: %f %f " % (float(p) / n, float(v) / n) for (p, v) in zip(pnl, vol)]))
