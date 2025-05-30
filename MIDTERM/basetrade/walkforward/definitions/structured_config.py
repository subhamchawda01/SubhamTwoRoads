#!/usr/bin/env python

"""
Variation of config for structured strategies. Extends it

Different utils for structured strategies
"""


import os
import sys
import json

from walkforward.definitions.config import config
from walkforward.definitions.defines import structured_keys


class StructuredConfig(config):
    # maintain vector of sub configs

    @classmethod
    def initialize(cls):
        cfg = cls('INVALID', 'INVALID', 'INVALID', 'INVALID', -1, 'INVALID', -1,
                  'INVALID', 'INVALID', 0, 0, 'INVALID')
        return cfg

    def __init__(self, shortcode, execlogic, start_time, end_time, strat_type, event_token,
                 query_id, config_type, config_json, simula_approved, type, description):
        # initialize the parent
        self.config_vector = []
        config.__init__(self, -1, shortcode, execlogic, start_time, end_time, strat_type,
                        event_token, query_id, config_type, config_json)

        self.simula_approved = simula_approved
        self.type = type
        self.description = description
        # json_struct=json.loads(self.config_json)
        self.is_structured = 1
        # if 'is_structured' in json_struct:
        #    self.is_structured = int(json_struct['is_structured'])

    def is_valid_config(self, check_configid=True):
        """
        Checking for extra params here, rest of them are not required
        :param check_configid: 
        :return: 
        """
        json_struct = json.loads(self.config_json)
        if 'sub_param_list' not in json_struct:
            sys.stderr.write('sub_param_list not specified..')
            return False

        if 'sub_model_list' not in json_struct:
            sys.stderr.write('sub_model_list not specified')
            return False

        if 'sub_strat_list' not in json_struct:
            sys.stderr.write('sub_strat_list not specified')
            return False

        if 'shortcode_list' not in json_struct:
            sys.stderr.write('shortcode_list not specified')
            return False

        if len(json_struct['sub_model_list']) != len(json_struct['sub_param_list']) \
                != len(json_struct['sub_strat_list']) != len(json_struct['shortcode_list']) != len(self.config_vector):
            # in case number of strat, model, param triplets are not same
            sys.stderr.write('LENGTHS are not same', str(len(json_struct['sub_model_list'])),
                             str(len(json_struct['sub_param_list'])), str(len(json_struct['sub_strat_list'])),
                             str(len(json_struct['shortcode_list'])))
            return False

        return super().is_valid_config(check_configid)

    def update_config_from_its_json(self):
        """
        Update the config fields from its json string
        :return: 
        """
        config_from_json = json.loads(self.config_json)

        # keeping these different than model_list as it would avoid any conflicts with common_param usage
        model_list = config_from_json['sub_model_list']
        param_list = config_from_json['sub_param_list']
        shortcode_list = config_from_json['shortcode_list']

        strategy_list = config_from_json['sub_strat_list']

        local_json = self.config_json
        if len(model_list) != len(param_list) != len(strategy_list):
            print(sys.stderr, 'Structured Strategy number of params, models and strategy_names are not same',
                  len(param_list), len(model_list), len(strategy_list))
            print('Exiting')
            exit(0)

        for index in range(len(model_list)):
            model = model_list[index]
            param = param_list[index]
            shortcode = shortcode_list[index]
            strategy = strategy_list[index]

            substrat_json_string = self.config_json
            substrat_json = json.loads(substrat_json_string)

            for key in structured_keys:
                substrat_json.pop(key)

            substrat_json['shortcode'] = shortcode
            substrat_json['model_list'] = [model]
            substrat_json['param_list'] = [param]
            substrat_json['cname'] = [strategy]
            substrat_json['sname'] = [strategy]
            # update the rest of the conents, if they are different

            local_config = config.initialize()
            local_config.config_json = json.dumps(substrat_json)
            local_config.add_cname(strategy)

            local_config.is_structured = 2
            local_config.update_config_from_its_json()

            # push it to list of vectors
            self.config_vector.append(local_config)

        # child class updates the rest of the fields
        super().update_config_from_its_json()
