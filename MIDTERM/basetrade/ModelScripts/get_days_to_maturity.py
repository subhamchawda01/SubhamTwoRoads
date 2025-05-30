#!/usr/bin/env python

"""

    For a given FI contract, it gives days to maturity,
    Currently the data is there as folloing :
    ZN  - 2003-2017
    ZB/ZF/ZT/UB 2013-2017


"""


import os
import sys
import argparse


sys.path.append(os.path.expanduser('~/basetrade'))

from walkforward.definitions.execs import paths


def get_days_to_maturity_for_contract(tradingdate, shortcode):
    """
    For a given FI contract, it gives days to maturity,
    Currently the data is there as folloing :
    ZN  - 2003-2017
    ZB/ZF/ZT/UB 2013-2017

    :param tradingdate:
    :param shortcode:
    :return: integer corresponding to number of days to maturity since given date
    """

    # the directory, this is generate by script
    # generate_daily_yield_files.py
    yield_path = os.path.join(paths().yield_data, str(tradingdate))

    # the file to be written
    this_contract_file_for_day = os.path.join(yield_path, shortcode)
    # print(this_contract_file_for_day)

    if not os.path.exists(this_contract_file_for_day):
        print('Could not find file for the day' + this_contract_file_for_day, file=sys.stderr)
        return 0

    file_lines = open(this_contract_file_for_day).readlines()

    # ZTM7 20170302 107.15720400000001 759 2.234181
    if len(file_lines) > 0:
        return int(file_lines[0].split()[3])


# parser step
parser = argparse.ArgumentParser()

parser.add_argument('-d', dest='tradingdate', help="tradingdate to generate file for", type=int, required=True)
parser.add_argument('-s', dest='shortcode', help='contract name for which we want to generate the data', required=True, type=str)

args = parser.parse_args()

print(get_days_to_maturity_for_contract(args.tradingdate, args.shortcode))