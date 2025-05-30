#!/usr/bin/env python

# \file ModelScripts/generate_indicator_stats_2+pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551

''' This script generates the correlation for all the  products added to the DB
and stores it in DB IndStats
'''
import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))
from pylib.indstats_db_access_manager import *
from walkforward.utils.date_utils import calc_prev_week_day
import walkforward.definitions.execs as execs

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print("USAGE <script> shc end_date num_days")
        sys.exit()

    # IndStatsObj = IndStatsDBAcessManager()
    # IndStatsObj.open_conn()
    #
    # shortcodes = IndStatsObj.get_shortcodes_in_prod()
    shc = sys.argv[1]
    end_date = sys.argv[2]
    # end_date = datetime.datetime.strptime(end_date,'%Y%m%d')
    num_days = sys.argv[3]
    start_date = calc_prev_week_day(end_date, int(num_days))
    # for shc in shortcodes:
    #cmd = "~/basetrade/ModelScripts/generate_multiple_indicator_stats_2_distributed.py -s " + shc + " -sd " + str(start_date) + \
    #      " -ed " + end_date
    cmd = " ".join([execs.execs().generate_multiple_indicator_stats_2_distributed, "-s", shc, "-sd", str(start_date), "-ed", end_date])
    print(cmd)
    os.system(cmd)
