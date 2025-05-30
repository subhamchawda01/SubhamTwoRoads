#!/usr/bin/env python

"""
Used by get_modelparams_for_config

"""

import subprocess


from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.definitions import execs
from walkforward.utils.sql_str_converter import sql_out_to_str


def get_modelparams_for_config(configname, tradingdate, lookback_days):
    # fetch the details of config from DB
    cfg = fetch_config_details(configname)

    # get the start from the end date and lookback days
    start_date_cmd = [execs.execs.calc_prev_week_day, str(tradingdate), str(lookback_days)]
    out = subprocess.Popen(' '.join(start_date_cmd), shell=True, stdout=subprocess.PIPE)
    start_date = int(out.communicate()[0].decode('utf-8').strip())

    # fetch all model and params from the test database
    fetch_query = ("SELECT date, models.modelfilename, params.paramfilename FROM wf_strats, models, params "
                   "WHERE models.modelid = wf_strats.modelid AND params.paramid = wf_strats.paramid AND "
                   "models.configid = %d AND date >= %d AND date <= %d" % (cfg.configid, start_date, tradingdate))

    cursor = connection().cursor()
    cursor.execute(fetch_query)

    data = cursor.fetchall()
    data = sql_out_to_str(data)
    # map to store date to model param pair
    date_to_model_param_pair = {}
    if len(data) > 0:
        for line in data:
            if len(line) >= 3:
                date_to_model_param_pair[line[0]] = (line[1], line[2])
        return date_to_model_param_pair

    else:
        print("NO entry found for config %s" % (configname))
