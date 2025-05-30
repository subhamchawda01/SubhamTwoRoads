
# This contains functions which can be used by anyone in any other code too.
# Don't add any risk allocation specific function/variables here

import numpy as np
import os, sys
import subprocess

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs

# Global map of session to supersession
# NOTE :- MAY NEED TO BE UPDATED WITH TIME
sessions_supersession_map = {'AS' : ['AS','ASD','ASM','PRE_AS','FS','HK','ASD_FESX','JGBL:SS','PRE_HK','HK1','PRENSE','AEUS','PRE_CME','AS2'],
                             'EU' : ['EU','PRE_EU','EQO','EXO','SS','JGBL:TS','HK2','SGX_IU:FS','MMSS'],
                             'US' : ['US','USD','TS','EQC','LS','EUS','SGX_IN:SS','SGX_TW:SS','MMTS','US_ProRata','USM','US_LP','US_HP','DI']}

# Get list of all sub sessions
session_values = list(sessions_supersession_map.values())
all_valid_session_list = [x for y in session_values for x in y]


def isvalid_shortcode(shortcode):
    """
    Check is shortcode is valid or not

    :param shortcode:   str
    :return:            bool
    """
    out = subprocess.check_output('~/basetrade/scripts/is_valid_shc.pl ' + shortcode, shell=True).decode('utf-8').splitlines()
    return int(out[0]) == 1


def isvalid_session(session):
    """
    Check if session is valid or not

    :param session:     str
    :return:            bool
    """

    if session not in all_valid_session_list:
        return False
    else:
        return True


def isvalid_exchange(exchange, input_date):
    """
    Check if exchange is valid or not

    :param exchange:                   str
    :param input_date:                 int
    :return:                           bool
    """

    # Get list of all exchanges
    get_exchange_command = execs.execs().get_contract_specs + " ALL " + str(input_date) + " EXCHANGE | cut -d' ' -f2 | sort | uniq"
    out = subprocess.check_output(get_exchange_command, shell=True)
    all_valid_exchange_list = out.decode('utf-8').strip().split('\n')

    if exchange in all_valid_exchange_list:
        return  True
    else:
        return False


def get_super_session_from_subsession(sub_session, shc=None):
    """
    Returns super session for a session and shortcode. 
    
    :param sub_session:     str 
    :param shc:             str (can be specified if some special handling is required for shortcode)
    :return:                str
    """

    # Handeling special cases in the if conditions. These are where shortcode is also important for getting the super session.
    if shc == 'JGBL_0':
        if sub_session == 'TS':
            return 'EU'
        if sub_session == 'SS':
            return 'AS'
    if shc == 'SGX_IU_0' and sub_session == 'FS':
        return 'EU'
    if shc == 'SGX_IN_0' and sub_session == 'SS':
        return 'SS'
    if shc == 'SGX_TW_0' and sub_session == 'SS':
        return 'SS'

    for ss in sessions_supersession_map.keys():
        if sub_session in sessions_supersession_map[ss]:
            return ss

    return None


def convert_list_to_tuple(input_list):
    """
    Returns tuple from a list. 
    
    :param input_list:  list 
    :return:            tuple
    
    """

    if len(input_list) == 1:
        if type(input_list[0]) == str:
            return "('" + input_list[0] + "')"
        elif (isinstance(input_list[0], np.int64) or isinstance(input_list[0], int)):
            return "(" + str(input_list[0]) + ")"
    else:
        return tuple(input_list)


def find_first_k_non_nulls(ser, k):
    """
    Given a series and k, returns the index till when k non null numbers can be found in series
    
    :param ser:     pandas series
    :param k:       int
    :return:        int        
    """

    non_null_index = np.where(ser.notnull().tolist())[0]
    if len(non_null_index) < k:
        print("Invalid k in function find_first_k_non_nulls")
        return None
    return non_null_index[k-1]


def get_intv_to_one_weights_vec(interval_dict):
    """
    Returns weight vector of ones according the interval dict provided. Length of each vector is same. 0 are appended at the end
    if input is {2: 1.0, 5: 0.7, 8:0.3}, then output will be {2:[1,1,0,0,0,0,0,0], 5:[1,1,1,1,1,0,0,0], 8:[1,1,1,1,1,1,1,1]}

    :param interval_dict:   dict having days and weight to give to it
    :return:                list of weights
    """

    date_list = {}
    max_len = max(interval_dict.keys())
    for current_interval in np.sort(list(interval_dict.keys())):
        date_list[current_interval] = [1.0] * current_interval + [0.0] * (max_len-current_interval)

    return date_list


def get_weight_vector_for_dates(interval_dict):
    """
    Returns weight vector according the interval dict provided
    if input is {2: 1.0, 5: 0.7, 10: 0.3}, then output will be [1.0,1.0,0.7,0.7,0.7,0.3,0.3,0.3,0.3,0.3]
    
    :param interval_dict:   dict having days and weight to give to it 
    :return:                list of weights 
    """

    weighted_date_list = []
    prev = 0
    for current_interval in np.sort(list(interval_dict.keys())):
        wt = interval_dict[current_interval]
        weighted_date_list.extend([wt]*len(range(prev,current_interval)))
        prev = current_interval

    return weighted_date_list


def get_start_end_time_from_pick_strat_config(shc, sub_session):
    """
    Return start end time from the pick strat config

    :param shc:             str        
    :param sub_session:     str
    :return:                list of start-end time. (mutiple can be present in pick strat config)
    """

    pick_strats_config_file_list = subprocess.check_output("find /home/dvctrader/modelling/pick_strats_config -iname "
                                                           + shc + '.' + sub_session + '.txt', shell=True).decode('utf-8').splitlines()

    if len(pick_strats_config_file_list) < 1:
        print("UNEXPECTED: Pick Strat config not found for ", shc, ", ", sub_session)
        return None
    elif len(pick_strats_config_file_list) > 1:
        print(
        "UNEXPECTED: More than 1 pick strat configs found for ", shc, sub_session, ". Picking first one from here")
        print(pick_strats_config_file_list)

    pick_strats_config_file = pick_strats_config_file_list[0]

    command_to_run = '~/basetrade/scripts/get_config_field.pl ' + pick_strats_config_file + ' FOLDER_TIME_PERIODS'
    out = subprocess.check_output(command_to_run, shell=True)
    out = out.decode('utf-8').strip()
    if out == '':
        print("UNEXPECTED: Folder Time period not available for ", pick_strats_config_file)
        return None
    else:
        out = out.split('\n')

    return out


def exit_code_with_message(msg, exit_code=123):
    """
    Exit from code with a given exit and printing a message

    :param msg:         str
    :param exit_code:   int
    :return:            None. exits the code
    """
    print(msg)
    sys.exit(exit_code)
