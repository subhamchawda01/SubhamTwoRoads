#!/usr/bin/env python

"Utility to insert a scaled model for date"

import sys
import os
import json
sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.wf_db_utils.fetch_prev_model_data_for_config_and_date import fetch_prev_model_data_for_config_and_date
from walkforward.wf_db_utils.fetch_dump_coeffs import insert_or_update_model_coeffs
from walkforward.wf_db_utils.insert_prev_strat_for_date import insert_prev_strat_for_date
from walkforward.utils.shortcode_utils import get_tick_ratio_for_dates

def insert_rescaled_model_for_date(this_cfg_p,date):
    config_json = json.loads(this_cfg_p.config_json)

    prev_model_refresh_data = fetch_prev_model_data_for_config_and_date(this_cfg_p.cname,
                                                                      date)
    prev_model_date = prev_model_refresh_data[0][0]
    model_id = prev_model_refresh_data[0][1]

    tick_ratio = get_tick_ratio_for_dates(config_json["shortcode"],date,prev_model_date)
    model_ind_coeffs = prev_model_refresh_data[0][2]
    model_ind_coeffs_list = list(map(str, model_ind_coeffs.split(',')))
    new_ind_coeffs = []
    for ind_coeffs in model_ind_coeffs_list:
        if ind_coeffs == '0':
            new_ind_coeffs.append(ind_coeffs)
        else:
            if config_json["reg_string"].split()[0] == "SIGLR" and len(ind_coeffs.split(":")) > 1:
                new_ind_coeffs.append(ind_coeffs.split(":")[0] + str(float(ind_coeffs.split(":")[
                                                                               1]) * tick_ratio))  # multiplying only beta for scaling the model in case of SIGLR
            else:
                new_ind_coeffs.append(str(float(ind_coeffs) * tick_ratio))

    new_model_coeffs = (','.join(new_ind_coeffs))

    insert_or_update_model_coeffs(this_cfg_p.configid, model_id, new_model_coeffs, int(date))
    insert_prev_strat_for_date(this_cfg_p.configid,prev_model_date,date)
