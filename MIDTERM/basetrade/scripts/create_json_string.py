import os
import json
from os import path
from walkforward.utils.date_utils import calc_prev_week_day


def create_json_string(strat_dir, start_date, end_date, date_list=None):
    """
        The function returns the json string needed for grid api to work


        strat_dir: str
                The absolute path of the directory containing the strategy files

        start_date: str
                The starting date for simulation run

        end_date: str
                The ending date for simulation run

        date_list: list
                The list of days to run simulations on. Default is None

    """

    json_dict = {}
    json_dict["job"] = "compute_pnl"
    json_dict["strategies"] = []
    if date_list is None:
        date_list = []
        curr_date = end_date
        while (curr_date >= start_date):
            date_list.append(curr_date)
            curr_date = calc_prev_week_day(curr_date, 1)

    for strat in os.listdir(strat_dir):
        if path.isfile(os.path.join(strat_dir, strat)):
            strat_line = open(os.path.join(strat_dir, strat), 'r').read()
            strat_line = strat_line.strip()
            strat_dict = {}
            strat_dict["name"] = strat
            strat_dict["strat_line"] = strat_line
            strat_dict["dates"] = list(map(str, date_list))
            json_dict["strategies"].append(strat_dict)
    return json.dumps(json_dict)
