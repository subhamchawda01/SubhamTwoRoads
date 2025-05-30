#!/usr/bin/env python

import os
import sys
import getpass
import subprocess
import time
import numpy as np

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.definitions import execs


def silentremove(filename):
    try:
        os.remove(filename)
    except OSError:
        pass


def generate_data(shortcode, ilist, param, date, num_days, start_time, end_time, msecs, l1, trades, eco, skip_days_file,
                  work_dir):
    """
    The function generates the data of indicators along with fill information. The last 10 columns have the fill information.
    The first column of the last 10 columns is the time when a buy order, placed currently would be executed in the future.
    The second column is the time when the buy order position would be closed out. The next two columns are the buy and sell prices
    to earn the pnl, the fifth column is the pnl in this process. The next five columns are similar for placing sell order 
    at the current timestamp.
    :param shortcode: shortcode to generate data
    :param ilist: initial ilist containing indicators to generate data
    :param date: end date of the block of days for which the data is to be generated
    :param num_days: number of days in the training data block
    :param start_time: start trading time
    :param end_time: end trading time
    :param msecs: msecs to sample as given in datagen
    :param l1: l1events to sample as given in datagen
    :param trades: trades to sample as given in datagen
    :param eco: eco mode to sample as given in datagen
    :param skip_days_file: skip days to avoid skipping data in datagen
    :param work_dir: working directory that stores all dated files with data
    """
    process_id_ = "1234"
    datagen_args = msecs + " " + l1 + " " + trades + " " + eco

    os.system("mkdir -p " + work_dir)

    dates_script = execs.execs().get_dates
    datagen_script = execs.execs().datagen
    contract_specs_script = execs.execs().get_contract_specs
    sim_strategy_script = execs.execs().sim_strategy

    dates_file = work_dir + "dates"
    # dumping all the dates to be generated data on, in dates_file
    os.system(dates_script + " " + shortcode + " " + num_days + " " + date + " > " + dates_file)

    # reading the dates from the dates_file
    f = open(dates_file, 'r')
    dates = f.read()
    f.close()
    dates = dates.split()
    dates.reverse()

    # creating a temporary model, param and strategy file to run sim_strategy. The sim_strategy will be run in FillTimeLogger
    # ExecLogic with DBG_CODE of FILL_TIME_INFO , that will help retrieve all fill based information
    temp_strat_file_ = work_dir + "/temp_strat_file"
    temp_model_file_ = work_dir + "/temp_model_file"
    temp_param_file_ = work_dir + "/temp_param_file"

    _model_ = "MODELINIT DEPBASE " + shortcode + " MktSizeWPrice MktSizeWPrice\n" \
                                                 "MODELMATH LINEAR CHANGE\n" \
                                                 "INDICATORSTART\n" \
                                                 "INDICATOREND"

    _param_ = "PARAMVALUE WORST_CASE_UNIT_RATIO 0\n" \
              "PARAMVALUE MAX_UNIT_RATIO 1000000\n" \
              "PARAMVALUE HIGHPOS_LIMITS_UNIT_RATIO 0\n" \
              "PARAMVALUE ZEROPOS_LIMITS_UNIT_RATIO 0\n" \
              "PARAMVALUE HIGHPOS_THRESH_FACTOR 0\n" \
              "PARAMVALUE HIGHPOS_SIZE_FACTOR 0\n" \
              "PARAMVALUE INCREASE_PLACE 0\n" \
              "PARAMVALUE INCREASE_KEEP 0\n" \
              "PARAMVALUE ZEROPOS_PLACE 0\n" \
              "PARAMVALUE ZEROPOS_KEEP 0 \n" \
              "PARAMVALUE DECREASE_PLACE 0\n" \
              "PARAMVALUE DECREASE_KEEP 0\n" \
              "PARAMVALUE MAX_LOSS 2000\n" \
              "PARAMVALUE MAX_OPENTRADE_LOSS 300000\n" \
              "PARAMVALUE COOLOFF_INTERVAL 1\n" \
              "PARAMVALUE NUM_NON_BEST_LEVELS_MONITORED 0\n" \
              "PARAMVALUE THROTTLE_MSGS_PER_SEC 20000"

    _strat_ = "STRATEGYLINE " + shortcode + " FillTimeLogger " + temp_model_file_ + \
        " " + temp_param_file_ + " " + start_time + " " + end_time + " 9990011"

    with open(temp_model_file_, 'w') as myfile:
        myfile.write(_model_)

    f = open(param, 'r')
    param_desc = f.read()
    f.close()

    with open(temp_param_file_, 'w') as myfile:
        myfile.write(param_desc)
        myfile.write("\n")
        myfile.write(_param_)

    with open(temp_strat_file_, 'w') as myfile:
        myfile.write(_strat_)

    out_file = work_dir + "t_dgen_outfile"
    catted_file = work_dir + "catted_datagen_filename"

    catted_writer = open(catted_file, 'ab')

    print(work_dir)

    # reading the skip days from the skip_days file
    skip_days = []
    if (skip_days_file != ""):
        f = open(skip_days_file, 'r')
        for line in f:
            skip_days.append(line.strip())
        f.close()
    skip_days = set(skip_days)

    # creating a temporary ilist and adding three mentioned below indicators as the first three indicators. These three
    # indicators are useful at computing L1Bias at every point
    temp_ilist = work_dir + "/temp_ilist"
    f = open(ilist, 'r')
    w = open(temp_ilist, 'w')
    first_ind = True
    first_line = True
    for line in f:
        tokens = line.strip().split()
        if len(tokens) > 0:
            if tokens[0] == "INDICATOR":
                if first_ind:
                    w.write("INDICATOR 1.00 L1Price " + shortcode + " BidPrice\n")
                    w.write("INDICATOR 1.00 L1Price " + shortcode + " AskPrice\n")
                    w.write("INDICATOR 1.00 L1Price " + shortcode + " " + base_px + "\n")
                    first_ind = False
                w.write(line.strip() + "\n")
            else:
                if first_line:
                    base_px = tokens[3]
                    first_line = False
                w.write(line.strip() + "\n")
    w.close()
    f.close()

    print(dates)
    for date in dates:

        if date in skip_days:
            print('Skipping ' + date)
            continue
        print('Generating for ' + date)

        # getting the contract specs for the shortcode for date
        os.system(contract_specs_script + " " + shortcode + " " + date + " ALL > " + work_dir + "contract_specs")
        f = open(work_dir + "contract_specs", 'r')
        s = f.readlines()
        f.close()
        tick_size = (float)((s[3].split())[1])
        n2d = (float)((s[2].split())[1])
        commish = (float)((s[5].split())[1])
        tick_value = tick_size * n2d

        trade_file_ = "/spare/local/logs/tradelogs/log." + date + "." + process_id_
        silentremove(trade_file_)

        # calling sim_stratgey for the date to generate the orders placed and their execution times
        os.system(
            sim_strategy_script + " SIM " + temp_strat_file_ + " " + process_id_ + " " + date + " ADD_DBG_CODE FILL_TIME_INFO &> " + work_dir + "sim_results")
        print(sim_strategy_script + " SIM " + temp_strat_file_ + " " + process_id_ +
              " " + date + " ADD_DBG_CODE FILL_TIME_INFO &> " + work_dir + "sim_results")

        # retrieving all sending and execution timestamps and CAOS of all orders
        os.system("grep EXECUTION " + trade_file_ + " | awk '{print $4,$5,$6,$7}' > " + work_dir + "executions")
        os.system("grep SENDING " + trade_file_ + " | awk '{print $4,$5,$6,$7}' > " + work_dir + "sending")

        # map with key as timestamp and value as CAOS : whats the CAOS of the order if an order is placed at timestamp
        place_buy_timestamp_caos = {}
        place_sell_timestamp_caos = {}

        # price of each order corresponding to the CAOS : key:CAOS, value:price
        caos_price_map = {}

        f = open(work_dir + "sending", 'r')
        for line in f:
            words = line.split()
            if len(words) < 4:
                continue
            if words[2] == "0":
                place_buy_timestamp_caos[(int)(words[0])] = (int)(words[3])
                caos_price_map[(int)(words[3])] = (int)(words[1])
            elif words[2] == "1":
                place_sell_timestamp_caos[(int)(words[0])] = (int)(words[3])
                caos_price_map[(int)(words[3])] = (int)(words[1])

        f.close()

        # map with key as CAOS and value as timestamp when the CAOS order is executed
        fill_caos_timestamp = {}
        # reverse map of above
        fill_timestamp_caos = {}

        f = open(work_dir + "executions", 'r')
        for line in f:
            words = line.split()
            if len(words) < 4:
                continue
            fill_caos_timestamp[(int)(words[3])] = (int)(words[0])
            if (int)(words[0]) in fill_timestamp_caos:
                continue
            else:
                fill_timestamp_caos[(int)(words[0])] = (int)(words[3])

        f.close()

        # all timestamps when the buy and sell order are placed and executed, helpful in finding the closest timestamps to look
        # when coming across a new timestamp
        timestamps_place_buy = []
        timestamps_fill_buy = []
        timestamps_place_sell = []
        timestamps_fill_sell = []

        # the map with the key as current timestamp and value as the timestamp when the buy order would be executed placed at current timestamp
        fill_buy_time = {}
        for timestamp in list(place_buy_timestamp_caos.keys()):
            if place_buy_timestamp_caos[timestamp] in fill_caos_timestamp:
                fill_buy_time[timestamp] = fill_caos_timestamp[place_buy_timestamp_caos[timestamp]]
                timestamps_place_buy.append(timestamp)
                timestamps_fill_buy.append(fill_buy_time[timestamp])
            else:
                timestamps_place_buy.append(timestamp)
                fill_buy_time[timestamp] = -1

        # the map with key as curernt timestamp and value as the timestamp when the sell order would be executed placed at current timestamp
        fill_sell_time = {}
        for timestamp in list(place_sell_timestamp_caos.keys()):
            if place_sell_timestamp_caos[timestamp] in fill_caos_timestamp:
                fill_sell_time[timestamp] = fill_caos_timestamp[place_sell_timestamp_caos[timestamp]]
                timestamps_place_sell.append(timestamp)
                timestamps_fill_sell.append(fill_sell_time[timestamp])
            else:
                timestamps_place_sell.append(timestamp)
                fill_sell_time[timestamp] = -1

        timestamps_place_buy.sort()
        timestamps_place_sell.sort()
        timestamps_fill_buy.sort()
        timestamps_fill_sell.sort()
        timestamps_place_buy = np.array(timestamps_place_buy)
        timestamps_place_sell = np.array(timestamps_place_sell)
        timestamps_fill_buy = np.array(timestamps_fill_buy)
        timestamps_fill_sell = np.array(timestamps_fill_sell)

        silentremove(out_file)

        # running the datagen to get indicators
        datagen_cmd = [datagen_script, temp_ilist, date, start_time, end_time, process_id_, out_file, datagen_args]
        print(' '.join(datagen_cmd))
        datagen_process = subprocess.Popen(' '.join(datagen_cmd), shell=True,
                                           stdout=subprocess.PIPE,
                                           stderr=subprocess.PIPE)
        out, err = datagen_process.communicate()
        errcode = datagen_process.returncode
        print(out, err)
        # os.system(
        #
        #     datagen_script + " " + ilist + " " + date + " " + start_time + " " + end_time + " " + process_id_ + " " + out_file + " " + datagen_args)
        if not os.path.exists(out_file):
            continue
        data = np.loadtxt(out_file)
        new_data = []

        # for each timestamp, populating the last 10 columns with fill information. Finds the closest timestamp in 300
        # second window matching to the timestamp for which we have fill information.
        for row in range(data.shape[0]):
            ts = data[row][0]

            bt = timestamps_place_buy.searchsorted(ts)
            st = timestamps_place_sell.searchsorted(ts)

            buy_time = -100
            sell_time = -100
            buy_price = -100
            sell_price = -100

            if bt < len(timestamps_place_buy) and timestamps_place_buy[bt] == ts and fill_buy_time[ts] > ts:
                buy_time = fill_buy_time[ts]
                buy_price = caos_price_map[place_buy_timestamp_caos[ts]]
            elif bt > 0 and ts - timestamps_place_buy[bt - 1] <= 300000 and fill_buy_time[
                    timestamps_place_buy[bt - 1]] > ts:
                buy_time = fill_buy_time[timestamps_place_buy[bt - 1]]
                buy_price = caos_price_map[place_buy_timestamp_caos[timestamps_place_buy[bt - 1]]]
            elif bt > 0 and ts - timestamps_place_buy[bt - 1] <= 300000 and fill_buy_time[
                    timestamps_place_buy[bt - 1]] == -1:
                buy_time = -1
                buy_price = caos_price_map[place_buy_timestamp_caos[timestamps_place_buy[bt - 1]]]

            if st < len(timestamps_place_sell) and timestamps_place_sell[st] == ts and fill_sell_time[ts] > ts:
                sell_time = fill_sell_time[ts]
                sell_price = caos_price_map[place_sell_timestamp_caos[ts]]
            elif st > 0 and ts - timestamps_place_sell[st - 1] <= 300000 and fill_sell_time[
                    timestamps_place_sell[st - 1]] > ts:
                sell_time = fill_sell_time[timestamps_place_sell[st - 1]]
                sell_price = caos_price_map[place_sell_timestamp_caos[timestamps_place_sell[st - 1]]]
            elif st > 0 and ts - timestamps_place_sell[st - 1] <= 300000 and fill_sell_time[
                    timestamps_place_sell[st - 1]] == -1:
                sell_time = -1
                sell_price = caos_price_map[place_sell_timestamp_caos[timestamps_place_sell[st - 1]]]

            if buy_time == -100 and sell_time == -100:
                continue

            next_sell_time = timestamps_fill_sell.searchsorted(buy_time)
            if next_sell_time < len(timestamps_fill_sell):
                next_sell_price = caos_price_map[fill_timestamp_caos[timestamps_fill_sell[next_sell_time]]]
            else:
                continue

            next_buy_time = timestamps_fill_buy.searchsorted(sell_time)
            if next_buy_time < len(timestamps_fill_buy):
                next_buy_price = caos_price_map[fill_timestamp_caos[timestamps_fill_buy[next_buy_time]]]
            else:
                continue

            data_point = data[row]
            buy_pnl = (next_sell_price - buy_price) * tick_value - 2 * commish
            sell_pnl = (sell_price - next_buy_price) * tick_value - 2 * commish

            # stacking all 10 columns with fill information
            data_point = np.hstack((data_point, buy_time, timestamps_fill_sell[next_sell_time], buy_price * tick_size,
                                    next_sell_price * tick_size, buy_pnl, sell_time, timestamps_fill_buy[next_buy_time],
                                    sell_price * tick_size, next_buy_price * tick_size, sell_pnl))
            new_data.append(data_point)

        new_data = np.array(new_data)
        np.savetxt(catted_writer, new_data, fmt='%8.5f')
    catted_writer.close()


if __name__ == "__main__":
    if len(sys.argv) < 11:
        print("USAGE: <shortcode> <ilist> <date> <num_days> <start_time> <end_time> <dgen_msecs> <dgen_l1events> <dgen_trades> <to_print_on_eco>")
        sys.exit(0)

    shortcode = sys.argv[1]
    ilist = sys.argv[2]
    date = sys.argv[3]
    num_days = sys.argv[4]
    start_time = sys.argv[5]
    end_time = sys.argv[6]
    msecs = sys.argv[7]
    l1 = sys.argv[8]
    trades = sys.argv[9]
    eco = sys.argv[10]
    if len(sys.argv) > 11:
        skip_days_file = sys.argv[11]
    else:
        skip_days_file = ""

    work_dir = "/spare/local/" + getpass.getuser() + "/fill_data/" + shortcode + "/" + str(
        int(time.time() * 1000)) + "/"

    generate_data(shortcode, ilist, date, num_days, start_time, end_time, msecs, l1, trades, eco, skip_days_file,
                  work_dir)
