
import os
import sys
import subprocess
import argparse

import pandas as pd
import numpy as np

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs
from walkforward.utils import date_utils


def get_index_for_features(feature_list_file):
    features_list = []
    with open(feature_list_file,'r') as f:
        for line in f:
            if not line.strip():
                continue
            line = line.strip()
            if line[0] == '#':
                continue

            features_list.append('-'.join(line))

    return features_list


def three_degree_events_dates(shortcode, start_date, num_days_to_compute_feature, st , et):

    cmd = "/home/dvctrader/basetrade/scripts/get_economic_events_for_product.pl " + shortcode + " " + str(start_date) + " " + str(num_days_to_compute_feature) + " " + st + " " + et
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, error = process.communicate()
    out = out.decode('utf-8').split("\n")
    date_array = []
    for line in out:
        words = line.split(" ")
        if len(words) > 4 and words[3] == "3" :
            date_array.append(words[0])
    date_array = list(set(date_array ))
    return date_array


def get_features_events_data_for_dates(shortcode, start_date, feature_list_file, num_days_to_compute_feature, st , et):

    avg_feature_val_cmd = [ '/home/dvctrader/basetrade_install/WKoDii/get_day_features.pl' ,shortcode,
                            str(start_date) ,str(num_days_to_compute_feature), feature_list_file, 'DAY', st, et]

    avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)

    y = avg_feature_out.communicate()[0].decode('utf-8')
    a = []
    for s in y.split('\n'):
        if (s != "" and s != " "):
            a.append(s.split())

    x = pd.DataFrame(a)
    y = x.T[1:]
    y.columns = list(map(str ,map(int ,x.T.iloc[0])))
    y = y.astype(float)

    features_index_list = get_index_for_features(feature_list_file)
    features_index_list.append('is_3_degree_event_present')

    three_degree_event_days_list =  three_degree_events_dates(shortcode, start_date, num_days_to_compute_feature, st , et)

    three_degree_event_on_day = []
    feature_dates = y.columns.tolist()
    for dt in feature_dates:
        if dt in three_degree_event_days_list:
            three_degree_event_on_day.append(1)
        else:
            three_degree_event_on_day.append(0)

    y.loc[y.shape[0]] = three_degree_event_on_day
    y.index = features_index_list

    return y

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('-shc', dest='shc', help="shortcode", type=str, required=True)
    parser.add_argument('-ff', dest='feature_file', help="path of file having a list of features. Sample rows are ZN_0 SSTREND , ZN_0 CORR NK_0", type=str, required=True)
    parser.add_argument('-ldays', dest='ldays', help="num days to lookback", type=str, required=True)
    parser.add_argument('-date', dest='end_date', help="end date", type=str, required=True)
    parser.add_argument('-st', dest='start_time', help="start time", type=str, required=True)
    parser.add_argument('-et', dest='end_time', help="end time", type=str, required=True)

    args = parser.parse_args()

    print(get_features_events_data_for_dates(args.shc, args.end_date, args.feature_file, args.ldays, args.st, args.et))
