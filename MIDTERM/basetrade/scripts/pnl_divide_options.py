#!/usr/bin/python

import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import re
import os
import uuid
import dateparser
import subprocess
from random import randint
from shutil import copyfile
import datetime as dt
import sys
import argparse

uid = uuid.uuid4()
uid_str = uid.urn[9:]
SEC_IN_YEARS = 3.170979e-08
MSEC_IN_YEARS = 3.170979e-11
SEC_IN_DAYS = 0.00001157407


def MakeModelForShortCode(model_file_, shc_, temp_model_file_):

    with open(model_file_) as myfile:
        content = myfile.readlines()

    shc_ = "NSE_" + shc_ + "_FUT0"
    content = [x for x in content if not x.startswith("NSE_") or shc_ in x]

    with open(temp_model_file_, 'w') as myfile:
        for line in content:
            myfile.write("%s" % line)


def GetAllShortCodesFromTradesFile(process_id_, date_):

    trade_file_ = "/spare/local/logs/tradelogs/trades." + date_ + "." + repr(process_id_)

    shc_list_ = []
    with open(trade_file_) as myfile:
        for line in myfile:
            if line.startswith("SIMRESULT"):
                words = line.split()
                if words[2] != "ALL" and words[2].startswith("NSE_"):
                    shc_list_.append(words[2])

    return shc_list_


def MakeStratFile(strat_file_, model_file_, start_time_="IST_920", end_time_="IST_1515", shc_=""):

    temp_strat_file_ = "/spare/local/usarraf/temp_options_strat_file_" + uid_str
    temp_model_file_ = "/spare/local/usarraf/temp_options_model_file_" + uid_str

    if not model_file_:
        with open(strat_file_) as myfile:
            words = myfile.read().split()
        model_file_ = words[2]

    if shc_:
        MakeModelForShortCode(model_file_, shc_, temp_model_file_)
    else:
        copyfile(model_file_, temp_model_file_)

    strat_string_ = "STRATEGYLINE OptionsTrading " + temp_model_file_ + \
        " kMethodBlackScholes 999  " + start_time_ + " " + end_time_ + "\n"
    with open(temp_strat_file_, 'w') as myfile:
        myfile.write(strat_string_)

    return temp_strat_file_


def silentremove(filename):
    try:
        os.remove(filename)
    except OSError:
        pass


def run_sim_strategy(strat_file_, process_id_, date_):

    trade_file_ = "/spare/local/logs/tradelogs/trades." + date_ + "." + repr(process_id_)
    silentremove(trade_file_)

    sim_cmd_args_ = ["/home/dvctrader/basetrade_install/bin/sim_strategy_options",
                     "SIM", strat_file_, repr(process_id_), date_]
    out_sim_ = subprocess.check_output(sim_cmd_args_)
    return out_sim_


def ComputeIVData(process_id_, date_):

    trade_file_ = "/spare/local/logs/tradelogs/trades." + date_ + "." + repr(process_id_)

    trades = pd.read_csv(trade_file_, header=None, delim_whitespace=True, error_bad_lines=False, warn_bad_lines=False)

    try:
        trades = trades[[trades.columns[0], trades.columns[2], trades.columns[3], trades.columns[4],
                         trades.columns[5], trades.columns[6], trades.columns[7], trades.columns[8], trades.columns[25]]]
    except:
        return pd.DataFrame(index=[], columns=['Pnl', 'DiffIV', 'Volume'])

    trades.columns = ['Timestamp', 'Shortcode', 'Side', 'Size', 'Price', 'Position', 'OpenPnl', 'TotalPnl', 'IV']

    lines = []
    with open(trade_file_) as myfile:
        for line in myfile:
            if line.startswith("SIMRESULT"):
                lines.append(line)

    regexp = re.compile(r'NSE_[A-Z]*_[C|P]0')
    results = pd.DataFrame([], columns=['Pnl', 'DiffIV', 'Volume'])

    i = 0
    for line in lines:
        words = line.split()
        if len(words) < 2:
            continue
        shc = words[2]
        if shc[:4] != "NSE_":
            continue
        pnl = float(words[3])
        df = trades[trades.Shortcode.str.contains(shc)]
        buy_trades = df[df.Side == "B"]
        sell_trades = df[df.Side == "S"]
        buy_volume = len(buy_trades.index)
        sell_volume = len(sell_trades.index)
        avg_buy_IV = 0
        avg_sell_IV = 0
        if regexp.search(shc) is not None and buy_volume != 0 and sell_volume != 0:
            avg_buy_IV = buy_trades["IV"].mean()
            avg_sell_IV = sell_trades["IV"].mean()
        diff_IV = avg_sell_IV - avg_buy_IV
        vol = min(sell_volume, buy_volume)
        results.loc[shc] = [pnl, diff_IV, vol]
        i += 1

    return results


def dateFind(in_date):
    tmp = "-d@" + str(in_date)
    p = subprocess.Popen(['date', tmp], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    return dateparser.parse(out)


def GetOptionDetails(list_shc_, date_):
    option_details_ = pd.DataFrame([], columns=['Expiry', 'Type', 'Strike'])

    for shortcode_ in list_shc_:
        if "_FUT0" not in shortcode_:
            opt_type_ = "1"
            if "_P0_" in shortcode_:
                opt_type_ = "-1"
            opt_cmd_args_ = ["/home/dvctrader/basetrade_install/bin/option_details", shortcode_, date_]
            opt_ = subprocess.check_output(opt_cmd_args_)
            words_ = opt_.split()
            expiry_ = pd.to_datetime(words_[0], format='%Y%m%d') + timedelta(hours=10)
            option_details_.loc[shortcode_] = [expiry_, opt_type_, words_[1]]

    return option_details_


def CalculateGreeks(trade, opt_type_, strike_, expiry_):
    rate_ = "0.0675"
    fut_price_ = str(trade['FutPrice'])
    years_to_expiry_ = str(((expiry_ - trade['DateTime']).total_seconds()) * SEC_IN_YEARS)
    option_price_ = str((float(trade.BidPrice) + float(trade.AskPrice)) / 2)
    iv_cmd_args_ = ["/home/dvctrader/basetrade_install/bin/get_option_greeks", "--type", opt_type_, "--strike",
                    strike_, "--future", fut_price_, "--price", option_price_, "--years", years_to_expiry_, "--rate", rate_]
    iv_out_ = subprocess.check_output(iv_cmd_args_)
    words_out_ = iv_out_.split()
    return pd.Series([float(words_out_[3]), float(words_out_[5]), float(words_out_[7]), float(words_out_[9]), float(words_out_[11])])


def PopulateGreeks(trades_, opt_type_, strike_, expiry_):

    trades_['DateTime'] = trades_.index
    greeks_ = trades_.apply(CalculateGreeks, args=(opt_type_, strike_, expiry_), axis=1)
    greeks_.columns = ['Actual_IV', 'Actual_Delta', 'Actual_Gamma', 'Actual_Theta', 'Actual_Vega']
    return greeks_


def DividePnlForATradeFile(process_id_, date_, shc_):

    n2d_cmd_args_ = ["/home/dvctrader/basetrade_install/bin/get_numbers_to_dollars", "NSE_NIFTY_FUT0", date_]
    n2d_ = subprocess.check_output(n2d_cmd_args_)
    n2d_ = float(n2d_)

    trade_file_ = "/spare/local/logs/tradelogs/trades." + date_ + "." + repr(process_id_)
    total_pnl_ = pd.DataFrame([], columns=['Date', 'Shortcode', 'Pnl_Delta', 'Pnl_Gamma',
                                           'Pnl_Vega', 'Pnl_Theta', 'Pnl_Actual', 'Pnl_Explained', 'Pnl_Actual_Vega'])

    trades = pd.read_csv(trade_file_, header=None, delim_whitespace=True, error_bad_lines=False, warn_bad_lines=False)
    try:
        trades = trades[[trades.columns[0], trades.columns[2], trades.columns[3], trades.columns[5], trades.columns[6], trades.columns[10], trades.columns[11], trades.columns[13],
                         trades.columns[14], trades.columns[17], trades.columns[20], trades.columns[21], trades.columns[22], trades.columns[23], trades.columns[24], trades.columns[25]]]
    except:
        return total_pnl_

    trades.columns = ['Timestamp', 'Shortcode', 'Side', 'Price', 'Position', 'BidSize', 'BidPrice',
                      'AskPrice', 'AskSize', 'TotalPnl', 'Delta', 'Gamma', 'Vega', 'Theta', 'FutPrice', 'IV']

    trades = trades[(trades['Timestamp'] != "STATS:") & (trades['Timestamp'] != "SIMRESULT")]

    if shc_:
        trades = trades[trades['Shortcode'].str.contains(shc_)]

    trades[['Position', 'Timestamp']] = trades[['Position', 'Timestamp']].apply(pd.to_numeric)
    trades.index = list(map(dateFind, list(trades['Timestamp'])))

    list_shc_ = list(set(trades['Shortcode']))
    list_shc_ = [shortcode_.split('.', 1)[0] for shortcode_ in list_shc_]
    i = 0

    option_details_ = GetOptionDetails(list_shc_, date_)

    for shortcode_ in list_shc_:
        trades_shc_ = trades[trades.Shortcode.str.contains(shortcode_)]
        trades_diff_ = pd.DataFrame([], columns=['Pnl_Delta', 'Pnl_Gamma', 'Pnl_Vega',
                                                 'Pnl_Theta', 'Pnl_Actual', 'Pnl_Explained', 'Pnl_Actual_Vega'])
        if "FUT0" in shortcode_:
            trades_diff_['Pnl_Delta'] = (trades_shc_['Price'].shift(-1) - trades_shc_['Price']) * trades_shc_.Position
            trades_diff_['Pnl_Gamma'] = 0
            trades_diff_['Pnl_Vega'] = 0
            trades_diff_['Pnl_Theta'] = 0
            trades_diff_['Pnl_Actual'] = (trades_shc_['Price'].shift(-1) - trades_shc_['Price']) * trades_shc_.Position
            trades_diff_['Pnl_Explained'] = (trades_shc_['Price'].shift(-1) -
                                             trades_shc_['Price']) * trades_shc_.Position

        else:
            opt_type_ = option_details_.loc[shortcode_].Type
            strike_ = option_details_.loc[shortcode_].Strike
            expiry_ = option_details_.loc[shortcode_].Expiry
            greeks_ = PopulateGreeks(trades_shc_, opt_type_, strike_, expiry_)

            trades_diff_['Pnl_Delta'] = (trades_shc_['FutPrice'].shift(-1) -
                                         trades_shc_['FutPrice']) * greeks_.Actual_Delta * trades_shc_.Position
            trades_diff_['Pnl_Gamma'] = ((trades_shc_['FutPrice'].shift(-1) - trades_shc_['FutPrice'])
                                         ** 2) * greeks_.Actual_Gamma * trades_shc_.Position * 0.50
            trades_diff_['Pnl_Vega'] = (trades_shc_['IV'].shift(-1) - trades_shc_['IV']) * \
                greeks_.Actual_Vega * trades_shc_.Position
            trades_diff_['Pnl_Theta'] = (trades_shc_['Timestamp'].shift(-1) - trades_shc_['Timestamp']
                                         ) * greeks_.Actual_Theta * trades_shc_.Position * SEC_IN_YEARS
            trades_diff_['Pnl_Actual'] = (trades_shc_['Price'].shift(-1) - trades_shc_['Price']) * trades_shc_.Position
            trades_diff_['Pnl_Explained'] = trades_diff_['Pnl_Delta'] + \
                trades_diff_['Pnl_Gamma'] + trades_diff_['Pnl_Vega'] + trades_diff_['Pnl_Theta']
            trades_diff_['Pnl_Actual_Vega'] = (greeks_['Actual_IV'].shift(-1) -
                                               greeks_['Actual_IV']) * greeks_.Actual_Vega * trades_shc_.Position

        trades_diff_ = trades_diff_.fillna(0)
        total_pnl_.loc[i] = [date_, shortcode_, sum(trades_diff_['Pnl_Delta']) * n2d_, sum(trades_diff_['Pnl_Gamma']) * n2d_, sum(trades_diff_['Pnl_Vega']) * n2d_, sum(
            trades_diff_['Pnl_Theta']) * n2d_, sum(trades_diff_['Pnl_Actual']) * n2d_, sum(trades_diff_['Pnl_Explained']) * n2d_, sum(trades_diff_['Pnl_Actual_Vega']) * n2d_]
        i += 1

    return total_pnl_


def DividePnlAllTradesFile(process_id_, dates_, shc_):

    total_pnl_ = pd.DataFrame([], columns=['Date', 'Shortcode', 'Pnl_Delta', 'Pnl_Gamma',
                                           'Pnl_Vega', 'Pnl_Theta', 'Pnl_Actual', 'Pnl_Explained', 'Pnl_Actual_Vega'])

    for date_ in dates_:
        pnl_ = DividePnlForATradeFile(process_id_, date_, shc_)
        total_pnl_ = pd.concat([total_pnl_, pnl_])

    return total_pnl_


def GenerateResultsForAllDates(strat_file_, process_id_, dates_):
    results = pd.DataFrame(index=[], columns=['Pnl', 'DiffIV', 'Volume'])

    for date_ in dates_:
        if strat_file_:
            out_sim_ = run_sim_strategy(strat_file_, process_id_, date_)
        t_results = ComputeIVData(process_id_, date_)
        if len(results.index) == 0:
            results = t_results
        else:
            results['Pnl'] = t_results['Pnl'] + results['Pnl']
            results['DiffIV'] = (results['DiffIV'] * results['Volume'] + t_results['DiffIV'] *
                                 t_results['Volume']) / (results['Volume'] + t_results['Volume'])
            results['Volume'] = results['Volume'] + t_results['Volume']

    if strat_file_:
        with open(strat_file_) as myfile:
            words = myfile.read().split()
        model_file_ = words[2]

        silentremove(model_file_)
        silentremove(strat_file_)

    return results

# Get list of dates


def GetListofdates(end_date_, lookback_days_):
    date_cmd_args_ = ["/home/usarraf/basetrade_install/scripts/get_list_of_dates_for_shortcode.pl",
                      "NSE_NIFTY_FUT0", end_date_, lookback_days_]
    out_date_ = subprocess.check_output(date_cmd_args_)
    return out_date_.split()


strat_file_ = ""
model_file_ = ""
start_time_ = "IST_920"
end_time_ = "IST_1515"
shc_ = ""
process_id_ = ""

parser = argparse.ArgumentParser()
parser.add_argument('end_date', help='End date ')
parser.add_argument('lookback_days', help='Number of days to lookback')
parser.add_argument('output_path_prefix', help='The data will be dumped here with ivdata and pnldata added as suffix')
parser.add_argument('--model_file', help='Model file on which to run SIM (if needed)')
parser.add_argument('--strat_file', help='Strat file on which to run SIM (if needed and model file not provided) ')
parser.add_argument('--shc', help='If we need to run only on this specific shortcode ')
parser.add_argument('--start_time', help='Start time of trading ')
parser.add_argument('--end_time', help='End time of trading')
parser.add_argument('--process_id', help='In case this needs to be run only in trades file (No SIM runs)')

args = parser.parse_args()

if args.end_date:
    end_date_ = args.end_date
else:
    sys.exit('Please provide end date')

if args.lookback_days:
    lookback_days_ = args.lookback_days
else:
    sys.exit('Please provide number of days to lookback')

if args.output_path_prefix:
    output_path_prefix_ = args.output_path_prefix
else:
    sys.exit('Please provide output_path_prefix to dump data')

if args.model_file:
    model_file_ = args.model_file

if args.strat_file:
    strat_file_ = args.strat_file

if args.start_time:
    start_time_ = args.start_time

if args.end_time:
    end_time_ = args.end_time

if args.shc:
    shc_ = args.shc

if args.process_id:
    process_id_ = int(args.process_id)

temp_strat_file_ = ""
dates_ = GetListofdates(end_date_, lookback_days_)

if strat_file_ or model_file_:
    temp_strat_file_ = MakeStratFile(strat_file_, model_file_, start_time_, end_time_, shc_)
    process_id_ = randint(100, 999)
else:
    if not process_id_:
        sys.exit('Please provide process id in case strat or model file not provided')

iv_data_ = GenerateResultsForAllDates(temp_strat_file_, process_id_, dates_)
pnl_divide_ = DividePnlAllTradesFile(process_id_, dates_, shc_)

iv_data_.to_csv(output_path_prefix_ + "_ivdata", sep=' ', mode='w')

pnl_divide_.to_csv(output_path_prefix_ + "_pnldivide", index=None, sep=' ', mode='w')
