#!/usr/bin/env python

"""
Utility script written to rescale the model for type 6 wf model 
"""
from __future__ import print_function

import json
import datetime

import MySQLdb


from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_config_details import fetch_config_details
from walkforward.wf_db_utils.update_config_in_db import update_config_in_db
from walkforward.utils.sql_str_converter import sql_out_to_str


def rescale_type6_model(configname, rescale_constant, start_date=19700101, end_date='TODAY'):
    """
    For a given type6 config, rescales the models
    :param configname: 
    :param rescale_constant: 
    :param start_date: 
    :param end_date: 
    :return:

    """

    if not start_date:
        start_date = 1970101

    if end_date == 'TODAY':
        end_date = int(datetime.date.today().strftime("%Y%m%d"))
    cfg = fetch_config_details(configname)
    if cfg.configid == -1:
        # if config is invalid then return
        raise ValueError("Invalid config provided")
    else:
        if cfg.config_type != 6:
            # currently supporting type 6 configs only as they have to reference separate table
            raise ValueError("Invalid config_type to scale " + str(cfg.config_type) + ". Required config_type = 6")
        else:

            # get the existing coeffs for given config between given dates (inclusive)
            sql_query = 'SELECT date, modelid, coeffs FROM wf_model_coeffs WHERE configid = %d AND date >= %d AND date ' \
                        '<= %d' % (cfg.configid, start_date, end_date)

            cursor = connection().cursor()

            try:
                cursor.execute(sql_query)
            except MySQLdb.Error as e:
                print(("Could not execeute query: " + sql_query + " Error: " + str(e)))

            data = cursor.fetchall()
            data = sql_out_to_str(data)
            # data is tuple
            data_to_be_inserted = []

            for line in data:
                coeffs_this_line = line[2]
                # rescale the coeffs and put them back into a single string
                coeffs_this_line = [str(float(coef) * rescale_constant) for coef in coeffs_this_line.split(',')]
                coeffs_this_line = ','.join(coeffs_this_line)

                data_to_be_inserted.append([line[0], line[1], coeffs_this_line])

            for line in data_to_be_inserted:
                sql_update_query = 'UPDATE wf_model_coeffs SET coeffs = \"%s\" where date = %d AND ' \
                                   'configid = %d' % (line[2], int(line[0]), cfg.configid)
                try:
                    cursor.execute(sql_update_query)
                except MySQLdb.Error as e:
                    print(("Could not execute query: " + sql_update_query + " Error " + str(e)))

            # coming here means the coeffs have been updated successfully, update the model_stdev value in json
            config_json = json.loads(cfg.config_json)

            if 'model_process_string' in config_json:
                original_stdev_string = config_json['model_process_string']
                original_stdev_list = original_stdev_string.split()

                # update only if the stdev is being rescaled and there is valid value for that in json
                if original_stdev_list[0] == '1' and len(original_stdev_list) > 1:
                    original_stdev_list[1] = str(float(original_stdev_list[1]) * rescale_constant)

                # reconstruct the json string again
                original_stdev_string = ' '.join(original_stdev_list)
                config_json['model_process_string'] = original_stdev_string

            cfg.config_json = json.dumps(config_json, sort_keys=True)

            update_config_in_db(configname, cfg)
