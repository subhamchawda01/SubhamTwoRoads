#!/usr/bin/env python

import sys
import subprocess
import random
import heapq
import shlex
import os
from multiprocessing.dummy import Pool

all_paramvals_ = []
paramnames_ = []
visited = {}
idx_to_num_ = {}
all_results = {}
reverse_params = set(["DECREASE_KEEP", "DECREASE_PLACE", "EXPLICIT_MAX_LONG_UNIT_RATIO", "EXPLICIT_WORST_CASE_LONG_UNIT_RATIO", "HIGHPOS_DECREASE_FACTOR", "HIGHPOS_LIMITS", "HIGHPOS_LIMITS_UNIT_RATIO", "HIGHPOS_SIZE_FACTOR",
                      "HIGHPOS_THRESH_DECREASE", "MAX_POSITION", "MAX_POSITION_TO_AGGRESS_UNIT_RATIO", "MAX_POSITION_TO_AGGRESS", "MAX_UNIT_RATIO", "UNIT_TRADE_SIZE", "ZEROPOS_DECREASE_DIFF", "ZEROPOS_LIMITS_UNIT_RATIO"])

DIR = ''
SUMMARY_SCRIPT = os.environ['HOME'] + '/basetrade/ModelScripts/run_sim_and_summarise.pl'
LOG_FILE = ''
log_fp = ''
model_ = ''
shc_ = ''
start_time_ = ''
end_time_ = ''
strat_ = ''
comparisonFunction = 'kCNAPnlAverage'
cores = 16
max_iterations = -1

# metric for comparison between two stats values


def compare(x, y):
    global comparisonFunction
    if comparisonFunction == 'kCNAGainPainRatio':
        if y[19] - x[19] > 0:
            return 1
        elif y[19] == x[19]:
            return 0
        else:
            return -1
    elif comparisonFunction == 'kCNAPnlAdjAverage':
        if y[4] - x[4] > 0:
            return 1
        elif y[4] == x[4]:
            return 0
        else:
            return -1
    elif comparisonFunction == 'kCNAPnlSharpe':
        if y[3] - x[3] > 0:
            return 1
        elif y[3] == x[3]:
            return 0
        else:
            return -1
    elif comparisonFunction == 'kCNAPnlVol':
        if y[0] * y[2] - x[0] * x[2] > 0:
            return 1
        elif y[0] * y[2] == x[0] * x[2]:
            return 0
        else:
            return -1
    elif comparisonFunction == 'kCNAPnlByMaxloss':
        if y[20] - x[20] > 0:
            return 1
        elif y[20] == x[20]:
            return 0
        else:
            return -1
    elif comparisonFunction == 'kCNAPnlMedianAverage':
        if y[16] - x[16] > 0:
            return 1
        elif y[16] == x[16]:
            return 0
        else:
            return -1
    elif comparisonFunction == 'kCNADDAdjPnlAverage':
        if y[17] - x[17] > 0:
            return 1
        elif y[17] == x[17]:
            return 0
        else:
            return -1
    elif comparisonFunction == 'kCNAPnlSharpeVol':
        if y[2] * y[3] - x[2] * x[3] > 0:
            return 1
        elif y[2] * y[3] == x[2] * x[3]:
            return 0
        else:
            return -1
    else:
        if y[0] - x[0] > 0:
            return 1
        elif y[0] == x[0]:
            return 0
        else:
            return -1


def ReadParamFile(_filename_):
    global all_paramvals_
    global paramnames_
    global log_fp
    global max_iterations

    log_fp.write("\nReading parameters\n")
    total_ = 1

    with open(_filename_) as fp:
        lines = fp.readlines()
        for line in lines:
            line = line.strip().split()
            if len(line) < 3:
                continue
            paramnames_.append(line[1])
            paramvalues_ = [float(pv) for pv in line[2:]]
            if line[1] not in reverse_params:
                paramvalues_.sort()
            else:
                paramvalues_.sort(reverse=True)
            total_ = total_ * len(paramvalues_)
            all_paramvals_.append(paramvalues_)

    if max_iterations == -1:
        max_iterations = total_

    log_fp.write("Total permutations possible=" + str(total_) + '\n')

    return len(paramnames_)

# given a string a_0a_1..a_n it will create a param file with the ith param
#  having its entry as values[ai]


def CreateParamFile(_idx_string_):
    global all_paramvals_
    global idx_to_num_
    global DIR

    unique_num_ = random.randint(1000, 100000)
    while unique_num_ in idx_to_num_.values():
        unique_num_ = random.randint(1000, 100000)
    idx_to_num_[_idx_string_] = unique_num_
    param_file_ = DIR + 'param_' + str(unique_num_)
    log_fp.write(param_file_ + '\n')
    fp = open(param_file_, 'w+')
    for i in xrange(len(_idx_string_)):
        fp.write('PARAMVALUE ' + paramnames_[i] + ' ' + str(all_paramvals_[i][int(_idx_string_[i])]) + '\n')
    fp.close()
    return (param_file_, _idx_string_)


def CreateParamListAndStratFile(_idx_string_vec_):
    global visited
    global shc_
    global strat_
    global model_
    global start_time_
    global end_time_
    global all_paramvals_
    global idx_to_num_
    global DIR

    strat_filenames_ = []
    multiple_params_ = []

    for _idx_string_ in _idx_string_vec_:
        params = []
        param_filenames_ = []
        params_idx_ = []
        strat_filename_ = DIR + 'strat_' + _idx_string_

        for i in xrange(len(_idx_string_)):
            new_idx_string_ = (_idx_string_[0:i] + str(int(_idx_string_[i]) + 1) + _idx_string_[i + 1:]
                               ) if int(_idx_string_[i]) < len(all_paramvals_[i]) - 1 else _idx_string_
            if new_idx_string_ not in visited:
                visited[new_idx_string_] = True
                params.append(new_idx_string_)
                log_fp.write("\nFor " + strat_filename_ + " creating the following param files\n")
                (param_fn, param_cn) = CreateParamFile(new_idx_string_)
                log_fp.write("End of param file creation for " + strat_filename_ + "\n")
                param_filenames_.append(param_fn)
                params_idx_.append(param_cn)

        fp = open(strat_filename_, 'w+')
        for i in xrange(len(params)):
            fp.write('STRATEGYLINE ' + shc_ + ' ' + strat_ + ' ' + model_ + ' ' +
                     param_filenames_[i] + ' ' + start_time_ + ' ' + end_time_ + ' ' + str(idx_to_num_[params_idx_[i]]) + '\n')
        fp.close()
        strat_filenames_.append(strat_filename_)
        multiple_params_.append(params)

    results_ = RunSimAndSummarise(strat_filenames_)
    return [(multiple_params_[i], results_[i]) for i in xrange(len(results_))]


def ReturnProcessOutput(process):
    return process.communicate()[0].split('\n')[:-1]

# summarise the sim runs on the date range given


def RunSimAndSummarise(multiple_strat_files_):

    global shc_
    global start_date_
    global end_date_
    global SUMMARY_SCRIPT
    global log_fp

    for _strat_file_ in multiple_strat_files_:
        log_fp.write("\nSummarising on " + SUMMARY_SCRIPT + " " + shc_ + " " +
                     start_date_ + " " + end_date_ + " " + _strat_file_ + "\n")

    processes = [subprocess.Popen(shlex.split(SUMMARY_SCRIPT + " " + shc_ + " " + start_date_ + " " + end_date_ +
                                              " " + _strat_file_), stdout=subprocess.PIPE, bufsize=1) for _strat_file_ in multiple_strat_files_]
    try:
        outputs = Pool(len(processes)).map(ReturnProcessOutput, processes)
    except:
        log_fp.write("Result summary script malfunctioned.Exiting\n")
        print "Result summary script malfunctioned.Exiting\n"
        exit(1)

    for idx_ in xrange(len(multiple_strat_files_)):
        log_fp.write("Summary for " + multiple_strat_files_[idx_] + "\n" + '\n'.join(outputs[idx_]) + '\n')

    results = []
    for output in outputs:
        results.append([list(map(float, stat.split()[1:])) for stat in output])
    return results

# perform gradient descent


def ObtainBestParam(_start_idx_string_):
    global model_
    global start_time_
    global end_time_
    global idx_to_num_
    global DIR
    global max_iterations
    global cores

    strat_filename_ = DIR + 'strat_init'
    log_fp.write("\nFor " + strat_filename_ + " creating the following param files\n")
    (param_file_, param_idx_) = CreateParamFile(_start_idx_string_)
    log_fp.write("End of param file creation for " + strat_filename_ + "\n")
    fp = open(strat_filename_, 'w+')
    fp.write('STRATEGYLINE ' + shc_ + ' ' + strat_ + ' ' + model_ + ' ' + param_file_ +
             ' ' + start_time_ + ' ' + end_time_ + ' ' + str(idx_to_num_[param_idx_]) + '\n')
    fp.close()
    all_results[_start_idx_string_] = RunSimAndSummarise([strat_filename_])[0][0]

    best_result_ = all_results[_start_idx_string_]
    best_param_ = _start_idx_string_

    queue_ = []
    queue_.append(_start_idx_string_)
    visited[_start_idx_string_] = True

    last_5_min_core_normalised_avg = float(subprocess.Popen(
        "uptime | grep \"load average\" | awk '{print $11}'", shell=True, stdout=subprocess.PIPE).communicate()[0].rstrip('\n').rstrip(',')) / cores
    iterations = 0

    while len(queue_) > 0 and iterations < max_iterations:
        str_vec_ = []
        max_idx_in_parallel_ = max(1, int((1.3 - last_5_min_core_normalised_avg) * cores))
        for i in xrange(min(max_idx_in_parallel_, len(queue_))):
            top_str = queue_.pop(0)
            if compare(best_result_, all_results[top_str]) > 0:
                best_result_ = all_results[top_str]
                best_param_ = top_str
            str_vec_.append(top_str)

        multiple_children_ = CreateParamListAndStratFile(str_vec_)
        for (children_params_, children_results_) in multiple_children_:
            # compare the results,append to the dictionary of results and populate the queue
            if len(children_results_) > 0:
                # sort according to a custom function and get the top two values
                sorted_results_ = sorted(children_results_, cmp=compare)
                max_children_ = [children_results_.index(sorted_results_[i])
                                 for i in xrange(min(1, len(children_results_)))]
                filtered_children_ = filter(lambda x: compare(
                    children_results_[x], all_results[top_str]) <= 0, max_children_)
                queue_.extend([children_params_[child] for child in filtered_children_])
                for child_ in filtered_children_:
                    all_results[children_params_[child_]] = children_results_[child_]
        iterations = iterations + 1

    return (best_result_, best_param_)


def _main_():

    global shc_
    global strat_
    global model_
    global param_filename_
    global start_time_
    global end_time_
    global start_date_
    global end_date_
    global DIR
    global LOG_FILE
    global comparisonFunction
    global log_fp
    global all_results
    global all_paramvals_
    global cores

    if len(sys.argv) < 9:
        print "Usage:<script><shortcode><strategy><modelfile><paramfile><start time><end time><start date><end date>[comparison function][max_iterations]"
        exit(1)

    shc_ = sys.argv[1]
    strat_ = sys.argv[2]
    model_ = sys.argv[3]
    param_filename_ = sys.argv[4]
    start_time_ = sys.argv[5]
    end_time_ = sys.argv[6]
    start_date_ = sys.argv[7]
    end_date_ = sys.argv[8]

    if len(sys.argv) > 9:
        comparisonFunction = sys.argv[9]

    if len(sys.argv) > 10:
        max_iterations = max(1, int(sys.argv[10]))

    folder_uid_ = str(random.randint(1000, 100000))
    DIR = '/spare/local/' + os.environ['USER'] + '/FBPA/' + shc_ + '/' + folder_uid_ + '/'
    if not os.path.exists(DIR):
        os.makedirs(DIR)
    LOG_FILE = DIR + '/main_log_file.txt'
    print "Log file: ", LOG_FILE

    log_fp = open(LOG_FILE, 'w+')
    number_of_params_ = ReadParamFile(param_filename_)

    cores = float(subprocess.Popen("nproc", shell=True, stdout=subprocess.PIPE).communicate()[0].rstrip('\n'))
    start_idx_string_ = ''.join(['0' for i in xrange(number_of_params_)])
    (best_result_, best_param_) = ObtainBestParam(start_idx_string_)

    print "Summary of results on original params"
    print ' '.join([str(st) for st in all_results[start_idx_string_]])
    print "Summary of results on the discovered params"
    print ' '.join([str(st) for st in best_result_])
    print "Improved params"
    for i in xrange(len(best_param_)):
        print 'PARAMVALUE', paramnames_[i], str(all_paramvals_[i][int(best_param_[i])])

    # write the to the log file as well
    log_fp.write("Summary of results on original params\n")
    log_fp.write(' '.join([str(st) for st in all_results[start_idx_string_]]) + '\n')
    log_fp.write("Summary of results on the discovered params\n")
    log_fp.write(' '.join([str(st) for st in best_result_]) + '\n')
    log_fp.write("Improved params\n")
    for i in xrange(len(best_param_)):
        log_fp.write('PARAMVALUE ' + paramnames_[i] + ' ' + str(all_paramvals_[i][int(best_param_[i])]) + '\n')
    log_fp.close()


_main_()
