#!/usr/bin/env python

import os
import sys
import getpass
import time
import numpy as np

sys.path.append('/media/shared/ephemeral16/dbhartiya/liblinear-weights-2.01/python')
sys.path.append(os.path.expanduser('~/basetrade/'))
from .generate_fill_time_data import generate_data

#from liblinearutil import *


def read_data(data_file):
    """
    This function reads the data generated from generate_fill_time_data.py , processes it to get the buy and
    sell pnl for each data point, and its corresponding weight.

    :param data_file: the data file containing the catted data generated from the generate_fill_time_data.py 
    :return: buy and sell corresponding data , labels and weights
    """
    data = np.loadtxt(data_file)
    data = data[:, 4:]
    weight_buy = []
    weight_sell = []
    labels_buy = []
    labels_sell = []
    buy_pnl = []
    sell_pnl = []
    ttc_buy = []
    ttc_sell = []
    for i in range(data.shape[0]):
        if data[i, -10] < 0:
            labels_buy.append(0)
            weight_buy.append(1)
            buy_pnl.append(0)
        else:
            labels_buy.append(1 * (data[i, -6] > 0))
            weight_buy.append(np.abs(data[i, -7] - data[i, -8]) / (1 + data[i, -9] - data[i, -10]))
            buy_pnl.append(data[i, -7] - data[i, -8])
            ttc_buy.append(1 + data[i, -9] - data[i, -10])
        if data[i, -5] < 0:
            labels_sell.append(0)
            weight_sell.append(1)
            sell_pnl.append(0)
        else:
            labels_sell.append(1 * (data[i, -1] > 0))
            weight_sell.append(np.abs(data[i, -3] - data[i, -2]) / (1 + data[i, -4] - data[i, -5]))
            sell_pnl.append(data[i, -3] - data[i, -2])
            ttc_sell.append(1 + data[i, -4] - data[i, -5])
    ttc_buy = np.mean(ttc_buy)
    ttc_sell = np.mean(ttc_sell)

    buy_mkt_bias = data[:, 2] - data[:, 0]
    sell_mkt_bias = data[:, 1] - data[:, 2]

    data = data[:, 3:-10]
    labels_buy = labels_buy * 1
    labels_sell = labels_sell * 1
    labels_buy = np.array(labels_buy)
    labels_sell = np.array(labels_sell)
    weight_buy = np.array(weight_buy)
    weight_sell = np.array(weight_sell)
    weight_buy = weight_buy * ttc_buy * 10
    weight_sell = weight_sell * ttc_sell * 10
    buy_pnl = np.array(buy_pnl)
    sell_pnl = np.array(sell_pnl)
    data_buy = np.column_stack((data, buy_mkt_bias))
    data_sell = np.column_stack((-data, sell_mkt_bias))

    return data_buy, labels_buy, weight_buy, buy_pnl, data_sell, labels_sell, weight_sell, sell_pnl


def pre_process(data_buy, labels_buy, weight_buy, buy_pnl, data_sell, labels_sell, weight_sell, sell_pnl, use_l1_bias):
    """
    This function removes zero tick pnl points from data_buy and data_sell , and their corresponding weights and labels. 
    It also removes points whose weights lie in the top 5% percentile of the points, since they are arbitrarily high and 
    are not encountered in real.
    :param data_buy: data corresponding to buy order placed at each point
    :param labels_buy: label for data_buy
    :param weight_buy: weight for each point in data_buy
    :param buy_pnl: the pnl obtained if buy order placed 
    :param data_sell: data corresponding to sell order placed at each point
    :param labels_sell: label for data_sell
    :param weight_sell: weight for each point in data_sell
    :param sell_pnl: the pnl obtained if sell order placed
    :param use_l1_bias: whether to use l1 bias as an indicator
    :return: all corresponding data, weights and labels after processing
    """
    db = data_buy[buy_pnl[:] != 0, :]
    ds = data_sell[sell_pnl[:] != 0, :]
    lb = labels_buy[buy_pnl[:] != 0]
    ls = labels_sell[sell_pnl[:] != 0]
    wb = weight_buy[buy_pnl[:] != 0]
    ws = weight_sell[sell_pnl[:] != 0]

    if wb.shape[0] == 0:
        threshold_buy = None
    else:
        threshold_buy = (np.sort(wb))[int(0.95 * wb.shape[0])]
    if ws.shape[0] == 0:
        threshold_sell = None
    else:
        threshold_sell = (np.sort(ws))[int(0.95 * ws.shape[0])]

    if threshold_buy is not None:
        pb = wb[:] < threshold_buy
    else:
        pb = wb[:] >= 0
    if threshold_sell is not None:
        sb = ws[:] < threshold_sell
    else:
        sb = ws[:] >= 0

    db = db[pb]
    ds = ds[sb]
    lb = lb[pb]
    ls = ls[sb]
    wb = wb[pb]
    ws = ws[sb]

    data_buy = db
    data_sell = ds
    labels_buy = lb
    labels_sell = ls
    weight_buy = wb
    weight_sell = ws

    if not use_l1_bias:
        data_buy = data_buy[:, :-1]
        data_sell = data_sell[:, :-1]

    return data_buy, labels_buy, weight_buy, data_sell, labels_sell, weight_sell


def train_with_weights(data, labels, weights):
    """
    The function trains a linear classifier on the data. It randomly selects equal points from both classes since both classes
    have a big difference in number. We undersample the points with 0 label and oversample the points with 1 label. The function
    then finds the best C parameter in svm using grid search, and then uses that C value to output the final trained model.
    :param data: The data to train the linear classifier on
    :param labels: the label for each point in the data samples
    :param weights: the weight for each point in data matrix, gives that much importance to points, useful for betting more on points with higher pnl
    :return: returns the trained model
    """
    data1 = np.column_stack((labels, data, weights))
    c0 = data1[data1[:, 0] == 0, :]
    c1 = data1[data1[:, 0] == 1, :]
    size = max(10, min(500000, c1.shape[0]))
    sample0 = np.random.choice(c0.shape[0], min(size, c0.shape[0]), replace=False)
    c0 = c0[sample0, :]
    sample1 = np.random.choice(c1.shape[0], min(size, c1.shape[0]), replace=True)
    c1 = c1[sample1, :]
    train_data = np.vstack((c0, c1))

    train_labels = train_data[:, 0]
    train_data = train_data[:, 1:-1]
    train_weights = train_data[:, -1]

    train_data = train_data.tolist()
    train_labels = train_labels.tolist()
    train_weights = train_weights.tolist()

    prob = problem(train_weights, train_labels, train_data)
    options1 = '-s 2 -e 0.00001 -C'
    param = parameter(options1)
    best_C, best_rate = train(prob, param)

    options2 = '-s 2 -e 0.00001 -c {0}'
    options2 = options2.format(best_C)
    param = parameter(options2)
    model = train(prob, param)

    return model


def get_stdev_model(weights, data_file, use_l1_bias):
    """
    The function takes the weights of the indicators, and computes the stdev of the model(stdev of sumvars)
    :param weights: the weights for the model trained
    :param data_file: the data file generated from generate_fill_time_data.py containing indicators and fill based information.
    :param use_l1_bias: whether l1 bias was used to train model
    :return: return the stdev of the model
    """
    data = np.loadtxt(data_file)
    data = data[:, 7:-10]
    if use_l1_bias:
        weights = weights[:-1]

    sumvars = np.zeros(data.shape[0])
    for i in range(len(weights)):
        sumvars += np.dot(weights[i], data[:, i])

    stdev = np.std(sumvars)
    return stdev


def create_model(ilist, weights, output_model):
    """
    The function creates the model file in output_model file using the weights learnt and the initial ilist provided.
    :param ilist: the initial ilist file
    :param weights: weights learnt from the classifier using fill based data
    :param output_model: the output model path
    """
    f = open(ilist, 'r')
    s = f.read()
    f.close()

    model_lines = s.splitlines()

    model_writer = open(output_model, 'w')
    line_indx = 0
    indx = 0
    for line in model_lines:
        line_indx += 1
        line = line.strip()
        tokens = line.strip().split()

        # If the config has an empty line we should ignore it.
        # This will help us make sure that the next line which compares
        # the first token does not encounter an error.
        if len(tokens) == 0:
            continue

        if tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])

            weight = weights[indx]
            model_writer.write("INDICATOR " + str(weight) + " " + indicator + "\n")
            indx += 1

        else:
            model_writer.write(line + "\n")

    model_writer.close()


def get_fill_based_model(shortcode, ilist, param, end_date, num_days, start_time, end_time,
                         msecs, events, trades, eco,
                         skip_days_file, target_stdev, use_l1_bias, output_model):
    """
    The function trains the model using fill based information and stores it in output_model file.
    :param shortcode: shortcode of the product
    :param ilist: the ilist containing the indicators to be used in fill_based_model generation
    :param end_date: the end date of the block of days for which the model is trained
    :param num_days: the number of days in the block of training days
    :param start_time: start trading time
    :param end_time: end trading time
    :param msecs: msecs to sample as given in datagen
    :param events: l1events to sample as given in datagen
    :param trades: trades to sample as given in datagen
    :param eco: eco mode to sample as given in dategen
    :param skip_days_file: skip days file to avoid generating data on the specific dates
    :param target_stdev: target model to scale the stdev of the model
    :param use_l1_bias: whether to use l1 bias while training the model
    :param output_model: output model file that stores the final trained model
    """
    if skip_days_file == "IF":
        skip_days_file = ""

    scaling = False
    if target_stdev == "0":
        scaling = False
    else:
        scaling = True
        target_stdev = float(target_stdev)

    if use_l1_bias == "1":
        use_l1_bias = True
    else:
        use_l1_bias = False

    work_dir = "/spare/local/" + getpass.getuser() + "/fill_data_models/" + shortcode + "/" + str(
        int(time.time() * 1000)) + "/"
    try:
        # generate data using generate_fill_time_data.py
        generate_data(shortcode, ilist, param, end_date, num_days, start_time, end_time, msecs, events, trades, eco,
                      skip_days_file,
                      work_dir)

        data_file = os.path.join(work_dir, "catted_datagen_filename")

        # reading and processing the data generated
        data_buy, labels_buy, weight_buy, buy_pnl, data_sell, labels_sell, weight_sell, sell_pnl = read_data(data_file)
        data_buy, labels_buy, weight_buy, data_sell, labels_sell, weight_sell = pre_process(data_buy, labels_buy,
                                                                                            weight_buy, buy_pnl,
                                                                                            data_sell, labels_sell,
                                                                                            weight_sell, sell_pnl,
                                                                                            use_l1_bias)
        data_combined = np.vstack((data_buy, data_sell))
        labels_combined = np.append(labels_buy, labels_sell)
        weight_combined = np.append(weight_buy, weight_sell)

        print(data_combined.shape)

        # training the classifier
        clf = train_with_weights(data_combined, labels_combined, weight_combined)

        [weight, bias] = clf.get_decfun()
        print(weight, bias)
        weights = [-1 * w for w in weight]
        print("\nWeights before scaling to target stdev")
        print(weights)
        stdev = get_stdev_model(weights, data_file, use_l1_bias)

        if scaling:
            weights = [w * target_stdev / stdev for w in weights]

        print(weights)
        create_model(ilist, weights, output_model)
    finally:
        os.system("rm -rf " + work_dir)


if __name__ == "__main__":

    if len(sys.argv) <= 14:
        print("USAGE: <shortcode> <ilist> <param> <end_date> <num_days> <start_time> <end_time> "
              "<msecs> <events> <trades> <eco> "
              "<skip_days_file/IF> <target_stdev/0> <use_l1_bias/0> <output_model>")
        sys.exit(0)

    shortcode = sys.argv[1]
    ilist = sys.argv[2]
    param = sys.argv[3]
    end_date = sys.argv[4]
    num_days = sys.argv[5]
    start_time = sys.argv[6]
    end_time = sys.argv[7]
    msecs = sys.argv[8]
    events = sys.argv[9]
    trades = sys.argv[10]
    eco = sys.argv[11]
    skip_days_file = sys.argv[12]
    target_stdev = sys.argv[13]
    use_l1_bias = sys.argv[14]
    output_model = sys.argv[15]

    get_fill_based_model(shortcode, ilist, param, end_date, num_days, start_time, end_time,
                         msecs, events, trades, eco,
                         skip_days_file, target_stdev, use_l1_bias, output_model)
