from pylib.pnl_modelling_utils.summarize_results_choose_strat import *
from pylib.pnl_modelling_utils.run_sim import run_sim_for_selected_strat


def kfold_validation_strat(work_dir, shortcode, training_start_date, training_end_date,
                           logfilename, sort_algo, num_strats,  num_folds=5, to_print=False):
    """
    Does a kfold validation of the days based on ranks, and returns the top num_strats most consistent strats that 
    performs well on all folds of dates

    :param work_dir: working directory containing results and strats
    :param shortcode: 
    :param training_start_date: start date for the training data
    :param training_end_date: end date for the training data
    :param logfilename: 
    :param sort_algo: 
    :param num_strats: num_strats to be returned, the most consistent ones
    :param to_print: 
    :return: 
    """
    dates = open(os.path.join(work_dir, "training_days_file"), 'r').read().splitlines()
    dates_val = open(os.path.join(work_dir, "validation_days_file"), 'r').read().splitlines()
    dates.extend(dates_val)
    strats_dir = os.path.join(work_dir, "strats_dir")
    local_results_base_dir = os.path.join(work_dir, "local_results_base_dir")

    K = num_folds
    rank_strats = {}
    for k in range(K):
        fold_dates = [x for i, x in enumerate(dates) if i % K == k]

        fold_dates_file = os.path.join(work_dir, "fold_dates_" + str(k))

        w = open(fold_dates_file, 'w')
        for date in fold_dates:
            w.write(date)
            w.write("\n")
        w.close()

        best_strats, _, _, _ = summarize_results_and_choose(shortcode, work_dir, str(training_start_date),
                                                            str(training_end_date), 10000, logfilename,
                                                            sort_algo=sort_algo, dates_file=fold_dates_file)
        for indx, strat in enumerate(best_strats):
            if strat not in rank_strats:
                rank_strats[strat] = 0
            rank_strats[strat] += indx

    for strat in rank_strats:
        rank_strats[strat] = 1.0 * rank_strats[strat] / len(rank_strats)

    consistent_strats = sorted(rank_strats.items(), key=lambda x: x[1])

    strats_to_return = consistent_strats[:num_strats]
    strats_to_return = [x[0] for x in strats_to_return]
    print(strats_to_return)

    best_strat = consistent_strats[0][0]
    median_strat = consistent_strats[int(len(consistent_strats) / 2)][0]
    worst_strat = consistent_strats[-1][0]

    summarize_cmd = [execs.execs().summarize_strategy, shortcode,
                     strats_dir, local_results_base_dir, str(training_start_date), str(training_end_date),
                     "IF", sort_algo, "0", "IF", "1"]

    out = subprocess.Popen(' '.join(summarize_cmd), shell=True, stdout=subprocess.PIPE)
    sum_out, sum_err = out.communicate()
    if sum_out is not None:
        sum_out = sum_out.decode('utf-8')
    if sum_err is not None:
        sum_err = sum_err.decode('utf-8')

    strat_results = {}
    summarize_output = sum_out.strip().splitlines()
    if len(summarize_output) > 1:
        for line in summarize_output:
            tokens = line.split()
            if len(tokens) > 1 and tokens[0] == 'STRATEGYFILEBASE':
                strat_name = tokens[1]
                if strat_name in strats_to_return:
                    strat_results[strat_name] = line
                if strat_name == best_strat:
                    print("BEST STRATEGY")
                    print(line)
                if strat_name == median_strat:
                    print("MEDIAN STRATEGY")
                    print(line)
                    median_strat_line = line
                if strat_name == worst_strat:
                    print("WORST STRATEGY")
                    print(line)
                    worst_strat_line = line

    mail_content = ""

    if to_print:
        print("Top " + str(num_strats) + " strats")
    for strat in strats_to_return:
        if to_print:
            print(strat_results[strat])
            mail_content += strat_results[strat] + "<br>"

    if to_print:
        mail_content += "<p>MEDIAN STRATEGY IN TRAINING PERIOD<br>"
        mail_content += median_strat_line + "</p><br>"
        mail_content += "<p>WORST STRATEGY IN TRAINING PERIOD<br>"
        mail_content += worst_strat_line + "</p><br>"

    logfile = open(logfilename, 'a')
    logfile.write("CREATING PICKED STRATS DIRECTORY FOR STORING SELECTED STRATS\n")
    picked_strats_dir = os.path.join(work_dir, "picked_strats")
    logfile.write(picked_strats_dir + "\n")
    os.system("mkdir " + picked_strats_dir)
    strat_dir = os.path.join(work_dir, "strats_dir")
    for strat in strats_to_return:
        strat_name = os.path.join(strat_dir, strat)
        new_strat_name = os.path.join(picked_strats_dir, strat)
        os.system("cp " + strat_name + " " + new_strat_name)
        logfile.write("cp " + strat_name + " " + new_strat_name + "\n")
    logfile.write("TRAINING DONE\n")
    logfile.write("-------------------------------------\n")
    logfile.flush()
    logfile.close()

    return strats_to_return, median_strat_line, worst_strat_line, mail_content


def find_best_strat_on_kfold(work_dir, shortcode, start_date, end_date, logfilename, sort_algo,
                             top_strats, weights, ilist, training_days_file, validation_days_file,
                             testing_days_file, using_grid):
    """
    Find the best strategy on the kfold validation, given the top consistent strats from the training period. Basically
    it returns the top strat from training period, its performance in the validation period, and the probability 
    distribution of the indicators in the top strats obtained from fold ranking
    :param work_dir: working diretory containing the results and strats
    :param shortcode: 
    :param training_start_date: start date of training period
    :param training_end_date: end date of training period, the folds have been calculated on the training period
    :param val_start_date: validation period start date
    :param val_end_date: validation period end date
    :param logfilename: 
    :param sort_algo: 
    :param top_strats: 
    :param weights: 
    :param ilist: 
    :return: 
    """
    mail_summary = ""
    strats_dir = os.path.join(work_dir, "strats_dir")
    local_results_base_dir = os.path.join(work_dir, "local_results_base_dir")

    logfile = open(logfilename, 'a')
    indicator_scores, prob_distribution = get_indicator_scores_prob(top_strats, weights)
    logfile.write("RELATIVE AVERAGE WEIGHTS OF INDICATORS :\n")
    indicator_scores = np.array(indicator_scores)
    logfile.close()
    logfile = open(logfilename, 'ab')
    np.savetxt(logfile, indicator_scores, fmt='%.2f')
    logfile.close()

    indicators, _ = read_indicators_from_ilist(ilist)
    indicator_prob_table = "<table border = \"2\">\n<tr>"
    indicator_prob_table += "<th align=center><font font-weight=\"bold\" size=\"2\" color=darkblue>INDICATORS</font></th>"
    for i in range(prob_distribution.shape[1]):
        indicator_prob_table += "<th align=center><font font-weight=\"bold\" size=\"2\" color=darkblue>P(" + str(
            i) + ")</font></th>"
    indicator_prob_table += "<th align=center><font font-weight=\"bold\" size=\"2\" color=darkblue>MEAN WEIGHT</font></th>"
    indicator_prob_table += "</tr>\n"
    for i in range(prob_distribution.shape[0]):
        indicator_prob_table += "<tr><td align=center>" + indicators[i] + "</td>"
        for j in range(prob_distribution.shape[1]):
            indicator_prob_table += "<td align=center>" + str(prob_distribution[i][j]) + "</td>\n"
        indicator_prob_table += "<td align=center>" + str(indicator_scores[i]) + "</td>\n"
        indicator_prob_table += "</tr>\n"
    indicator_prob_table += "</table>"
    time.sleep(120)
    mail_summary += indicator_prob_table

    logfile = open(logfilename, 'a')
    chosen_strat = top_strats[0]

    selected_strats_dir = os.path.join(work_dir, "selected_strat")
    logfile.write(selected_strats_dir + "\n")
    os.system("mkdir " + selected_strats_dir)
    strat_name = os.path.join(strats_dir, chosen_strat)
    new_strat_name = os.path.join(selected_strats_dir, chosen_strat)
    os.system("cp " + strat_name + " " + new_strat_name)

    logfile.write("cp " + strat_name + " " + new_strat_name + "\n")

    chosen_strat = top_strats[0]
    print(chosen_strat)

    chosen_strat_file = os.path.join(strats_dir, chosen_strat)
    logfile.write("Best Strat: " + chosen_strat_file + "\n")

    summarize_cmd = [execs.execs().summarize_strategy, shortcode,
                     strats_dir, local_results_base_dir, str(start_date), str(end_date),
                     "IF", sort_algo, "0", validation_days_file, "1"]
    out = subprocess.Popen(' '.join(summarize_cmd), shell=True, stdout=subprocess.PIPE)
    sum_out, sum_err = out.communicate()
    if sum_out is not None:
        sum_out = sum_out.decode('utf-8')
    if sum_err is not None:
        sum_err = sum_err.decode('utf-8')

    summarize_output = sum_out.strip().splitlines()
    if len(summarize_output) > 1:
        for line in summarize_output:
            tokens = line.split()
            if len(tokens) > 1 and tokens[0] == 'STRATEGYFILEBASE':
                strat_name = tokens[1]
                if strat_name == chosen_strat:
                    print("TOP STRAT's PERFORMANCE IN VALIDATION PERIOD")
                    print(line)
                    mail_summary += "TOP STRAT's PERFORMANCE IN VALIDATION PERIOD<br>"
                    mail_summary += line + "<p><br>"

    logfile.write("Running Simulation for test period\n")
    logfile.flush()
    logfile.close()
    run_sim_for_selected_strat(work_dir, shortcode, str(start_date), str(end_date),
                               testing_days_file, logfilename, using_grid)
    logfile = open(logfilename, 'a')

    mail_summary += "<p>SELECTED STRATS PERFORMANCE IN TESTING PERIOD" + "<br>"
    _, _, _, strat_lines = summarize_results_and_choose(shortcode, work_dir, str(start_date),
                                                        str(end_date), 1,
                                                        logfilename,
                                                        strat_list=selected_strats_dir, sort_algo=sort_algo,
                                                        to_print=True,
                                                        dates_file=testing_days_file)
    mail_summary += strat_lines + "</p>"

    mail_summary_ranks = get_training_validation_rank(shortcode, work_dir, start_date, end_date,
                                                      chosen_strat, weights.shape[0], training_days_file,
                                                      validation_days_file,
                                                      testing_days_file, sort_algo='kCNAPnlSharpeAverage')
    mail_summary += mail_summary_ranks

    with open(chosen_strat_file) as strat:
        best_strat_line = strat.readline()
        logfile.write(best_strat_line)
        logfile.write("\n")
        strat_words = best_strat_line.split()
        if len(strat_words) > 6:
            best_model = strat_words[3]
            best_param = strat_words[4]

    f = open(best_model, 'r')
    chosen_weights = f.read()
    f.close()

    plot_pnl(chosen_strat, start_date, end_date, work_dir, shortcode, sort_algo)

    # printing the final model
    mail_summary += "<br><p>FINAL MODEL" + "</p><br>"
    mail_summary += "<br>".join(chosen_weights.splitlines()) + "<br>"
    print("The final Model : ")
    logfile.write("Final Model:\n")
    print(chosen_weights)
    logfile.write(chosen_weights)
    logfile.flush()
    logfile.close()
    return mail_summary, chosen_strat, best_model, int(best_param.split("_")[-1])
