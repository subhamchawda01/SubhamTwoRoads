import random
import time
import subprocess
import os
import pandas as pd
from utils import silent_remove
from get_pnl_stats_multiple import create_multiple_stats_from_content


def add_result_line(strat_name_, date_, result_line_, results_folder_, to_db):
    if not to_db:
        if len(result_line_) == 0:
            return
        shc_ = result_line_[0]
        strat_name = strat_name_ + "_" + shc_
        result_line_ = result_line_[1:]
        result_line_ = strat_name + " " + date_ + " " + ' '.join(str(v) for v in result_line_)
        result_directory_ = os.path.join(results_folder_, shc_, date_[0:4],
                                         date_[4:6], date_[6:8])
        if not os.path.exists(result_directory_):
            os.makedirs(result_directory_)

        results_file_ = os.path.join(result_directory_, "results_database.txt")

        if os.path.isfile(results_file_):
            try:
                results_ = pd.read_csv(results_file_, delim_whitespace=True,
                                       header=None, error_bad_lines=False)
                results_.set_index(0, inplace=True)
                results_.loc[strat_name] = result_line_.split()[1:]
                results_.to_csv(results_file_, sep=' ', encoding='utf-8', header=False)
            except Exception as e:
                with open(results_file_, "a") as text_file:
                    text_file.write('\n'+result_line_)
        else:
            with open(results_file_, "w") as text_file:
                text_file.write(result_line_)


def get_results_for_single_run(exec_, live_file_path_, date_, strat_name_, results_folder_):


    random.seed(time.time())
    runtime_id_ = random.randint(1000000, 9000000)
    trades_file_ = "/spare/local/logs/tradelogs/trades." + date_ + "." + str(runtime_id_)
    logs_file_ = "/spare/local/logs/tradelogs/log." + date_ + "." + str(runtime_id_)
    error_log_ = os.path.join(results_folder_, "error_log")

    try:
        output = subprocess.getoutput(' '.join([exec_, "SIM", live_file_path_, str(runtime_id_), date_]))
        output = subprocess.getoutput(' '.join(['egrep', 'ERROR', logs_file_]))
        output += subprocess.getoutput(' '.join(['egrep', 'HIT', logs_file_]))
        with open(error_log_, "a") as text_file:
            error_data_ = [strat_name_ + " " + date_ + " " + line for line in output.split('\n')
                           if "WRONG SIZES" not in line and "INVALID BOOK" not in line]
            text_file.write("\n")
            text_file.write('\n'.join(error_data_))
    except subprocess.CalledProcessError:
        return []
    except Exception as e:
        print(e)

    with open(trades_file_) as f:
        content_ = f.readlines()
        # you may also want to remove whitespace characters like `\n` at the end of each line
    content_ = [y.strip() for y in content_]

    pnl_stats_ = create_multiple_stats_from_content(content_)

    result_list_ = []

    for pnl_stat in pnl_stats_:
        result_list_.append([strat_name_, date_, pnl_stat, results_folder_, 0])

    silent_remove(trades_file_)
    silent_remove(logs_file_)

    return result_list_
