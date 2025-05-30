#!/usr/bin/env python

"""

Permute the fields of config based on the given delimeter

"""
import os
import json

from walkforward.definitions.defines import list_of_keys


def permute_config(original_config, original_configname, configs_dir):
    logging = False
    config_json = json.loads(original_config.config_json)
    dummy_config = {}
    for key in list(config_json.keys()):
        dummy_config[key] = ""

    diff_vec = []
    num_params = 1

    for key in list(config_json.keys()):
        if key not in list_of_keys:
            print(("Key " + key + " Not in defined list of keys, please check the name"))

        val = config_json[key]

        if logging:
            print(("Key: ", key, "Pair:", config_json[key]))

        # if the type is list then do nothing for now
        if key == 'model_list' or key == 'param_list':
            diff_vec.append((key, [config_json[key]]))
            continue
        elif type(val) is list:
            # it could be like ['STDEV', '0.5:0.6','0.8:0.9']
            this_list = [item.split(':') for item in val]
            new_list = find_permuations_from_list_of_list(this_list)
            diff_vec.append((key, new_list))
            num_params *= len(new_list)
        else:
            val_words = val.split(':')
            if len(val_words) > 0:
                # there are more than 1 value of the config field found, create permutations from this
                diff_vec.append((key, val_words))
                num_params *= len(val_words)

    if logging:
        print(("Len: ", len(diff_vec), " Vec: ", diff_vec, " NumParam: ", num_params))

    # create list with same size as number of params
    json_vector = [{} for i in range(num_params)]

    if num_params > 1:
        # create all the configs with the params
        """
        This code is python version of param_permute.pl
        """
        num_repetitions = len(json_vector)
        for diff_idx in range(len(diff_vec)):
            key = diff_vec[diff_idx][0]  # key
            permutation = diff_vec[diff_idx][1]  # permutations  given for the key
            # compute how many times given key is going to be repeated
            num_repetitions /= len(permutation)
            json_idx = 0
            key_val = 0
            while json_idx < len(json_vector):
                repeat_idx = 0
                while repeat_idx < num_repetitions:
                    json_vector[json_idx][key] = diff_vec[diff_idx][1][key_val]
                    json_idx += 1
                    repeat_idx += 1

                key_val += 1
                if key_val == len(permutation):
                    key_val = 0

    # dump the computed json string into the config files

    config_files = []
    for json_idx in range(len(json_vector)):
        # get the unique id from the working dir
        unique_id = os.path.dirname(configs_dir).split('/')[-1]

        # create the configfilename
        this_configname = os.path.basename(original_configname) + '_' + unique_id + '_' + str(json_idx)
        this_configname = os.path.join(configs_dir, this_configname)
        # maintain the list of configfilenames to return later
        config_files.append(this_configname)
        with open(this_configname, 'w') as configfile:
            json.dump(json_vector[json_idx], configfile)
            configfile.close()
        if logging:
            print(("Filename: ", (this_configname), ' Content: ', json.dumps(json_vector[json_idx])))

    return config_files


def find_permuations_from_list_of_list(original_list):
    # return [''.join(item) for item in original_list]
    #print ("This List", original_list)
    return_list = []
    total_num = 1
    for item in original_list:
        total_num *= len(item)
    if total_num == 1:
        #print ("Return: ", [''.join(item) for item in original_list])
        return [[''.join(item) for item in original_list]]
    elif total_num > 1:
        for items in original_list[0]:
            new_list = find_permuations_from_list_of_list(original_list[1:])
            new_list = [[items] + content for content in new_list] if len(new_list) > 0 else [[items]]
            #print ("NL: ", items, new_list)
            return_list += new_list
    #print ('Return List:', return_list)
    return return_list
