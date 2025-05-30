#!/usr/bin/env python


"""
there is a primary key, unique key and two multple keys
modelid, modelfilename, shortcode and configid

so,
a) ADD
b) MODIFY (no function yet)
d) DELETE (put together with other tables, so we dont have dangling records)
d) FETCH/SELECT 


modelid, shortcode, modelfilename, regression, modelmath,
training_sd, training_ed, training_st, tradining_et
filter, pred_dur, pred_algo, sample_timeouts
stdev_or_l1norm, change_or_return, last_update_date
model_desc config
"""
from __future__ import print_function

import os
import json
import datetime

import MySQLdb

from walkforward.wf_db_utils.db_handles import connection


def insert_models(model_list_filenames, config_id, config_struct):
    for modelfile in set(model_list_filenames):
        insert_model(modelfile, config_id, config_struct)

    # we insert modelfilename as basename + configid
    # we also insert model_contents


def insert_model(modelfile, config_id, config_struct):
    """

    inserts model and dumps its contents into DB
    modelfilename: basename + configid
    model_desc: should not be none/empty

    :param modelfile: the path of the modelfile in filesystem to dump in DB
    :param config_id:
    :param config_struct: config_struct (used only in type-6 for training_info_json_string)
    :return:
    """

    try:
        with open(modelfile, 'r') as mfh:
            model_desc = mfh.read()
            mfh.close()
    except:
        raise ValueError(modelfile + " cannot be opened")

    json_dict = json.loads(config_struct.config_json)
    today = datetime.date.today().strftime("%Y%m%d")

    if config_struct.config_type == 6:
        modelmath = "LINEAR"
        if (json_dict["reg_string"].split())[0] == "SIGLR":
            modelmath = "SIGLR"

        change_or_return = "CHANGE"
        if config_struct.execlogic == "ReturnsBasedAggressiveTrading":
            change_or_return = "RETURNS"

        model_process_string = json_dict["model_process_string"]
        if int(model_process_string.split()[0]) == 0:
            model_process_string = 0
        else:
            model_process_string = model_process_string.split()[1]

        training_info_json_string = "{regression:\"" + str(json_dict["reg_string"]) + "\", modelmath:\"" + str(modelmath) + \
                                    "\", training_sd:\"" + str(json_dict["walk_start_date"]) + "\", training_ed:\"" + str(json_dict["ddays_string"]) + \
                                    "\", training_st:\"" + str(config_struct.start_time) + "\", training_et:\"" + str(config_struct.end_time) + \
                                    "\", filter:\"" + str(json_dict["rdata_process_string"]) + "\", pred_dur:\"" + str(json_dict["rd_string"].split()[1]) + \
                                    "\", pred_algo:\"" + str(json_dict["rd_string"].split()[0]) + "\", sample_timeouts:\"" + str(json_dict["td_string"]) + \
                                    "\", stdev_or_l1norm:\"" + str(model_process_string) + "\", change_or_return:\"" + str(change_or_return[0]) + \
                                    "\", last_update_date:\"" + str(today) + "\"}"
    else:
        training_info_json_string = "{last_update_date:\"" + str(today) + "\"}"

    mfn_2_insert = os.path.basename(modelfile)
    if mfn_2_insert.split("_")[-1] != str(config_id):
        mfn_2_insert = mfn_2_insert + "_" + str(config_id)

    if len(mfn_2_insert) > 255:
        raise ValueError("can not insert model " + mfn_2_insert + " .. filename too long")

    if not model_desc or not mfn_2_insert:
        raise ValueError("can not insert model with empty name or empty model_desc")

    insert_query = "INSERT INTO models(shortcode, modelfilename, model_desc, configid, training_info) VALUES"\
                   "(\"%s\", \"%s\", \"%s\", %s, \'%s\');"\
                   % (config_struct.shortcode, mfn_2_insert, model_desc, str(config_id), training_info_json_string)

    print(insert_query)
    print("\n")
    try:
        cursor = connection().cursor()
        cursor.execute(insert_query)
    except MySQLdb.Error as e:
        raise ValueError("could not insert model the command", e)


# insert
# if inserting we need config_id
# we send filesystem_filename

# fetch
# fullmodelfilename or basefilename + configid
