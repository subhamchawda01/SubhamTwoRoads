#!/usr/bin/env python

from __future__ import print_function

import MySQLdb

from walkforward.wf_db_utils.db_handles import connection


# updates the wf_strats entry for date with the entry for prev_model_date_

def insert_prev_strat_for_date(configid_, prev_model_date_, date_):
    """
    It will dump the strat for configid and prev_model_date for the date given in wf_strats
    :param configid_:  
    :param prev_model_date_: Date from which you want to insert the strat
    :param date_: Date for which you want to insert the strat
    :return: 
    """

    insert_query = "insert into wf_strats (configid,date,paramid,modelid) (select configid, %d, paramid, modelid from wf_strats where configid = %d and \
                    date = %d)" % ( date_, configid_, prev_model_date_)

    cursor = connection().cursor()

    try:
        cursor.execute(insert_query)
    except MySQLdb.Error as e:
        print(("Could not execute the command %s " % (insert_query), e))
