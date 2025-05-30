import sys
import os

sys.path.append(os.path.expanduser('~/basetrade/'))
from scripts.ind_pnl_based_stats import *
from walkforward.utils.date_utils import calc_prev_week_day
import walkforward.definitions.execs as execs

if __name__=="__main__":
    # prod_ids = get_all_prod_ids()
    if len(sys.argv) < 4:
        print("USAGE <script> shc end_date num_days")
        sys.exit()

    shc = sys.argv[1]
    end_date = sys.argv[2]
    num_days = sys.argv[3]

    start_date = calc_prev_week_day(end_date, int(num_days))
    prod_ids = get_prod_ids_for_shortcode(shc)
    if len(prod_ids) ==0:
        print("There is no product for this shortcode in db.")
    for prod_id in prod_ids:
        # shc = get_shortcode_from_prod_id(prod_id)
        start_time = get_start_time_from_prod_id(prod_id)
        end_time = get_end_time_from_prod_id(prod_id)
        #cmd = " ".join(["python", os.environ["HOME"] + "/basetrade/scripts/pnl_based_stats_handler.py", "-s", shc, "-st",
        #                start_time, "-et", end_time, "-sd", start_date, "-ed", end_date, "-m", "GENERATE"])
        cmd = " ".join([execs.execs().pnl_based_stats_handler, "-s", shc, "-st", start_time, "-et", end_time, "-sd", str(start_date), "-ed", end_date, "-m GENERATE"])
        print(cmd)
        os.system(cmd)
