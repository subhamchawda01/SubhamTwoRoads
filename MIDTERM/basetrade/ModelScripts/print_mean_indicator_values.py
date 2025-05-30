#!/usr/bin/python

import os
import sys
import random
import commands


def GetTimeInSec(time_):
    if (time_ > 2400):
        return time_ % 86400
    else:
        hh_ = time_ / 100
        mm_ = time_ % 100
        return hh_ * 3600 + mm_ * 60


def GetTimeInMicroSec(time_):
    sec_ = long(time_.split('.')[0])
    usec_ = long(time_.split('.')[1])
    return sec_ * 1000000 + usec_


USAGE = "SCRIPT STRAT_FILE DATE START_TIME END_TIME"

if (len(sys.argv) < 5):
    print USAGE
    exit(0)

strat_file_ = sys.argv[1]
date_ = sys.argv[2]
start_time_sec_ = GetTimeInSec(int(sys.argv[3]))
end_time_sec_ = GetTimeInSec(int(sys.argv[4]))
unique_id_ = random.randrange(100000, 200000)

# Running SIM strategy with ADD_DBG_CODE and TRADING_INFO
exec_cmd_ = '/home/dvctrader/LiveExec/bin/sim_strategy SIM ' + strat_file_ + \
    ' ' + str(unique_id_) + ' ' + date_ + ' 0 0 0.0 0 ADD_DBG_CODE TRADING_INFO'
print exec_cmd_
output_ = commands.getoutput(exec_cmd_)

# Log file path
log_file_ = '/spare/local/logs/tradelogs/log.' + date_ + '.' + str(unique_id_)
print log_file_

log_file_handle_ = open(log_file_, 'r')
log_file_lines_ = log_file_handle_.readlines()

previous_time_microsec_ = 0
current_time_microsec_ = 0
first_recorded_time_microsec_ = 10000000000000000
last_recorded_time_microsec_ = 0
indicator_to_cumulative_value_ = {}
indicator_to_previous_value_ = {}

total_lines_ = len(log_file_lines_)
iter_ = 0

while (iter_ < total_lines_):
    line_ = log_file_lines_[iter_]

    if ('ShowIndicatorValues' in line_):
        current_time_ = line_.split(' ')[0]  # 1418110227.278627
        previous_time_microsec_ = current_time_microsec_
        current_time_microsec_ = GetTimeInMicroSec(current_time_)  # 1418110227278627
        current_time_sec_ = int(current_time_.split('.')[0]) % 86400
        if (current_time_sec_ < start_time_sec_):
            iter_ += 1
            continue
        if (current_time_sec_ > end_time_sec_):
            break

        # This timestamp is within (start_time_sec_, end_time_sec_)
        if (indicator_to_previous_value_):
            for indicator_ in indicator_to_previous_value_.keys():
                if (indicator_ not in indicator_to_cumulative_value_.keys()):
                    #indicator_to_cumulative_value_[indicator_] = [indicator_to_previous_value_[indicator_]]
                    indicator_to_cumulative_value_[indicator_] = indicator_to_previous_value_[
                        indicator_] * (current_time_microsec_ - previous_time_microsec_)
                else:
                    # indicator_to_cumulative_value_[indicator_].append(indicator_to_previous_value_[indicator_])
                    indicator_to_cumulative_value_[
                        indicator_] += indicator_to_previous_value_[indicator_] * (current_time_microsec_ - previous_time_microsec_)
            last_recorded_time_microsec_ = current_time_microsec_
            indicator_to_previous_value_ = {}

        # Skip ================
        iter_ += 2

        line_ = log_file_lines_[iter_]

        if ('Current Regime:' in line_):  # Regime model
            current_regime_ = int(line_.split(' ')[3])
            # print current_regime_
            iter_ += 2
            while (iter_ < total_lines_):
                line_ = log_file_lines_[iter_]
                if ('ShowIndicatorValues' in line_):
                    iter_ -= 1
                    break

                if ('MODEL:' in line_):
                    model_idx_ = int(line_.split(' ')[1])

                if ('value:' in line_ and model_idx_ + 1 == current_regime_):
                    line_parts_ = line_.split(' of ')
                    value_part_ = line_parts_[0]
                    indicator_ = line_parts_[1][:-1]
                    value_ = float(value_part_.split(' ')[2])
                    indicator_to_previous_value_[indicator_] = value_
                    first_recorded_time_microsec_ = min(first_recorded_time_microsec_, current_time_microsec_)

                iter_ += 1

        else:  # Normal model
            while (iter_ < total_lines_):
                line_ = log_file_lines_[iter_]
                if ('=====' in line_):
                    break
                if ('sum_vars' in line_):
                    iter_ += 1
                    continue
                line_parts_ = line_.split(' of ')
                value_part_ = line_parts_[0]
                indicator_ = line_parts_[1][:-1]
                value_ = float(value_part_.split(' ')[2])
                indicator_to_previous_value_[indicator_] = value_
                first_recorded_time_microsec_ = min(first_recorded_time_microsec_, current_time_microsec_)
                iter_ += 1
    iter_ += 1


MAX_INDICATOR_LEN = 80

for indicator_name_ in indicator_to_cumulative_value_.keys():
    cumulative_value_ = indicator_to_cumulative_value_[indicator_name_]
    #print (last_recorded_time_microsec_ - first_recorded_time_microsec_)
    avg_value_ = cumulative_value_ / (last_recorded_time_microsec_ - first_recorded_time_microsec_)
    #avg_value_ = sum(cumulative_value_)/len(cumulative_value_)
    if (len(indicator_name_) < MAX_INDICATOR_LEN):
        indicator_name_ = indicator_name_ + ' ' * (MAX_INDICATOR_LEN - len(indicator_name_))
    else:
        indicator_name_ = indicator_name_[0:MAX_INDICATOR_LEN]
    print indicator_name_ + ': ' + str(avg_value_)

output_ = commands.getoutput('rm ' + log_file_)
