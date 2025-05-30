#!/usr/bin/env python

"""
contains the config fields
and utils
"""

import os
import sys
import json
import datetime
from walkforward.definitions.defines import type4_keys, type5_keys, type6_keys


class config():
    configid = -1
    cname = "INVALID"
    shortcode = "INVALID"
    execlogic = "INVALID"
    start_time = "INVALID"
    end_time = "INVALID"
    sname = "INVALID"
    strat_type = "INVALID"
    event_token = "INVALID"
    query_id = -1
    config_type = 3
    simula_approved = 0
    type = 'S'
    config_json = "INVALID"
    description = "INVALID"
    pooltag = ''
    expect0vol = 0
    is_structured = 0
    structured_id = 0

    @classmethod
    def initialize(cls):
        cfg = cls(-1, "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", "INVALID", -1, 3, "INVALID")
        return cfg

    def __init__(self, configid, shortcode, execlogic, start_time, end_time, strat_type, event_token, query_id, config_type, config_json):
        self.configid = configid
        self.shortcode = shortcode
        self.execlogic = execlogic
        self.start_time = start_time
        self.end_time = end_time
        self.strat_type = strat_type
        self.event_token = event_token
        self.query_id = query_id
        self.config_type = config_type
        self.config_json = config_json
        self.description = "This config is created on " + datetime.date.today().isoformat()

    def add_cname(self, configname):
        self.cname = configname

    def add_sname(self, stratname):
        self.sname = stratname

    def add_simula_approved(self, simula_approved):
        self.simula_approved = int(simula_approved)

    def add_type(self, type):
        self.type = type

    def sanitize(self):
        self.configid = 1 if not self.configid else self.configid
        self.shortcode = "INVALID" if not self.shortcode else self.shortcode
        self.execlogic = "INVALID" if not self.execlogic else self.execlogic
        self.start_time = "UTC_0001" if not self.start_time else self.start_time
        self.end_time = "UTC_2359" if not self.end_time else self.end_time
        self.strat_type = "Regular" if not self.strat_type else self.strat_type
        self.event_token = "" if not self.event_token else self.event_token
        self.config_type = 3 if not self.config_type else self.config_type
        self.config_json = "" if not self.config_json else self.config_json
        self.pooltag = "" if not self.pooltag else self.pooltag
        self.expect0vol = 0 if not self.expect0vol else self.expect0vol
        self.is_structured = 0 if not self.is_structured else self.is_structured
        self.structured_id = -1 if not self.is_structured or not self.structured_id else self.structured_id

    def check_modelparam_exists_in_fs(self):
        # check that the model_list and the param_list entries exist with non-zero elements
        config_from_json = json.loads(self.config_json)
        modelparam_keys = ["model_list", "param_list"]
        for key in modelparam_keys:
            if key not in list(config_from_json.keys()) or len(config_from_json[key]) == 0:
                return False

        model_list = config_from_json["model_list"]
        param_list = config_from_json["param_list"]

        # check that the filepaths mentioned in model_list,param_list exists
        for model in model_list:
            if not os.path.exists(model):
                return False
        for param in param_list:
            if not os.path.exists(param):
                return False

        return True

    def is_valid_config(self, check_configid=True):
        # these are general checks for every config
        if check_configid and self.configid < 1:
            sys.stderr.write('Expected valid configid. Found ' + str(self.configid) + '\n')
            return False

        if not self.shortcode or self.shortcode == 'INVALID' \
                or not self.execlogic or self.execlogic == 'INVALID' \
                or not self.start_time or self.start_time == 'INVALID' \
                or not self.end_time or self.end_time == 'INVALID' \
                or not self.strat_type or self.strat_type == 'INVALID' \
                or not self.query_id or self.query_id < 0 \
                or not self.config_type or self.config_type < 1 \
                or not self.config_json or self.config_json == 'INVALID':
            sys.stderr.write('INVALID CONFIG' + self.shortcode + self.execlogic + self.start_time + self.end_time,
                             self.strat_type + str(self.query_id) + str(self.config_type) + self.config_json)
            return False

        config_from_json = json.loads(self.config_json)

        # check that the model_list and the param_list entries exist with non-zero elements
        modelparam_keys = ["model_list", "param_list"]
        for key in modelparam_keys:
            if key not in list(config_from_json.keys()) or len(config_from_json[key]) == 0:
                sys.stderr.write('MODEL_LIST, PARAM_LIST not found')
                return False

        model_list = config_from_json["model_list"]
        param_list = config_from_json["param_list"]

        # check for config 4
        if self.config_type == 4:
            for key in type4_keys:
                if key not in list(config_from_json.keys()):
                    return False

        # check for config 5
        elif self.config_type == 5:
            for key in type5_keys:
                if key not in list(config_from_json.keys()):
                    return False
            if len(config_from_json["feature_switch_threshold"]) != (len(model_list) - 1) and len(config_from_json["feature_switch_threshold"]) != (len(param_list) - 1):
                return False

        # check for config 6
        elif self.config_type == 6:
            for key in type6_keys:
                if key not in list(config_from_json.keys()):
                    return False

        return True

    def print_config(self, detailed=False):
        if detailed:
            config_json_dict = json.loads(self.config_json)
            for key in list(config_json_dict.keys()):
                val = config_json_dict[key]
                if type(val) is list:
                    print("%20s : " % (key))
                    for elem in val:
                        print('%20s : %s' % ("", elem))
                else:
                    print("%20s : %s" % (key, config_json_dict[key]))
        else:
            print("CONFIGID:    " + str(self.configid))
            print("SHORTCODE:   " + self.shortcode)
            print("EXECLOGIC:   " + self.execlogic)
            print("START_TIME:  " + self.start_time)
            print("END_TIME:    " + self.end_time)
            print("STRAT_TYPE:  " + self.strat_type)
            if self.event_token:
                print("EVENT_TOKEN: " + self.event_token)
            print("QUERY_ID:    " + str(self.query_id))
            print("CONFIG_TYPE: " + str(self.config_type))
            print("CONFIG_JSON: " + self.config_json)

            if self.pooltag:
                print("POOLTAG: " + self.pooltag)
            if self.expect0vol:
                print("EXPECT_0VOL: " + str(self.expect0vol))

    def print_model(self):
        config_from_json = json.loads(self.config_json)
        model_list = config_from_json["model_list"]
        if len(model_list) != 0:
            cmds = ['echo ' + x + ' ;' + 'cat ' + x for x in model_list]
            list(map(lambda x: os.system(x), cmds))
        else:
            print('Model list for config is empty.')

    def print_param(self):
        config_from_json = json.loads(self.config_json)
        param_list = config_from_json["param_list"]
        if len(param_list) != 0:
            cmds = ['echo ' + x + ' ;' + 'cat ' + x for x in param_list]
            list(map(lambda x: os.system(x), cmds))
        else:
            print('Param list for config is empty.')

    def update_config_from_its_json(self):
        config_from_json = json.loads(self.config_json)
        if 'cname' in config_from_json:
            self.add_cname(config_from_json["cname"])
        self.shortcode = config_from_json["shortcode"]
        self.execlogic = config_from_json["execlogic"]
        self.start_time = config_from_json["start_time"]
        self.end_time = config_from_json["end_time"]
        self.strat_type = config_from_json["strat_type"]
        self.query_id = int(config_from_json["query_id"])
        self.config_type = int(config_from_json["config_type"])

        self.simula_approved = 0 if not self.simula_approved else self.simula_approved
        self.type = 'S' if not self.type else self.type
        self.pooltag = '' if not self.pooltag else self.pooltag
        self.expect0vol = 0 if not self.expect0vol else self.expect0vol
        self.event_token = config_from_json["event_token"] if 'event_token' in config_from_json else self.event_token

        if not self.description:
            self.description = "This config is created on " + datetime.date.today().isoformat()

        if 'is_structured' in config_from_json:
            self.is_structured = config_from_json['is_structured']

    def model_list(self):
        config_from_json = json.loads(self.config_json)
        return config_from_json["model_list"]

    def param_list(self):
        config_from_json = json.loads(self.config_json)
        return config_from_json["param_list"]

    def get_json_value_for_key(self, key):
        config_from_json = json.loads(self.config_json)
        if key in config_from_json:
            return config_from_json[key]
        return None
