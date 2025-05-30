#!/usr/bin/env python

import os
import sys
import subprocess
import numpy as np
import pandas as pd
import math
import argparse
import time

sys.path.append(os.path.expanduser('~/basetrade'))
from sklearn import linear_model
from walkforward.definitions.execs import execs
from scipy import optimize

pd.set_option('display.max_rows', 500)
pd.set_option('display.max_columns', 500)
pd.set_option('display.width', 1000)


def generate_data(work_dir, features_list, shortcode, start_time, end_time, end_date, num_days,
                  prev_session_start_time=None, prev_session_end_time=None):
    features_file = os.path.join(work_dir, "features_file")
    events_file = os.path.join(work_dir, "events_file")
    sample_data_file = os.path.join(work_dir, "sample_data")
    prev_sample_data_file = os.path.join(work_dir, "prev_sample_data")

    with open(features_file, 'w') as handle:
        for feat in features_list:
            handle.write(feat)
            handle.write("\n")

    # Generate data for events
    events_command = "%s %s %s %s %s %s > %s" % (execs.get_economic_events_for_product,
                                                 shortcode, end_date, num_days, start_time, end_time, events_file)
    os.system(events_command)

    # Generate sample data for features
    sample_data_command = "%s %s %s %s %s SAMPLE %s %s | tr '_' ' ' > %s" % (execs.get_day_features, shortcode,
                                                                             end_date, num_days, features_file,
                                                                             start_time, end_time, sample_data_file)
    os.system(sample_data_command)

    # Generate sample data for previous session
    if prev_session_start_time is not None and prev_session_end_time is not None:
        sample_data_command = "%s %s %s %s %s SAMPLE %s %s | tr '_' ' ' > %s" % (execs.get_day_features, shortcode,
                                                                                 end_date, num_days, features_file,
                                                                                 prev_session_start_time,
                                                                                 prev_session_end_time,
                                                                                 prev_sample_data_file)
        os.system(sample_data_command)

    return features_file, events_file, sample_data_file, prev_sample_data_file


def load_data(features_list, events_file, sample_data_file, prev_sample_data_file):
    events_data = pd.DataFrame(columns=["Date", "Time", "Flat Secs", "Severity", "EZ", "Event"])
    if os.path.getsize(events_file) > 0:
        events_data = pd.read_csv(events_file, sep=" ", header=None)
        events_data.columns = ["Date", "Time", "Flat Secs", "Severity", "EZ", "Event"]

    sample_data = pd.read_csv(sample_data_file, sep=" ", header=None)
    sample_data.drop([sample_data.shape[1] - 1], inplace=True, axis=1)
    sample_data.columns = ["Date", "Slot"] + features_list

    prev_sample_data = None
    if os.path.exists(prev_sample_data_file) and os.path.getsize(prev_sample_data_file) > 0:
        prev_sample_data = pd.read_csv(prev_sample_data_file, sep=" ", header=None)
        prev_sample_data.drop([prev_sample_data.shape[1] - 1], inplace=True, axis=1)
        prev_sample_data.columns = sample_data.columns

    return events_data, sample_data, prev_sample_data


def get_event_stats(event, sample_data, events_data, shortcode):
    event_data = events_data.loc[(events_data['EZ'] == event[0])]
    event_data = event_data.loc[(event_data['Event'] == event[1])]
    dates = np.intersect1d(event_data['Date'], sample_data['Date'])
    event_data = event_data.loc[event_data["Date"].isin(dates)]
    event_relevant_data = None
    stdev_feature = shortcode + " STDEV"
    vol_feature = shortcode + " VOL"
    headers = [stdev_feature + " -1", stdev_feature + " +1", stdev_feature + " +2", stdev_feature + " +3",
               vol_feature + " -1", vol_feature + " +1", vol_feature + " +2", vol_feature + " +3"]
    for date in dates:
        event_time = (event_data.loc[event_data["Date"] == date]["Time"])
        dated_sample_data = sample_data.loc[sample_data["Date"] == date]
        mins = event_time % 100 + 60 * math.floor(event_time / 100)
        slot = math.floor(mins / 15) + 1
        dated_sample_data_before = dated_sample_data.loc[dated_sample_data["Slot"] == slot - 1]
        if dated_sample_data_before.shape[0] == 0:
            feature_dict = {stdev_feature + " -1": 0.0,
                            vol_feature + " -1": 0.0}
        else:
            feature_dict = {stdev_feature + " -1": dated_sample_data_before[stdev_feature].as_matrix()[0],
                            vol_feature + " -1": dated_sample_data_before[vol_feature].as_matrix()[0]}
        dated_sample_data_after = dated_sample_data.loc[dated_sample_data["Slot"] > slot]
        size = min(dated_sample_data_after.shape[0], 3)
        if size == 0:
            continue

        for sl in range(size):
            if dated_sample_data_after.loc[dated_sample_data["Slot"] == slot + sl + 1].shape[0] == 0:
                continue
            feature_dict[stdev_feature + " +" + str(sl + 1)] = \
                dated_sample_data_after.loc[dated_sample_data["Slot"] == slot + sl + 1][stdev_feature].as_matrix()[0]
            feature_dict[vol_feature + " +" + str(sl + 1)] = \
                dated_sample_data_after.loc[dated_sample_data["Slot"] == slot + sl + 1][vol_feature].as_matrix()[0]

        relevant_data = pd.DataFrame(feature_dict, index=[0])
        if event_relevant_data is None:
            event_relevant_data = relevant_data
        else:
            event_relevant_data = pd.concat([event_relevant_data, relevant_data])
    if event_relevant_data is not None:
        final_matrix = event_relevant_data.mean(axis=0, skipna=True).as_matrix().T.reshape((1, -1))
        final_df = pd.DataFrame(final_matrix, columns=event_relevant_data.columns)
    else:
        final_matrix = np.array([0, 0, 0, 0, 0, 0, 0, 0])
        final_df = pd.DataFrame(final_matrix.reshape((1, -1)), columns=headers, index=[0])
    return final_df


def get_event_data_samples(all_events, events_scales, events_data, shortcode):
    index = 0
    stdev_feature = shortcode + " STDEV"
    vol_feature = shortcode + " VOL"

    event_data_samples = []

    for indx, ev in all_events.iterrows():
        event = [ev["EZ"], ev["Event"]]

        event_data = events_data.loc[(events_data['EZ'] == event[0])]
        event_data = event_data.loc[(event_data['Event'] == event[1])]
        scale = events_scales.iloc[[index]]
        event_time = event_data[["Date", "Time"]].as_matrix()

        hrs = list(map(math.floor, (event_time[:, 1] / 100) * 60))
        mins = [x % 100 for x in event_time[:, 1]]
        slots = [math.floor(sum(x) / 15) for x in zip(hrs, mins)]
        slots = [x + 1 for x in slots]
        event_time[:, 1] = slots

        slot1_stdev_change = scale[stdev_feature + " +1"] - scale[stdev_feature + " -1"]
        slot1_vol_change = scale[vol_feature + " +1"] - scale[vol_feature + " -1"]
        event_slot1_change = [slot1_stdev_change.as_matrix()[0], slot1_vol_change.as_matrix()[0]]
        event_slot1_change = np.array(event_slot1_change).reshape((1, -1))
        ev1 = np.hstack((event_time, np.repeat(np.array(event_slot1_change), event_time.shape[0], axis=0)))
        event_data_samples.extend(ev1)

        event_time[:, 1] = event_time[:, 1] + 1
        slot2_stdev_change = scale[stdev_feature + " +2"] - scale[stdev_feature + " -1"]
        slot2_vol_change = scale[vol_feature + " +2"] - scale[vol_feature + " -1"]
        event_slot2_change = [slot2_stdev_change.as_matrix()[0], slot2_vol_change.as_matrix()[0]]
        event_slot2_change = np.array(event_slot2_change).reshape((1, -1))
        ev2 = np.hstack((event_time, np.repeat(np.array(event_slot2_change), event_time.shape[0], axis=0)))
        event_data_samples.extend(ev2)

        event_time[:, 1] = event_time[:, 1] + 1
        slot3_stdev_change = scale[stdev_feature + " +3"] - scale[stdev_feature + " -1"]
        slot3_vol_change = scale[vol_feature + " +3"] - scale[vol_feature + " -1"]
        event_slot3_change = [slot3_stdev_change.as_matrix()[0], slot3_vol_change.as_matrix()[0]]
        event_slot3_change = np.array(event_slot3_change).reshape((1, -1))
        ev3 = np.hstack((event_time, np.repeat(np.array(event_slot3_change), event_time.shape[0], axis=0)))
        event_data_samples.extend(ev3)

        index += 1

    event_data_samples = np.array(event_data_samples)
    col_headers = ["Date", "Slot", "EVENTSTDEV", "EVENTVOL"]
    if event_data_samples.shape[0] == 0:
        event_data_samples = pd.DataFrame(columns=col_headers)
        return event_data_samples
    index = [str(i) for i in range(event_data_samples.shape[0])]
    event_data_samples = pd.DataFrame(event_data_samples, columns=col_headers, index=index)

    event_data_samples = event_data_samples.sort_values(["Date", "Slot"])

    event_data_samples = event_data_samples.groupby(["Date", "Slot"],
                                                    as_index=False)[["EVENTSTDEV", "EVENTVOL"]].max()
    return event_data_samples


def get_all_events_stats_data(events_data, sample_data, shortcode):
    all_events = events_data[["EZ", "Event"]].drop_duplicates()
    events_scales = pd.DataFrame()
    for indx, ev in all_events.iterrows():
        event = [ev["EZ"], ev["Event"]]
        event_scale_data = get_event_stats(event, sample_data, events_data, shortcode)
        if events_scales is None:
            events_scales = event_scale_data
        else:
            events_scales = pd.concat([events_scales, event_scale_data])
    events_scales.fillna(0, inplace=True)
    event_data_samples = get_event_data_samples(all_events, events_scales, events_data, shortcode)
    return event_data_samples


def get_df_from_model(work_dir, model_file, shortcode, start_time, end_time, end_date, num_days, prev_session_start_time=None,
                      prev_session_end_time=None, last_day_prev_session=False):
    weight_feature_map, features, target_feature = get_features_weights_from_model_file(model_file)
    features_list = [x for x in set(features)]

    features_file, events_file, sample_data_file, prev_sample_data_file = generate_data(work_dir, features_list,
                                                                                        shortcode, start_time, end_time,
                                                                                        end_date, num_days,
                                                                                        prev_session_start_time,
                                                                                        prev_session_end_time)
    events_data, sample_data, prev_sample_data = load_data(
        features_list, events_file, sample_data_file, prev_sample_data_file)

    use_event = False
    if ("EVENTSTDEV" in weight_feature_map or "EVENTVOL" in weight_feature_map) and \
            (weight_feature_map["EVENTSTDEV"] != 0 or weight_feature_map["EVENTVOL"] != 0):
        use_event = True
    train_data_df = generate_all_training_data(
        shortcode, sample_data, events_data, prev_sample_data, last_day_prev_session, target_feature, use_event)

    return weight_feature_map, train_data_df


def get_features_weights_from_model_file(model_file):
    features = []
    weights_feature_map = {}
    target_feature = ""
    f = open(model_file, 'r')
    for line in f:
        tokens = line.split()
        if len(tokens) == 0:
            continue
        if tokens[1] == "EVENTSTDEV":
            weights_feature_map["EventStdev"] = float(tokens[0])
        elif tokens[1] == "EVENTVOL":
            weights_feature_map["EventVol"] = float(tokens[0])
        elif tokens[1] == "INTERCEPT":
            weights_feature_map["Intercept"] = float(tokens[0])
        elif tokens[0] == "TARGET":
            target_feature = " ".join(tokens[1:])
        elif tokens[0] in ["START_TIME", "END_TIME", "PREVSESSION_0", "PREVSESSION_1", "PREV_SESS_START_TIME",
                           "PREV_SESS_END_TIME"]:
            continue
        else:
            if tokens[1]=="PREVSESSION_0" or tokens[1]=="PREVSESSION_1":
                weights_feature_map["PREVSESSION " + " ".join(tokens[2:])] = float(tokens[0])
            else:
                weights_feature_map[" ".join(tokens[1:])] = float(tokens[0])
            features.append(" ".join(tokens[2:]))
    f.close()
    return weights_feature_map, features, target_feature


def generate_all_training_data(shortcode, sample_data, events_data, prev_sample_data, last_day_prev_session,
                               target_feature, use_event=False):
    if use_event:
        event_data_samples = get_all_events_stats_data(events_data, sample_data, shortcode)

    dates = np.sort(np.unique(sample_data["Date"].as_matrix()))
    features = sample_data.as_matrix()[:, 2:]
    prev_session_features = None
    last_20days_prev_session_features = None
    if prev_sample_data is not None:
        prev_session_features = prev_sample_data.as_matrix()[:, 2:]
        prev_session_features = np.mean(prev_session_features, axis=0)
        last_20days_prev_session_features = np.repeat(prev_session_features.reshape((1, -1)), 20, axis=0)

    last_20days_target_feature = np.repeat(np.mean(sample_data[target_feature].as_matrix(), axis=0).reshape((1, -1)),
                                           20, axis=0)

    last_date_features = np.mean(features, axis=0)
    last_5days_features = np.repeat(last_date_features.reshape((1, -1)), 5, axis=0)
    last_20days_features = np.repeat(last_date_features.reshape((1, -1)), 20, axis=0)
    datewise_training_data = []

    last_date = None

    for date in dates:
        dated_sample_data = sample_data.loc[sample_data["Date"] == date].iloc[:, 2:].as_matrix()

        target_feature_value = np.mean(sample_data.loc[sample_data["Date"] == date][target_feature].as_matrix(), axis=0)
        last_20days_target_feature = np.vstack((last_20days_target_feature, target_feature_value))
        target_feature_value /= np.mean(last_20days_target_feature[:20], axis=0)
        last_20days_target_feature = last_20days_target_feature[1:]

        dated_event_data = None
        if use_event:
            dated_event_data = event_data_samples.loc[event_data_samples["Date"] == date]
        if (not use_event) or dated_event_data.shape[0] == 0:
            dated_event_data = np.array([0, 0])
        else:
            dated_event_data = np.mean(dated_event_data[["EVENTSTDEV", "EVENTVOL"]].as_matrix(), axis=0)

        if prev_sample_data is not None:
            if last_day_prev_session and last_date is None:
                prev_session_features = prev_session_features
            elif last_day_prev_session and last_date is not None:
                prev_dated_sample_data_matrix = prev_sample_data.loc[prev_sample_data["Date"] == last_date].iloc[:,
                                                                                                                 2:].as_matrix()
                if prev_dated_sample_data_matrix.shape[0] == 0:
                    prev_session_features = np.zeros(len(sample_data.columns[2:]))
                else:
                    prev_session_features = np.mean(prev_dated_sample_data_matrix, axis=0)
            else:
                prev_dated_sample_data_matrix = prev_sample_data.loc[prev_sample_data["Date"] == date].iloc[:,
                                                                                                            2:].as_matrix()
                if prev_dated_sample_data_matrix.shape[0] == 0:
                    prev_session_features = np.zeros(len(sample_data.columns[2:]))
                else:
                    prev_session_features = np.mean(prev_dated_sample_data_matrix, axis=0)

        last_20days_features_avg = np.mean(last_20days_features, axis=0)
        if prev_sample_data is not None:
            last_20days_prev_session_features_avg = np.mean(last_20days_prev_session_features, axis=0)
            all_data = np.hstack((date, target_feature_value, last_date_features / last_20days_features_avg,
                                  np.mean(last_5days_features, axis=0) / last_20days_features_avg,
                                  dated_event_data, prev_session_features / last_20days_prev_session_features_avg))
            last_20days_prev_session_features = np.vstack((last_20days_prev_session_features[1:, :],
                                                           prev_session_features))
        else:
            all_data = np.hstack((date, target_feature_value, last_date_features / last_20days_features_avg,
                                  np.mean(last_5days_features, axis=0) / last_20days_features_avg,
                                  dated_event_data))
        datewise_training_data.append(all_data)

        last_date_features = np.mean(dated_sample_data, axis=0)
        last_5days_features = np.vstack((last_5days_features[1:, :], last_date_features))
        last_20days_features = np.vstack((last_20days_features[1:, :], last_date_features))
        last_date = date

    datewise_training_data = np.array(datewise_training_data)
    train_data_columns = ["Date", "Target Volatility"] + ["LASTDAY " + x for x in sample_data.columns[2:]] + [
        "LAST5DAYS " + x for x in sample_data.columns[2:]] + ["EVENTSTDEV", "EVENTVOL"]
    if prev_sample_data is not None:
        train_data_columns += ["PREVSESSION " + x for x in sample_data.columns[2:]]
    index = [str(i) for i in range(datewise_training_data.shape[0])]
    train_data_df = pd.DataFrame(datewise_training_data, columns=train_data_columns, index=index)
    train_data_df.fillna(0, inplace=True)

    return train_data_df


def train_model(work_dir, train_data_df, paramset, use_lasso=False, only_pos=True, split=0.7):
    y = train_data_df["Target Volatility"].as_matrix()
    x = train_data_df.iloc[:, 2:].as_matrix()

    train_y = y[:int(split * len(y))]
    test_y = y[int(split * len(y)):]
    train_x = x[:int(split * len(x)), :]
    test_x = x[int(split * len(x)):, :]

    zero_w = []

    if use_lasso:
        clf = linear_model.Lasso(alpha=0.2)
        clf.fit(train_x, train_y)
        weights = clf.coef_
        for indx, w in enumerate(weights):
            if w == 0:
                zero_w.append(indx)

    non_zero_w = []
    for indx in range(len(train_data_df.columns[2:])):
        if indx in zero_w:
            continue
        non_zero_w.append(indx)
    non_zero_w = [w + 2 for w in non_zero_w]

    x = train_data_df[train_data_df.columns[non_zero_w]].as_matrix()
    train_x = x[:int(split * len(x)), :]
    test_x = x[int(split * len(x)):, :]

    weights = []
    intercept = 0
    if only_pos:
        weights, _ = optimize.nnls(np.hstack((train_x, np.ones((train_x.shape[0], 1)))), train_y)
        intercept = weights[-1]
        weights = weights[:-1]
    else:
        clf = linear_model.Ridge(alpha=0.2)
        clf.fit(train_x, train_y)
        weights = clf.coef_
        intercept = clf.intercept_

    print("Train Correlation: ", np.corrcoef(intercept + np.dot(train_x, weights), train_y)[1][0])
    print("Test Correlation: ", np.corrcoef(intercept + np.dot(test_x, weights), test_y)[1][0])
    print("\n")

    zi = 0
    wi = 0
    final_weights = []
    for indx in range(len(train_data_df.columns[2:])):
        if len(zero_w) > 0 and zero_w[zi] == indx:
            final_weights.append(0)
            zi += 1
        else:
            final_weights.append(weights[wi])
            wi += 1

    weight_features = zip(final_weights, train_data_df.columns[2:])
    trained_model_file = os.path.join(work_dir, "trained_model_file")
    print("Trained model stored in " + trained_model_file)
    writer = open(trained_model_file, 'w')
    writer.write("TARGET " + target_feature + "\n")
    writer.write("START_TIME " + paramset["start_time"] + "\n")
    writer.write("END_TIME " + paramset["end_time"] + "\n")
    if paramset["prev_session_start_time"] is not None:
        writer.write("PREV_SESS_START_TIME " + paramset["prev_session_start_time"] + "\n")
    if paramset["prev_session_end_time"] is not None:
        writer.write("PREV_SESS_END_TIME " + paramset["prev_session_end_time"] + "\n")

    for w, f in weight_features:
        if w == 0:
            continue
        if f.split()[0] == "PREVSESSION":
            if paramset["last_day_prev_session"]:
                writer.write(str(w) + " " + f.split()[0]+"_1" + " " + " ".join(f.split()[1:]) + "\n")
            else:
                writer.write(str(w) + " " + f.split()[0] + "_0" + " " + " ".join(f.split()[1:]) + "\n")
        else:
            writer.write(str(w) + " " + f + "\n")
    writer.write(str(intercept) + " INTERCEPT\n")
    writer.close()
    print(open(trained_model_file, 'r').read())
    return final_weights, intercept


def get_pnl_stats_on_dates(strat_file, selected_dates, work_dir, shortcode, results_dir, dates):
    summarize_exec = execs.summarize_strategy
    dates_file = os.path.join(work_dir, "dates")
    with open(dates_file, 'w') as handle:
        for date in selected_dates:
            handle.write(str(date))
            handle.write("\n")
    summarize_command = list(map(str, [summarize_exec, shortcode, strat_file, results_dir, dates[0], dates[-1], "IF",
                                       "kCNAPnlSharpeAverage", "0", dates_file, "1"]))

    proc = subprocess.Popen(" ".join(summarize_command), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc.communicate()
    if out is not None:
        out = out.decode('utf-8')
    results = []
    for line in out.splitlines():
        sr = []
        tokens = line.strip().split()
        if len(tokens) == 0:
            continue
        sr.append(tokens[1])
        sr.append(tokens[2])
        sr.append(tokens[4])
        sr.append(tokens[5])
        sr.append(tokens[21])
        sr.append(len(selected_dates))
        results.append(sr)

    return results


def get_split_results(strat_file, high_stdev_dates, low_stdev_dates, work_dir, shortcode, results_dir, dates):
    high_stdev_results = get_pnl_stats_on_dates(strat_file, high_stdev_dates, work_dir, shortcode, results_dir, dates)
    high_stdev_results = pd.DataFrame(high_stdev_results,
                                      columns=["Strat", "Avg Pnl(HV)", "Avg Vol(HV)", "Sharpe(HV)", "Pnl/ML(HV)",
                                               "Dates(HV)"], index=[str(i) for i in range(len(high_stdev_results))])

    low_stdev_results = get_pnl_stats_on_dates(strat_file, low_stdev_dates, work_dir, shortcode, results_dir, dates)
    low_stdev_results = pd.DataFrame(low_stdev_results,
                                     columns=["Strat", "Avg Pnl(LV)", "Avg Vol(LV)", "Sharpe(LV)", "Pnl/ML(LV)",
                                              "Dates(LV)"], index=[str(i) for i in range(len(low_stdev_results))])

    return high_stdev_results.join(low_stdev_results.set_index('Strat'), on='Strat').sort_values(["Strat"])


def get_results_on_threshold(shortcode, work_dir, strat_file, results_dir, dates, train_data_df, weights, intercept,
                             threshold, dates_file=None, split=0.7):
    train_dates = dates[:int(split * len(dates))]
    test_dates = dates[int(split * len(dates)):]

    y = train_data_df["Target Volatility"].as_matrix()
    x = train_data_df.iloc[:, 2:].as_matrix()

    train_y = y[:int(split * len(y))]
    test_y = y[int(split * len(y)):]
    train_x = x[:int(split * len(x)), :]
    test_x = x[int(split * len(x)):, :]

    predicted_train_volatility = intercept + np.dot(train_x, weights)  # clf.predict(train_x)
    stdev_threshold = np.percentile(predicted_train_volatility, threshold)
    predicted_high_stdev_train_dates = train_dates[predicted_train_volatility[:] >= stdev_threshold]
    predicted_low_stdev_train_dates = train_dates[predicted_train_volatility[:] < stdev_threshold]

    predicted_test_volatility = intercept + np.dot(test_x, weights)
    predicted_high_stdev_test_dates = test_dates[predicted_test_volatility[:] >= stdev_threshold]
    predicted_low_stdev_test_dates = test_dates[predicted_test_volatility[:] < stdev_threshold]

    predicted_train_df = get_split_results(strat_file, predicted_high_stdev_train_dates,
                                           predicted_low_stdev_train_dates, work_dir, shortcode, results_dir,
                                           train_dates)
    predicted_test_df = get_split_results(strat_file, predicted_high_stdev_test_dates, predicted_low_stdev_test_dates,
                                          work_dir, shortcode, results_dir, test_dates)

    actual_stdev_threshold = np.percentile(train_y, threshold)
    actual_train_high_stdev_dates = train_dates[train_y[:] >= actual_stdev_threshold]
    actual_train_low_stdev_dates = train_dates[train_y[:] < actual_stdev_threshold]
    actual_test_high_stdev_dates = test_dates[test_y[:] >= actual_stdev_threshold]
    actual_test_low_stdev_dates = test_dates[test_y[:] < actual_stdev_threshold]

    lookahead_train_df = get_split_results(strat_file, actual_train_high_stdev_dates, actual_train_low_stdev_dates,
                                           work_dir, shortcode, results_dir, train_dates)
    lookahead_test_df = get_split_results(strat_file, actual_test_high_stdev_dates, actual_test_low_stdev_dates,
                                          work_dir, shortcode, results_dir, test_dates)

    return predicted_train_df, predicted_test_df, stdev_threshold, \
        lookahead_train_df, lookahead_test_df, actual_stdev_threshold


def get_best_threshold(shortcode, work_dir, strat, results_dir, dates, train_data_df, weights, intercept,
                       dates_file=None, split=0.7):
    strat_file = os.path.join(work_dir, "strat_file")
    with open(strat_file, 'w') as handle:
        handle.write(strat)
        handle.write("\n")

    max_sharpe_diff = 0
    best_threshold = 0
    thresholds = [30, 40, 50, 60, 70]
    for threshold in thresholds:

        ptrain, _, _, _, _, _ = get_results_on_threshold(shortcode, work_dir, strat_file, results_dir, dates,
                                                         train_data_df, weights, intercept, threshold, dates_file,
                                                         split)
        sharpe_diff = math.fabs(float(ptrain['Sharpe(HV)'].as_matrix()[0]) - float(ptrain['Sharpe(LV)'].as_matrix()[0]))

        if sharpe_diff > max_sharpe_diff:
            best_threshold = threshold
            max_sharpe_diff = sharpe_diff

    ptrain, ptest, pthresh, ltrain, ltest, lthresh = get_results_on_threshold(shortcode, work_dir, strat_file,
                                                                              results_dir, dates, train_data_df,
                                                                              weights, intercept, best_threshold,
                                                                              dates_file, split)
    print("\n\n")
    print("Best Percentile threshold: ", best_threshold)
    print("Strategy: ", strat)
    print("TRAINING")
    print("With Lookahead(Threshold = ", lthresh, "): ")
    print(ltrain.iloc[:, 1:])
    print("\n")
    print("Without Lookahead(Threshold = ", pthresh, "): ")
    print(ptrain.iloc[:, 1:])
    print("\n")
    print("TESTING")
    print("With Lookahead")
    print(ltest.iloc[:, 1:])
    print("\n")
    print("Without Lookahead")
    print(ptest.iloc[:, 1:])
    print("\n")
    print("**********************************************************************************************************")


def read_params_from_config(config_file):
    paramset = {"use_lasso": False, "only_pos": False, "use_event": False, "strats": [], "results_dir": "DB",
                "features_list": [], "prev_session_start_time": None, "prev_session_end_time": None,
                "last_day_prev_session": False}
    current_instruction = None

    try:
        with open(config_file, 'r') as cfile:
            for line in cfile:
                if not line.strip():
                    current_instruction = None
                    continue

                line = line.strip()
                if line[0] == '#':
                    continue

                if current_instruction is None:
                    current_instruction = line
                else:
                    if current_instruction == "SHORTCODE":
                        paramset["shortcode"] = line
                    if current_instruction == "START_TIME":
                        paramset["start_time"] = line
                    if current_instruction == "END_TIME":
                        paramset["end_time"] = line
                    if current_instruction == "FEATURES":
                        paramset["features_list"].append(line)
                    if current_instruction == "END_DATE":
                        paramset["end_date"] = line
                    if current_instruction == "NUM_DAYS":
                        paramset["num_days"] = line
                    if current_instruction == "PREV_SESS_START_TIME":
                        paramset["prev_session_start_time"] = line
                    if current_instruction == "PREV_SESS_END_TIME":
                        paramset["prev_session_end_time"] = line
                    if current_instruction == "SELECTION":
                        if line != "0":
                            paramset["use_lasso"] = True
                    if current_instruction == "ONLY_POS":
                        if line != "0":
                            paramset["only_pos"] = True
                    if current_instruction == "USE_EVENT":
                        if line != "0":
                            paramset["use_event"] = True
                    if current_instruction == "PREV_SESS_LAST_DAY":
                        if line == "0":
                            paramset["last_day_prev_session"] = False
                        else:
                            paramset["last_day_prev_session"] = True
                    if current_instruction == "TARGET_FEATURE":
                        paramset["target_feature"] = line
                    if current_instruction == "MODEL_FILE":
                        paramset["model_file"] = line
                    if current_instruction == "STRATS":
                        paramset["strats"].append(line)
                    if current_instruction == "THRESHOLD":
                        paramset["threshold"] = int(line)
                    if current_instruction == "RESULTS_DIR":
                        paramset["results_dir"] = line
    except:
        print(config_file + " not readable")
        raise ValueError(config_file + " not readable.")

    if "shortcode" not in paramset:
        raise ValueError("SHORTCODE is required")
    if "start_time" not in paramset:
        raise ValueError("START_TIME is required")
    if "end_time" not in paramset:
        raise ValueError("END_TIME is required")
    if "end_date" not in paramset:
        raise ValueError("END_DATE is required")
    if "num_days" not in paramset:
        raise ValueError("NUM_DAYS is required")

    return paramset


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", "-m", help="TRAIN/CHECK_STRATS/GET_DAYS", required=True)
    parser.add_argument('-c', dest='configname',
                        help="config file with parameters for volatility estimation[TRAIN]/ "
                             "config file containing path to trained model and list of strats[CHECK_STRATS] /"
                             "config file contatinig path to trained model to get set of dates[GET_DAYS]", type=str,
                        required=True)
    args = parser.parse_args()
    mode = args.mode

    work_dir = "/media/shared/ephemeral21/vol_estimation/" + str(int(time.time() * 1000)) + "/"
    os.system("rm -rf " + work_dir)
    os.system("mkdir -p " + work_dir)
    print("WORK DIR: ", work_dir)

    paramset = read_params_from_config(args.configname)
    if mode == "TRAIN":
        if "target_feature" not in paramset:
            raise ValueError("TARGET_FEATURE is required")

        target_feature = paramset["target_feature"]
        features_list = paramset["features_list"]
        shortcode = paramset["shortcode"]
        if target_feature not in features_list:
            features_list.append(target_feature)
        if paramset["use_event"]:
            if shortcode + " STDEV" not in features_list:
                features_list.append(shortcode + " STDEV")
            if shortcode + " VOL" not in features_list:
                features_list.append(shortcode + " VOL")
        features_file, events_file, sample_data_file, prev_sample_data_file = generate_data(work_dir, features_list,
                                                                                            shortcode,
                                                                                            paramset["start_time"],
                                                                                            paramset["end_time"],
                                                                                            paramset["end_date"],
                                                                                            paramset["num_days"],
                                                                                            paramset["prev_session_start_time"],
                                                                                            paramset["prev_session_end_time"])

        events_data, sample_data, prev_sample_data = load_data(
            features_list, events_file, sample_data_file, prev_sample_data_file)

        train_data_df = generate_all_training_data(
            shortcode, sample_data, events_data, prev_sample_data, paramset["last_day_prev_session"], target_feature,
            paramset["use_event"])
        print(train_data_df.iloc[:, 1:].corr()['Target Volatility'])

        weights, intercept = train_model(work_dir, train_data_df, paramset, paramset["use_lasso"], paramset["only_pos"],
                                         split=0.7)
        model_dir =  "/home/dvctrader/modelling/FeatureModels/" + paramset["shortcode"] + "/" + paramset["start_time"] \
                     + "_" + paramset["end_time"]
        os.system("mkdir -p " + model_dir)
        model_file = "/home/dvctrader/modelling/FeatureModels/" + paramset["shortcode"] + "/" + paramset["start_time"] \
                     + "_" + paramset["end_time"] + "/" + \
                     "_".join(paramset["target_feature"].split()) + "_model_" + str(int(time.time() * 1000))

        model_contents = open(os.path.join(work_dir, "trained_model_file"), 'r').read()
        f = open(model_file, 'w')
        f.write(model_contents)
        f.close()
        print("Trained model stored in " + model_file)


    elif mode == "CHECK_STRATS":
        if len(paramset["strats"]) == 0:
            raise ValueError("STRATS is required")
        if "model_file" not in paramset:
            raise ValueError("MODEL_FILE is required")
        weight_feature_map, train_data_df = get_df_from_model(work_dir, paramset["model_file"], paramset["shortcode"],
                                                              paramset["start_time"],
                                                              paramset["end_time"], paramset["end_date"],
                                                              paramset["num_days"],
                                                              paramset["prev_session_start_time"],
                                                              paramset["prev_session_end_time"])
        weights = []
        for column in train_data_df.columns[2:]:
            if column in weight_feature_map:
                weights.append(weight_feature_map[column])
            else:
                weights.append(0)
        intercept = weight_feature_map["Intercept"]
        dates = train_data_df['Date'].as_matrix()

        if "threshold" not in paramset:
            for strat in paramset["strats"]:
                get_best_threshold(paramset["shortcode"], work_dir, strat, paramset["results_dir"], dates,
                                   train_data_df, weights, intercept, dates_file=None, split=0.7)
        else:
            for strat in paramset["strats"]:
                strat_file = os.path.join(work_dir, "strat_file")
                with open(strat_file, 'w') as handle:
                    handle.write(strat)
                    handle.write("\n")
                ptrain, ptest, pthresh, ltrain, ltest, lthresh = get_results_on_threshold(paramset["shortcode"],
                                                                                          work_dir,
                                                                                          strat_file,
                                                                                          paramset["results_dir"],
                                                                                          dates,
                                                                                          train_data_df,
                                                                                          weights, intercept,
                                                                                          paramset["threshold"],
                                                                                          dates_file=None, split=0.7)
                print("\n\n")
                print("Strategy: ", strat)
                print("TRAINING")
                print("With Lookahead(Threshold = ", lthresh, "): ")
                print(ltrain.iloc[:, 1:])
                print("\n")
                print("Without Lookahead(Threshold = ", pthresh, "): ")
                print(ptrain.iloc[:, 1:])
                print("\n")
                print("TESTING")
                print("With Lookahead")
                print(ltest.iloc[:, 1:])
                print("\n")
                print("Without Lookahead")
                print(ptest.iloc[:, 1:])
                print("\n")
                print("******************************************************"
                      "****************************************************")
    elif mode == "GET_DAYS":
        if "threshold" not in paramset:
            raise ValueError("THRESHOLD is required")

        if "model_file" not in paramset:
            raise ValueError("MODEL_FILE is required")

        weight_feature_map, train_data_df = get_df_from_model(work_dir, paramset["model_file"], paramset["shortcode"],
                                                              paramset["start_time"],
                                                              paramset["end_time"], paramset["end_date"],
                                                              paramset["num_days"],
                                                              paramset["prev_session_start_time"],
                                                              paramset["prev_session_end_time"])
        weights = []
        for column in train_data_df.columns:
            if column in weight_feature_map:
                weights.append(weight_feature_map[column])
            else:
                weights.append(0)
        intercept = weight_feature_map["Intercept"]
        predicted = intercept + np.dot(train_data_df.as_matrix(), weights)
        predicted_threshold = np.percentile(predicted, paramset["threshold"])
        hv_dates = list(map(int, train_data_df["Date"].as_matrix()[predicted[:] > predicted_threshold]))
        lv_dates = list(map(int, train_data_df["Date"].as_matrix()[predicted[:] <= predicted_threshold]))
        print("Dates above threshold (stored in " + os.path.join(work_dir, "high_dates") + ")")
        print(" ".join(map(str, hv_dates)))
        with open(os.path.join(work_dir, "high_dates"), 'w') as writer:
            for date in list(hv_dates):
                writer.write(str(date) + "\n")
        print("\n")
        print("Dates below threshold (stored in " + os.path.join(work_dir, "low_dates") + ")")
        print(" ".join(map(str, lv_dates)))
        with open(os.path.join(work_dir, "low_dates"), 'w') as writer:
            for date in list(lv_dates):
                writer.write(str(date) + "\n")
