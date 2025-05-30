#!/usr/bin/env python
import subprocess


import MySQLdb

from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.file_utils import get_all_files_in_dir
from walkforward.utils.sql_str_converter import sql_out_to_str


def get_strats_for_shortcode(shortcode, exclude_special=True):
    """
    Runs bash ls command to get the list of strats for given shortcode
    :param shortcode:
    :return:
    """

    list_of_strats = []
    # dir = '/home/dvctrader/modelling/wf_strats/' + shortcode + '/'
    # list_of_strats = get_all_files_in_dir(dir)

    # list_of_strats = [ strat for strat in list_of_strats if strat.find('EBT') < 0 ]
    # return list_of_strats

    if shortcode == "EQIALL":
        sql_query = 'SELECT cname FROM wf_configs WHERE shortcode = \"%s\" AND strat_type != \"%s\" ORDER BY RAND() LIMIT 10' % (
        shortcode, "EBT")
    else:
        sql_query = 'SELECT cname FROM (SELECT * FROM wf_configs WHERE shortcode = \"%s\" AND strat_type != \"%s\" ORDER BY RAND())' \
                    'as z GROUP BY z.config_type' % (shortcode, "EBT")


    cursor = connection().cursor()

    try:
        cursor.execute(sql_query)
        data = cursor.fetchall()
        data = sql_out_to_str(data)

        if len(data) > 0:
            for strat in data:
                list_of_strats.append(strat[0])

    except MySQLdb.Error as e:
        print(("Could not execute the command %s " % sql_query, e))

    return list_of_strats
