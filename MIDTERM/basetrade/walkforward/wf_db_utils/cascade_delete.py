
from __future__ import print_function

import MySQLdb

from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str


def delete_config_from_configid(configid):
    """
    delete all DB contents related to this configid
    :param configid:
    :return:
    """

    cursor = connection().cursor()
    clear_configid_contents_from_schema(configid)

    delete_query = 'DELETE FROM wf_configs where configid = %d ' % (configid)
    try:
        cursor.execute(delete_query)
        print("deleted wf_configs entry")
    except MySQLdb.Error as e:
        print(("Could not execute " + str(delete_query) + " ERROR: " + str(e)))


def clear_configid_contents_from_schema(config_id):
    print("in cascade delete")
    clear_models_params_coeffs_from_db(config_id)


def clear_models_params_coeffs_from_db(config_id):
    """

    Removes the model,model coefficients ,param and daily strategy entry from the wf_strats table for the given config_id. Only valid for type 6 config

    config_id:int
            Config id whose entry has to be removed from the different tables

    returns: None

    """
    # wf_model_coeffs
    # yes only config rows
    if int(config_id) < 0:
        print(config_id)
        return

    delete_from_wf_model_coeffs(config_id)

    # wf_models
    delete_model_for_config_id(config_id)

    # wf_params
    delete_param_for_config_id(config_id)

    # wf_strats
    delete_strat_for_config_id(config_id)


def delete_from_wf_model_coeffs(config_id):

    cursor = connection().cursor()

    delete_query = ("DELETE FROM wf_model_coeffs WHERE configid = %d" % (config_id))
    try:
        cursor.execute(delete_query)
        print("deleted wf_model_coeffs entry")
    except MySQLdb.Error as e:
        print(("Could not execute " + str(delete_query) + " ERROR: " + str(e)))


def delete_model_for_config_id(config_id):
    # wf_models
    cursor = connection().cursor()
    # delete only if the configid exists => specific to config
    delete_query = ("DELETE FROM models WHERE configid = %d" % (config_id))
    try:
        cursor.execute(delete_query)
        print("deleted models entry")
    except MySQLdb.Error as e:
        print(("Could not execute " + str(delete_query) + " ERROR: " + str(e)))


def delete_param_for_config_id(config_id):
    # wf_params
    cursor = connection().cursor()
    # delete only if the configid exists => specific to config
    delete_query = ("DELETE FROM params WHERE configid = %d" % (config_id))
    try:
        cursor.execute(delete_query)
        print("deleted params entry")
    except MySQLdb.Error as e:
        print(("Could not execute " + str(delete_query) + " ERROR: " + str(e)))


def delete_strat_for_config_id(config_id):
    """
    :param config_id: 
    :return: 
    """
    cursor = connection().cursor()
    # delete only if the configid exists => specific to config
    delete_query = ("DELETE FROM wf_strats WHERE configid = %d" % (config_id))
    try:
        cursor.execute(delete_query)
        print("deleted wf_strats entry")
    except MySQLdb.Error as e:
        print(("Could not execute " + str(delete_query) + " ERROR: " + str(e)))
