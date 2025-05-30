import subprocess
import numpy as np
from walkforward.definitions import execs
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.wf_db_utils.get_trading_days_for_shortcode import get_trading_days_for_shortcode


def check_correlations(model, shortcode, date, lookback_days, indicator_sharpe_threshold, model_sharpe_threshold):
    f = open(model, 'r')
    i = 4
    indx = 0
    weight_map = {}
    indicator_map = {}
    indicator_correlations = {}

    for line in f:
        tokens = line.split()
        if tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = " ".join(x for x in tokens[2:end_indx])
            weight_map[indx] = float(tokens[1])
            indicator_map[indx] = indicator
            indicator_correlations[indx] = []
            i += 1
            indx += 1

    prev_date = calc_prev_week_day(date)
    dates = get_trading_days_for_shortocde(shortcode, prev_date, lookback_days)

    model_correlations = []
    for d in dates:
        if not os.path.exists(work_dir + "regdata_" + d):
            continue
        data = np.loadtxt(work_dir + "regdata_" + d)
        if data.shape[0] == 0:
            continue

        sumvars = np.zeros((data.shape[0]))
        for i in range(1, data.shape[1]):
            corr = (np.corrcoef(data[:, 0], data[:, i]))[0][1]
            indicator_correlations[i - 1].append(corr)
            sumvars += weight_map[i - 1] * data[:, i]
        model_corr = (np.corrcoef(data[:, 0], sumvars))[0][1]
        model_correlations.append(model_corr)

    for indicator in list(indicator_correlations.keys()):
        indicator_sharpe = np.mean(indicator_correlations[key]) / np.std(indicator_correlations[key])
        if indicator_sharpe < indicator_sharpe_threshold:
            flag = 1
            print(indicator_map[indicator] +
                  " : indicator correlation sharpe less than threshold " + indicator_sharpe_threshold)
    model_sharpe = np.mean(model_correlations) / np.std(model_correlations)
    if model_sharpe < model_sharpe_threshold:
        flag = 1
        print("Model correlation sharpe over lookback days less than threshold " + model_sharpe_threshold)

    if flag == 1:
        return False
    else:
        return True
