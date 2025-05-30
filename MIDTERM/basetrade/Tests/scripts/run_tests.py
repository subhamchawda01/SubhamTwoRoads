#!/usr/bin/env python

"""

Main file for running global tests
Following Set of tests are available
-t GLOBAL :
    This runs two sim_strategies and compares the results. Does the exact match and then exits if it fails
    
-t WF :
    Runs the wf tests which includes utility functions as well ass access to database functions.
    
-t ALL :
   Runs all of the above
   
"""

import os
import sys
import argparse
import unittest
import logging
import subprocess
import warnings

warnings.filterwarnings("ignore")

if os.environ.get('WORKDIR'):
    sys.path.append(os.environ['WORKDIR'])
else:
    sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions.execs import paths
from Tests.global_tests.results_test import ResultsTests
from Tests.walkforward.wf_db_utils.test_get_trading_days_for_shortcode import TestGetTradingDayForShortcode
from Tests.walkforward.wf_db_utils.test_fetch_config_details import TestFetchConfigDetails
from Tests.walkforward.wf_db_utils.test_fetch_structured_config_details import TestFetchStructuredConfigDetails
from Tests.walkforward.wf_db_utils.test_dump_config_to_db import DumpConfigToDBTests
from Tests.walkforward.wf_db_utils.test_fetch_latest_model_date import TestFetchLatestModelDate
from Tests.walkforward.wf_db_utils.test_fetch_strat_from_config_and_date import TestFetchStratFromConfigAndDate
from Tests.walkforward.wf_db_utils.test_fetch_strat_from_config_struct_and_date import TestFetchStratFromConfigStructAndDate

from Tests.walkforward.utils.test_get_continuous_and_random_dates import TestGetContinuousAndRandomDatesForShortcode

from Tests.global_tests_utils.get_test_data_full_path import get_test_data_full_path

# use python logger
logging.basicConfig(stream=sys.stderr)
logging.getLogger('GlobalLogger').setLevel(logging.DEBUG)


# argument parsing
parser = argparse.ArgumentParser()
dest_type_help_string = "type of test being done [GLOBAL/WF]"
stable_sim_exec_help_string = "path of sim strategy exec to be compared against"
exchange_help_string = "exchange to be tested (should be valid in some cases only)"
shortcode_help_string = "shortcode to be tested"

# none of these is being used here as of now, will add it in subsequent updates
parser.add_argument('-t', dest='test_type', help=dest_type_help_string, type=str, required=True)
parser.add_argument('-f', dest='stable_sim_exec', help=stable_sim_exec_help_string, type=str, required=True)
parser.add_argument('-e', dest='exchange', help=exchange_help_string, type=str, required=False)
parser.add_argument('-s', dest='shortcode', help=shortcode_help_string, type=str, required=False)

args = parser.parse_args()

# create a test suite, helps in running multiple tests together
suite = unittest.TestSuite()

if args.test_type == 'GLOBAL' or args.test_type == 'ALL':
    list_of_shortcode_exchanges = open(get_test_data_full_path('shortcodes_to_test.txt')).readlines()
    list_of_shortcode_exchanges = [shc.strip() for shc in list_of_shortcode_exchanges]
    list_of_shortcodes = [x.split()[0] for x in list_of_shortcode_exchanges]
    if args.shortcode is not None:
        list_of_shortcodes = [args.shortcode]
    else:
        if args.exchange is not None:
            list_of_shortcodes = [x.split()[0] for x in list_of_shortcode_exchanges if x.split()[1] == args.exchange]
            if len(list_of_shortcodes) == 0:
                print("Exchange provided in input is not present in the list mentioned in the file",
                      get_test_data_full_path('shortcodes_to_test.txt'), ". Please check.")

    if len(list_of_shortcodes) == 0:
        print("No shortcode available. Not runnning pnl test cases.")
    else:
        for shortcode in list_of_shortcodes:
            if shortcode=="MRT":
                shortcode="EQIALL"
                print("MRT strat")
            # adding just one test case to run, we can add further tests depending on the argument passed
            suite.addTest(ResultsTests('test_shortcode_results', shortcode, args.stable_sim_exec))


if args.test_type == 'WF' or args.test_type == 'ALL':
    suite.addTest(TestGetTradingDayForShortcode('test_get_list_of_dates_for_shortcode'))
    suite.addTest(TestGetTradingDayForShortcode('test_get_trading_days_for_shortcode'))

    suite.addTest(TestFetchConfigDetails('test_fetch_config_name'))
    suite.addTest(TestFetchConfigDetails('test_fetch_paramid_from_paramname'))
    suite.addTest(TestFetchConfigDetails('test_fetch_paramname_from_paramid'))

    suite.addTest(TestFetchStructuredConfigDetails('test_fetch_config_name'))
    suite.addTest(TestFetchStructuredConfigDetails('test_fetch_paramid_from_paramname'))
    suite.addTest(TestFetchStructuredConfigDetails('test_fetch_paramname_from_paramid'))

    suite.addTest(DumpConfigToDBTests('test_dump_type3'))
    suite.addTest(DumpConfigToDBTests('test_dump_type4'))
    suite.addTest(DumpConfigToDBTests('test_dump_type5_p'))
    suite.addTest(DumpConfigToDBTests('test_dump_type5_mp'))
    suite.addTest(DumpConfigToDBTests('test_dump_type6'))
    suite.addTest(DumpConfigToDBTests('test_dump_param'))

    suite.addTest(TestFetchLatestModelDate('test_fetch_latest_model_date'))
    suite.addTest(TestFetchStratFromConfigAndDate('test_fetch_strat_from_config_and_date_type3'))
    suite.addTest(TestFetchStratFromConfigAndDate('test_fetch_strat_from_config_and_date_type6'))
    suite.addTest(TestFetchStratFromConfigStructAndDate('test_fetch_strat_from_config_struct_and_date_type3'))
    suite.addTest(TestFetchStratFromConfigStructAndDate('test_fetch_strat_from_config_struct_and_date_type6'))
    suite.addTest(TestGetContinuousAndRandomDatesForShortcode('test_get_continuous_and_random_dates'))

# actually running it
runner = unittest.TextTestRunner()
return_code = not runner.run(suite).wasSuccessful()

# return exit code in case of failures
sys.exit(return_code)
