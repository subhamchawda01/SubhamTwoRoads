import os
import numpy as np
import subprocess
from walkforward.definitions import execs


def create_strats(work_dir, relative_weights, model_weights, model_weights_stdev_dict, shortcode, execlogic, start_time,
                  end_time,
                  param_file_list,
                  ilist, logfilename):
    '''

    Create the strategy file 

    :return: 
    '''

    strats_dir = os.path.join(work_dir, "strats_dir")
    models_dir = os.path.join(work_dir, "models_dir")
    params_dir = os.path.join(work_dir, "params_dir")
    query_id = "2222"

    logfile = open(logfilename, 'a')
    os.system('mkdir -p ' + str(strats_dir))
    os.system('mkdir -p ' + str(models_dir))
    os.system('mkdir -p ' + str(params_dir))

    logfile.write("CREATING TEMPORARY STRATS\n")
    logfile.write("Strats Directory: " + strats_dir + "\n")
    logfile.write("Models Directory: " + models_dir + "\n")
    logfile.write("Params Directory: " + params_dir + "\n")

    f = open(ilist, 'r')
    indicators = []
    pre_indicators_line = []
    for line in f:
        tokens = line.strip().split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            indicators.append(indicator)
        elif len(tokens) > 0 and tokens[0] != "INDICATOREND":
            pre_indicators_line.append(line)

    # param file_list is a list

    for param_idx, param_file in enumerate(param_file_list):
        param_basename = os.path.basename(param_file)
        temp_param_file = os.path.join(params_dir, param_basename + "_" + str(param_idx))
        os.system("cp " + param_file + " " + temp_param_file)
        indx = 0
        for weight in model_weights:
            model_file = os.path.join(models_dir, "model_" + str(indx) + "_" + str(param_idx))
            if os.path.exists(model_file):
                indx += 1
                continue
            writer = open(model_file, 'w')
            writer.write(''.join(pre_indicators_line))
            for weight_index in range(len(weight)):
                if weight[weight_index] == 0:
                    continue
                writer.write("INDICATOR " + str(weight[weight_index]) + " " + indicators[weight_index] + "\n")
            writer.write("INDICATOREND\n")
            writer.write(
                "# Indx: " + str(indx) + " Relative weights: " + ' '.join(
                    map(str, relative_weights[indx])) + " Stdev for model: " + str(
                    model_weights_stdev_dict[tuple(weight)]) + "\n")
            writer.close()

            strat_writer = open(os.path.join(strats_dir, "strat_" + str(indx)) + "_" + str(param_idx), 'w')
            strat_writer.write("STRATEGYLINE " + str(shortcode) + str(" ") +
                               execlogic + " " + model_file + " " + temp_param_file + " " +
                               start_time + " " + end_time + " " + query_id)
            strat_writer.close()
            indx += 1

    logfile.write("\nCREATING TEMP STRATS FINISHED\n")
    logfile.write("-------------------------------------\n")
    logfile.close()


def get_indicator_scores_prob(picked_strats, weights):
    '''

    Gives the probability of an indicator weight computed from the top strats 

    picked_strats: list
            The strats list where indicator contribution is to be computed
    weights: 2D array  num_indicators x weight_combination


    returns:
        sum_indicators: 1D array
                The sum of weights of a particular indicator among all strats provided by picked strats

        ind_value_prob: 2D array
                The probability of the indicator weight computed from the top strats

    '''
    max_ind_value = np.amax(np.absolute(weights))

    # initialize the indicator_probab array and the sum array

    ind_value_prob = np.zeros((weights.shape[1], max_ind_value + 1))
    sum_indicators = np.zeros(weights.shape[1])

    for strat in picked_strats:
        model_indices = strat.split("_")[1:]
        weight_index = (int)(model_indices[0])
        sum_indicators += weights[weight_index]
        for indx in range(weights.shape[1]):
            ind_value_prob[indx][abs(int(weights[weight_index][indx]))] += 1

    print("Relative average weights of indicators in top strats: ")
    sum_indicators = sum_indicators / len(picked_strats)
    ind_value_prob = ind_value_prob / len(picked_strats)
    print(sum_indicators)
    print(ind_value_prob)

    return sum_indicators, ind_value_prob


def get_rank_of_strat_in_time(shortcode, work_dir, start_date, end_date, strat, dates_file, sort_algo='kCNAPnlSharpeAverage'):
    '''

    Gives the rank of a particular strategy from the result summarized between start and end date

    shortcode: str
                The product shortcode whose strategy rank is to be computed
    work_dir: str
                The pnl modelling directory

    start_date: str
                The start date of summarization

    end_date: str
                The end date of summarization

    strat:  str
                The name of strategy whose rank is to be computed

    sort_algo: str
                The sort algo to be used for summarization


    return:
        rank:int
                The rank of the strategy

    '''

    local_results_base_dir = os.path.join(work_dir, 'local_results_base_dir')
    strats_dir = os.path.join(work_dir, 'strats_dir')

    summarize_cmd = [execs.execs().summarize_strategy, shortcode,
                     strats_dir, local_results_base_dir, start_date, end_date, "IF", sort_algo, "0", dates_file, "1"]
    out = subprocess.Popen(' '.join(summarize_cmd), shell=True, stdout=subprocess.PIPE)
    sum_out, sum_err = out.communicate()
    if sum_out is not None:
        sum_out = sum_out.decode('utf-8')
    if sum_err is not None:
        sum_err = sum_err.decode('utf-8')

    summarize_output = sum_out.strip().splitlines()
    indx = 0
    rank = 0
    if len(summarize_output) >= 1:
        for line in summarize_output:
            tokens = line.split()
            if len(tokens) > 1 and tokens[0] == 'STRATEGYFILEBASE':
                indx += 1
                if tokens[1] == strat:
                    rank = indx

    return rank


def create_regime_trading_params(work_dir, regime_trading_indicator_list, regimes_to_trade, param_file_list, temp_param_file_list):
    regime_params_dir = os.path.join(work_dir, "regime_params_dir")
    os.system('mkdir -p ' + str(regime_params_dir))

    for param_file in (param_file_list):
        index = 0
        for indicator in regime_trading_indicator_list:
            expression = []
            expression_trade = []
            num_tokens = indicator.index('PARAMS')
            expression = ("PARAMVALUE REGIMEINDICATOR Expression SPLIT_REGIME " +
                          str((num_tokens) - 1) + " 1.00 " + ' '.join(indicator))
            expression_trade = ("PARAMVALUE REGIMES_TO_TRADE " + ' '.join(regimes_to_trade[index]))
            param_basename = os.path.basename(param_file)
            temp_param_file = os.path.join(regime_params_dir, param_basename + "_" + str(index))
            index = index + 1
            os.system("cp " + param_file + " " + temp_param_file)
            os.system("sed -i '/REGIMEINDICATOR/d' " + temp_param_file)
            os.system("sed -i '/REGIMES_TO_TRADE/d' " + temp_param_file)
            f = open(str(temp_param_file), 'a')
            f.write((expression) + "\n")
            f.write((expression_trade))
            f.close()
            temp_param_file_list.append(temp_param_file)
