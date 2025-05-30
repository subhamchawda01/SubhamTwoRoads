#!/usr/bin/env python


"""
Test script for ~/basetrade/walkforward/wf_db_utils/fetch_strat_from_config_struct_and_date.py


"""

import os
import sys
import json
import unittest

from walkforward.definitions.config import config

from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.wf_db_utils.dump_config_to_db import dump_config_to_db
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id
from walkforward.wf_db_utils.fetch_dump_param import *

from walkforward.utils.process_config_utils import prune_config

from Tests.global_tests_utils.walkforward_test_utils import get_sample_wf_config

from walkforward.wf_db_utils.fetch_strat_from_config_and_date import fetch_strat_from_config_and_date
from walkforward.wf_db_utils.dump_strat_for_config_for_day import dump_strat_for_config_for_day
from walkforward.wf_db_utils.insert_model_into_db_wf_config_type6 import insert_model_coeffs_in_db
from Tests.global_tests_utils.get_test_data_full_path import get_test_data_full_path


class TestFetchStratFromConfigAndDate(unittest.TestCase):
    type3_cfg = config.initialize()
    type6_cfg = config.initialize()
    new_model_file = get_test_data_full_path("sample_model_4", "walkforward")

    cfglist = []

    def setUp(self):
        """
        initialize th config variables
        Set backtest to true

        :return: 
        """
        set_backtest(True)

        json_struct = get_sample_wf_config('type3_config')
        self.type3_cfg.config_json = json.dumps(json_struct)
        self.type3_cfg.update_config_from_its_json()
        self.type3_cfg.add_cname('type3_config')

        # intializing type 6 config with sample model 3 as model file which has 1 as its weight
        json_struct = get_sample_wf_config('type6_config_1')
        config_json = json.dumps(json_struct)
        self.type6_cfg.config_json = config_json
        self.type6_cfg.update_config_from_its_json()
        cname = 'type6_config_1'
        config_json_struct = json.loads(config_json)
        init_model = config_json_struct["model_list"][0]
        init_param = config_json_struct["param_list"][0]
        self.type6_cfg.add_cname(cname)
        tradingdate = 20170710

        cfglist = [self.type3_cfg, self.type6_cfg]

        for cfg in cfglist:
            (exists, configid) = dump_config_to_db(cfg, True)
            self.assertEqual(exists, 1)
            cfg.configid = configid

        # adding the same sample model 3 to 10july17 which is monday
        dump_strat_for_config_for_day(init_model, init_param, tradingdate, self.type6_cfg.cname)
        # changing model weight to sample model 4. Now if model is fetched after 10july17 we should be able to see this.
        insert_model_coeffs_in_db(init_model, self.new_model_file, self.type6_cfg.configid, tradingdate)

    def tearDown(self):
        # prune the config from db
        for cfg in self.cfglist:
            f_configid = fetch_config_id(cfg.cname)
            if f_configid is not None:
                prune_config(cfg.cname, False)

    def test_fetch_strat_from_config_and_date_type3(self):
        start_date = 20170501
        (shortcode, execlogic, modelname, paramname, start_time, end_time, strat_type,
         event_token, query_id) = fetch_strat_from_config_and_date("type3_config", start_date, 1)
        self.assertEqual(self.type3_cfg.shortcode, shortcode)
        self.assertEqual(self.type3_cfg.execlogic, execlogic)
        self.assertEqual(self.type3_cfg.start_time, start_time)
        self.assertEqual(self.type3_cfg.end_time, end_time)
        self.assertEqual(self.type3_cfg.strat_type, strat_type)
        self.assertEqual(self.type3_cfg.event_token, event_token)
        self.assertNotEqual(modelname, 'INVALID')
        self.assertNotEqual(paramname, 'INVALID')
        self.assertTrue(os.path.exists(modelname))
        self.assertTrue(os.path.exists(paramname))

    def test_fetch_strat_from_config_and_date_type6(self):
        tradingdate = 20170711
        # fetching model for 11july17 should give us sample model 4 weights.
        (shortcode, execlogic, modelname, paramname, start_time, end_time, strat_type, event_token,
         query_id) = fetch_strat_from_config_and_date("type6_config_1", tradingdate, 1)
        config_json_struct = json.loads(self.type6_cfg.config_json)
        self.assertEqual(self.type6_cfg.shortcode, shortcode)
        self.assertEqual(self.type6_cfg.execlogic, execlogic)
        self.assertEqual(self.type6_cfg.start_time, start_time)
        self.assertEqual(self.type6_cfg.end_time, end_time)
        self.assertEqual(self.type6_cfg.strat_type, strat_type)
        self.assertEqual(self.type6_cfg.event_token, event_token)

        # checking if what we got was actually sample model 4 weights my matching the contents of the files exactly.
        # we can't match the paths of files as different path is stored inside databases
        with open(self.new_model_file, 'r') as myfile:
            data = myfile.read()

        with open(modelname, 'r') as testfile:
            testdata = testfile.read()

        self.assertTrue(data, testdata)
