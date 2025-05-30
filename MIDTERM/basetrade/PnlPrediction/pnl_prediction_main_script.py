
# coding: utf-8

# ### Aim of the notebook is to predict pnl of next day for a set of strategies.

import sys
import os
import argparse
import time
import getpass
import numpy as np

#sys.path.append(os.path.expanduser('/home/mehulg/basetrade/'))
sys.path.append(os.path.expanduser('~/basetrade/'))

from PnlPrediction.pnl_prediction_functions import *
#from generate_feature_event_data import *

from walkforward.utils.date_utils import calc_next_week_day

# In[68]:
# Inputs

np.random.seed(2345)

if __name__ == '__main__':

    #parse_arguments

    parser = argparse.ArgumentParser()
    parser.add_argument('-shc', dest='shc', help="the shortcode", type=str, required=True)
    parser.add_argument('-clist', dest='list_of_configs',help='list/dir of configs/strats',type=str, required=True)
    parser.add_argument('-st', dest='p_start_time', help='Start time', type=str, required=True)
    parser.add_argument('-et', dest='p_end_time', help='End Time', type=str, required=True)
    parser.add_argument('-ed', dest='end_date',help='End date',type=str, required=True)
    parser.add_argument('-ldays', dest='num_days',help='Number of days',type=int, required=True)
    parser.add_argument('-results_folder', dest='results_folder', help='Results folder if not in DB', type=str, required=False, default = "DB")
    parser.add_argument('-skip_days', dest='skip_days_file',help='file with days to be removed', type=str, required=False,default=None)
    parser.add_argument('-feature_file', dest='feature_file', help='file with features', type=str,required=False, default=None)
    parser.add_argument('-feature_prediction', dest='feature_value_estimate_file', \
                        help='file with estimated or predicted value of features for next day. \
                        Fill in nan if the value is unknown. \
                        If file is not provided, average of past_days_average string is taken to predict next day feature ', \
                        type=str, required=False, default=None)
    parser.add_argument('-past_days_average', dest='past_days_average',help='Summary indicators for past average days pnl. Eg :- 5_20 for 5 and 20 days average. Default is 5,20 and 100.', type=str, required=False,default='5_20_200')
    parser.add_argument('-past_average_metric', dest='past_average_metric',help='For summarizing past days pnl, we can use mean/median as a measure. Default is mean.', type=str, required=False,default='mean')
    parser.add_argument('-use_fixed_stored_data',dest='use_stored',help='Created a offline data for testing purposes. Run on that.',type=int,required=False,default=0)
    parser.add_argument('-learn_predict_flag', dest='learn_predict_flag',
                        help='0. Get only walkforward learning accuracy numbers and not the prediction.\
                              1. Get only prediction of next day. \
                              2. Get both accuracy numbers and prediction.', type=int, required=False, default=2)
    parser.add_argument('-workdir', dest='workdir',help='work directory for any output files', type=str, required=False, default=None)
    parser.add_argument('-method', dest='method', help='Comma sepreated list of prediction method to be used. Mention All for looking at output of All methods', type=str,
                        required=False, default='All')
    parser.add_argument('-portfolio_report', dest='portfolio_report', help='Extra argument for generating report similar to portfolio report mail.', type=int,
                        required=False, default=0)
    parser.add_argument('-session_name', dest='session_name', help='Session Name like AS, EU, US. Required for portfolio report.', type=str, required=False, default=None)

    args = parser.parse_args()

    # Can change method names here which user can provide
    # Please maintain order of the names
    method_user_side_name = ['Base','Zero_Model','Linear','Reg_Linear',\
           'Naive_PCA','Regression_PCA','Lasso_PCA', \
           'Naive_CollFill', 'Regression_CollFill', 'Lasso_CollFill', \
            'Feature_based_MF_PCA', 'Feature_based_MF_CollFill']

    method_code_side_name = ['Base','Zero_Model','Linear','Reg_Linear',\
           'Naive_PCA','Regression_PCA','Lasso_PCA', \
           'Naive_CollFill', 'Regression_CollFill', 'Lasso_CollFill', \
            'Feature_based_MF_PCA', 'Feature_based_MF_CollFill']

    method_dict = dict(zip(method_user_side_name, method_code_side_name))

    if args.method == 'All':
        # Not using method_dict.values() to maintain order of methods
        method_list = [method_dict[method_name] for method_name in method_user_side_name]
    else:
        method_list = [method_dict[method_name] for method_name in method_user_side_name if method_name in args.method.split(',')]

    alpha = 0.0002
    beta = 0.02
    steps = 1000
    components = 5

    shc = args.shc
    num_days = args.num_days
    end_date = args.end_date
    skip_days_file = args.skip_days_file
    past_average_metric = args.past_average_metric
    average_over_days = list(map(int,args.past_days_average.split("_")))
    work_dir = args.workdir
    feature_value_estimate_file = args.feature_value_estimate_file
    portfolio_report = args.portfolio_report

    weights = [1.0/len(average_over_days)]*(len(average_over_days))
    zero_weights = [0]*(len(average_over_days))
    PCA_Days = max(average_over_days)+1
    average_over_days_pnl = {}
    pnl_prediction = {}

    print("\nRunning for shortcode = ",shc," with list of configs as", args.list_of_configs)
    print("Using averages of past ", ",".join(list(map(str,average_over_days))), " days for future prediction")

    print("Reading Data")

    config_date_pnl = {}
    start_date = str(GetListOfDates(end_date,num_days,shc)[-1])

    if args.use_stored == 0:
        load_config_date_pnl_map(shc, args.p_start_time, args.p_end_time, start_date, end_date, config_date_pnl,
                                 args.list_of_configs,args.results_folder)
        #load_config_date_pnl_map(shc, args.list_of_configs, start_date, end_date, config_date_pnl)
        pnl_map_main = pd.DataFrame.from_dict(config_date_pnl).T
        pnl_main = pnl_map_main
    else:
        print("Using data of ZN staged_Strats stored offline")
        pnl_map_main = pd.read_csv('/media/shared/ephemeral23/pivyas/next_day_prediction/pnl_data_strats_v2.csv')
        pnl_map_main = pnl_map_main.pivot(index='ConfigId', columns='Date', values='Pnl')
        pnl_map_main.columns.name = None
        del pnl_map_main.index.name
        date_col_list_string = list(map(str, pnl_map_main.columns))
        pnl_map_main.columns = date_col_list_string

        config_list = [29968, 29969, 29978, 29985, 29986, 30001, 30010, 30016, 30028, 30029, 30030, 40938, 40940, 40947,
                       40948, 29976, 29982, 29984, 30031, 40951, 29995, 29971, 29972, 29977, 29980, 29989, 29990, 29992,
                       29999, 30003, 30006, 30009, 30011, 30023, 30025, 30026, 30027, 30032, 48524, 48536, 48539, 48543,
                       48544, 48547, 48553, 48557, 48559, 48560, 29991, 29993, 30034, 48525, 48562, 29998, 41158, 41161,
                       48545, 29994, 30000, 30002, 30004, 30007, 30018, 30019, 30022, 41162, 41166, 48540, 48541, 48542,
                       48546, 48548, 48549, 48550, 4855]
        date_filter_list = [x for x in date_col_list_string if x > '20141101']
        pnl_main = pnl_map_main.ix[config_list]
        pnl_main = pnl_main[date_filter_list]
        pnl_main = pnl_main.dropna()
        pnl_main = pnl_main.dropna(axis=1)

    orig_num_strats = pnl_main.shape[0]
    print("Total Strategies = ", orig_num_strats)

    # List some outlier dates in the remove dates list. These dates will be removed from learning and analysis.<br>
    # Dates for which any strategy have a null pnl are removed too here.
    print("Filtering Data")

    skip_days = []
    if skip_days_file is not None and skip_days_file != "":
        with open(skip_days_file) as f:
            skip_days = f.read().splitlines()

    original_days = pnl_main.columns.tolist()

    pnl_map = pnl_main.drop(skip_days, axis=1, errors='ignore')
    after_skip_days = pnl_map.shape[1]
    print("Days removed because of skip_days = ", len(original_days) - after_skip_days, " New size = ", after_skip_days)

    strats_with_many_null_results = pnl_main[pnl_main.T.isnull().sum() > 0.3*after_skip_days].index.tolist()
    if len(strats_with_many_null_results) > 0:
        all_strats = pnl_main.index.tolist()
        enough_days_strats = [i for i in all_strats if i not in strats_with_many_null_results]
        pnl_map = pnl_map.loc[enough_days_strats,:]
        print("Strats removed because of NA results for more than 30% days = ", len(strats_with_many_null_results), " New strat size = ", len(enough_days_strats))
        print("Names of Strats removed are ", strats_with_many_null_results)
        print("Please check and regenerate results for these strats. Continuing for now by removing these.")

    pnl_map = pnl_map.dropna(axis=1)
    drop_NA_days = pnl_map.shape[1]
    print("Days removed because of NA results for some strategies = ", after_skip_days - drop_NA_days, " New size = ", drop_NA_days)

    pnl_map = pnl_map.loc[:, ~(pnl_map == 0).all(axis=0)]
    after_removing_all_0_num_days = pnl_map.shape[1]
    print("Days removed because of all 0 results on all strats = ", drop_NA_days - after_removing_all_0_num_days, " New size = ", after_removing_all_0_num_days)

    final_days = pnl_map.columns.tolist()
    print("Total Days removed = ", len(original_days) - len(final_days))

    main_date_list = pnl_map.columns.tolist()
    main_date_list.sort()

    if ('Feature_based_MF_PCA' in method_list or 'Feature_based_MF_CollFill' in method_list) and args.feature_file == None:
        default_file = '/spare/local/tradeinfo/day_features/stratstory_configs/' + str(shc) + '_config.txt'
        print('Feature based method called but feature file not specified. Using default file = ' + default_file )
        args.feature_file = default_file

    # Using Feature Data
    if args.feature_file != None:

        print("Getting features data.")
        feature_file = args.feature_file  #'/spare/local/tradeinfo/day_features/stratstory_configs/ZN_0_config.txt'
        features_data = get_features_events_data_for_dates(shc, end_date, feature_file, num_days , args.p_start_time, args.p_end_time)

        # Align Dates of features and pnl map
        print("Aligning dates of Pnl and features data. This may remove some dates further.")
        feature_dates_in_pnl = [x for x in main_date_list if x in features_data.columns]
        features_map = features_data[feature_dates_in_pnl]

        main_date_list = [x for x in main_date_list if x in feature_dates_in_pnl]
        pnl_map = pnl_map[main_date_list]

        print(pnl_map.shape)
        print(features_map.shape)

    if feature_value_estimate_file != None:
        next_day_feature_value = []
        lines = open(feature_value_estimate_file, 'r').read().splitlines()
        for line in lines:
            if line.strip() == 'nan':
                next_day_feature_value.append(np.nan)
            else:
                next_day_feature_value.append(float(line.strip()))

        next_day_feature_value = np.array(next_day_feature_value)
    else:
        next_day_feature_value = None

    main_date_list = pnl_map.columns.tolist()
    main_date_list.sort()
    if (end_date != max(main_date_list)):
        end_date = max(main_date_list)
        print("Result/feature data not available for all strategies for provided end_date. End date changed to ", end_date)

    print("Total Strategies = ", pnl_map.shape[0])
    print("Total Days = ", pnl_map.shape[1])
    print("Days in testing set may be less because of walkforward prediction. If past 20 days average is needed, we predict from 21st date.")

    list_of_dates = GetListOfDates_v2(end_date, num_days, main_date_list)
    list_of_dates = list_of_dates[::-1]
    averaged_matrix_main = {}
    max_days = max(average_over_days)-1
    list_of_dates_2 = list_of_dates[max_days:]

    # For each date, fetch and store the average of past days to avoid re-computation.

    for day in list_of_dates_2:
        averaged_matrix_main[day] = get_averaged_matrix(day, pnl_map, main_date_list, average_over_days)

    prediction_correlation_error = []
    counter = 0
    zero_counter = 0
    positive_coeffs = True

    categorical_columns = ['is_3_degree_event_present']

    if args.learn_predict_flag != 1:

        all_predicted_pnls_for_all_days = {}

        if 'Base' in method_list:
            model_type_names = method_list
        else:
            model_type_names = ['Base'] + method_list

        print("\n-------------------------------------------------------------------------------------")
        print("\n Calculating accuracy of different methods in walkforward setup.")

        for day in list_of_dates_2[2:-1]:

            tomorrow = list_of_dates_2[list_of_dates_2.index(day)+1]
            actual_pnls = pnl_map[tomorrow].values

            # Maintain the order of ifs same as order of list 'method_code_side_name'. This is important to output
            prediction_list = []

            # Appending Base output by default for comparision
            base_model_prediction = list(averaged_matrix_main[day].dot(weights))
            prediction_list.append(base_model_prediction)

            if 'Zero_Model' in method_list:
                #Zero Model output. Model always predicts 0 for each day, each strat
                zero_model_prediction = list(averaged_matrix_main[day].dot(zero_weights))
                prediction_list.append(zero_model_prediction)

            if 'Linear' in method_list:
                # Learnt weights walkforward prediction regression
                regr = fit_model_on_day(day, averaged_matrix_main, pnl_map, main_date_list, average_over_days)
                linear_pnl_predict = list(regr.predict(averaged_matrix_main[day])) # Given X, predict y_hat
                prediction_list.append(linear_pnl_predict)

            if 'Reg_Linear' in method_list:
                # Learnt weights walkforward prediction lasso regression
                clf = fit_model_on_day_lasso(day, averaged_matrix_main, pnl_map, main_date_list, average_over_days,
                                             positive_coeffs)
                reg_linear_pnl_predict = list(clf.predict(averaged_matrix_main[day])) # Given X, predict y_hat
                prediction_list.append(reg_linear_pnl_predict)

            if 'Naive_PCA' in method_list:
                # PCA split of SxD. Naive averaging prediction of next day from 5xD
                naive_pca_pnl_predict = list(predict_next_day_pnl(day,pnl_map, PCA_Days, main_date_list, average_over_days, weights, components,  alpha, beta, steps, False ))
                prediction_list.append(naive_pca_pnl_predict)

            if 'Regression_PCA' in method_list:
                # PCA split of SxD. Time series regression of next day (5x1(of d+1th day)) from 5xD
                regression_pca_pnl_predict = list(predict_next_day_pnl_method_2(day,pnl_map, main_date_list, average_over_days, components, alpha, beta, steps, False, False))
                prediction_list.append(regression_pca_pnl_predict)

            if 'Lasso_PCA' in method_list:
                # PCA split of SxD. Time series lasso regression of next day (5x1(of d+1th day)) from 5xD
                lasso_pca_pnl_predict = list(predict_next_day_pnl_method_2(day,pnl_map, main_date_list, average_over_days, components, alpha, beta, steps, True, False ))
                prediction_list.append(lasso_pca_pnl_predict)

            if 'Naive_CollFill' in method_list:
                # CollFill split of SxD. Naive averaging prediction of next day from 5xD
                naive_coll_fill_pnl_predict = list(predict_next_day_pnl(day,pnl_map, PCA_Days, main_date_list, average_over_days, weights, components,  alpha, beta, steps, True ))
                prediction_list.append(naive_coll_fill_pnl_predict)

            if 'Regression_CollFill' in method_list:
                # CollFill split of SxD. Time series regression of next day (5x1(of d+1th day)) from 5xD
                regression_coll_fill_pnl_predict = list(predict_next_day_pnl_method_2(day,pnl_map, main_date_list, average_over_days, components, alpha, beta, steps, False, True))
                prediction_list.append(regression_coll_fill_pnl_predict)

            if 'Lasso_CollFill' in method_list:
                # CollFill split of SxD. Time series lasso regression of next day (5x1(of d+1th day)) from 5xD
                lasso_coll_fill_pnl_predict = list(predict_next_day_pnl_method_2(day,pnl_map, main_date_list, average_over_days, components, alpha, beta, steps, True, True ))
                prediction_list.append(lasso_coll_fill_pnl_predict)

            if 'Feature_based_MF_PCA' in method_list:
                # MF using PCA and also using features space. While generating accuracy numbers, we take next days feature value as average prediction of last days.
                feature_based_MF_PCA_pnl_predict = list(predict_next_day_pnl_features_method_1(day, pnl_map, features_map, None, categorical_columns, "PCA", main_date_list, \
                                                                                                                average_over_days, weights, components, alpha, beta, steps))
                prediction_list.append(feature_based_MF_PCA_pnl_predict)

            if 'Feature_based_MF_CollFill' in method_list:
                # MF using Collaborative Filtering and also using features space. While generating accuracy numbers, we take next days feature value as average prediction of last days.
                feature_based_MF_collaborative_filtering_pnl_predict = list(predict_next_day_pnl_features_method_1(day, pnl_map, features_map, None, categorical_columns, "Col_Fill", main_date_list, \
                                                                                                                average_over_days, weights, components, alpha, beta, steps))
                prediction_list.append(feature_based_MF_collaborative_filtering_pnl_predict)

            prediction_df = pd.DataFrame(prediction_list, columns = pnl_map.index, index = model_type_names)
            all_predicted_pnls_for_all_days[day] = prediction_df.T

            day_errors = [day]
            measure_list = ['Rank_Corr', 'SSE', 'top_10_pnl', 'top_5_pnl']
            for measure in measure_list:
                for pred in prediction_list:
                    if measure == 'Rank_Corr':
                        day_errors.append(get_rank_correlation(actual_pnls, pred))
                    elif measure == 'SSE':
                        day_errors.append(get_normalized_error_for_day(actual_pnls, pred))
                    elif measure == 'top_10_pnl':
                        day_errors.append(
                            compare_top_n_predicted_strats_with_base(actual_pnls, pred, base_model_prediction, 10))
                    else:
                        day_errors.append(
                            compare_top_n_predicted_strats_with_base(actual_pnls, pred, base_model_prediction, 5))

            prediction_correlation_error.append(day_errors)

        accuracy_matrix = get_accuracy_matrix(model_type_names, prediction_correlation_error, measure_list, pnl_map.shape[0])
        print(accuracy_matrix)
        print("\n-------------------------------------------------------------------------------------")

        if portfolio_report == 1:
            # pnl_map is SxD. all_predicted_pnls_for_all_days is dictonary with key as date and value as dataframe of DxM
            get_portfolio_report(shc, pnl_map, all_predicted_pnls_for_all_days, args.session_name)

    if args.learn_predict_flag != 0:
        next_date = calc_next_week_day(list_of_dates_2[-1])
        print("\nCalculating model output for next week day which is ", next_date)
        for day in [list_of_dates_2[-1]]:

            # Maintain the order of ifs same as order of list 'method_code_side_name'. This is important to output
            prediction_list = []

            if 'Base' in method_list:
                #Base model output
                base_model_prediction = list(averaged_matrix_main[day].dot(weights))
                prediction_list.append(base_model_prediction)

            if 'Zero_Model' in method_list:
                #Zero Model output. Model always predicts 0 for each day, each strat
                zero_model_prediction = list(averaged_matrix_main[day].dot(zero_weights))
                prediction_list.append(zero_model_prediction)

            if 'Linear' in method_list:
                # Learnt weights walkforward prediction regression
                regr = fit_model_on_day(day, averaged_matrix_main, pnl_map, main_date_list, average_over_days)
                linear_pnl_predict = list(regr.predict(averaged_matrix_main[day])) # Given X, predict y_hat
                prediction_list.append(linear_pnl_predict)

            if 'Reg_Linear' in method_list:
                # Learnt weights walkforward prediction lasso regression
                clf = fit_model_on_day_lasso(day, averaged_matrix_main, pnl_map, main_date_list, average_over_days,
                                             positive_coeffs)
                reg_linear_pnl_predict = list(clf.predict(averaged_matrix_main[day])) # Given X, predict y_hat
                prediction_list.append(reg_linear_pnl_predict)

            if 'Naive_PCA' in method_list:
                # PCA split of SxD. Naive averaging prediction of next day from 5xD
                naive_pca_pnl_predict = list(predict_next_day_pnl(day,pnl_map, PCA_Days, main_date_list, average_over_days, weights, components,  alpha, beta, steps, False ))
                prediction_list.append(naive_pca_pnl_predict)

            if 'Regression_PCA' in method_list:
                # PCA split of SxD. Time series regression of next day (5x1(of d+1th day)) from 5xD
                regression_pca_pnl_predict = list(predict_next_day_pnl_method_2(day,pnl_map, main_date_list, average_over_days, components, alpha, beta, steps, False, False))
                prediction_list.append(regression_pca_pnl_predict)

            if 'Lasso_PCA' in method_list:
                # PCA split of SxD. Time series lasso regression of next day (5x1(of d+1th day)) from 5xD
                lasso_pca_pnl_predict = list(predict_next_day_pnl_method_2(day,pnl_map, main_date_list, average_over_days, components, alpha, beta, steps, True, False ))
                prediction_list.append(lasso_pca_pnl_predict)

            if 'Naive_CollFill' in method_list:
                # CollFill split of SxD. Naive averaging prediction of next day from 5xD
                naive_coll_fill_pnl_predict = list(predict_next_day_pnl(day,pnl_map, PCA_Days, main_date_list, average_over_days, weights, components,  alpha, beta, steps, True ))
                prediction_list.append(naive_coll_fill_pnl_predict)

            if 'Regression_CollFill' in method_list:
                # CollFill split of SxD. Time series regression of next day (5x1(of d+1th day)) from 5xD
                regression_coll_fill_pnl_predict = list(predict_next_day_pnl_method_2(day,pnl_map, main_date_list, average_over_days, components, alpha, beta, steps, False, True))
                prediction_list.append(regression_coll_fill_pnl_predict)

            if 'Lasso_CollFill' in method_list:
                # CollFill split of SxD. Time series lasso regression of next day (5x1(of d+1th day)) from 5xD
                lasso_coll_fill_pnl_predict = list(predict_next_day_pnl_method_2(day,pnl_map, main_date_list, average_over_days, components, alpha, beta, steps, True, True ))
                prediction_list.append(lasso_coll_fill_pnl_predict)

            if 'Feature_based_MF_PCA' in method_list:
                # MF using PCA and also using features space. While generating accuracy numbers, we take next days feature value as average prediction of last days.
                feature_based_MF_PCA_pnl_predict = list(predict_next_day_pnl_features_method_1(day, pnl_map, features_map, None, categorical_columns, "PCA", main_date_list, \
                                                                                                                average_over_days, weights, components, alpha, beta, steps))
                prediction_list.append(feature_based_MF_PCA_pnl_predict)

            if 'Feature_based_MF_CollFill' in method_list:
                # F using Collaborative Filtering and also using features space. While generating accuracy numbers, we take next days feature value as average prediction of last days.
                feature_based_MF_collaborative_filtering_pnl_predict = list(predict_next_day_pnl_features_method_1(day, pnl_map, features_map, None, categorical_columns, "Col_Fill", main_date_list, \
                                                                                                                average_over_days, weights, components, alpha, beta, steps))
                prediction_list.append(feature_based_MF_collaborative_filtering_pnl_predict)

            model_type_names = method_list

            out_df = pd.DataFrame(np.array(prediction_list).T,columns=model_type_names)
            out_df.index = pnl_map.index
            out_df = out_df.reset_index()
            out_df.rename(columns = {'index':'Strat'}, inplace=True)
            out_df = out_df.round(pd.Series([2]*len(model_type_names), index = model_type_names))

            if work_dir == None:
                work_dir = "/media/shared/ephemeral16/" + getpass.getuser() + "/pnl_prediction/" + shc + "/" + \
                       str(int(time.time() * 1000)) + "/"
                os.system("mkdir -p " + work_dir)

            out_file = work_dir+ "/prediction_" + str(next_date)

            out_df.to_csv(out_file, index = False)
            print("File with predictions is : ", out_file)
