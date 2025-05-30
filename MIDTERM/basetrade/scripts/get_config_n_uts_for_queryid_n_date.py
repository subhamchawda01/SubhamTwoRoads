#!/usr/bin/env python


"""
Get the config and uts for a given query id and date
"""
import argparse
import os
import sys
import gzip

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details


def get_uts_from_param(param_file_name):
    """
    gets the uts from the paramfile
    :param param_file_name: 
    :return: 
    """
    uts = 0
    if os.path.exists(param_file_name):
        fh = open(param_file_name, "r")
        lines = fh.read().splitlines()
        fh.close()
        for line in lines:
            words = line.split()
            if len(words) > 2:
                if words[0] == "PARAMVALUE" and words[1] == "UNIT_TRADE_SIZE":
                    uts = words[2]

    return uts


def get_date_to_config_to_uts_map_for_queries_n_dates(query_start_id_, query_end_id_, list_of_dates_):
    """

    :param query_start_id_: 
    :param query_end_id_: 
    :param list_of_dates_: 
    :return: map of date to confignames to their UTS 
    """
    date_to_config_to_uts_map_ = {}
    date_to_config_to_pnl_map_ = {}
    for dt in list_of_dates_:
        config_to_uts_map_for_date_temp = {}
        config_to_pnl_map_for_date_temp = {}
        for query in range(int(query_start_id_), int(query_end_id_) + 1):

            config_maps = get_config_to_uts_map_for_query_id_n_date(query, dt)

            config_to_uts_map_temp = config_maps[0]
            config_to_pnl_map_temp = config_maps[1]
            config_to_uts_map_for_date_temp.update(config_to_uts_map_temp)
            config_to_pnl_map_for_date_temp.update(config_to_pnl_map_temp)

        date_to_config_to_uts_map_[dt] = config_to_uts_map_for_date_temp
        date_to_config_to_pnl_map_[dt] = config_to_pnl_map_for_date_temp
    return [date_to_config_to_uts_map_, date_to_config_to_pnl_map_]


def get_config_to_uts_map_for_query_id_n_date(query_id_, date_):
    """

    gets the dictionay of confignames to corresponding uts it was run given the query_id and date 
    :param query_id_: 
    :param date_: 
    :return: 
    """

    # get the log file
    yyyy = str(date_)[0:4]
    mm = str(date_)[4:6]
    dd = str(date_)[6:8]

    query_log_dir = "/NAS1/logs/QueryLogs/" + yyyy + "/" + mm + "/" + dd + "/"
    query_trades_dir = "/NAS1/logs/QueryTrades/" + yyyy + "/" + mm + "/" + dd + "/"
    query_log_file = query_log_dir + "log." + str(date_) + "." + str(query_id_)
    query_log_file_gz = query_log_file + ".gz"
    query_trade_file = query_trades_dir + "trades." + str(date_) + "." + str(query_id_)

    if os.path.exists(query_log_file):
        f = open(query_log_file, "r")
    elif os.path.exists(query_log_file_gz):
        f = gzip.open(query_log_file_gz, "rb")
    else:
        return [{}, {}]

    query_log_lines = f.read().splitlines()
    f.close()

    if os.path.exists(query_trade_file):
        f = open(query_trade_file, "r")
    else:
        return [{}, {}]

    query_trade_lines = f.read().splitlines()
    f.close()

    # get the map from the logfile; load only those configs which are valid, in this case, not deleted
    cnames_to_uts = {}
    cnames_to_pnl = {}
    for line in query_log_lines:
        if b"STRATEGYLINE" in line:
            words = line.strip().split()
            cname = words[-1].decode('ascii')
            cfg = fetch_config_details(cname)
            if cfg:
                if not cfg.is_valid_config():
                    pnl = int(query_trade_lines[-1].split()[8])
                    cnames_to_pnl[cname] = int(pnl)
                    cnames_to_uts[cname] = 0
                    continue
                else:
                    cnames_to_pnl[cname] = 0
            else:
                continue
            param_file = words[4]
            param_file_in_logs_dir = os.path.join(query_log_dir, os.path.basename(param_file.decode('ascii')))

            uts = get_uts_from_param(param_file_in_logs_dir)
            cnames_to_uts[cname] = uts

    return [cnames_to_uts, cnames_to_pnl]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', dest='query_id', type=str, required=True)
    parser.add_argument('-d', dest='date', type=str, required=True)

    args = parser.parse_args()
    query_id = args.query_id
    date = args.date

    cnames_to_uts_map = get_config_to_uts_map_for_query_id_n_date(query_id, date)
