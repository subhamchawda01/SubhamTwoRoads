#!/usr/bin/env python


"""
Test script for ~/basetrade/walkforward/wf_db_utils/fetch_config_details.py
Test for fetch_config_name function will check if the fn returns correct name for the dummy config

"""


from walkforward.wf_db_utils.fetch_config_details import fetch_config_name


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


class TestFetchConfigDetails(unittest.TestCase):
    type3_cfg = config.initialize()

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

        (exists, configid) = dump_config_to_db(self.type3_cfg, True)
        self.assertEqual(exists, 1)
        self.type3_cfg.configid = configid

    def tearDown(self):
        # prune the config from db
        f_configid = fetch_config_id(self.type3_cfg.cname)
        if f_configid is not None:
            prune_config(self.type3_cfg.cname, False)

    def test_fetch_config_name(self):
        """
        for the dummy config created above, check if the fetch_config_name functions returns its config id, then assert if they are the same
        """
        f_configid = fetch_config_id(self.type3_cfg.cname)
        self.assertIsNotNone(f_configid)
        self.assertEqual(f_configid, self.type3_cfg.configid)
        config_name = fetch_config_name(f_configid)
        self.assertEqual(config_name, self.type3_cfg.cname)

    def test_fetch_paramid_from_paramname(self):
        """
        :return:
        """

        pfile = (self.type3_cfg.param_list())[0]
        npid = fetch_paramid_from_paramname(pfile, False, self.type3_cfg.configid)
        self.assertGreater(npid, 0)

    def test_fetch_paramname_from_paramid(self):
        """

        :return:
        """

        pfile = (self.type3_cfg.param_list())[0]
        npid = fetch_paramid_from_paramname(pfile, False, self.type3_cfg.configid)
        self.assertGreater(npid, 0)

        base_param_name = os.path.basename(pfile)
        expected_param_name = base_param_name + '_' + str(self.type3_cfg.configid)

        # fetch the param name from the recently inserted id
        new_param_name = fetch_paramname_from_paramid(paramid=npid)
        self.assertEqual(expected_param_name, new_param_name)
