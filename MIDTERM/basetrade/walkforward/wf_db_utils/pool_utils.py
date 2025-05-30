#!/usr/bin/env python


"""
Prints number of configs / list of configs which satisfy the given criteria.
Currently it's being used for checking number of staged and normal configs

@shortcode: The shortcode for which we want to see the config
@normal_or_staged: The configs are from normal or staged pool
   'N' : normal
   'S' : staged
   'A' : all (default)
   
"""

from __future__ import print_function

import MySQLdb


from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.utils.sql_str_converter import sql_out_to_str

require_pooltag_shc_vec = ['BR_DOL_0', 'BR_WDO_0', 'BR_WIN_0']


# print number of configs for a pool
def get_num_configs_for_pool(shortcode, start_time, end_time, normal_or_staged='A', pooltag='', strat_type='Regular', event_token='INVALID'):
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()

    if strat_type == 'Regular':
        search_query = ("SELECT COUNT(*) FROM  wf_configs WHERE shortcode = \"%s\" AND strat_type = \"%s\" AND start_time = \"%s\" AND end_time = \"%s\""
                        % (shortcode, strat_type, start_time, end_time))
    elif strat_type == 'EBT':
        search_query = ("SELECT COUNT(*) FROM  wf_configs WHERE shortcode = \"%s\" AND strat_type = \"EBT\"" % (shortcode))
        if event_token != 'INVALID':
            search_query += ' AND event_token = \"%s\"' % event_token
        #        if start_time is not None and end_time is not None:
        #            search_query += ' AND start_time = \"%s\" AND end_time = \"%s\"' % (start_time, end_time)

    if shortcode in require_pooltag_shc_vec or pooltag != '':
        search_query += ' AND pooltag = \"%s\"' % pooltag

    # if specifically asked for normal or staged then add that condition
    if normal_or_staged == 'N' or normal_or_staged == 'S':
        search_query += ' AND type = \"%s\"' % normal_or_staged

    try:
        cursor.execute(search_query)
    except MySQLdb.Error as e:
        print(("Could not execute command %s" % (search_query), e))

    data = cursor.fetchall()
    data = sql_out_to_str(data)

    num_configs = 0
    if len(data) > 0:
        num_configs = int(data[0][0])

    return num_configs


# print list of configs for a pool
def get_configs_for_pool(shortcode, start_time, end_time, normal_or_staged='N', pooltag='', strat_type='Regular', event_token='INVALID'):
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()

    if strat_type == 'Regular':
        search_query = ("SELECT cname FROM  wf_configs WHERE shortcode = \"%s\" AND strat_type = \"%s\" AND start_time = \"%s\" AND end_time = \"%s\""
                        % (shortcode, strat_type, start_time, end_time))
    elif strat_type == 'EBT':
        search_query = ("SELECT cname FROM  wf_configs WHERE shortcode = \"%s\" AND strat_type = \"EBT\"" % (shortcode))
        if event_token != 'INVALID':
            search_query += ' AND event_token = \"%s\"' % event_token
        #        if start_time is not None and end_time is not None:
        #            search_query += ' AND start_time = \"%s\" AND end_time = \"%s\"' % (start_time, end_time)

    if shortcode in require_pooltag_shc_vec or pooltag != '':
        search_query += ' AND pooltag = \"%s\"' % pooltag

    # if specifically asked for normal or staged then add that condition
    if normal_or_staged == 'N' or normal_or_staged == 'S':
        search_query += ' AND type = \"%s\"' % normal_or_staged

    try:
        cursor.execute(search_query)
    except MySQLdb.Error as e:
        print(("Could not execute command %s" % (search_query), e))

    data = cursor.fetchall()
    data = sql_out_to_str(data)
    outdata = [line[0] for line in data]

    return outdata


# print list of configs for a pool
def get_configs_for_shortcode(shortcode, normal_or_staged='N'):
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()

    search_query = ("SELECT cname FROM  wf_configs WHERE shortcode = \"%s\"" % shortcode)

    # if specifically asked for normal or staged then add that condition
    if normal_or_staged == 'N' or normal_or_staged == 'S':
        search_query += ' AND type = \"%s\"' % normal_or_staged

    try:
        cursor.execute(search_query)
    except MySQLdb.Error as e:
        print(("Could not execute command %s" % (search_query), e))

    data = cursor.fetchall()
    data = sql_out_to_str(data)
    outdata = [line[0] for line in data]

    return outdata


# print list of configs for a pool given a config
def get_pool_configs_for_config(config, normal_or_staged='N'):
    cfg = fetch_config_details(config)
    if cfg.configid <= 0:
        return

    return get_configs_for_pool(cfg.shortcode, cfg.start_time, cfg.end_time, normal_or_staged, cfg.pooltag, cfg.strat_type, cfg.event_token)


def get_pools_for_shortcode(shortcode, normal_or_staged='N', strat_type='Regular'):
    # prepare a cursor object using cursor() method
    cursor = connection().cursor()

    search_query = ("SELECT distinct start_time,end_time FROM wf_configs WHERE shortcode = \"%s\""
                    % (shortcode))

    if shortcode in require_pooltag_shc_vec:
        search_query = ("SELECT distinct start_time,end_time,pooltag FROM wf_configs WHERE shortcode = \"%s\""
                        % (shortcode))

    if strat_type == 'EBT':
        search_query = ("SELECT distinct event_token FROM wf_configs WHERE shortcode = \"%s\"" % (shortcode))

    if strat_type == 'Regular' or strat_type == 'EBT' or strat_type == 'MRT':
        search_query += ' AND strat_type = \"%s\"' % strat_type

        # if specifically asked for normal or staged then add that condition
    if normal_or_staged == 'N' or normal_or_staged == 'S':
        search_query += ' AND type = \"%s\"' % normal_or_staged

    try:
        cursor.execute(search_query)
    except MySQLdb.Error as e:
        print(("Could not execute command %s" % (search_query), e))

    data = cursor.fetchall()
    data = sql_out_to_str(data)
    outdata = []
    for line in data:
        # if token for pooltag is none or empty string
        if len(line) > 2 and (line[2] is None or line[2] == ''):
            line = line[0:2]
        outdata.append("-".join(line))

    return outdata
