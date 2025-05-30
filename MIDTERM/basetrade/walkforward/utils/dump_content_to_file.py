#!/usr/bin/env python

import os

from walkforward.wf_db_utils.fetch_dump_model import fetch_model_desc_from_modelname
from walkforward.wf_db_utils.fetch_dump_param import fetch_param_desc_from_paramname
from walkforward.definitions import execs


def write_model_to_file(refreshed_model_filename, initial_modelfilename, coeffs, is_siglr, model_desc=None):

    if model_desc is None:
        model_desc = fetch_model_desc_from_modelname(initial_modelfilename)

    if model_desc == "":
        raise ValueError(initial_modelfilename + "model_desc is empty !")
    else:
        model_lines = model_desc.splitlines()

    refreshed_model_writer = open(refreshed_model_filename, 'w')
    indx = 0
    line_indx = 0

    if coeffs != None:
        coeffs = coeffs.split(",")

    for line in model_lines:
        line_indx += 1
        line = line.strip()
        tokens = line.strip().split()

        # If the config has an empty line we should ignore it.
        # This will help us make sure that the next line which compares
        # the first token does not encounter an error.
        if len(tokens) == 0:
            continue

        if tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            to_write = True

            if coeffs is None:
                weight = tokens[1]
            else:
                weight = coeffs[indx]
                indx += 1
                if is_siglr:
                    weight1 = float(weight.split(":")[0])
                    if weight1 == 0.0:
                        to_write = False
                else:
                    if float(weight) == 0.0:
                        to_write = False

            if to_write:
                refreshed_model_writer.write("INDICATOR " + weight + " " + indicator + "\n")
        else:
            if is_siglr and line_indx == 2 and tokens[0] == "MODELMATH":
                refreshed_model_writer.write("MODELMATH SIGLR CHANGE\n")
            else:
                refreshed_model_writer.write(line + "\n")

    refreshed_model_writer.close()


def write_param_to_file(refreshed_param_filename, paramfilename, param_desc=None):

    if param_desc is None:
        param_desc = fetch_param_desc_from_paramname(paramfilename)

    if param_desc == "":
        raise ValueError(paramfilename + "param_desc is empty !")
    else:
        f = open(refreshed_param_filename, "w")
        f.write(param_desc)
        f.close()


def write_model_param_files(data, configid, config_name, coeffs_present=False, is_siglr=False, out_location=None):
    """

    This function takes the data obtained from sql query, writes model and param files for each line to a temp location
    and returns their paths

    data :              2D list.
                        Each list have Trade date, modelfilename(from db), paramfilename(from db)
                        It could additionally have coeffs also (if calling from type 6)

    configid :          int
                        config id from which data was fetched

    config_name :       str
                        Name of config

    coeffs_present :    bool
                        True if coeffs are also present in the lists

    is_siglr :          bool
                        True if regress exec of config is siglr

    out_location :      str
                        Location of folder in which temporary model and param folders will be created

    returns:            2D list for which valid model and param was obtained
                        Each list now additionally have the path of the model and param file names written to the file system

    """

    if out_location is None:
        temp_location = execs.get_temp_location()
    else:
        temp_location = out_location

    model_dir = temp_location + "/temp_models/"
    param_dir = temp_location + "/temp_params/"

    os.system("mkdir --parents " + model_dir)
    os.system("mkdir --parents " + param_dir)

    out_data = []
    for i in range(len(data)):
        line = list(data[i])
        if coeffs_present:
            (trade_date, modelfilename, paramfilename, coeffs) = (line[0], line[1], line[2], line[3])
        else:
            (trade_date, modelfilename, paramfilename, coeffs) = (line[0], line[1], line[2], None)

        modelparam_found = True
        refreshed_model_filename = model_dir + "w_model_" + str(configid) + '_' + str(trade_date)
        try:
            write_model_to_file(refreshed_model_filename, modelfilename, coeffs, is_siglr)
        except ValueError:
            modelparam_found = False
            print("Model Desc is empty for config name %s and date %d. Model File Name = %s" % (
                  config_name, int(trade_date), modelfilename))

        base_paramfilename = os.path.basename(paramfilename)
        refreshed_param_filename = param_dir + base_paramfilename + "_" + str(trade_date)
        try:
            write_param_to_file(refreshed_param_filename, paramfilename)
        except ValueError:
            modelparam_found = False
            print("Param Desc is empty for config name %s and date %d. Param File Name = %s" % (
                  config_name, int(trade_date), paramfilename))

        if modelparam_found == True:
            line.extend([refreshed_model_filename, refreshed_param_filename])
            out_data.append(line)

    return out_data
