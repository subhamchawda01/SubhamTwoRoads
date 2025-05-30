#!/usr/bin/env python
# I don't feel comfortable writing /home/dvctrader/anaconda2/bin/python for the interpreter
#
# fetch_strat_from_config_and_date.py - Fetch the model and param corresponding to the from a MySQL database
# Pseudocode :
# read args with argparse and call function
# In function establish SQL connection
# Get the model and param from database where config and tradedate are what have been specified.
# In case of a failure, try previous tradedate and return value. Keep trying previous dates for 100 times. TODO : I am not certain if this shuld be the correct failure hanlding.
# In case of any failure, even if recovered, log it somewhere. Perhaps log it locally right now.


import os

from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.wf_db_utils.fetch_strat_from_config_struct_and_date import fetch_strat_from_config_struct_and_date
from walkforward.wf_db_utils.fetch_strat_from_config_struct_and_date import fetch_strat_from_config_struct_and_dates


def fetch_strat_from_config_and_date(wf_config, trade_date, days_to_look=1):
    """

    Get the strat for the given config and date

    wf_config: str
            Full path of the walkforward config
    trade_date: str
            End date for fetching the config 
    days_to_look: int 
            Number of lookback days

    return:
         tuple

         (shortcode, execlogic, modelfilename, paramfilename, start_time, 
            end_time, strat_type, event_token, query_id)


    """
    wf_config = os.path.basename(wf_config)

    config_struct = fetch_config_details(wf_config)
    config_struct.sanitize()

    return fetch_strat_from_config_struct_and_date(config_struct, int(trade_date), days_to_look)


def fetch_strat_from_config_and_dates(wf_config, trade_dates, days_to_look=1):
    """

    Get the strat for the given config and list of dates

    wf_config: str
            Full path of the walkforward config
    trade_date: list
            List of dates for fetching the config
    days_to_look: int
            Number of lookback days

    return:
         tuple

         (shortcode, execlogic, modelfilename, paramfilename, start_time,
            end_time, strat_type, event_token, query_id)


    """
    wf_config = os.path.basename(wf_config)

    config_struct = fetch_config_details(wf_config)
    config_struct.sanitize()

    return fetch_strat_from_config_struct_and_dates(config_struct, list(map(int, trade_dates)), days_to_look)
