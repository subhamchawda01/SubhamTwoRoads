import os
import multiprocessing
import sys
import shlex
import subprocess
import time


def exec_bash(command):
    process = subprocess.Popen(command.split(), stdout=subprocess.PIPE)
    output = process.communicate()[0]
    return output


def GetSimPnlInFiles(simulation_bin_path_, mode, strat_file_path_, handle, args, days_array_, result_dir_):
    processes = set()
    max_processes_ = FindCoresToUse()
    print "Number of cores to use = " + str(max_processes_)
    for day in xrange(len(days_array_)):
        command = simulation_bin_path_ + " " + mode + " " + strat_file_path_ + \
            " " + handle + str(day) + " " + days_array_[day] + " " + args
        with open(result_dir_ + days_array_[day], "w") as outfile:
            processes.add(subprocess.Popen(command.split(), stdout=outfile))
        while len(processes) >= max_processes_:
            time.sleep(1)
            processes.difference_update([
                p for p in processes if p.poll() is not None])
    while len(processes) > 0:
        processes.difference_update([
            p for p in processes if p.poll() is not None])


def FindCoresToUse():
    num_cores_ = multiprocessing.cpu_count()
    max_core_use_ = num_cores_ / 3.0
    last_five_min_core_load_avg_ = os.getloadavg()[0] / num_cores_
    core_use_during_load_ = max(1, (1.3 - last_five_min_core_load_avg_) * num_cores_)
    cores_i_should_use = min(max_core_use_, core_use_during_load_)
    return int(cores_i_should_use)


def PrintArray(array):
    for i in xrange(len(array)):
        for j in xrange(len(array[0])):
            print array[i][j]


def InitializeArray(size):
    avg_pnl_array_ = []
    for i in xrange(size):
        avg_pnl_array_.append([0, 0, 0, 0, 0, 0])
    return avg_pnl_array_


def ReadFiles(location_of_files_, days_array_, size, avg_pnl_array_):
    for day in xrange(len(days_array_)):
        with open(location_of_files_ + days_array_[day], "r") as ins:
            day_pnl_array_ = []
            for line in ins:
                temp = line.split()
                if temp[0] == "SIMRESULT":
                    day_pnl_array_.append(temp)
            for i in xrange(size):
                for j in xrange(6):
                    avg_pnl_array_[i][j] += int(day_pnl_array_[i][j + 1])


def FindCommandsInAFile(location_of_files_, days_array_):
    day = days_array_[0]
    command = 0
    with open(location_of_files_ + day, "r") as ins:
        for line in ins:
            temp = line.split()
            if temp[0] == "SIMRESULT":
                command = command + 1
    return command


def GetAvgPnl(days_array_, location_of_files_):
    size = FindCommandsInAFile(location_of_files_, days_array_)
    avg_pnl_array_ = InitializeArray(size)
    ReadFiles(location_of_files_, days_array_, size, avg_pnl_array_)
    for i in xrange(len(avg_pnl_array_)):
        for j in xrange(len(avg_pnl_array_[0])):
            avg_pnl_array_[i][j] = avg_pnl_array_[i][j] / len(days_array_)
    return avg_pnl_array_


def PrintToFile(output, result_dir_):
    file_save_location_ = result_dir_ + "output_tmp"
    f1 = open(file_save_location_, 'w+')
    output_string_ = ""
    for i in xrange(len(output)):
        for j in xrange(len(output[0])):
            output_string_ += str(output[i][j]) + " "
        f1.write(output_string_ + "\n")


def MainFunc(arg_arr_):
    mode = arg_arr_[1]
    strat_file_path_ = arg_arr_[2]
    handle = arg_arr_[3]
    start_date_ = arg_arr_[4]
    other_args_ = ""
    for i in xrange(5, len(arg_arr_) - 3):
        other_args_ += arg_arr_[i] + " "
    result_dir_ = arg_arr_[len(arg_arr_) - 3]
    short_code_ = arg_arr_[len(arg_arr_) - 2]
    num_days_ = arg_arr_[len(arg_arr_) - 1]

    if not os.path.isdir(result_dir_ + "tmp/"):
        command = "mkdir " + result_dir_ + "tmp"
        os.system(command)

    days = exec_bash("/home/vdarda/basetrade/scripts/get_list_of_dates_for_shortcode.pl" +
                     " " + short_code_ + " " + start_date_ + " " + num_days_)
    business_days_ = days.split()
    GetSimPnlInFiles("/home/vdarda/basetrade_install/bin/sim_strategy", mode, strat_file_path_,
                     handle, other_args_, business_days_, result_dir_ + "tmp/")

    ans = GetAvgPnl(business_days_, result_dir_ + "tmp/")
    PrintToFile(ans, result_dir_)
    print ans
    print ""


arg_arr_ = sys.argv
if len(arg_arr_) < 10:
    print "Invalid Arguments"
    print "PYTHONSCRIPTPATH SIM STRATEGYDESCFILENAME PROGID TRADINGDATE ARGS_SAME_AS_SIM_STGY RESULT_DIR SHORTCODE NO_OF_DAYS"
    exit()

MainFunc(arg_arr_)


'''
days = exec_bash("/home/vdarda/basetrade/scripts/get_list_of_dates_for_shortcode.pl USD000UTSTOM 20151210 100")
business_days_ = days.split()

simulation_bin_path_ = "/home/vdarda/basetrade_install/bin/sim_strategy"
mode = "SIM"
strat_file_path_ = "/home/vdarda/regdata/models/strat1"
handle = "2222"
args = "ADD_DBG_CODE -1"
result_dir_ = "/home/vdarda/regdata/tmp/"
GetSimPnlInFiles(simulation_bin_path_, mode, strat_file_path_, handle, args, business_days_, result_dir_)
'''
