#!/usr/bin/env python


"""

"""
import os
import sys
import argparse
import datetime
import subprocess

sys.path.append(os.path.expanduser('~/basetrade'))

from walkforward.definitions.execs import execs, paths
from walkforward.utils.date_utils import days_between_dates


def compute_yields(shortcode, tradingdate, coupon, frequency, recompute):
    """

    :param shortcode:
    :param tradingdate:
    :return:
    """

    # get min-price increment, to compute prices 100 ticks up and down
    mpr_cmd = [execs().get_min_price_increment, shortcode, str(tradingdate)]
    min_price_increment = float(subprocess.check_output(mpr_cmd).decode('utf-8'))

    # exchange symbol cmd, to know the exact contract we want to place order
    exchange_symbol_cmd = [execs().get_exchange_symbol, shortcode, str(tradingdate)]
    exch_symbol = subprocess.check_output(exchange_symbol_cmd).decode('utf-8')

    # price-info, to get the max-min prices
    price_info_cmd = [execs().get_price_info_for_day, shortcode, str(tradingdate)]
    price_info_out = subprocess.check_output(price_info_cmd).decode('utf-8').splitlines()

    # compute the max and min prices for the given contract
    min_px = 0
    max_px = 0
    for line in price_info_out:
        if 'min_px_' in line:
            min_px = int(float(line.split()[-1])/min_price_increment)*min_price_increment
        elif 'max_px_' in line:
            max_px = int(float(line.split()[-1])/min_price_increment)*min_price_increment

    # taking prices 10 ticks up and down for consideration, just in case
    min_px -= 100*min_price_increment
    max_px += 100*min_price_increment

    # the files are saved in format of ZNM16, to differentiate between decades
    # on exchange it trades as ZNM6 for 06,16,26

    temporary_sym = exch_symbol
    temporary_sym = temporary_sym[:-1] + str(tradingdate)[2:3] + temporary_sym[-1]

    # get bloomberg csv data for each contract
    cheapest_to_deliver_filename = os.path.join(paths().hctd, temporary_sym + '.csv')

    if not os.path.exists(cheapest_to_deliver_filename):
        print('Cheapest to deliver data is not available for date', tradingdate, exch_symbol, file=sys.stderr)
        print('filename, ', cheapest_to_deliver_filename, file=sys.stderr)
        return

    cheapest_to_deliver = open(cheapest_to_deliver_filename).readlines()

    # format the date in a way that it matches with bloomberg file date format
    formatted_tradingdate = datetime.datetime.strptime(str(tradingdate), '%Y%m%d')
    formatted_tradingdate = formatted_tradingdate.strftime('%m/%d/%y')

    cheapest_to_deliver = [line for line in cheapest_to_deliver if formatted_tradingdate in line]
    if len(cheapest_to_deliver) <= 0:
        print('No entry for this day in file ', cheapest_to_deliver_filename, file=sys.stderr)
        return

    hctd_line = cheapest_to_deliver[0].split(',')
    #  03/21/12,110.09375,0.357,1.894,0.9263,T 1 ½ 12/31/13,101.98,1.791,1.02,0.587

    maturity_date = datetime.datetime.strptime(hctd_line[5].split(' ')[-1].replace('"', ''), '%m/%d/%y')

    days_to_maturity = days_between_dates(str(tradingdate), maturity_date.strftime('%Y%m%d') )

    # the directory to be written
    yield_path = os.path.join(paths().yield_data, str(tradingdate))
    if not os.path.exists(yield_path):
        os.makedirs(yield_path)

    # the file to be written
    this_contract_file_for_day = os.path.join(yield_path, shortcode)
    print(this_contract_file_for_day)

    if os.path.exists(this_contract_file_for_day) and os.stat(this_contract_file_for_day).st_size > 0:
        print('File already exists with values, returning..', file=sys.stderr)
        return
    else:
        write_file = open(this_contract_file_for_day, 'w')
        current_price = min_px
        while current_price <= max_px:
            # get yield command, its R script uses optim to find the best yield which matches the price
            yield_cmd = [execs().get_yield, str(current_price), str(days_to_maturity), str(coupon), str(frequency)]
            yield_val = subprocess.check_output(yield_cmd).decode('utf-8').split(' ')[-1]

            write_string = exch_symbol + ' ' + str(tradingdate) + ' ' + str(current_price) + ' ' \
                           + str(days_to_maturity) + ' ' + str(yield_val)

            # write the string to file
            write_file.write(write_string)
            # increment the price
            current_price += min_price_increment

        write_file.close()


# parser step
parser = argparse.ArgumentParser()
parser.add_argument('-d', dest='tradingdate', help="tradingdate to generate file for", type=int, required=True)
parser.add_argument('-c', dest='contract_name', help='contract name for which we want to generate the data', required=True)
parser.add_argument('-cp', dest='coupon', help='coupon used for this contract', type=float, required=True)
parser.add_argument('-f', dest='frequency', help='frequency of coupon delivery', type=int, default=2)
parser.add_argument('-r', dest='recompute', help='recompute the values', type=int, default=0)

args = parser.parse_args()

compute_yields(args.contract_name, args.tradingdate, args.coupon, args.frequency, args.recompute)
