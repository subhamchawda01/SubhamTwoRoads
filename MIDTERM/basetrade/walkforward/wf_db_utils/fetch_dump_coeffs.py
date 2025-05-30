#!/usr/bin/env python

from __future__ import print_function

import MySQLdb

from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str

# inserts or updates the model coefficients for a particular config, and date


def insert_or_update_model_coeffs(configid, modelid, coeffs, date):
    """

    Inserts the model coefficient in wf_model_coeffs table. Takes confgid,modelid and date and adds the model coefficient for the same. This is for type 6 config


    configid: int
                configid whose coeffs have to be updated

    modelid:  int
                modelid whose coeffs have to be updated

    coeffs:   str
                model coeffs that has to be updated

    date:     str     
                the date for which the coeff has to be udpated


    returns :
            None



    """
    sql_select = "SELECT configid, modelid FROM wf_model_coeffs WHERE configid = " + str(
        configid) + " and modelid = " + str(modelid) + " and date = " + str(date) + ";"
    print(sql_select)
    cursor = connection().cursor()
    cursor.execute(sql_select)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    t_modelid = None
    if len(data) > 0:
        t_modelid = int(data[0][0])
        # if model already present for date, then update, else insert
        sql_update = "UPDATE wf_model_coeffs SET coeffs = \"%s\" WHERE configid = %d and modelid = %d and date = %d" % (
            coeffs, configid, modelid, date)
        print(sql_update)
        try:
            cursor.execute(sql_update)
        except MySQLdb.Error as e:
            print(("Could not execute the command %s " % (sql_update), e))
    else:
        print("inserting new, no entry for modelid " + str(modelid) + " in coeff table for date " + str(date))
        insert_query = "INSERT INTO wf_model_coeffs (configid, modelid, coeffs, date) VALUES ('" + str(
            configid) + "','" + str(modelid) + "','" + coeffs + "','" + str(date) + "');"
        print(insert_query)
        try:
            cursor.execute(insert_query)
        except MySQLdb.Error as e:
            print(("Could not execute the command %s" % (insert_query), e))

    return modelid
