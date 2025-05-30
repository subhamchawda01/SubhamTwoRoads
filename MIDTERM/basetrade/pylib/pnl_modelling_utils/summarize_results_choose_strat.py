import subprocess
import os
import time
import numpy as np
import matplotlib

matplotlib.use('Agg')
from matplotlib import pyplot as plt
import datetime as dt
from walkforward.definitions import execs
from pylib.pnl_modelling_utils.utils import get_indicator_scores_prob
from pylib.pnl_modelling_utils.utils import get_rank_of_strat_in_time
from pylib.pnl_modelling_utils.ilist_utils import read_indicators_from_ilist
from pylib.pnl_modelling_utils.run_sim import run_sim_for_selected_strat


def summarize_results_and_choose(shortcode, work_dir, start_date, end_date, num_strats, logfilename, strat_list="IF",
                                 sort_algo='kCNAPnlSharpeAverage', to_print=False, dates_file=None):
    '''

    Summarizes the results in the results directory and returns best , median and the worst strat form the summarized output.


    shortcode:str
                Product shortcode 

    work_dir:str
                The full path of the pnl modelling work directory 


    start_date: str
                The start date for result summarization

    end_date: str
                The end date for result summarization


    num_strats: int
                Number of top strats to choose

    logfilename:str
                The full path of the main log file

    strat_list: str

    sort_algo:str
                The sort algo for summarizing the results

    to_print:boolean
                Whether to print the best strats or not

    dates_file:str
                A file that has the list of specefic dates to summarize the results on



    returns:
            best_strat: list
                    The list of names of the top strats.


            median_strat_line: str
                    The pnl result of the median performing strat

            worst_strat_line: str
                    The pnl result of the worst performing strat

            mail_content: str
                    The mail string


    '''

    local_results_base_dir = os.path.join(work_dir, 'local_results_base_dir')
    strats_dir = ""
    mail_content = ""
    if strat_list == "IF":
        strats_dir = os.path.join(work_dir, 'strats_dir')
    else:
        strats_dir = strat_list

    logfile = open(logfilename, 'a')
    summarize_cmd = [execs.execs().summarize_strategy, shortcode,
                     strats_dir, local_results_base_dir, start_date, end_date, "IF", sort_algo, "0", "IF", "1"]

    if dates_file is not None:
        summarize_cmd = [execs.execs().summarize_strategy, shortcode,
                         strats_dir, local_results_base_dir, start_date, end_date, "IF", sort_algo, "0", dates_file,
                         "1"]

    logfile.write("SUMMARIZE CMD:")
    logfile.write(' '.join(summarize_cmd))
    logfile.write("\n")

    print(("SUMMARIZE_CMD:\n" + ' '.join(summarize_cmd)))
    logfile.flush()

    # summarize_output = subprocess.check_output(summarize_cmd).splitlines()
    out = subprocess.Popen(' '.join(summarize_cmd), shell=True, stdout=subprocess.PIPE)
    sum_out, sum_err = out.communicate()
    if sum_out is not None:
        sum_out = sum_out.decode('utf-8')
    if sum_err is not None:
        sum_err = sum_err.decode('utf-8')

    logfile.write(sum_out)

    summarize_output = sum_out.strip().splitlines()
    best_strats = []

    if len(summarize_output) >= 1:
        median_strat_line = summarize_output[int(len(summarize_output) / 2)]
        worst_strat_line = summarize_output[-1]
    else:
        median_strat_line = ""
        worst_strat_line = ""
    if len(summarize_output) >= 1:
        for line in summarize_output:
            tokens = line.split()
            if len(tokens) > 1 and len(best_strats) < num_strats:
                if tokens[0] == 'STRATEGYFILEBASE':
                    best_strat_name = tokens[1]
                    best_strats.append(best_strat_name)
                    if to_print:
                        print(line)
                        mail_content += line + "<br>"

        if to_print:
            print("BEST_STRATS: ")
            print(best_strats)

        logfile.write("BEST STRATS:\n")
        logfile.write(' '.join(best_strats))

    logfile.write("-------------------------------------\n")
    logfile.flush()

    out = subprocess.Popen(' '.join(summarize_cmd[:-1]) + " 0", shell=True, stdout=subprocess.PIPE)
    sum_out, sum_err = out.communicate()
    if sum_out is not None:
        sum_out = sum_out.decode('utf-8')
    if sum_err is not None:
        sum_err = sum_err.decode('utf-8')
    summarize_output = sum_out.strip().splitlines()
    total_results = 0
    if len(summarize_output) >= 1:
        for line in summarize_output:
            tokens = line.split()
            if len(tokens) > 1:
                if tokens[0] == 'STRATEGYFILEBASE':
                    if tokens[1] != best_strats[0]:
                        break
                else:
                    total_results += 1
    print("RESULTS GENERATED FOR " + str(total_results - 1) + " days")
    logfile.write("RESULTS GENERATED FOR " + str(total_results - 1) + " days")
    logfile.close()
    mail_content += "RESULTS GENERATED FOR " + str(total_results - 1) + " days<br>"

    return best_strats, median_strat_line, worst_strat_line, mail_content


def plot_pnl(strat, training_start_date, validation_end_date, work_dir, shortcode, sort_algo="kCNAPnlSharpeAverage"):
    strats_dir = os.path.join(work_dir, "strats_dir")
    local_results_base_dir = os.path.join(work_dir, "local_results_base_dir")
    start_date = str(training_start_date)
    end_date = str(validation_end_date)

    summarize_cmd = [execs.execs().summarize_strategy, shortcode,
                     strats_dir, local_results_base_dir, start_date, end_date, "IF", sort_algo, "0", "IF", "0"]

    process = subprocess.Popen(' '.join(summarize_cmd), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')

    out = out.strip().splitlines()
    ready = False
    pnls = []
    dates = []
    if len(out) > 0:
        for line in out:
            tokens = line.split()
            if len(tokens) == 0:
                continue
            if tokens[0] == "STRATEGYFILEBASE":
                if tokens[1] == strat:
                    ready = True
            elif ready and tokens[0] == "STATISTICS":
                ready = False
                break
            elif ready:
                pnls.append(int(tokens[1]))
                dates.append(int(tokens[0]))

    pnls = np.array(pnls)
    dates = np.array(dates)
    pnls = np.cumsum(pnls)
    fig = plt.figure(figsize=(23, 23))
    plt.plot([dt.datetime.strptime(str(elem), "%Y%m%d").date() for elem in dates], pnls, label="Cumulative Pnl")
    fig.savefig(work_dir + "/graph.png")
    print(strat)
    print("Plot saved to " + work_dir + "/graph.png")


def find_best_strat_on_validation_days(work_dir, shortcode, start_date, end_date,
                                       logfilename, sort_algo, num_strats, weights, ilist, training_days_file,
                                       validation_days_file, testing_days_file, using_grid):
    '''

    Takes the summarized results from the training and the validation periods and writes it in the log file and mail string.
    Returns the mail string to send, best model and the best param found in the best strategy.

    work_dir:str
                The full path of the work directory

    shortcode:str
                Product shortcode 

    training_start_date: str
                The start date for result summarization

    training_end_date: str
                The end date for result summarization

    val_start_date:str
                The validation start date

    val_end_date:str
                The validation end date

    logfilename:str
                The full path of the main log file

    sort_algo:str
                The sort algo on which to summarize results on

    num_strats:int
                The number of best strat to choose

    weights:    2D array     num_indicators x max_allowed_weight
                The weight matrix

    num_regimes: int
                Number of regimes in the regime indicators

    ilist: str
                The full path of the ilist file 


    testing_dates_file: str
                The full path of the file to test the strategy on


    returns:

            mail_summary: str
                        The summary mail to be sent

            best_model: str
                        The full path of the best model found from the pnl modelling 

            best_param:
                        The full path of the best param found from the pnl modelling


    '''

    logfile = open(logfilename, 'a')
    logfile.write("SUMMARIZING ON TRAINING DAYS\n")
    logfile.close()

    mail_summary = "<p>TOP " + str(num_strats) + " IN TRAINING PERIOD" + "<br>"

    picked_strats, median_strat_line, worst_strat_line, strat_lines = summarize_results_and_choose(shortcode, work_dir,
                                                                                                   str(
                                                                                                       start_date),
                                                                                                   str(
                                                                                                       end_date),
                                                                                                   num_strats,
                                                                                                   logfilename,
                                                                                                   sort_algo=sort_algo,
                                                                                                   to_print=True,
                                                                                                   dates_file=training_days_file)

    mail_summary += strat_lines + "</p><br>"

    mail_summary += "<p>MEDIAN STRAT IN TRAINING PERIOD" + "<br>"
    mail_summary += median_strat_line + "</p><br>"
    print("\nMEDIAN STRAT IN TRAINING PERIOD")
    print(median_strat_line)

    mail_summary += "<p>WORST STRAT IN TRAINING PERIOD" + "<br>"
    mail_summary += worst_strat_line + "</p><br>"
    print("\nWORST STRAT IN TRAINING PERIOD")
    print(worst_strat_line + "\n")

    # extracting top strats into picked_strats directory
    logfile = open(logfilename, 'a')
    logfile.write("CREATING PICKED STRATS DIRECTORY FOR STORING SELECTED STRATS\n")
    picked_strats_dir = os.path.join(work_dir, "picked_strats")
    logfile.write(picked_strats_dir + "\n")
    os.system("mkdir " + picked_strats_dir)
    strat_dir = os.path.join(work_dir, "strats_dir")
    for strat in picked_strats:
        strat_name = os.path.join(strat_dir, strat)
        new_strat_name = os.path.join(picked_strats_dir, strat)
        os.system("cp " + strat_name + " " + new_strat_name)
        logfile.write("cp " + strat_name + " " + new_strat_name + "\n")
    logfile.write("TRAINING DONE\n")
    logfile.write("-------------------------------------\n")
    logfile.flush()

    indicator_scores, prob_distribution = get_indicator_scores_prob(picked_strats, weights)
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

    _, median_strat_line, worst_strat_line, _ = summarize_results_and_choose(shortcode, work_dir, str(start_date),
                                                                             str(end_date),
                                                                             num_strats, logfilename,
                                                                             sort_algo=sort_algo, to_print=False,
                                                                             dates_file=validation_days_file)

    mail_summary += "<p>MEDIAN STRAT IN VALIDATION PERIOD" + "<br>"
    mail_summary += median_strat_line + "</p><br>"
    print("\nMEDIAN STRAT IN VALIDATION PERIOD")
    print(median_strat_line)

    mail_summary += "<p>WORST STRAT IN VALIDATION PERIOD" + "<br>"
    mail_summary += worst_strat_line + "</p><br>"
    print("\nWORST STRAT IN VALIDATION PERIOD")
    print(worst_strat_line + "\n")

    logfile = open(logfilename, 'a')
    logfile.write("VALIDATION STARTING\n")
    # choose top strat from validation days

    mail_summary += "<p>TOP STRAT AMONG TOP STRATS SELECTED FROM TRAINING PERIOD IN VALIDATION PERIOD" + "<br>"
    val_best_strat, _, _, strat_lines = summarize_results_and_choose(shortcode, work_dir, str(start_date),
                                                                     str(end_date), 1,
                                                                     logfilename,
                                                                     strat_list=picked_strats_dir, sort_algo=sort_algo,
                                                                     to_print=True,
                                                                     dates_file=validation_days_file)
    mail_summary += strat_lines + "</p>"

    if len(val_best_strat) == 0:
        dummy_model_file = work_dir + "/dummy_file"
        os.system('touch ' + dummy_model_file)
        return mail_summary, dummy_model_file, -1

    selected_strats_dir = os.path.join(work_dir, "selected_strat")
    logfile.write(selected_strats_dir + "\n")
    os.system("mkdir " + selected_strats_dir)
    strat_name = os.path.join(strat_dir, val_best_strat[0])
    new_strat_name = os.path.join(selected_strats_dir, val_best_strat[0])
    os.system("cp " + strat_name + " " + new_strat_name)

    logfile.write("cp " + strat_name + " " + new_strat_name + "\n")

    logfile.write("Running Simulation for test period\n")
    logfile.flush()
    logfile.close()
    run_sim_for_selected_strat(work_dir, shortcode, str(start_date), str(end_date),
                               testing_days_file, logfilename, using_grid)
    logfile = open(logfilename, 'a')

    mail_summary += "<p>SELECTED STRATS PERFORMANCE IN TESTING PERIOD" + "<br>"
    chosen_strat, _, _, strat_lines = summarize_results_and_choose(shortcode, work_dir, str(start_date),
                                                                   str(end_date), 1,
                                                                   logfilename,
                                                                   strat_list=selected_strats_dir, sort_algo=sort_algo,
                                                                   to_print=True,
                                                                   dates_file=testing_days_file)
    mail_summary += strat_lines + "</p>"

    chosen_strat = val_best_strat[0]
    print(chosen_strat)

    mail_summary_ranks = get_training_validation_rank(shortcode, work_dir, start_date, end_date,
                                                      chosen_strat, weights.shape[0], training_days_file,
                                                      validation_days_file,
                                                      testing_days_file, sort_algo='kCNAPnlSharpeAverage')
    mail_summary += mail_summary_ranks

    plot_pnl(chosen_strat, start_date, end_date, work_dir, shortcode, sort_algo)
    chosen_strat_file = os.path.join(picked_strats_dir, chosen_strat)
    logfile.write("Best Strat: " + chosen_strat + "\n")

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


def get_training_validation_rank(shortcode, work_dir, start_date, end_date,
                                 chosen_strat, num_of_strats, training_days_file, validation_days_file,
                                 testing_days_file, sort_algo='kCNAPnlSharpeAverage'):
    """

    :param shortcode: 
    :param work_dir: 
    :param start_date: 
    :param end_date: 
    :param chosen_strat: 
    :param training_days_file: 
    :param sort_algo: 
    :return: 
    """

    training_rank = get_rank_of_strat_in_time(shortcode, work_dir, str(start_date), str(end_date),
                                              chosen_strat, training_days_file, sort_algo=sort_algo)
    validation_rank = get_rank_of_strat_in_time(shortcode, work_dir, str(start_date), str(end_date),
                                                chosen_strat, validation_days_file, sort_algo=sort_algo)
    test_rank = get_rank_of_strat_in_time(shortcode, work_dir, str(start_date), str(end_date),
                                          chosen_strat, testing_days_file, sort_algo=sort_algo)

    o_rank = 1.0 * (validation_rank - training_rank) / num_of_strats
    print("TRAINING RANK: " + str(training_rank))
    print("VALIDATION RANK: " + str(validation_rank))
    print("OVERFITTING RANK: " + str(o_rank))

    mail_summary_temp = "<p><br>TRAINING RANK: " + str(training_rank) + "</br>"
    mail_summary_temp += "<br>VALIDATION RANK: " + str(validation_rank) + "</br>"
    mail_summary_temp += "<br>OVERFITTING RANK: " + str(o_rank) + "</br></p>"

    return mail_summary_temp
