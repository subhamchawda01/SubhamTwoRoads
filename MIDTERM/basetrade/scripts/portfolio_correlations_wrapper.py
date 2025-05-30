#!/usr/bin/env python

"""
Wrapper script to call portfolio_correlations.py over shortcodes and sessions - generate portfolio correlations by iterating through US AS and EU portfolios, please see portfolio_correlations.py for more details.
"""

import numpy as np
import pandas as pd
import argparse
import random
import string
import os
from os.path import expanduser
import datetime
import shutil

home = expanduser("~")


def usage():
    print('script -h')


def generateCall(shortcode, portfolio, time_slot, session, num_days, base_directory, pickle):
    return home + '/basetrade/scripts/portfolio_correlations.py -s ' + str(shortcode) + ' -t "' + str(
        time_slot) + '" -p "' + str(portfolio) + '" -n ' + str(num_days) + ' -S ' + str(session) + ' -D ' + str(
        base_directory) + ' -P ' + str(pickle)


def runCall(shortcode, portfolio, time_slot, session, num_days, base_directory, pickle):
    print("Doing this portfolio...")
    print((generateCall(shortcode, portfolio, time_slot, session, num_days, base_directory, pickle)))
    os.system(generateCall(shortcode, portfolio, time_slot, session, num_days, base_directory, pickle))
    return 0


def processSheet(inputSheet, session, num_days, base_directory, debug, pickle):
    products = pd.read_csv(inputSheet,
                           sep="|")
    shortcodes = products.ix[:, 0].tolist()
    portfolios = products.ix[:, 1].tolist()
    portfolios = [x.replace(" ", "") for x in portfolios]
    portfolios = [" ".join(x.split(',')) for x in portfolios]

    time_slots = products.ix[:, 2].tolist()
    sessions = [session for number in range(len(shortcodes))]
    num_days_sequence = [num_days for number in range(len(shortcodes))]
    base_directory_sequence = [base_directory for number in range(len(shortcodes))]
    pickle_sequence = [pickle for number in range(len(shortcodes))]

    date_ = datetime.datetime.now()
    jobs_file = base_directory + '/correlation_analysis/joblist/correlation_jobs_' + session
    all_jobs = list(map(generateCall, shortcodes, portfolios, time_slots, sessions, num_days_sequence,
                        base_directory_sequence, pickle_sequence))
    thefile = open(jobs_file, 'w')
    for item in all_jobs:
        thefile.write("%s\n" % item)
    if debug == 0:
        print('Running in real mode...')
        run_all_jobs = list(map(runCall, shortcodes, portfolios, time_slots, sessions, num_days_sequence,
                                base_directory_sequence, pickle_sequence))
    else:
        print('Running in debug mode...')
        print(all_jobs)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument('-n', dest='num_days', help="num days to compute correlation > 5", type=int)
    parser.add_argument('-S', dest='session', help="ALL US EU AS or IST", type=str)
    parser.add_argument('-debug', dest='debug', help="if 1 then only generate list of jobs", type=int)

    parser.add_argument('-D', dest='base_directory',
                        help="this is the path of the globalMacro folder, including itself", type=str)

    # if portfolios are changed ever, this pickle stuff has to be set to 0. Pickle functionality is assuming nothing changes.
    parser.add_argument('-P', dest='pickle',
                        help="if 1 look for pickle file in pickles folder use the pickle to reduce datagen load",
                        type=int)

    args = parser.parse_args()
    session = args.session
    num_days = args.num_days
    debug = args.debug
    base_directory = args.base_directory

    if base_directory is None:
        base_directory = '/media/shared/ephemeral0/globalMacro/'

    # default is 10 days
    if num_days is None:
        num_days = 10

    if args.pickle is None:
        pickle = 0
    else:
        pickle = args.pickle

    if session == "ALL":
        processSheet(base_directory + '/correlation_analysis/portfolios/portfolio_US_pandas', 'US', num_days,
                     base_directory, debug, pickle)
        processSheet(base_directory + '/correlation_analysis/portfolios/portfolio_EU_pandas', 'EU', num_days,
                     base_directory, debug, pickle)
        processSheet(base_directory + '/correlation_analysis/portfolios/portfolio_AS_pandas', 'AS', num_days,
                     base_directory, debug, pickle)
        processSheet(base_directory + '/correlation_analysis/portfolios/portfolio_IST_pandas', 'IST', num_days,
                     base_directory, debug, pickle)
    elif session == "US":
        processSheet(base_directory + '/correlation_analysis/portfolios/portfolio_US_pandas', 'US', num_days,
                     base_directory, debug, pickle)
    elif session == "EU":
        processSheet(base_directory + '/correlation_analysis/portfolios/portfolio_EU_pandas', 'EU', num_days,
                     base_directory, debug, pickle)
    elif session == "AS":
        processSheet(base_directory + '/correlation_analysis/portfolios/portfolio_AS_pandas', 'AS', num_days,
                     base_directory, debug, pickle)
    elif session == "IST":
        processSheet(base_directory + '/correlation_analysis/portfolios/portfolio_IST_pandas', 'IST', num_days,
                     base_directory, debug, pickle)
