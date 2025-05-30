#!/usr/bin/env python

import time
import subprocess
from walkforward.definitions import execs

"""

module used mostly by refresh_model for now

we want to call this c++ exec thats it !

Usage: datagen INDICATORLISTFILENAME TRADINGDATE UTC_STARTHHMM UTC_ENDHHMM PROGID OUTPUTFILENAME/"STATS"/"STATS_SAMPLES" MSECS_PRINT l1EVENTS_PRINT/SAMPLE_USING_CORE_SHC(c1/c2/c3) NUM_TRADES_PRINT ECO_MODE [ USE_FAKE_FASTER_DATA 0/1 = 1 ] [ ADD_SAMPLING_CODES ZN_0[0.5] 6M_0 [0.5] 6E_0 [0.5]... -1 ] [ REGIME 1 -1 ] [ TRADED_EZONE ] [ ADD_DBG_CODE DBG_CODE1 DBG_CODE2 ... ]

"""


def run_datagen(ilist, date, stime, etime, output_file, ptime, events, trades):

    uniqid = str(int(time.time()))
    command = [execs.execs().datagen, ilist, date, stime, etime, uniqid, output_file, ptime, events, trades, '0']
    process = subprocess.Popen(' '.join(command), shell=True,
                               stderr=subprocess.PIPE,
                               stdout=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = process.returncode
    return out, err, errcode

    #~/basetrade_install/bin/datagen ~/ilist 20170323 EST_800 UTC_1900 2897 ~/kp_dout 1000 0 0 0
