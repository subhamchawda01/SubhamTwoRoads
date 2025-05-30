#!/usr/bin/env python

"""

Provides utility functions for walkforward tests

"""

import os
import json

from Tests.global_tests_utils.get_test_data_full_path import get_test_data_full_path


def get_sample_wf_config(cname):
    """
    Get a sample config from repo (location: ~/basetrade/Tests/data/walkforward,

    :return: config_json

    """

    data_folder = 'walkforward'
    full_path = get_test_data_full_path(cname, data_folder)

    config_json = {}
    with open(full_path) as filehandle:
        config_json = json.load(filehandle)
        if 'param_list' in config_json:
            config_json['param_list'] = [get_test_data_full_path(os.path.basename(param), data_folder)
                                         for param in config_json['param_list']]
        if 'model_list' in config_json:
            config_json['model_list'] = [get_test_data_full_path(os.path.basename(model), data_folder)
                                         for model in config_json['model_list']]
        if 'sub_model_list' in config_json:
            config_json['sub_model_list'] = [get_test_data_full_path(os.path.basename(model), data_folder)
                                             for model in config_json['sub_model_list']]
        if 'sub_param_list' in config_json:
            config_json['sub_param_list'] = [get_test_data_full_path(os.path.basename(param), data_folder)
                                             for param in config_json['sub_param_list']]

    return config_json

