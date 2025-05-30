#!/usr/bin/env python

"""

Script that generates predictive and contemporary correlation for: shortcode, start_time, end_time, portfolios, num_days, session, base_directory
Script has additional optimization ability of using an existing pickle to minimize load on datagen queries and speed gain


Pseudo Code:
------------

1.input time_start time_end portfolios
2.roll through the input portfolios to see if they are valid
3.drop portfolios that are not valid

4.assemble port indicators into ilist and call datagen for past 30 days (default) from today to dump tmp data into tmp directory
5. Add self trend to get to contemporaneous correlations
6. Check older pickle to find dates that are new for datagen call
7. Call datagen on new dates
8. assemble matrix where 1st column is delta-y and last column is selftrend, rest all are trend of portfolion/indep shortcodes
9. print and return correlation matrices for last 10 days mid 10 days and away 10 days, for both predictive and contemporaenous and store in json

Run for three sessions to store data as jsons that are picked up by a perl aggregator and finally by javascript, use
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
import sys

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils import get_trading_days_for_shortcode

home = expanduser("~")


def usage():
    print(
        'script -t "<time_start> <time_end>" -p "<portfolios>" eg. script -t "EST_700 EST_1000" -p "CUBFUT UEQUI" -P 0 ')


def pad(x):
    """ pad a month or date for two character convenience"""
    tmp = str(x)
    return tmp.zfill(2)


def create_directory(directory):
    """if directory does not exist create it"""
    if not (os.path.exists(directory)):
        print(("Created directory " + directory))
        os.makedirs(directory)


def remove_directory(directory):
    """delete directory from file system"""
    print(("Removing directory " + directory))
    shutil.rmtree(directory)


def indicatorsToData(shortcode, session, ilist, start_time, end_time, num_days, base_directory, pickle):
    """given a python list of indicators, create ilist and call datagen, return a pandas dataframe of the data - standard datagen sampling is used"""
    random_directory = base_directory + '/correlation_analysis/data/globalmacro_' + ''.join(
        random.choice(string.ascii_uppercase + string.digits) for _ in range(6))
    create_directory(random_directory)
    first_line_ilist = 'MODELINIT DEPBASE ' + shortcode + ' MktSizeWPrice MktSizeWPrice'
    header = [first_line_ilist, 'MODELMATH LINEAR CHANGE', 'INDICATORSTART']
    fname = random_directory + '/ilist_tmp'
    f = open(fname, 'w')
    f.write("\n".join(header + ilist + ["INDICATOREND"]))  # python will convert \n to os.linesep
    f.close()
    filter_ = "fsg0.5"
    today_ = datetime.datetime.today().strftime('%Y%m%d')

    # read pickle work to determine if the data we need already exists
    if pickle == 1:
        principal_dates = get_trading_days_for_shortcode.get_list_of_dates_for_shortcode(shortcode, today_, num_days)
        pickle_name = base_directory + '/correlation_analysis/pickles/' + shortcode + '_' + session
        dataframe = pd.read_pickle(pickle_name)

        all_pickle_dates = dataframe.iloc[:, -1].unique()

        print("dates from pickle")
        print(all_pickle_dates)

        # determine dates that need to be queried (if daily run this should be only one day)
        all_new_dates = [x for x in principal_dates if x not in all_pickle_dates]
        print("new dates on top of pickle")
        print(all_new_dates)

        list_of_dates = "'" + " ".join([str(date) for date in all_new_dates]) + "'"

        if len(all_new_dates) != 0:
            cmd = home + '/basetrade/scripts/get_regdata.py ' + shortcode + ' ' + fname + ' ' + today_ + ' ' + str(len(
                all_new_dates)) + ' ' + start_time + ' ' + end_time + ' 1000 0 0 0 300 na_t3 ' + filter_ + ' ' + random_directory + '/ 0 ' + list_of_dates
            print(cmd)
            os.system(cmd)

            try:
                new_dataframe = pd.read_csv(random_directory + "/catted_regdata_outfile_withdates", sep=" ", header=-1)
                new_dataframe.columns = dataframe.columns
                final_dataframe = dataframe.append(new_dataframe)

                final_dataframe['seq'] = list(range(0, final_dataframe.shape[0]))
                final_dataframe = final_dataframe.sort(['Date', 'seq'], ascending=True)
                del final_dataframe['seq']

                final_dates = final_dataframe.iloc[:, -1].unique()

                print("final dates from appended file")
                print(final_dates)

            except:
                print(
                    "Either could not read catted datagen output file, check ilist or time period, or if you are trying to append an older pickle file, the formats may be inconsistent. This may be because of a changed portfolio set.")
                print(
                    "Returning older pickle, please check if this is right - it could be that datagen does not find today's data (datacopy not done?) or ilist is bad !!!")
                return {'dataframe': dataframe, 'output_directory': random_directory}
        else:
            final_dataframe = dataframe
    else:  # else for pickle
        cmd = home + '/basetrade/scripts/get_regdata.py ' + shortcode + ' ' + fname + ' ' + today_ + ' ' + num_days + \
            ' ' + start_time + ' ' + end_time + ' 1000 0 0 0 300 na_e3 ' + filter_ + ' ' + random_directory + '/'
        print(cmd)
        os.system(cmd)
        try:
            final_dataframe = pd.read_csv(random_directory + "/catted_regdata_outfile_withdates", sep=" ", header=-1)
        except:
            print("Could not read catted datagen output file, check ilist or time period.")
            return {'dataframe': None, 'output_directory': random_directory}

    remove_directory(random_directory)
    return {'dataframe': final_dataframe, 'output_directory': random_directory}


def returnExistingPortfolios(portfolios):
    """in the set of portfolios, if there is a portfolio that is not existent or miss-spelt, reject it. For shortcodes, return as is."""
    accepted_portfolios = []
    accepted_portfolios.extend([x for x in portfolios if len(x.split("_")) != 0])

    with open('/spare/local/tradeinfo/PCAInfo/portfolio_inputs', 'rU') as f:
        for line in f:
            accepted_portfolios.append(line.split(" ")[1])

    filtered_portfolios = [x for x in portfolios if x in accepted_portfolios]
    print(filtered_portfolios)
    return filtered_portfolios


def generatePortIndicator(portfolio):
    """for a given portfolio, shortcode or index(NSE) generate the correct indicator"""
    if len(portfolio.split("_")) >= 2:
        if ((portfolio.split("_")[-1] == '0') | (portfolio.split("_")[-1] == '1')):
            return 'INDICATOR 1.00 SimpleTrend ' + portfolio + ' 300 Midprice'
        else:
            return 'INDICATOR 1.00 SimpleTrendPort ' + portfolio + ' 300 Midprice'
    else:
        return 'INDICATOR 1.00 SimpleTrendPort ' + portfolio + ' 300 Midprice'


def generatePortIndicator2(portfolio):
    """for a given portfolio, shortcode or index(NSE) generate the correct indicator"""
    all_legit_portfolios = []
    with open('/spare/local/tradeinfo/PCAInfo/portfolio_inputs', 'rU') as f:
        for line in f:
            all_legit_portfolios.append(line.split(" ")[1])

    if portfolio in all_legit_portfolios:
        return 'INDICATOR 1.00 SimpleTrendPort ' + portfolio + ' 300 Midprice'
    else:
        return 'INDICATOR 1.00 SimpleTrend ' + portfolio + ' 300 Midprice'


def new_corr(dataframe):
    """work around some pandas issue of having to cast as float before calling correlation on a dataframe"""
    dataframe2 = dataframe.astype(float)
    return dataframe2.corr()


def chunks(l, n):
    """Yield successive n-sized chunks from l."""
    for i in range(0, len(l), n):
        yield l[i:i + n]


def correlation(shortcode, start_time, end_time, portfolios, num_days, session, base_directory, pickle):
    """most important function that divides the data set equally into three, measures and outputs correlation on all three sets.
    We are measuring correlation for last 20 days, the 20 days before that and then again. Correlation stability inspection etc. is the goal."""

    # drop duplicates one-liner and keep order
    portfolios = pd.DataFrame({'portfolios': portfolios}).drop_duplicates()['portfolios'].tolist()

    print([generatePortIndicator2(x) for x in portfolios])
    ilist = [generatePortIndicator2(x) for x in portfolios]
    # ilist has only the indicators as a list

    # add the product itself for contemporaneous correlations
    ilist.append(generatePortIndicator2(shortcode))

    data_and_folder = indicatorsToData(shortcode, session, ilist, start_time=start_time, end_time=end_time,
                                       num_days=num_days, base_directory=base_directory, pickle=pickle)
    dataframe = data_and_folder['dataframe']
    output_folder = data_and_folder['output_directory']

    if dataframe is None:
        print("Could not get indicator data either because portfolio constituents or shortcode data is not available.")
        return None

    dataframe.columns = [shortcode] + portfolios + ['SelfTrend', 'Date']

    # dump pandas pickle
    pickle_name = base_directory + '/correlation_analysis/pickles/' + shortcode + '_' + session
    dataframe.to_pickle(pickle_name)

    all_dates = dataframe.iloc[:, -1].unique()

    print("dates")
    print(all_dates)

    most_recent_date = dataframe.iloc[:, -1].max()

    # indexer for recent and far term dates
    N = int(len(all_dates) / 3)

    far_end, mid_end, near_end = list(chunks(all_dates, N))[0:3]

    print(near_end)
    print(mid_end)
    print(far_end)

    # 1 is at near term, as regdata is increasing in date
    dates = dataframe.Date
    correlation_data3 = dataframe[dates.isin(far_end)]
    correlation_data2 = dataframe[dates.isin(mid_end)]
    correlation_data1 = dataframe[dates.isin(near_end)]

    del correlation_data3['Date']
    del correlation_data2['Date']
    del correlation_data1['Date']
    correlation_matrix3 = new_corr(correlation_data3)
    correlation_matrix2 = new_corr(correlation_data2)
    correlation_matrix1 = new_corr(correlation_data1)

    data_for_json = {'Shortcode': [shortcode for number in range(len(portfolios))],
                     'Session': [session for number in range(len(portfolios))], 'Portfolio.Shortcode': portfolios,
                     'NearEnd(0-10d)': correlation_matrix1.ix[0, 1:-1].tolist(),
                     'MidEnd(10-20d)': correlation_matrix2.ix[0, 1:-1].tolist(),
                     'FarEnd(20-30d)': correlation_matrix3.ix[0, 1:-1].tolist(),
                     'Last-Update': [most_recent_date for number in range(len(portfolios))]}

    data_for_json_pandas = pd.DataFrame(data_for_json)

    data_for_json_pandas = data_for_json_pandas[
        ['Session', 'Shortcode', 'Portfolio.Shortcode', 'NearEnd(0-10d)', 'MidEnd(10-20d)', 'FarEnd(20-30d)',
         'Last-Update']]
    data_for_json_pandas.index = list(map(lambda x, y, z: x + "_" + y + "_" + z, data_for_json_pandas['Session'].tolist(),
                                          data_for_json_pandas['Shortcode'].tolist(),
                                          data_for_json_pandas['Portfolio.Shortcode'].tolist()))

    correlation_json = data_for_json_pandas.to_json(orient='index', double_precision=2)

    json_outfile = base_directory + '/correlation_analysis/jsons/predictive_json_' + shortcode + '_' + session

    with open(json_outfile, 'w') as f:
        f.write(correlation_json)

    # now do the same analysis for contemporaneous trend, note last entry is for self trend hence -1 index on correlations
    data_for_json = {'Shortcode': [shortcode for number in range(len(portfolios))],
                     'Session': [session for number in range(len(portfolios))], 'Portfolio.Shortcode': portfolios,
                     'NearEnd(0-10d)': correlation_matrix1.ix[-1, 1:-1].tolist(),
                     'MidEnd(10-20d)': correlation_matrix2.ix[-1, 1:-1].tolist(),
                     'FarEnd(20-30d)': correlation_matrix3.ix[-1, 1:-1].tolist(),
                     'Last-Update': [most_recent_date for number in range(len(portfolios))]}

    data_for_json_pandas = pd.DataFrame(data_for_json)

    data_for_json_pandas = data_for_json_pandas[
        ['Session', 'Shortcode', 'Portfolio.Shortcode', 'NearEnd(0-10d)', 'MidEnd(10-20d)', 'FarEnd(20-30d)', 'Last-Update']]
    data_for_json_pandas.index = list(map(lambda x, y, z: x + "_" + y + "_" + z, data_for_json_pandas['Session'].tolist(),
                                          data_for_json_pandas['Shortcode'].tolist(),
                                          data_for_json_pandas['Portfolio.Shortcode'].tolist()))

    correlation_json = data_for_json_pandas.to_json(orient='index', double_precision=2)

    json_outfile = base_directory + '/correlation_analysis/jsons/contemporaneous_json_' + shortcode + '_' + session

    with open(json_outfile, 'w') as f:
        f.write(correlation_json)
    return "Success!"


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', dest='shortcode', help="CGB_0", type=str)
    parser.add_argument('-t', dest='timeperiod', help="e.g EST_700 EST_1530", type=str)
    parser.add_argument('-p', dest='portfolios', help="CUBFUT UBFUT2S", type=str)
    parser.add_argument('-n', dest='num_days', help="num days to compute correlation > 5", type=str)
    parser.add_argument('-S', dest='session', help="US EU or AS", type=str)
    parser.add_argument('-D', dest='base_directory',
                        help="this is the path of the globalMacro folder, including itself", type=str)
    # if portfolios are changed ever, this pickle stuff has to be set to 0. Pickle functionality is assuming nothing changes.
    parser.add_argument('-P', dest='pickle',
                        help="if 1 look for pickle file in pickles folder use the pickle to reduce datagen load",
                        type=int)

    args = parser.parse_args()
    shortcode = args.shortcode
    start_time, end_time = args.timeperiod.split("-")
    accepted_portfolios = args.portfolios.split(" ")
    base_directory = args.base_directory
    if base_directory is None:
        base_directory = '/media/shared/ephemeral0/globalMacro/'

    if args.pickle is None:
        pickle = 0
    else:
        pickle = args.pickle

    num_days = args.num_days
    session = args.session
    print((correlation(shortcode, start_time, end_time, accepted_portfolios, num_days, session, base_directory, pickle)))
