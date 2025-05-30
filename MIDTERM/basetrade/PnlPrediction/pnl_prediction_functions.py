
# coding: utf-8

# Master function list, contains all algorithms for Pnl Prediction

# In[11]:

import os
import sys
import io
import operator
import subprocess

import pandas as pd
import numpy as np
import shutil

#from matplotlib import pyplot as plt
from datetime import datetime

import scipy.stats as ss
from scipy.stats.stats import pearsonr
from scipy.optimize import minimize

from sklearn import preprocessing
from sklearn.cluster import KMeans
from sklearn.decomposition import PCA
from sklearn import datasets, linear_model

sys.path.append(os.path.expanduser('~/basetrade/'))

from walkforward.definitions import execs
from walkforward.utils import date_utils
from walkforward.utils.run_exec import exec_function
from walkforward.wf_db_utils.fetch_config_details import fetch_config_id
from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.utils.date_utils import week_days_between_dates

from scripts.get_config_n_uts_for_queryid_n_date import get_date_to_config_to_uts_map_for_queries_n_dates

import warnings
warnings.filterwarnings('ignore')

# In[12]:

# Used to load data from DB if file is not present to get data


def load_config_date_pnl_map(shc, p_start_time, p_end_time, start_date, end_date, config_date_pnl, strats_folder, results_folder):
    # Get the PnL data for the pool using summarize_strategy
    summarize_pnl_command = [execs.execs().summarize_strategy, shc, strats_folder, results_folder,
                             str(start_date), str(end_date), "IF", "kCNAPnlSharpe", "0", "IF", "0"]
    summarize_pnl_command = " ".join(summarize_pnl_command)
    print(summarize_pnl_command)
    PnL_data = exec_function(summarize_pnl_command)[0].strip()

    # fill the map of config_date_pnl from output of summarize strategy results
    PnL_data = PnL_data.split("\n")
    for line in PnL_data:
        if line == "":
            continue
        words = line.split(" ")
        if words[0] == "STRATEGYFILEBASE":
            config = words[1]
            config_date_pnl[config] = {}
            continue
        if words[0] == "STATISTICS":
            continue
        date_ = words[0]
        config_date_pnl[config][date_] = int(words[1])


# In[13]:

# Gets list of dates to run the pnl prediction methods on
def GetListOfDates(start_date, num_days, shc):
    date_command = ["/home/dvctrader/basetrade/scripts/get_list_of_dates_for_shortcode.pl",
                    shc, start_date, str(num_days)]
    date_command = (' '.join(date_command))
    # print date_command
    process = subprocess.Popen(date_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    date_list, err = process.communicate()
    date_list = date_list.decode('utf-8')
    # print date_list
    date_list = date_list.split()
    return date_list


def GetListOfDates_v2(start_date, num_days, main_date_list):
    last_date_index = main_date_list.index(start_date) + 1
    if last_date_index <= num_days:
        date_list = main_date_list[:last_date_index]
    else:
        date_list = main_date_list[last_date_index - num_days:last_date_index]
    date_list.sort()
    return date_list[::-1]


# In[14]:

# This function prints statistics for given model names and outputs
def get_accuracy_matrix(model_type_names, prediction_correlation_error, measure_list, num_strategies):
    output_columns = ['Date']
    for measure in measure_list:
        for model in model_type_names:
            output_columns.append(model + '_' + measure)

    day_wise_predicted_rank_corrs = pd.DataFrame(prediction_correlation_error, columns=output_columns)

    print("\n Total Days = ", day_wise_predicted_rank_corrs.shape[0])
    print("Total Strategies = ", num_strategies, "\n")  # demeaned_ID.shape[0], "\n"

    data_list = [[] for i in range(len(measure_list))]

    for i in range(0, len(measure_list)):
        measure = measure_list[i]
        for model in model_type_names:
            data_list[i].append(np.mean(day_wise_predicted_rank_corrs[model + '_' + measure]))

    accurancy_matrix_columns = ['Model_Name'] + measure_list
    accuracy_matrix = pd.DataFrame(columns=accurancy_matrix_columns)
    accuracy_matrix[accurancy_matrix_columns[0]] = model_type_names

    for i in range(0, len(measure_list)):
        accuracy_matrix[accurancy_matrix_columns[i + 1]] = data_list[i]

    return accuracy_matrix

# In[15]:

# Model accuracy metric functions


def get_rank_correlation(actual, predicted):
    return pearsonr(ss.rankdata(actual), ss.rankdata(predicted))[0]


def get_sum_squared_error(actual, predicted):
    return np.sum((np.array(actual) - np.array(predicted))**2)


def get_normalized_error_for_day(actual, predicted):
    return np.sqrt(np.mean((np.array(actual) - np.array(predicted))**2)) / np.sqrt(np.mean((np.array(actual))**2))


def compare_top_n_predicted_strats_with_base(actual, predicted, base_predicted, n):

    df = pd.DataFrame(np.asarray([actual, predicted, base_predicted]).T.tolist(),
                      columns=['Actual', 'Predicted', 'Base_Predicted'])

    df.sort_values(by='Predicted', ascending=False, inplace=True)
    predicted_top_n_actual_pnl_sum = np.sum(df['Actual'].tolist()[:n])

    df.sort_values(by='Base_Predicted', ascending=False, inplace=True)
    base_predicted_top_n_actual_pnl_sum = np.sum(df['Actual'].tolist()[:n])

    error_wrt_base = predicted_top_n_actual_pnl_sum - base_predicted_top_n_actual_pnl_sum
    return error_wrt_base


# In[64]:

def get_averaged_pnl_series(average_over_days, pnl_series, list_of_dates):
    pnl_output_over_days = []
    for days in average_over_days:
        pnl_output_over_days.append(pnl_series[list_of_dates[days]].mean())
    return pnl_output_over_days


def get_averaged_matrix(wf_start_date, pnl_map, main_date_list, average_over_days):
    list_of_dates = {}
    for days in average_over_days:
        list_of_dates[days] = GetListOfDates_v2(wf_start_date, days, main_date_list)
    averaged_matrix = pnl_map.apply(lambda row: get_averaged_pnl_series(average_over_days, row, list_of_dates), axis=1)
    averaged_matrix = pd.DataFrame(list(averaged_matrix.values), columns=average_over_days)
    return averaged_matrix


def learn_model_and_get_walkforward_prediction(list_of_dates, averaged_matrix_main, pnl_map):
    prediction = []
    for day in list_of_dates:
        regr = fit_model_on_day(day, averaged_matrix_main, pnl_map)

        prediction.append(list(regr.predict(averaged_matrix_main[day])))  # Given X, predict y_hat

    prediction_df = pd.DataFrame(prediction).T
    prediction_df.columns = list_of_dates
    prediction_df.index = pnl_map.index

    return prediction_df


def predict_next_day_element_method_1(row, average_over_days, weights):
    next_day_element = 0
    for i in range(1, len(average_over_days)):
        weight = weights[i]
        average_days = average_over_days[i]
        next_day_element += weights[i] * (row[-average_days:].mean())
    return next_day_element


def get_PCA_matrixes(X, components):
    pca = PCA(n_components=components, whiten=True)
    W = pca.fit_transform(X)
    H = pca.components_
    return W, H


def predict_next_day_method_1(H, average_over_days, weights):
    predicted_next_day = np.apply_along_axis(predict_next_day_element_method_1, 1, H, average_over_days, weights)
    return predicted_next_day


def standardize(X):
    means = X.mean(axis=1)
    std = X.std(axis=1)
    demeaned_X = X.sub(means, axis=0)
    standardized_X = demeaned_X.div(std, axis=0)
    return standardized_X, means, std


def demean(X):
    means = X.mean(axis=1)
    demeaned_X = X.sub(means, axis=0)
    return demeaned_X, means


def predict_next_day_pnl(day, pnl_map, PCA_Days, main_date_list, average_over_days, weights, components, alpha, beta, steps, CollFill=False):
    list_of_days = GetListOfDates_v2(day, PCA_Days, main_date_list)
    X = pnl_map[list_of_days]

    demeaned_X, means_X, std_X = standardize(X)
    if CollFill == False:
        W, H = get_PCA_matrixes(demeaned_X, components)
    else:
        W, H = mf_using_coll_fill(demeaned_X, components, alpha, beta, steps)
    predicted_next_day = predict_next_day_method_1(H, average_over_days, weights)
    next_day_pnl = means_X.add(std_X.mul(W.dot(predicted_next_day)))
    return next_day_pnl

# linear regression method of predicting weights


def get_averaged_row(row, average_over_days):
    averaged_row = np.empty((0))
    for i in range(0, len(average_over_days)):
        average_days = average_over_days[i]
        averaged_row = np.append(averaged_row, row[-average_days:].mean())
    return averaged_row

# Below three functions are for collaborative filtering


def matrix_sum_squares(M):
    a = np.sum(np.array(M)**2)
    return a


def update_coll_fill_matrices(X, W, H, alpha, beta):
    X_dash = W.dot(H)
    E = np.subtract(X, X_dash)
#    print "X = ", X
 #   print "X_dash = ", X_dash
#  print "E = ", E
    s = W.shape[0]
    d = H.shape[1]
    components = W.shape[1]
    W_transpose = np.transpose(W)
    H_transpose = np.transpose(H)
    W_dash = np.add(np.multiply(W, 1 - alpha * beta), np.multiply(E.dot(H_transpose), 2 * alpha))
    H_dash = np.add(np.multiply(H, 1 - alpha * beta), np.multiply(W_transpose.dot(E), 2 * alpha))

    return W_dash, H_dash


def update_coll_fill_matrices_holes(X, W, H, alpha, beta):
    X_dash = W.dot(H)
    E = np.subtract(X, X_dash)
    s = W.shape[0]
    d = H.shape[1]
    components = W.shape[1]

    W_dash = W.copy()
    H_dash = H.copy()
    for i in range(0, s):
        for j in range(0, d):
            if (not np.isnan(X[i][j])):
                for k in range(0, components):
                    increment_1 = 2 * alpha * E[i][j] * H[k][j] - alpha * beta * W[i][k]
                    W_dash[i][k] += increment_1
                    increment_2 = 2 * alpha * E[i][j] * W[i][k] - alpha * beta * H[k][j]
                    H_dash[k][j] += increment_2

    return W_dash, H_dash


def mf_using_coll_fill_holes(X, components, alpha, beta, steps):
    X = np.array(X)
    # print X
    s = X.shape[0]
    d = X.shape[1]
    W = np.random.rand(s, components)
    H = np.random.rand(components, d)
    for step in range(0, steps):
        W, H = update_coll_fill_matrices_holes(X, W, H, alpha, beta)
        # print step
        # print "W = ", W
        # print "H = ", H
        X_reconstructed = W.dot(H)
        # print "X_dash = ", X_reconstructed
        # print("Looping")
        diff_error = 0
        for i in range(0, s):
            for j in range(0, d):
                if (not np.isnan(X[i][j])):
                    diff_error += (X[i][j] - X_reconstructed[i][j])**2
        # print "E = ", E
        l2_error = (matrix_sum_squares(W) + matrix_sum_squares(H)) * beta / 2
        error = l2_error + diff_error
        # print "l2_error = ",l2_error
        # print "error = ", error
        if (error < 0.05):
            break
    # print "Steps:", step+1
    # print "L2 Error:", l2_error
    # print "Final Error:", error
    return W, H


def mf_using_coll_fill(X, components, alpha, beta, steps):
    X = np.array(X)
    # print X
    s = X.shape[0]
    d = X.shape[1]
    W = np.random.rand(s, components)
    H = np.random.rand(components, d)
    for step in range(0, steps):
        W, H = update_coll_fill_matrices(X, W, H, alpha, beta)
        # print step
        # print "W = ", W
        # print "H = ", H
        X_reconstructed = W.dot(H)
        # print "X_dash = ", X_reconstructed
        E = np.subtract(X, X_reconstructed)
        # print "E = ", E
        l2_error = (matrix_sum_squares(W) + matrix_sum_squares(H)) * beta / 2
        error = l2_error + matrix_sum_squares(E)
        # print "l2_error = ",l2_error
        # print "error = ", error
        if (error < 0.05):
            break
    # print "Steps:", step+1
    # print "L2 Error:", l2_error
    # print "Final Error:", error
    return W, H


def coll_fill_wrapper(X, components, alpha, beta, steps):
    standardized_X, means, std = standardize(X)
    W, H = mf_using_coll_fill(standardized_X, components, alpha, beta, steps)
    reconstructed_X = W.dot(H)
    reconstructed_X = pd.DataFrame(reconstructed_X)
    reconstructed_X = reconstructed_X.multiply(std, axis=0).add(means, axis=0)
    return reconstructed_X


def fit_model_on_day(prediction_date, averaged_matrix_main, pnl_map, main_date_list, average_over_days,  target=None):
    todays_index = main_date_list.index(prediction_date)
    averaged_matrix_all_dates = []
    pnl_actual_all_dates = []
    # print todays_index,prediction_date,max(average_over_days)
    for i in range(max(average_over_days), todays_index):
        wf_start_date = main_date_list[i - 1]  # GetListOfDates_v2(prediction_date,2)[1]
        # X #this should be wf_start_date.. otherwise this is not walkforward
        averaged_matrix = averaged_matrix_main[wf_start_date]
        averaged_matrix_all_dates.append(averaged_matrix)
        pnl_actual = pnl_map[main_date_list[i]].values  # y
        pnl_actual_all_dates.extend(pnl_actual)
        # if i%20 == 0:
        # print i, wf_start_date, main_date_list[todays_index],len(averaged_matrix_all_dates), len(pnl_actual_all_dates)

    averaged_matrix = pd.concat(averaged_matrix_all_dates)
    pnl_actual = pnl_actual_all_dates

    # Create linear regression object
    regr = linear_model.LinearRegression()
    # Train the model using the training sets
    regr.fit(averaged_matrix, pnl_actual)  # X, y
    # Make predictions using the testing set
    return regr


def fit_model_on_day_lasso(prediction_date, averaged_matrix_main, pnl_map, main_date_list, average_over_days, positive_coeffs=False, target=None):
    todays_index = main_date_list.index(prediction_date)
    averaged_matrix_all_dates = []
    pnl_actual_all_dates = []
    # print todays_index,prediction_date
    for i in range(max(average_over_days), todays_index):
        wf_start_date = main_date_list[i - 1]  # GetListOfDates_v2(prediction_date,2)[1]
        averaged_matrix = averaged_matrix_main[wf_start_date]  # X
        averaged_matrix_all_dates.append(averaged_matrix)
        pnl_actual = pnl_map[main_date_list[i]].values  # y
        pnl_actual_all_dates.extend(pnl_actual)
        # if i%20 == 0:
        # print i, wf_start_date, main_date_list[todays_index],len(averaged_matrix_all_dates), len(pnl_actual_all_dates)

    averaged_matrix = pd.concat(averaged_matrix_all_dates)
    pnl_actual = pnl_actual_all_dates

    # Create lasso linear regression object
    clf = linear_model.Lasso(positive=positive_coeffs, fit_intercept=False, copy_X=True)
    # Train the model using the training sets
    clf.fit(averaged_matrix, pnl_actual)  # X, y
    return clf


def predict_next_day_method_2(H, lasso, average_over_days):

    # For predicting the next day using the model trained above
    averaged_matrix_test = np.apply_along_axis(get_averaged_row, 1, H, average_over_days)
    predicted_next_day = []

    regr_list = []
    for j in range(0, H.shape[0]):
        averaged_matrix_all_dates = []
        target_actual_all_dates = []
        for i in range(max(average_over_days), H.shape[1]):  # Leaving the initial days from learning.
            # This should be from (0, H.shape[1]-max(average_over_days))
            # No error coming because get_averaged_rows does array[-average_days:].mean(). If average_days exceeds length of matrix then it simply gives all elemtns in return and does not throw error
            prev_H = H[j, :i]
            averaged_matrix_train = np.apply_along_axis(get_averaged_row, 0, prev_H, average_over_days)
            target = H[j, i]
            averaged_matrix_all_dates.append(averaged_matrix_train)
            target_actual_all_dates.append(target)

        averaged_matrix_train = averaged_matrix_all_dates
        if lasso == True:
            regr = linear_model.Lasso(fit_intercept=False, copy_X=True)
        else:
            regr = linear_model.LinearRegression()

        # Train the model using the training sets
        regr.fit(averaged_matrix_train, target_actual_all_dates)
        predicted_next_day.extend(regr.predict(averaged_matrix_test[j, :]))

    return predicted_next_day


def predict_next_day_pnl_method_2(day, pnl_map, main_date_list, average_over_days, components,  alpha, beta, steps, lasso=False, CollFill=False):
    #todays_index = main_date_list.index(day)
    # list_of_days = main_date_list[:todays_index+1]         #GetListOfDates_v2(day, PCA_Days)
    list_of_days = GetListOfDates_v2(day, 250, main_date_list)
    X = pnl_map[list_of_days]
    demeaned_X, means_X, std_X = standardize(X)
    if CollFill == False:
        W, H = get_PCA_matrixes(demeaned_X, components)
    else:
        W, H = mf_using_coll_fill(demeaned_X, components, alpha, beta, steps)
    predicted_next_day = predict_next_day_method_2(H, lasso, average_over_days)
    next_day_pnl = means_X.add(std_X.mul(W.dot(predicted_next_day)))
    return next_day_pnl


# In[130]:

def predict_next_day_pnl_features_method_1(day, pnl_map, features_map, next_day_feature_value, categorical_columns, factor_method,
                                           main_date_list, average_over_days, weights, components, alpha, beta, steps, lasso=False):

    list_of_days = GetListOfDates_v2(day, 250, main_date_list)
    # print list_of_days

    # Factorize SD matrix
    SD = pnl_map[list_of_days]
    #demeaned_SD, means_SD = demean(SD)
    demeaned_SD, means_SD, std_SD = standardize(SD)

    if factor_method == 'PCA':
        W_SD, H_SD = get_PCA_matrixes(demeaned_SD, components)
    elif factor_method == 'Col_Fill':
        # print(steps)
        W_SD, H_SD = mf_using_coll_fill(demeaned_SD, components, alpha, beta, steps)
    elif factor_method == 'Col_Fill_Holes':
        # print(steps)
        W_SD, H_SD = mf_using_coll_fill_holes(demeaned_SD, components, alpha, beta, steps)

    # Factorize ID using SD date factors (which is H_SD here)
    ID = features_map[list_of_days]

    #demeaned_ID, means_ID = demean(ID)
    #demeaned_ID = pd.DataFrame(preprocessing.scale(ID,axis=1))
    demeaned_ID, means_ID, std_ID = standardize(ID)

    all_features = demeaned_ID.index.tolist()

    # Do not standarize the categorical columns
    demeaned_ID.ix[categorical_columns] = ID.ix[categorical_columns]

    # print type(demeaned_ID), type(ID)
    #demeaned_ID = ID
    W_ID = get_matrix_factor_given_one_factor(demeaned_ID, H_SD, known_matrix_side='right')

    # print W_ID
    # Predict next day features if not already provided
    if next_day_feature_value == None:
        demeaned_ID_predicted_next_day = predict_next_day_ID(demeaned_ID, average_over_days, weights)
    else:
        demeaned_ID_predicted_next_day = (next_day_feature_value - means_ID) / std_ID
        demeaned_ID_predicted_next_day.ix[categorical_columns] = [next_day_feature_value[ind]
                                                                  for ind in [all_features.index(i) for i in all_features if i in categorical_columns]]

    # Get date factors for the next day
    H_SD_predicted_next_day = get_matrix_factor_given_one_factor(
        demeaned_ID_predicted_next_day, W_ID, known_matrix_side='left')

    H_SD_predicted_next_day_list = [j for i in H_SD_predicted_next_day for j in i]
    # print H_SD.T[1]
    # print H_SD_predicted_next_day_list

    # Predict next day using the predicted date vector for next day
    x = W_SD.dot(H_SD_predicted_next_day_list)

    next_day_pnl = means_SD.add(std_SD.mul(x))
    #next_day_pnl = means_SD.add(x)

    return next_day_pnl


def get_matrix_factor_given_one_factor(matrix_factorised, known_factor_matrix, known_matrix_side):
    # Using regression to get the other factor
    if known_matrix_side == 'left':
        regressions_to_run = 1
        components = 1
        if len(matrix_factorised.shape) != 1:
            regressions_to_run = matrix_factorised.shape[1]
        if len(known_factor_matrix.shape) == 1:
            components = 1
        else:
            components = known_factor_matrix.shape[1]
        unknown_matrix = np.zeros((regressions_to_run, components))
        for i in range(0, regressions_to_run):
            regr = linear_model.Ridge(fit_intercept=False, copy_X=True)
            if len(matrix_factorised.shape) == 1:
                non_nan_indices = [j[0] for j in np.argwhere(~np.isnan(matrix_factorised)).tolist()]
                non_nan_y = matrix_factorised[non_nan_indices]
                non_nan_x = known_factor_matrix[non_nan_indices]
                regr.fit(non_nan_x, non_nan_y)
            else:
                regr.fit(known_factor_matrix, matrix_factorised.iloc[:, i])
            unknown_matrix[i] = list(regr.coef_)
        return unknown_matrix.T
    else:
        unknown_matrix = np.zeros((matrix_factorised.shape[0], known_factor_matrix.shape[0]))
        for i in range(0, matrix_factorised.shape[0]):
            regr = linear_model.LinearRegression()
            #regr = linear_model.Ridge(fit_intercept=False,copy_X=True)
            regr.fit(known_factor_matrix.T, matrix_factorised.iloc[i])
            unknown_matrix[i] = list(regr.coef_)
        return unknown_matrix


def predict_next_day_ID(demeaned_ID, average_over_days, weights):
    # Using just a simple average model here. Need to improve it
    return predict_next_day_method_1(demeaned_ID, average_over_days, weights)


def get_avg_sample_data(shortocde, feature_list, date_list, start_time, end_time):
    num_days_to_compute_feature = week_days_between_dates(min(date_list), max(date_list))
    counter = 0
    for feature in feature_list:
        avg_feature_val_cmd = [execs.execs().avg_samples, shortocde, str(max(date_list)), str(
            num_days_to_compute_feature), start_time, end_time, '1', feature]
        avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)
        x = pd.read_fwf(io.BytesIO(avg_feature_out.communicate()[0]),
                        header=None, widths=[8, 7], skiprows=1, delimiter=' ')
        x[0] = x[0].astype(int)
        x[1] = x[1].astype(float)
        x.columns = ['Date', feature]
        if counter == 0:
            all_x = x
        else:
            all_x = pd.merge(all_x, x, how='left')
        counter += 1
    return all_x


def get_features_data_for_dates(shortcode, start_date, feature_list_file,
                                num_days_to_compute_feature, st, et):
    # /home/dvctrader/basetrade_install/WKoDii/get_day_features.pl FGBL_0 20170822 10
    # /spare/local/tradeinfo/day_features/product_configs/ZN_0_config.txt DAY CET_900 EST_800

    counter = 0

    avg_feature_val_cmd = ['/home/dvctrader/basetrade_install/WKoDii/get_day_features.pl', shortcode,
                           str(start_date), str(num_days_to_compute_feature), feature_list_file, 'DAY', st, et]
    # print ' '.join(avg_feature_val_cmd)
    avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)

    y = avg_feature_out.communicate()[0]
    a = []
    for s in y.split('\n'):
        if (s != "" and s != " "):
            a.append(s.split())

    x = pd.DataFrame(a)
    y = x.T[1:]
    y.columns = list(map(str, map(int, x.T.iloc[0])))
    y = y.astype(float)

    return y


def get_index_for_features(feature_list_file):
    features_list = []
    with open(feature_list_file, 'r') as f:
        for line in f:
            if not line.strip():
                continue
            line = line.strip()
            if line[0] == '#':
                continue
            line = line.split()
            features_list.append('-'.join(line))
    return features_list


def three_degree_events_dates(shortcode, start_date, num_days_to_compute_feature, st, et):
    cmd = "/home/dvctrader/basetrade/scripts/get_economic_events_for_product.pl " + shortcode + " " + str(start_date) + \
          " " + str(num_days_to_compute_feature) + " " + st + " " + et
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, error = process.communicate()
    out = out.decode('utf8').split("\n")
    date_array = []
    for line in out:
        words = line.split(" ")
        if len(words) > 4 and words[3] == "3":
            date_array.append(words[0])
    date_array = list(set(date_array))
    return date_array


def get_features_events_data_for_dates(shortcode, start_date, feature_list_file,
                                       num_days_to_compute_feature, st, et):
    avg_feature_val_cmd = ['/home/dvctrader/basetrade_install/WKoDii/get_day_features.pl', shortcode,
                           str(start_date), str(num_days_to_compute_feature), feature_list_file, 'DAY', st, et]
    # print(' '.join(avg_feature_val_cmd))
    avg_feature_out = subprocess.Popen(' '.join(avg_feature_val_cmd), shell=True, stdout=subprocess.PIPE)
    out = avg_feature_out.communicate()[0].decode('utf8')
    a = []
    for s in out.split('\n'):
        if (s != "" and s != " "):
            a.append(s.split())

    x = pd.DataFrame(a)
    y = x.T[1:]
    y.columns = list(map(str, map(int, x.T.iloc[0])))
    y = y.astype(float)
    y.reset_index(inplace=True, drop=True)

    features_index_list = get_index_for_features(feature_list_file)
    features_index_list.append('is_3_degree_event_present')

    three_degree_event_days_list = three_degree_events_dates(shortcode, start_date, num_days_to_compute_feature,
                                                             st, et)

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


# Portfolio Report functions

def get_start_end_query_ids(pick_strat_cname_tuple):

    cursor = connection().cursor()
    search_query = ("Select config_name, start_queryid, end_queryid from PickstratConfig where config_name = '%s';" %
                    (pick_strat_cname_tuple,))
    # print(search_query)
    # execute the SQL query using execute() method.
    cursor.execute(search_query)
    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    # print the rows

    if len(data) > 0:
        return data

    return None


def get_real_uts_for_dates(shc, session, list_of_dates):

    pick_strat_cname = shc + '.' + session + '.txt'
    start_id_end_id_data = get_start_end_query_ids(pick_strat_cname)
    query_start_id_ = start_id_end_id_data[0][1]
    query_end_id_ = start_id_end_id_data[0][2]
    return get_date_to_config_to_uts_map_for_queries_n_dates(query_start_id_, query_end_id_, list_of_dates)[0]


def scale_pnl_by_real_uts(pnl_map, predicted_pnl_map, real_uts_map):
    number_strats = pnl_map.shape[0]
    out_pnl_scaled_sim = pd.DataFrame(columns=pnl_map.columns)
    methods_used = list(predicted_pnl_map.values())[0].columns
    for date in predicted_pnl_map.keys():
        if date in real_uts_map.keys():
            real_uts_for_one_day = list(map(int, real_uts_map[date].values()))
            sorted_real_uts = sorted(real_uts_for_one_day, reverse=True) + \
                [0] * (number_strats - len(real_uts_for_one_day))
            uts_matrix = []
            for method in methods_used:
                copy_sorted_real_uts = sorted_real_uts[:]
                rearranged_real_uts = [x for _, x in sorted(
                    zip(predicted_pnl_map[date][method].tolist(), copy_sorted_real_uts), reverse=True)]
                uts_matrix.append(rearranged_real_uts)
            uts_matrix = np.array(uts_matrix)
            sim_pnl_on_day = (np.array([pnl_map[date].tolist()])).T
            out_pnl_scaled_sim[date] = (uts_matrix.dot(sim_pnl_on_day)).T[0]
        else:
            out_pnl_scaled_sim.drop(date, axis=1, inplace=True)
    out_pnl_scaled_sim.index = methods_used
    out_pnl_scaled_sim = out_pnl_scaled_sim.reindex_axis(sorted(out_pnl_scaled_sim.columns, reverse=True), axis=1)
    return out_pnl_scaled_sim


def get_portfolio_report(shc, pnl_map, all_predicted_pnls_for_all_days, session):
    list_of_dates = list(all_predicted_pnls_for_all_days.keys())
    real_uts_map = get_real_uts_for_dates(shc, session, list_of_dates)
    pnl_map = pnl_map[list_of_dates]
    real_uts_scaled_sim_ranked = scale_pnl_by_real_uts(pnl_map, all_predicted_pnls_for_all_days, real_uts_map)
    method_avg_by_days = pd.DataFrame(columns=["Avg_over_20_days", "Avg_over_60_days", "Avg_over_100_days"])
    method_avg_by_days["Avg_over_20_days"] = (
        real_uts_scaled_sim_ranked[real_uts_scaled_sim_ranked.columns[:20]]).mean(axis=1)
    method_avg_by_days["Avg_over_60_days"] = (
        real_uts_scaled_sim_ranked[real_uts_scaled_sim_ranked.columns[:60]]).mean(axis=1)
    method_avg_by_days["Avg_over_100_days"] = (
        real_uts_scaled_sim_ranked[real_uts_scaled_sim_ranked.columns[:100]]).mean(axis=1)
    print(method_avg_by_days)
