#!/usr/bin/env python


"""
Test script for ~/basetrade/walkforward/wf_db_utils/get_trading_days_for_shortcode.py


"""

import unittest

from walkforward.wf_db_utils.get_trading_days_for_shortcode import get_list_of_dates_for_shortcode
from walkforward.wf_db_utils.get_trading_days_for_shortcode import get_trading_days_for_shortcode


class TestGetTradingDayForShortcode(unittest.TestCase):

    def setUp(self):
        print("Setting UP")

    def tearDown(self):
        print("TearDown")

    def test_get_list_of_dates_for_shortcode(self):
        shortcode = 'FGBM_0'
        end_date = 20170501
        lookback = 100
        list_of_dates = get_list_of_dates_for_shortcode(shortcode, end_date, lookback)
        list_of_dates = sorted(list_of_dates)
        actual_dates = '20170428 20170427 20170426 20170425 20170424 20170421 20170420 20170419 ' \
                       '20170418 20170413 20170412 20170411 20170410 20170407 20170406 20170405 ' \
                       '20170404 20170403 20170331 20170330 20170329 20170328 20170327 20170324 ' \
                       '20170323 20170322 20170321 20170320 20170317 20170316 20170315 20170314 ' \
                       '20170313 20170310 20170309 20170308 20170307 20170306 20170303 20170302 ' \
                       '20170301 20170228 20170227 20170224 20170223 20170222 20170221 20170220 ' \
                       '20170217 20170216 20170215 20170214 20170213 20170210 20170209 20170208 ' \
                       '20170207 20170206 20170203 20170202 20170201 20170131 20170130 20170127 ' \
                       '20170126 20170125 20170124 20170123 20170120 20170119 20170118 20170117 ' \
                       '20170116 20170113 20170112 20170111 20170110 20170109 20170106 20170105 ' \
                       '20170104 20170103 20170102 20161230 20161229 20161228 20161227 20161223 ' \
                       '20161222 20161221 20161220 20161219 20161216 20161215 20161214 20161213 ' \
                       '20161212 20161209 20161208 20161207'
        actual_dates = actual_dates.split()
        actual_dates = list(map(int, actual_dates))
        actual_dates = sorted(actual_dates)

        self.assertEqual(len(list_of_dates), len(actual_dates))
        for i in range(len(list_of_dates)):
            self.assertEqual(int(list_of_dates[i]), int(actual_dates[i]))

    def test_get_trading_days_for_shortcode(self):
        shortcode = 'FGBM_0'
        end_date = 20170101
        num_days = 5
        lookback = 10
        list_of_dates = get_trading_days_for_shortcode(shortcode, num_days, end_date, lookback)
        list_of_dates = sorted(list_of_dates)

        actual_dates = '20161230 20161229 20161228 20161227 20161223 20161222 20161221 20161220 ' \
                       '20161219 20161216 20161215 20161214 20161213 20161212 20161209'

        actual_dates = actual_dates.split()
        actual_dates = list(map(int, actual_dates))
        actual_dates = sorted(actual_dates)

        print('LD', list_of_dates)
        print('AD', actual_dates)
        self.assertEqual(len(list_of_dates), len(actual_dates))
        for i in range(len(list_of_dates)):

            ld = int(list_of_dates[i])
            ad = int(actual_dates[i])
            self.assertEqual(ld, ad)
