#!/usr/bin/env python


"""
Test script for ~/basetrade/walkforward/utils/get_continuous_and_random_dates.py


"""

import unittest

from walkforward.utils.get_continuous_and_random_dates import get_continuous_and_random_dates


class TestGetContinuousAndRandomDatesForShortcode(unittest.TestCase):

    def setUp(self):
        print("Setting UP")

    def tearDown(self):
        print("TearDown")

    def test_get_continuous_and_random_dates(self):
        """
        test if the script is able to return set of continuous and random dates correctly
        :return: 
        """

        shortcode = 'FGBM_0'
        end_date = 20170501
        lookback_continuous = 10
        lookback_random = 100
        num_days_random = 10
        skip_days = [20170103, 20170104]

        list_of_dates = get_continuous_and_random_dates(shortcode, end_date, lookback_continuous,
                                                        lookback_random, num_days_random, skip_days)
        list_of_dates = sorted(list_of_dates)

        # to compare with
        actual_dates = '20170428 20170427 20170426 20170425 20170424 20170421 20170420 20170419 ' \
                       '20170418'

        actual_dates = actual_dates.split()
        actual_dates = list(map(int, actual_dates))
        actual_dates = sorted(actual_dates)

        self.assertEqual(len(list_of_dates), 20)

        # assert if continuous days are there in output
        for date in actual_dates:
            self.assertTrue(date in list_of_dates)

        # assert if the skip days are not there in output
        for date in skip_days:
            self.assertFalse(date in list_of_dates)
