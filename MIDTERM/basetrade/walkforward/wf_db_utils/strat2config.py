#!/usr/bin/env python


"""

This is a core module that helped migrate base strategies to type 3 configs.
Given a legacy strat, it converts it to a type 3 config.
It also creates a wf_ directory if the directory does not exist already.

Given a strategy, it converts it to a config that yields the model param and also the json that represents the config.

"""

# Only needed for Python 2
import argparse
import os
import json


def usage():
    """ Just usage guidance """
    print("strat2config.py -p <stratpath> -w <write_new_stratconfig_into_folder>")
    print(
        "Example: strat2config.py -p /home/dvctrader/modelling/strats/HSI_0/HKT_1305-HKT_1605/strat4_1_emiction_100_ilist_HSI_0_rhoamommth_20160104_20160119_HKT_1305_HKT_1605_215787824_1000_e1_ts1_0_fsr_0.5_6_transform_1 -w 1")


def create_directory(directory):
    """ If a directory does not exist on path, print message and create it """
    if not (os.path.exists(directory)):
        print("Created directory " + directory)
        os.makedirs(directory)


def convert_legacy_strat_to_walkforward_config(legacy_strat_path):
    """

    This is a core function that helped migrate base strategies to type 3 configs.
    Given a legacy strat, it converts it to a type 3 config.
    It also creates a wf_ directory if the directory does not exist already

    Main purpose is to convert a legacy strat with a model and param to a config.
    This function returns the config as a python dict object and also the model param file path. For details see below.

    Usage : strat_config , new_strat_path , directory = convert_legacy_strat_to_walkforward_config 
    ( args.legacy_strat_path )




    legacy_strat_path: str
                       full path of the old strategy file

    returns:
           list 
           [config_dictionary, destination_config_path, directory_name, model_file_path, param_file_path, shortcode, base_strat_name]


    """

    base_strat_name = os.path.basename(legacy_strat_path)
    if not os.path.exists(legacy_strat_path):
        raise ValueError("Strat doesn't exist in filesystem")

    destination_config_path = legacy_strat_path.replace('staged_strats', 'wf_staged_strats')
    if destination_config_path == legacy_strat_path:
        # if already not replaed
        destination_config_path = legacy_strat_path.replace('strats', 'wf_strats')

    destination_config_path = destination_config_path + '.config'

    directory_name = os.path.dirname(destination_config_path)

    # read, clean and figure out the legacy config file
    try:
        with open(legacy_strat_path) as data:
            content = data.readlines()
            content = [line.strip() for line in content if len(line) > 0 and line[0] != '#']
            content = content[0]
            legacy_strat = content.split(" ")
            legacy_strat = [x for x in legacy_strat if x != ""]
    except:
        print(legacy_strat_path + " not readable !")
        txt = legacy_strat_path
        raise ValueError('strat not readable!')

    if len(legacy_strat) > 9 or legacy_strat[0] != "STRATEGYLINE":
        print(legacy_strat_path + " not a valid regular strat")
        txt = legacy_strat_path
        raise ValueError(legacy_strat_path + " not a valid regular strat, structured strat support not yet avl.")
    else:
        model_list = [legacy_strat[3]]
        param_list = [legacy_strat[4]]
        model_file_path, param_file_path, shortcode = legacy_strat[3], legacy_strat[4], legacy_strat[1]
        start_time, end_time = legacy_strat[5], legacy_strat[6]

        if len(legacy_strat) > 8:
            token = legacy_strat[-1]
            strat_type = "EBT"
        else:
            token = "INVALID"
            strat_type = "Regular"

        if legacy_strat[2]== 'MeanRevertingTrading' or legacy_strat[2] == 'IndexFuturesMeanRevertingTrading':
            strat_type = 'MRT'

        strat_config = {"config_type": "3", "execlogic": legacy_strat[2], "shortcode": shortcode,
                        "model_list": model_list, "param_list": param_list, "start_time": start_time,
                        "end_time": end_time, "strat_type": strat_type, "event_token": token,
                        "query_id": legacy_strat[7]}
        return [strat_config, destination_config_path, directory_name, model_file_path, param_file_path, shortcode, base_strat_name]


def write_strat(strat_config, new_strat_path):
    """

    Write the json config to the file system


    strat_config: json_object
                  the json for the strat to be written 


    new_strat_path: str
                   the full path of the destination where the json file has to be written

    return:
          None


    """
    print("Writing strat..done")
    text_file = open(new_strat_path, "w")
    text_file.write(json.dumps(strat_config, sort_keys=True))
    text_file.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', dest='legacy_strat_path', help="complete path to the legacy strategy", type=str,
                        required=True)
    parser.add_argument('-w', dest='write_new_strat',
                        help="should we create new directory structure and write the new strat config?", type=int,
                        const=0, nargs='?', required=True)
    parser.add_argument('-v', dest='verbose', help="show new config?", type=int, const=0, nargs='?')

    args = parser.parse_args()

    try:
        strat_config, new_strat_path, directory = convert_legacy_strat_to_walkforward_config(args.legacy_strat_path)[
            0:3]
        if args.verbose == 1:
            print(strat_config)

        if args.write_new_strat == 1:
            create_directory(directory)

        if args.write_new_strat == 1:
            write_strat(strat_config, new_strat_path)
    except Exception as e:
        print(e)
