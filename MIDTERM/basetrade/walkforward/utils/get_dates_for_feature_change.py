import os
import sys
import argparse
import subprocess

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.utils.date_utils import calc_next_week_day
from walkforward.utils.date_utils import calc_prev_week_day
from walkforward.definitions import execs


def get_dates_for_feature_change(shortcode, start_date, end_date, start_time, end_time,
                                 feature, num_days_to_compute_feature, percentage_change):
    """
    Get the list of dates between start and end date where abs((Y(i) -
    Y(i-1))/Y(i-1)) > threshold(say 0.2). Where Y(i) is the feature(say
    STDEV) for given last days(say 20) from date X(i).

    :param shortcode: 
    :param start_date: 
    :param end_date: 
    :param start_time: 
    :param end_time: 
    :param feature: 
    :param num_days_to_compute_feature: 
    :param percentage_change: 
    :return: 
    """
    avg_feature_val_cmd = [execs.execs().avg_samples, shortcode, str(start_date), str(
        num_days_to_compute_feature), start_time, end_time, '0', feature]

    avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)
    y_prev = float(avg_feature_out.communicate()[0].decode('utf-8').strip().split()[-1])

    out_list = [int(start_date)]
    current_date = calc_next_week_day(start_date, 1)

    while int(current_date) <= int(end_date):
        avg_feature_val_cmd = [execs.execs().avg_samples, shortcode, str(current_date), str(
            num_days_to_compute_feature), start_time, end_time, '0', feature]

        avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)
        y_current = float(avg_feature_out.communicate()[0].decode('utf-8').strip().split()[-1])

        if y_prev != 0:
            if abs(1.0 * (y_current - y_prev) / y_prev) > float(percentage_change):
                out_list.append(current_date)
                y_prev = y_current
        else:
            if y_current != 0:
                out_list.append(current_date)
                y_prev = y_current

        current_date = calc_next_week_day(current_date, 1)

    return out_list


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument(dest='shortcode', help="Shortcode of Product", type=str)
    parser.add_argument(dest='start_date', help="Start Date", type=int)
    parser.add_argument(dest='end_date', help='End Date', type=int)
    parser.add_argument(dest='start_time', help='Start Time', type=str)
    parser.add_argument(dest='end_time', help='End Time', type=str)
    parser.add_argument(dest='feature', help='Feature like STDEV/VOL', default='STDEV', type=str)
    parser.add_argument(dest='num_days_to_compute_feature',
                        help='Num days for which feature value will be computed', default=20, type=int)
    parser.add_argument(dest='percentage_change',
                        help='Change in feature value for date to be chosen. Sample value like 0.1,0.2', default=0.2, type=float)

    args = parser.parse_args()
    out = get_dates_for_feature_change(args.shortcode, args.start_date, args.end_date, args.start_time,
                                       args.end_time, args.feature, args.num_days_to_compute_feature, args.percentage_change)
    print(out)
