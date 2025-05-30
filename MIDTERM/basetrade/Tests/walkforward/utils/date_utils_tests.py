#!/usr/bin/env python

"""
Writing tests for datetime in python, comparing it with calc_prev_day execs
"""

import os
import sys
import time
import unittest
import datetime
import subprocess

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs


from walkforward.utils.date_utils import calc_next_day
from walkforward.utils.date_utils import calc_prev_day

from walkforward.utils.date_utils import calc_next_week_day
from walkforward.utils.date_utils import calc_prev_week_day


class TestDateMethods(unittest.TestCase):

    def test_calc_prev_week_day(self):
        # take today's date
        tradingdate = datetime.date.today().strftime('%Y%m%d')
        lookback = 100

        # call from the boost c++ code
        start_date_cmd = [execs.execs().calc_prev_week_day, str(tradingdate), str(lookback)]
        out = subprocess.Popen(' '.join(start_date_cmd), shell=True, stdout=subprocess.PIPE)
        date_from_boost = int(out.communicate()[0].strip())

        date_from_utils = calc_prev_week_day(tradingdate, lookback)

        self.assertEqual(date_from_boost, date_from_utils)

    def test_calc_prev_day(self):
        tradingdate = datetime.date.today().strftime('%Y%m%d')
        lookback = 100
        # call from the boost c++ code
        start_date_cmd = [execs.execs().calc_prev_day, str(tradingdate), str(lookback)]
        out = subprocess.Popen(' '.join(start_date_cmd), shell=True, stdout=subprocess.PIPE)
        date_from_boost = int(out.communicate()[0].strip())

        date_from_utils = calc_prev_day(tradingdate, lookback)

        self.assertEqual(date_from_boost, date_from_utils)

    def test_calc_next_week_day(self):
        tradingdate = datetime.date.today().strftime('%Y%m%d')
        lookback = 100

        # call from the boost c++ code
        start_date_cmd = [execs.execs().calc_next_week_day, str(tradingdate), str(lookback)]
        out = subprocess.Popen(' '.join(start_date_cmd), shell=True, stdout=subprocess.PIPE)
        date_from_boost = int(out.communicate()[0].strip())

        date_from_utils = calc_next_week_day(tradingdate, lookback)

        self.assertEqual(date_from_boost, date_from_utils)

    def test_calc_next_day(self):
        tradingdate = datetime.date.today().strftime('%Y%m%d')
        lookback = 100

        # call from the boost c++ code
        start_date_cmd = [execs.execs().calc_next_day, str(tradingdate), str(lookback)]
        out = subprocess.Popen(' '.join(start_date_cmd), shell=True, stdout=subprocess.PIPE)
        date_from_boost = int(out.communicate()[0].strip())

        date_from_utils = calc_next_day(tradingdate, lookback)

        self.assertEqual(date_from_boost, date_from_utils)


if __name__ == "__main__":
    unittest.main()
