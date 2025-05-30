#!/usr/bin/env python

"""

For a given shortcode, runs sim_strategy and checks with the older results

"""
import os
import datetime
import unittest
import subprocess
import logging
import sys
import io

from walkforward.definitions.execs import execs
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.wf_db_utils.fetch_strat_from_config_and_date import fetch_strat_from_config_and_date

from Tests.global_tests_utils.get_strats_for_shortcode import get_strats_for_shortcode


class ResultsTests(unittest.TestCase):

    shortcode = ""
    stable_sim_strategy_exec = ""

    def __init__(self, method_name, shortcode, stable_sim_strategy_exec):
        """
        # given the unittesting doesn't allow directly passing an arguemnt to test
        # creating separate test class for each shortcode
        :param method_name:
        :param shortcode:

        """

        super(ResultsTests, self).__init__(method_name)
        self.shortcode = shortcode
        self.stable_sim_strategy_exec = stable_sim_strategy_exec

    def get_stratname_from_config_and_date(self, shortcode, strat, date):
        """
        For a given date and config, create temporary strat file and return the path for that

        :param strat: 
        :param date: 
        :return: 
        """

        save_stdout = sys.stdout
        sys.stdout = io.StringIO()

        (shortcode, execlogic, modelname, paramname, start_time, end_time, strat_type, event_token,
         query_id) = fetch_strat_from_config_and_date(strat, date, 1)

        sys.stdout = save_stdout

        filepath = execs().shared_ephemeral_fbpa + '/strat_' + strat
        if 'WORKDIR' in os.environ:
            filepath = os.path.join(os.environ.get('WORKDIR'), 'trash/strat_' + strat)

        strategy_string = ' '  # initialize with empty string

        if modelname != "IF" and paramname != "IF":
            if shortcode=="EQIALL":
                strategy_string = "PORT_STRATEGYLINE" + " " + shortcode + " " + execlogic + " " \
                              + modelname + " " + paramname + " " + start_time + " " + end_time + " " \
                              + str(query_id) + " " + event_token
            else:
                strategy_string = "STRATEGYLINE" + " " + shortcode + " " + execlogic + " " \
                                  + modelname + " " + paramname + " " + start_time + " " + end_time + " " \
                                  + str(query_id) + " " + event_token

        strat_file = open(filepath, 'w')
        strat_file.write(strategy_string)
        strat_file.close()

        return filepath

    def test_shortcode_results(self):
        """
        Given a shortcode, fetch the older results and compare them with the newly computed results
        Check if results pass maximum deviation criteria

        :return:
        """
        logger = logging.getLogger('globals')
        logger.debug("Test")

        self.test_one_shortcode_results(self.shortcode, self.stable_sim_strategy_exec)

    def test_one_shortcode_results(self, shortcode, stable_sim_strategy_exec):
        """
        Function testing results for one single shortcode. Gets called from different modules depending on the
        category of the shortcoder
        :param shortcode:
        :return:

        """
        yesterday = (datetime.datetime.today() - datetime.timedelta(days=1)).strftime('%Y%m%d')
        strat_list = get_strats_for_shortcode(shortcode)
        date = yesterday
        for strat in strat_list:
            num_days = 2
            new_sim_strategy_exec = execs().sim_strategy
            older_sim_strategy_exec = stable_sim_strategy_exec

            while num_days > 0:
                new_failed = False
                old_failed = False
                # compute results with new sim

                filepath = None
                try:
                    filepath = self.get_stratname_from_config_and_date(shortcode, strat, date)
                except:
                    pass

                if filepath is None:
                    num_days -= 1
                    date = calc_prev_week_day(date)
                    continue

                new_run_strat_cmd = [new_sim_strategy_exec, 'SIM', filepath, "1234", str(date), 'ADD_DBG_CODE', '-1']
                #new_run_strat_cmd = [execs().run_strat, strat, str(date), new_sim_strategy_exec]

                # new_results = subprocess.check_output(new_run_strat_cmd).splitlines()
                process = subprocess.Popen(' '.join(new_run_strat_cmd), shell=True,
                                           stderr=subprocess.PIPE, stdout=subprocess.PIPE)

                out, err = process.communicate()


                if out is not None:
                    out = out.decode('utf-8')
                new_results = out.splitlines()
                # get the sim-results output line

                new_results = [line for line in new_results if line.find('SIMRESULT') >= 0]
                if len(new_results) > 0:
                    new_results = new_results[0].split()
                else:
                  #  print("Couldn't compute the results. run_strat: COUT " + out + " CERR: " + err)
                    new_failed = True
                    new_results = 'SIMRESULT 0 0 0 0 0 0 '.split()

                # compute results with old sim
                old_run_strat_cmd = [older_sim_strategy_exec, 'SIM', filepath, "1234", str(date), 'ADD_DBG_CODE', '-1']
                #old_run_strat_cmd = [execs().run_strat, strat, str(date), older_sim_strategy_exec]


                # old_results = subprocess.check_output(old_run_strat_cmd).splitlines()
                process = subprocess.Popen(' '.join(old_run_strat_cmd), shell=True,
                                           stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                out, err = process.communicate()
                if out is not None:
                    out = out.decode('utf-8')
                old_results = out.splitlines()

                # get the sim-results output line
                old_results = [line for line in old_results if line.find('SIMRESULT') >= 0]

                if len(old_results) > 0:
                    old_results = old_results[0].split()
                else:
                   # print("Couldn't compute old results. run_strat: COUT " + out + " CERR: " + err)
                    old_failed = True
                    old_results = 'SIMRESULT 0 0 0 0 0'.split()

                if not new_failed or not old_failed:
                    # either of them has failed, try matching
                    # if both of result computation failed then it would most likely because
                    # of some common error which we are not testing here
                    # match both results, the results should match exactly
                    self.match_sim_results(date, strat, old_results, new_results)
                elif new_failed and old_failed:
                    print("Both New and old result computation failed for strat : " +
                          strat + " on date : " + str(date) + ". Please check. Continuing...")

                os.remove(filepath)
                num_days -= 1
                date = calc_prev_week_day(date)

    def match_sim_results(self, date, strat, old_sim_results, new_sim_results):
        """
        # line are like thsi 
        # SIMRESULTS 100 10 0 0 0 0
        :param date:
        :param strat:
        :param old_sim_results:
        :param new_sim_results:
        :return:
        """

        # match the pnl string, giving space for 5% pnl change
        first = float(old_sim_results[1])
        second = float(new_sim_results[1])
        deltaval = 1  # max(abs(first), abs(second))/20
        pnl_error_string = 'PNL Mismatch: ' + self.shortcode + ' ' + str(date) + ' OLD ' + old_sim_results[1] +\
                           ' NEW ' + new_sim_results[1] + ' STRAT ' + strat
        self.assertAlmostEqual(first, second, places=1, msg=pnl_error_string)
        # somehow python is throwing error with unexpected argument delta
        # self.assertAlmostEqual(first, second, places=None, msg=pnl_error_string, delta=1)

        # match the volume
        first = int(old_sim_results[2])
        second = int(new_sim_results[2])
        volume_error_string = 'VOL Mismatch: ' + self.shortcode + ' ' + str(date) + ' OLD ' + old_sim_results[2] +\
                              ' NEW ' + new_sim_results[2] + ' STRAT ' + strat
        self.assertAlmostEqual(first, second, places=1, msg=volume_error_string)
        # somehow python is throwing error with unexpected argument delta
        # self.assertAlmostEqual(first, second, places=None, msg=volume_error_string, delta=1)
