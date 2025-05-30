#!/usr/bin/env python


"""
Test script for ~/basetrade/walkforward/wf_db_utils/dump_config_to_db.py


"""


import os
import sys
import json
import unittest

from walkforward.definitions.config import config

from walkforward.wf_db_utils.db_handles import set_backtest
from walkforward.wf_db_utils.dump_config_to_db import dump_config_to_db
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id
from walkforward.wf_db_utils.fetch_dump_model import fetch_modelid_from_modelname
from walkforward.wf_db_utils.fetch_dump_param import *
from walkforward.wf_db_utils.fetch_strat_from_config_struct_and_date import fetch_strat_from_config_struct_and_date

from walkforward.utils.process_config_utils import prune_config

from Tests.global_tests_utils.walkforward_test_utils import get_sample_wf_config


class DumpConfigToDBTests(unittest.TestCase):
    type3_cfg = config.initialize()
    type4_cfg = config.initialize()
    type5_cfg_p = config.initialize()  # multiple params
    type5_cfg_mp = config.initialize()  # multiple models and params
    type6_cfg = config.initialize()
    cfglist = []

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

        json_struct = get_sample_wf_config('type4_config')
        self.type4_cfg.config_json = json.dumps(json_struct)
        self.type4_cfg.update_config_from_its_json()
        self.type4_cfg.add_cname('type4_config')

        json_struct = get_sample_wf_config('type5_config')
        self.type5_cfg_p.config_json = json.dumps(json_struct)
        self.type5_cfg_p.update_config_from_its_json()
        self.type4_cfg.add_cname('type5_config_p')

        json_struct = get_sample_wf_config('type5_config1')
        self.type5_cfg_mp.config_json = json.dumps(json_struct)
        self.type5_cfg_mp.update_config_from_its_json()
        self.type4_cfg.add_cname('type5_config_mp')

        json_struct = get_sample_wf_config('type6_config')
        self.type6_cfg.config_json = json.dumps(json_struct)
        self.type6_cfg.update_config_from_its_json()
        self.type4_cfg.add_cname('type6_config')

        cfglist = [self.type3_cfg, self.type4_cfg, self.type5_cfg_p, self.type5_cfg_mp, self.type6_cfg]

    def tearDown(self):
        # prune the config from db
        for cfg in self.cfglist:
            config_id = fetch_config_id(cfg.cname)
            if config_id is not None:
                prune_config(cfg.cname, False)

    def test_dump_param(self):
        """
        :return:
        """

        (exists, configid) = dump_config_to_db(self.type3_cfg, True)
        self.assertEqual(exists, 1)
        self.assertIsNotNone(configid)
        self.assertGreater(configid, 0)
        self.type3_cfg.configid = configid

        pfile = (self.type3_cfg.param_list())[0]
        original_param_desc = open(pfile).read()

        base_param_name = os.path.basename(pfile)
        new_param_name = base_param_name + '_' + str(self.type3_cfg.configid)

        # fetch the param description from the param_name
        param_desc_from_name = fetch_param_desc_from_paramname(paramname=new_param_name)
        self.assertEqual(original_param_desc, param_desc_from_name)

        # fetch the param description from the configid
        param_desc_from_id = fetch_param_desc_from_configid(self.type3_cfg.configid)
        self.assertEqual(original_param_desc, param_desc_from_id)

    def test_dump_type3(self):
        """
        :return: 
        """

        (exists, configid) = dump_config_to_db(self.type3_cfg, True)
        self.assertEqual(exists, 1)
        self.assertIsNotNone(configid)
        self.assertGreater(configid, 0)
        self.type3_cfg.configid = configid

        f_configid = fetch_config_id(self.type3_cfg.cname)
        self.assertIsNotNone(f_configid)
        self.assertEqual(f_configid, configid)

        for pfile in self.type3_cfg.param_list():
            npid = fetch_paramid_from_paramname(pfile, False, configid)
            self.assertGreater(npid, 0)

        for mfile in self.type3_cfg.model_list():
            nmid = fetch_modelid_from_modelname(mfile, False, configid)
            self.assertGreater(nmid, 0)

        (shortcode, execlogic, modelname, paramname, start_time, end_time, strat_type,
         event_token, query_id) = fetch_strat_from_config_struct_and_date(self.type3_cfg, 19700101)
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

    def test_dump_type4(self):
        """
        :return:
        """

        (exists, configid) = dump_config_to_db(self.type4_cfg, True)
        self.assertEqual(exists, 1)
        self.assertIsNotNone(configid)
        self.assertGreater(configid, 0)
        self.type4_cfg.configid = configid

        f_configid = fetch_config_id(self.type4_cfg.cname)
        self.assertIsNotNone(f_configid)
        self.assertEqual(f_configid, configid)

        for pfile in self.type4_cfg.param_list():
            npid = fetch_paramid_from_paramname(pfile, False, configid)
            self.assertGreater(npid, 0)

        for mfile in self.type4_cfg.model_list():
            nmid = fetch_modelid_from_modelname(mfile, False, configid)
            self.assertGreater(nmid, 0)

    def test_dump_type5_p(self):
        """
        :return:
        """
        (exists, configid) = dump_config_to_db(self.type5_cfg_p, True)
        self.assertEqual(exists, 1)
        self.assertIsNotNone(configid)
        self.assertGreater(configid, 0)
        self.type5_cfg_p.configid = configid

        f_configid = fetch_config_id(self.type5_cfg_p.cname)
        self.assertIsNotNone(f_configid)
        self.assertEqual(f_configid, configid)

        for pfile in self.type5_cfg_p.param_list():
            npid = fetch_paramid_from_paramname(pfile, False, configid)
            self.assertGreater(npid, 0)

        for mfile in self.type5_cfg_p.model_list():
            nmid = fetch_modelid_from_modelname(mfile, False, configid)
            self.assertGreater(nmid, 0)

    def test_dump_type5_mp(self):
        """
        :return:
        """
        (exists, configid) = dump_config_to_db(self.type5_cfg_mp, True)
        self.assertEqual(exists, 1)
        self.assertIsNotNone(configid)
        self.assertGreater(configid, 0)
        self.type5_cfg_mp.configid = configid

        f_configid = fetch_config_id(self.type5_cfg_mp.cname)
        self.assertIsNotNone(f_configid)
        self.assertEqual(f_configid, configid)

        for pfile in self.type5_cfg_mp.param_list():
            npid = fetch_paramid_from_paramname(pfile, False, configid)
            self.assertGreater(npid, 0)

        for mfile in self.type5_cfg_mp.model_list():
            nmid = fetch_modelid_from_modelname(mfile, False, configid)
            self.assertGreater(nmid, 0)

    def test_dump_type6(self):
        """
        :return:
        """
        (exists, configid) = dump_config_to_db(self.type6_cfg, True)
        self.assertEqual(exists, 1)
        self.assertIsNotNone(configid)
        self.assertGreater(configid, 0)
        self.type6_cfg.configid = configid

        f_configid = fetch_config_id(self.type6_cfg.cname)
        self.assertIsNotNone(f_configid)
        self.assertEqual(f_configid, configid)

        for pfile in self.type6_cfg.param_list():
            npid = fetch_paramid_from_paramname(pfile, False, configid)
            self.assertGreater(npid, 0)

        for mfile in self.type6_cfg.model_list():
            nmid = fetch_modelid_from_modelname(mfile, False, configid)
            self.assertGreater(nmid, 0)

        config_json = json.loads(self.type6_cfg.config_json)

        (shortcode, execlogic, modelname, paramname, start_time, end_time, strat_type, event_token, query_id) = \
            fetch_strat_from_config_struct_and_date(self.type6_cfg, config_json['walk_start_date'])

        self.assertNotEqual(modelname, 'INVALID')
        self.assertNotEqual(paramname, 'INVALID')
        self.assertTrue(os.path.exists(modelname))
        self.assertTrue(os.path.exists(paramname))
