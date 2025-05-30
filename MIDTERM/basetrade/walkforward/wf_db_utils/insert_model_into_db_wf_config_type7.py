"""
Description:
This is utility script only used for type 6 configs.
For a given model file, it inserts the indicator list and weights separately in two tables

"""

import os
import subprocess
from walkforward.wf_db_utils.db_handles import connection
from walkforward.wf_db_utils.fetch_dump_model import fetch_modelid_from_modelname
from walkforward.wf_db_utils.fetch_dump_model import fetch_model_id_from_config_id_and_date
from walkforward.wf_db_utils.fetch_dump_coeffs import insert_or_update_model_coeffs
from walkforward.wf_db_utils.fetch_dump_model import fetch_model_desc


def insert_model_coeffs_in_db(init_model, model_file, configid, tradingdate):
    """

    Following things are done here:
    a) Read ilist ( insert if already not there)
    b) Read model file
    c) Get the weights for each indicator ( null if indicator is not there in model)
    d) Insert weights in appropriate tables

    :param init_model: ilist
    :param model_file: model with weights
    :param configid: config for which we want to add
    :param tradingdate: tradingdate
    :return: None

    """

    modelid = fetch_model_id_from_config_id_and_date(model_file, configid, True, tradingdate)
    model_desc = fetch_model_desc(modelid)
    print(("inserting new coeffs with modelid " + str(modelid) + " and configid " + str(configid)))
    if model_desc is None:
        print(("reading from filesystem " + init_model))
        f = open(init_model, 'r')
        model_desc = f.read()
        model_lines = model_desc.splitlines()
        f.close()
    else:
        model_lines = model_desc.splitlines()

    indicators_list = []
    weight_map = {}

    for line in model_lines:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            #print (indicator)
            weight_map[indicator] = 0
            indicators_list.append(indicator)

    # ifh.close()

    if not os.path.exists(model_file):
        raise ValueError(model_file + " : this model file doesnt not exists")

    table_entry = ""
    is_empty_model = True

    mfh = open(model_file, 'r')

    for line in mfh:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = " ".join(x for x in tokens[2:end_indx])
            if indicator in weight_map:
                weight_map[indicator] = (tokens[1])
            else:
                raise ValueError(indicator + ": this indicator KEY is missing in init_model weights map !! did you"
                                             " change ilist while process is running ?")
            is_empty_model = False

    mfh.close()

    # if final model is empty, don't dump in DB
    if is_empty_model:
        return
    for indicator in indicators_list:
        table_entry += str(weight_map[indicator]) + ","

    table_entry = table_entry[:-1]

    insert_or_update_model_coeffs(int(configid), modelid, table_entry, tradingdate)

    # check if coeffs already exists ( update )


def insert_model_coeffs_in_db_for_dicbt(init_model, model_file, configid, tradingdate):
    modelid = fetch_modelid_from_modelname(init_model, False, configid)
    model_desc = fetch_model_desc(modelid)

    for i in range(0, 20):
        table_entry += str(1) + ","

    table_entry = table_entry[:-1]

    insert_or_update_model_coeffs(int(configid), modelid, table_entry, tradingdate)
