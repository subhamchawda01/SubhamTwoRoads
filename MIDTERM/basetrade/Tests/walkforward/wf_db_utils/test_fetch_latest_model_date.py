#!/usr/bin/env python

"""
Test script for ~/basetrade/walkforward/wf_db_utils/fetch_latest_model_date
"""

import unittest
import json

from walkforward.definitions.config import config
from Tests.global_tests_utils.walkforward_test_utils import get_sample_wf_config
from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id
from walkforward.utils.process_config_utils import prune_config
from walkforward.wf_db_utils.dump_config_to_db import dump_config_to_db
from walkforward.wf_db_utils.fetch_latest_model_date import fetch_latest_model_date
from walkforward.wf_db_utils.fetch_config_details import fetch_config_name


class TestFetchLatestModelDate(unittest.TestCase):
    type3_cfg = config.initialize()
    type4_cfg = config.initialize()
    type5_cfg_p = config.initialize()  # multiple params
    type5_cfg_mp = config.initialize()  # multiple models and params
    type6_cfg = config.initialize()

    def setUp(self):
        """
        initialize the config variables and dump them to DB
        Set backtest to true

        :return: 
        """
        set_backtest(True)

        json_struct = get_sample_wf_config('type3_config')
        self.type3_cfg.config_json = json.dumps(json_struct)
        self.type3_cfg.update_config_from_its_json()
        self.type3_cfg.add_cname('type3_config')

        json_struct = get_sample_wf_config('type6_config')
        self.type6_cfg.config_json = json.dumps(json_struct)
        self.type6_cfg.update_config_from_its_json()
        self.type4_cfg.add_cname('type6_config')

        self.cfglist = [self.type3_cfg, self.type6_cfg]

    def tearDown(self):
        # prune the config from db
        for cfg in self.cfglist:
            config_id = fetch_config_id(cfg.cname)
            if config_id is not None:
                prune_config(cfg.cname, False)

    def test_fetch_latest_model_date(self):
        """
        Testing basetrade/walkforward/wf_db_utils/fetch_latest_model_date.py
        Checking for type 3 and type 6
        """

        # checking for type 3 config, if fetched for date after it, date should be 19700101,  otherwise None
        (exists, configid) = dump_config_to_db(self.type3_cfg, True)
        cname = fetch_config_name(configid)
        latest_date = fetch_latest_model_date(cname, 20160101)
        self.assertEqual(latest_date, 19700101)
        latest_date = fetch_latest_model_date(cname, 19650101)
        self.assertIsNone(latest_date)

        # checking for type 6 config, if fetched for date after it, date should be walk_start_date, otherwise None
        (exists, configid) = dump_config_to_db(self.type6_cfg, True)
        cname = fetch_config_name(configid)
        latest_date = fetch_latest_model_date(cname, 20170701)
        self.assertEqual(latest_date, int((json.loads(self.type6_cfg.config_json))["walk_start_date"]))
        latest_date = fetch_latest_model_date(cname, 19650101)
        self.assertIsNone(latest_date)
