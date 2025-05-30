
import sys
import os
import argparse
import time
import getpass
import numpy as np
import pandas as pd
import MySQLdb
import json

from constraints_related_functions import *
from create_data_related_functions import *
from optimization_related_functions import *
from risk_specific_functions_variables import *

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.utils.date_utils import get_week_day_list_from_day
from walkforward.definitions import execs

import warnings
warnings.filterwarnings("ignore", category = MySQLdb.Warning)


def get_risk_output_unconstrained(pnl_series_df_pivot, shc_in_consideration, tree_df, run_date, weight_vector_dates_to_use, debug_mode):
    """
    Run unconstrained optimization for all levels and create output 
    
    :param pnl_series_df_pivot:             dataframe
    :param shc_in_consideration:            list
    :param tree_df:                         dataframe
    :param run_date:                        int
    :param weight_vector_dates_to_use:      list
    :param work_dir:                        str
    :return:
    
    tree_complete_df:                       dataframe. with unconstrained weights. 
    shc_stats_df:                           dataframe. Stats for all shortcodes
    """

    date_pnl_with_all_products = pnl_series_df_pivot['Date'].tolist()

    tree_df_rest_shcs = tree_df[~tree_df['Shc_session'].isin(shc_in_consideration)]
    tree_df_useful_shcs = tree_df[tree_df['Shc_session'].isin(shc_in_consideration)]

    # Defining empty weight dfs for different levels
    exchange_weights = pd.DataFrame(columns=['Exchange', 'Exchange_weight'])
    super_session_weights = pd.DataFrame(columns=['Exchange', 'Super_session', 'Super_session_weight'])
    shc_weights = pd.DataFrame(columns=['Exchange', 'Super_session', 'Shc_session', 'Shc_weight'])

    # We have got all data for the config which is provided.
    # Now, we can go on to loop over exchanges and the session to find their pnl_series and weights
    exchange_level_pnl = pd.DataFrame()
    exchange_level_pnl['Date'] = date_pnl_with_all_products
    exchange_list = []
    shc_stats_df_list = []
    for exchange in set(tree_df_useful_shcs['Exchange'].tolist()):
        if debug_mode:
            print('\nRunning for exchange : ' + exchange)

        super_session_level_pnl = pd.DataFrame()
        super_session_level_pnl['Date'] = date_pnl_with_all_products
        super_session_level_pnl['Date'] = super_session_level_pnl['Date'].astype(int)
        super_session_list = []
        for super_session in set(tree_df_useful_shcs[tree_df_useful_shcs['Exchange'] == exchange]['Super_session'].tolist()):
            if debug_mode:
                print('\nRunning for super_session : ' + super_session)

            tree_df_exchange_super_session = tree_df_useful_shcs[(tree_df_useful_shcs['Exchange'] == exchange) & (tree_df_useful_shcs['Super_session'] == super_session)]
            shc_session_list = list(set(tree_df_exchange_super_session['Shc_session'].tolist()))

            pnl_series_df_v2 = pnl_series_df_pivot[['Date'] + shc_session_list].copy()
            weight_and_combined_pnl_output = get_weights_and_combined_pnl_series(pnl_series_df_v2, 'Shc',
                                                                                 weight_vector_dates_to_use, debug_mode)
            super_session_level_pnl[super_session] = list(weight_and_combined_pnl_output[1])

            for shc, wt in weight_and_combined_pnl_output[0]:
                shc_weights.loc[shc_weights.shape[0]] = [exchange, super_session, shc, wt]

            temp_stats_df = weight_and_combined_pnl_output[2]
            temp_stats_df['Session'] = super_session
            temp_stats_df['Exchange'] = exchange
            temp_stats_df['Date'] = run_date
            shc_stats_df_list.append(temp_stats_df)

        print("\nExchange = " + exchange)
        weight_and_combined_pnl_output = get_weights_and_combined_pnl_series(super_session_level_pnl, 'Super_Session',
                                                                             weight_vector_dates_to_use, debug_mode)
        exchange_level_pnl[exchange] = list(weight_and_combined_pnl_output[1])

        for super_sess, wt in weight_and_combined_pnl_output[0]:
            super_session_weights.loc[super_session_weights.shape[0]] = [exchange, super_sess, wt]

    print("\nOverall combined")
    weight_and_combined_pnl_output = get_weights_and_combined_pnl_series(exchange_level_pnl, 'Exchange',
                                                                         weight_vector_dates_to_use, debug_mode)

    for ex, wt in weight_and_combined_pnl_output[0]:
        exchange_weights.loc[exchange_weights.shape[0]] = [ex, wt]

    shc_stats_df = pd.concat(shc_stats_df_list, axis=0, ignore_index=True)

    # Get all the weights in a single table for easy reading
    tree_df_useful_shcs = tree_df_useful_shcs.merge(shc_weights, on = ['Exchange', 'Super_session', 'Shc_session'], how = 'left')
    tree_df_useful_shcs = tree_df_useful_shcs.merge(super_session_weights, on=['Exchange', 'Super_session'], how='left')
    tree_df_useful_shcs = tree_df_useful_shcs.merge(exchange_weights, on=['Exchange'], how='left')

   # print("\nFinal tree directory with weights for date : ", run_date)

    tree_complete_df = pd.concat([tree_df_useful_shcs, tree_df_rest_shcs], axis=0).reset_index()

    tree_complete_df = tree_complete_df[['Exchange', 'Super_session', 'Shc_session', 'Shc_weight', 'Super_session_weight', 'Exchange_weight',
                                         'Exchange_min_risk','Exchange_max_risk', 'Super_session_min_risk','Super_session_max_risk',
                                         'Shc_session_min_risk', 'Shc_session_max_risk']]
    tree_complete_df.rename(columns = {'Super_session' : 'Session', 'Shc_session' : 'Shc', 'Super_session_weight' : 'Session_wt', 'Shc_weight' : 'Shc_wt', 'Exchange_weight' : 'Exchange_wt'}, inplace=True)
    #print(tree_complete_df)

    tree_complete_df['Date'] = run_date

    return tree_complete_df, shc_stats_df


def get_risk_output_constrained(pnl_series_df_pivot, shc_in_consideration, tree_df, run_date, weight_vector_dates_to_use,
                                all_days_real_pnl_data_norm, all_days_real_pnl_data_actual,
                                risk_factor, regularization_factor, min_risk_cap,
                                prev_weights_last, temporal_constraint, temporal_change_allowance,
                                backtest_mode, debug_mode, work_dir):
    """
    
    :param pnl_series_df_pivot:             dataframe
    :param shc_in_consideration:            list
    :param tree_df:                         dataframe
    :param run_date:                        int
    :param weight_vector_dates_to_use:      list
    :param all_days_real_pnl_data_norm:     dataframe
    :param all_days_real_pnl_data_actual:   dataframe
    :param prev_weights_last:               dataframe
    :param risk_factor:                     dictionary of risk factors of session,exchange and overall firm level
    :param temporal_constraint:             int 1/0
    :param temporal_change_allowance:       dict
    :param backtest_mode:                   int 1/0
    :param work_dir:                        str
    :return:
     
    tree_complete_df:                       dataframe.
    shc_stats_df:                           dataframe.
    prev_weights:                           dataframe. Updated with the current weights. 
    TODO WILL NEED TO REMOVE THIS WHEN READ FROM DB!!!!!
    TODO Remove unnecessary logging and column creation in production
    """


    # Get date weighted mean and cov matrices
    mean_pnl_series, cov_df, corr_df = get_mean_covariance(pnl_series_df_pivot[shc_in_consideration],
                                                           weight_vector_dates_to_use)

    mean_pnl_series.to_csv(work_dir + 'meanMatrix')
    cov_df.to_csv(work_dir + 'covMatrix')
    corr_df.to_csv(work_dir + 'corrMatrix')

    # Get unconstrained weights
    if debug_mode:
        print("\n  Getting unconstrained weights ")
    unconstrained_risk_output = get_risk_output_unconstrained(pnl_series_df_pivot, shc_in_consideration, tree_df,
                                                         run_date, weight_vector_dates_to_use, debug_mode)
    unconstrained_risk_df = unconstrained_risk_output[0]

    # Create wts for shc level which add up to 1
    unconstrained_risk_df['unconstrained_wts_for_shc'] = list(
        map(lambda x, y, z: x * y * z, unconstrained_risk_df['Exchange_wt'], unconstrained_risk_df['Session_wt'],
            unconstrained_risk_df['Shc_wt']))

    # Merge unconstrained wts with the final df
    wts_to_merge = unconstrained_risk_df[['Shc', 'unconstrained_wts_for_shc']].rename(columns={'Shc': 'Shc_session'})
    tree_df_final_wts = pd.merge(tree_df, wts_to_merge, on='Shc_session', how='left')

    # Assign risk to products which have less data but trader has given a minimum risk to it.
    # Minimum risk is provided to such products. Constraints are also updated accordingly
    tree_df_final_wts_min_risk_products_adjusted, min_risk_shc_weights = assign_min_risk_to_products(tree_df_final_wts)
    new_total_risk = tree_df_final_wts_min_risk_products_adjusted['Overall_risk'].tolist()[0]

    # Create cosntraints based on min_max risks, cov matrix and the initial uncontrained weights
    if debug_mode:
        print("\n\n  Getting constrained weights now")
        print("\nCreating constraints")
    constraint_matrix_df = create_constraint_matrix(tree_df_final_wts_min_risk_products_adjusted, new_total_risk, cov_df, risk_factor, debug_mode)

    temporal_constraints_df = pd.DataFrame()
    if temporal_constraint != 0:
        if debug_mode:
            print("\nCreating Temporal Constraints")
        # Returns empty df if last 5 days weights are not available
        temporal_constraints_df = create_temporal_constraint(prev_weights_last, run_date, temporal_change_allowance)

    print("\nSummary Stats for all shortcodes ")
    stats_df = get_pnl_stats_for_all_columns(pnl_series_df_pivot, weight_vector_dates_to_use)
    print(stats_df)
    stats_df['Date'] = run_date

    if len(min_risk_shc_weights) > 0:
        print("\nShortcodes (and weight) with less data but minimum risk constraints ")
        print("Shortcodes: ", " ".join(list(np.array(min_risk_shc_weights).T[0])))
        print("Final Weights: ", " ".join(list(map(str, np.array(min_risk_shc_weights).T[1]))))

    print("\nFinding optimal weights for shortcodes under constraints")
    # Find optimal weights wrt to the constraints
    shc_weights_list = get_weight_constrained_sharpe_optim(mean_pnl_series, cov_df, constraint_matrix_df, risk_factor, new_total_risk,
                                                           regularization_factor, temporal_constraints_df, temporal_change_allowance["push_risk"], backtest_mode)


    # Add minimum risk constrained shortcodes which have insufficient data
    shc_weights_list.extend(min_risk_shc_weights)
    shc_weights = pd.DataFrame(data=shc_weights_list, columns=['Shc_session', 'Shc_weight'])
    shc_weights['Shc_weight'] = shc_weights['Shc_weight'].round(0)
    # TODO : Can change this cap if needed
    shc_weights['Shc_weight'] = list(map(lambda x: 0 if x < min_risk_cap else x, shc_weights['Shc_weight']))

    # Merge the optimal weights with the current df
    tree_df_final_wts = tree_df_final_wts.merge(shc_weights, on=['Shc_session'], how='left')

    # Create columns for session and exchange risk
    tree_df_final_wts = make_sum_of_risk_columns(tree_df_final_wts, corr_df, risk_factor)

    tree_df_final_wts.drop(['Shc', 'Sub_session'], axis=1, errors='ignore', inplace=True)
    tree_df_final_wts['Date'] = run_date

    # Create few more columns for display
    tree_df_final_wts = make_actual_system_pnl_columns(tree_df_final_wts, shc_weights, shc_in_consideration, products_data_available,
                                   all_days_real_pnl_data_norm, all_days_real_pnl_data_actual, run_date)

    tree_df_final_wts = make_adjusted_system_pnl_columns(tree_df_final_wts, corr_df, risk_factor)

    tree_df_final_wts = make_sharpe_columns(tree_df_final_wts, cov_df, mean_pnl_series)

    # Update prev_weights. Will be fetching from DB later
    prev_weights = prev_weights_last
    if backtest_mode and temporal_constraint:
        prev_weights_cols = prev_weights_last.columns.tolist()
        prev_weights_current = tree_df_final_wts[['Date', 'Overall_wt_wrt_corr', 'Shc_session', 'Shc_weight']].pivot_table(
            values='Shc_weight', index=['Date', 'Overall_wt_wrt_corr'], columns='Shc_session').reset_index()
        del (prev_weights_current.columns.name)
        prev_weights = pd.concat([prev_weights_last, prev_weights_current])
        prev_weights = prev_weights[prev_weights_cols]

    # Make output for a day in line to output of other ifs
    risk_output_for_day = []
    risk_output_for_day.append(tree_df_final_wts)
    risk_output_for_day.append(stats_df)

    return tree_df_final_wts, stats_df, prev_weights


def get_cumulative_pnl_data(all_days_real_pnl_data, all_run_date_list, num_days_to_look_back):
    """
    Get cumulative pnl values
    
    :param all_days_real_pnl_data:      dataframe 
    :param all_run_date_list:           list
    :param num_days_to_look_back:       int
    :return:                            dataframe. Date x Shc of cumulative pnl
    """


    all_days_real_pnl_data_sorted = all_days_real_pnl_data.sort_values('Date')
    cum_df = all_days_real_pnl_data_sorted.set_index('Date').fillna(0).cumsum(axis=0)
    cum_df_v2 = cum_df.iloc[cum_df.shape[0] - len(all_run_date_list)-1:-1,:]
    cum_df_v2.index = all_run_date_list
    cum_df_v2 = cum_df_v2.reset_index()
    cum_df_v2.rename(columns= {'index':'Date'}, inplace=True)
    return cum_df_v2


def make_sum_of_risk_columns(tree_df_final_wts, corr_df, risk_factor):
    """
    Create columns of session/exchange/overall level risk in two fashion :-
    1. Simple summation of risks
    2. Corr adjusted summation of risks
    
    :param tree_df_final_wts:   dataframe 
    :param corr_df:             dataframe
    :param risk_factor:         dictionary of risk factors of session,exchange and overall firm level
    :return:                    Updates the tree_df_final_wts
    """
    # Sum up optimal weights of shortcode for a session to get session level risk
    tree_df_final_wts = tree_df_final_wts.groupby(['Exchange', 'Super_session']).apply(add_shc_weights,
                                                                                       'Shc_weight',
                                                                                       'Super_session_weight')

    # Sum up optimal weights of shortcode for a exchange to get exchange level risk
    tree_df_final_wts = tree_df_final_wts.groupby('Exchange').apply(add_shc_weights, 'Shc_weight',
                                                                    'Exchange_weight')

    # Sum up optimal weights of shortcode to get overall level risk
    tree_df_final_wts = tree_df_final_wts.groupby('Overall_risk').apply(add_shc_weights, 'Shc_weight',
                                                                        'Overall_sum_weight')

    # Sum up optimal weights of shortcode for a session to get session level risk
    # but considering cov matrix sqrt(w.T*corr*w)
    session_factor = risk_factor['session']
    exchange_factor = risk_factor['exchange']
    overall_factor = risk_factor['overall']
    if len(set(tree_df_final_wts['Exchange'].tolist())) == 1:
        overall_factor = exchange_factor
    tree_df_final_wts = tree_df_final_wts.groupby(['Exchange', 'Super_session']).apply(add_shc_weights_wrt_cov,
                                                                                       'Shc_weight',
                                                                                       'Session_wt_wrt_corr', corr_df,
                                                                                       'Shc_session', session_factor)

    # Sum up optimal weights of shortcode for a exchange to get exchange level risk
    # but considering cov matrix sqrt(w.T*corr*w)
    tree_df_final_wts = tree_df_final_wts.groupby('Exchange').apply(add_shc_weights_wrt_cov, 'Shc_weight',
                                                                    'Exchange_wt_wrt_corr', corr_df, 'Shc_session', exchange_factor)

    # Sum up optimal weights of shortcode to get overall level risk
    # but considering cov matrix sqrt(w.T*corr*w)
    tree_df_final_wts = tree_df_final_wts.groupby('Overall_risk').apply(add_shc_weights_wrt_cov, 'Shc_weight',
                                                                        'Overall_wt_wrt_corr', corr_df, 'Shc_session', overall_factor)

    return tree_df_final_wts


def make_actual_system_pnl_columns(tree_df_final_wts, shc_weights, shc_in_consideration, products_data_available,
                                   all_days_real_pnl_data_norm, all_days_real_pnl_data_actual, run_date):
    """
    Creates columns for display in ppt.
    
    :param tree_df_final_wts: 
    :param shc_weights: 
    :param shc_in_consideration: 
    :param all_days_real_pnl_data_norm: 
    :param all_days_real_pnl_data_actual: 
    :param run_date: 
    :return: 
    """

    wts_real_pnl_data_overall = []
    real_pnl_data_overall = []
    real_pnl_data_norm_overall = []
    if run_date in all_days_real_pnl_data_norm['Date'].tolist():

        wts = shc_weights[shc_weights['Shc_session'].isin(shc_in_consideration)]
        shc_with_non_zero_weights = shc_weights[shc_weights['Shc_weight'] > 1]['Shc_session'].tolist()
        days_real_pnl = all_days_real_pnl_data_norm[all_days_real_pnl_data_norm['Date'] == run_date][shc_in_consideration].iloc[0].reset_index()
        days_real_pnl.columns = ['Shc_session', 'Pnl_per_unit_Risk']
        wts_real_pnl_data = pd.merge(wts, days_real_pnl, how='left')
        wts_real_pnl_data['system_pnl'] = list(map(lambda x, y: x * y, wts_real_pnl_data['Shc_weight'], wts_real_pnl_data['Pnl_per_unit_Risk']))
        wts_real_pnl_data['Date'] = run_date
        wts_real_pnl_data_overall.append(wts_real_pnl_data[['Date', 'Shc_session', 'system_pnl']])
        tree_df_final_wts['PNL_using_risk_provided_by_system'] = np.nansum(wts_real_pnl_data['Shc_weight'] * wts_real_pnl_data['Pnl_per_unit_Risk'])
        tree_df_final_wts['Actual_PNL_of_products_assigned_non_zero_risk'] = all_days_real_pnl_data_actual[all_days_real_pnl_data_actual['Date'] == run_date][shc_with_non_zero_weights].sum(axis=1).iloc[0]
        tree_df_final_wts['Actual_PNL_of_all_products_considered'] = all_days_real_pnl_data_actual[all_days_real_pnl_data_actual['Date'] == run_date][shc_in_consideration].sum(axis=1).iloc[0]
        tree_df_final_wts['Actual_PNL_of_all_products'] = all_days_real_pnl_data_actual[all_days_real_pnl_data_actual['Date'] == run_date][products_data_available].sum(axis=1).iloc[0]
        real_pnl_data = all_days_real_pnl_data_actual[all_days_real_pnl_data_actual['Date'] == run_date][products_data_available].T.reset_index()
        real_pnl_data.columns = ['Shc_session', 'actual_pnl']
        real_pnl_data['Date'] = run_date
        real_pnl_data_overall.append(real_pnl_data)
        real_pnl_data_norm = all_days_real_pnl_data_norm[all_days_real_pnl_data_norm['Date'] == run_date][
            products_data_available].T.reset_index()
        real_pnl_data_norm.columns = ['Shc_session', 'actual_pnl_norm']
        real_pnl_data_norm['Date'] = run_date
        real_pnl_data_norm_overall.append(real_pnl_data_norm)
    else:
        tree_df_final_wts['PNL_using_risk_provided_by_system'] = 0
        tree_df_final_wts['Actual_PNL_of_products_assigned_non_zero_risk'] = 0
        tree_df_final_wts['Actual_PNL_of_all_products_considered'] = 0
        tree_df_final_wts['Actual_PNL_of_all_products'] = 0

    if len(wts_real_pnl_data_overall) != 0:
        wts_real_pnl_data_overall_df = pd.concat(wts_real_pnl_data_overall, axis=0)
        tree_df_final_wts = pd.merge(tree_df_final_wts, wts_real_pnl_data_overall_df, on=['Date', 'Shc_session'],
                                     how='left')
    else:
        tree_df_final_wts['system_pnl'] = 0

    if len(real_pnl_data_overall) != 0:
        real_pnl_data_overall_df = pd.concat(real_pnl_data_overall, axis=0)
        tree_df_final_wts = pd.merge(tree_df_final_wts, real_pnl_data_overall_df, on=['Date', 'Shc_session'], how='left')
    else:
        tree_df_final_wts['actual_pnl'] = 0

    if len(real_pnl_data_norm_overall) != 0:
        real_pnl_data_norm_overall_df = pd.concat(real_pnl_data_norm_overall, axis=0)
        tree_df_final_wts = pd.merge(tree_df_final_wts, real_pnl_data_norm_overall_df, on=['Date', 'Shc_session'],
                                     how='left')
    else:
        tree_df_final_wts['actual_pnl_norm'] = 0

    return tree_df_final_wts


def make_adjusted_system_pnl_columns(tree_df_final_wts ,corr_df, risk_factor):
    """
    Create columns for system adjusted pnl values

    :param tree_df_final_wts:   dataframe
    :param corr_df:             dataframe
    :param risk_factor:         dictionary of risk factors of session,exchange and overall firm level
    :return:
    """

    tree_df_final_wts['actual_risk'] = list(map(lambda x,y: x/y if (y!=0 and ~np.isnan(y)) else 0, tree_df_final_wts['actual_pnl'], tree_df_final_wts['actual_pnl_norm']))
    session_factor = risk_factor['session']
    exchange_factor = risk_factor['exchange']
    overall_factor = risk_factor['overall']
    if len(set(tree_df_final_wts['Exchange'].tolist())) == 1:
        overall_factor = exchange_factor

    tree_df_final_wts = tree_df_final_wts.groupby(['Exchange', 'Super_session']).apply(add_shc_weights_wrt_cov,
                                                                                       'actual_risk',
                                                                                       'Real_Session_wt_wrt_corr', corr_df,
                                                                                       'Shc_session', session_factor)

    # Sum up optimal weights of shortcode for a exchange to get exchange level risk
    # but considering cov matrix sqrt(w.T*corr*w)
    tree_df_final_wts = tree_df_final_wts.groupby('Exchange').apply(add_shc_weights_wrt_cov, 'actual_risk',
                                                                    'Real_Exchange_wt_wrt_corr', corr_df, 'Shc_session', exchange_factor)

    # Sum up optimal weights of shortcode to get overall level risk
    # but considering cov matrix sqrt(w.T*corr*w)
    tree_df_final_wts = tree_df_final_wts.groupby('Overall_risk').apply(add_shc_weights_wrt_cov, 'actual_risk',
                                                                        'Real_Overall_wt_wrt_corr', corr_df, 'Shc_session', overall_factor)

    tree_df_final_wts['Adjusted_System_PNL'] = list(map(lambda x,y,z: x*y/z if (z!=0 and ~np.isnan(z)) else 0,
                                                        tree_df_final_wts['PNL_using_risk_provided_by_system'],
                                                        tree_df_final_wts['Real_Exchange_wt_wrt_corr'],
                                                        tree_df_final_wts['Exchange_wt_wrt_corr']))

    return tree_df_final_wts


def make_sharpe_columns(tree_df_final_wts ,cov_df, mean_series):
    """
    Create columns for system adjusted pnl values

    :param tree_df_final_wts:   dataframe
    :param cov_df:             dataframe
    :param mean_series:         pandas series
    :param risk_factor:         dictionary of risk factors of session,exchange and overall firm level
    :return:
    """

    tree_df_final_wts = tree_df_final_wts.groupby(['Exchange', 'Super_session']).apply(get_sharpe_of_group, 'Shc_weight', 'Session_sharpe', cov_df,
                                                                                       mean_series, 'Shc_session')

    tree_df_final_wts = tree_df_final_wts.groupby('Exchange').apply(get_sharpe_of_group, 'Shc_weight',
                                                                    'Exchange_sharpe', cov_df, mean_series, 'Shc_session')

    tree_df_final_wts = tree_df_final_wts.groupby('Overall_risk').apply(get_sharpe_of_group, 'Shc_weight',
                                                                        'Overall_sharpe', cov_df, mean_series, 'Shc_session')

    return tree_df_final_wts


def convert_to_db_format(final_out):
    """
    convert the final output generated to a format which db can accept

    :param final_out: dataframe
    :return:          dataframe
    """

    required_cols = ['Date', 'Overall_risk', 'Overall_wt_wrt_corr', \
                     'Exchange', 'Exchange_max_risk', 'Exchange_min_risk', 'Exchange_wt_wrt_corr', \
                     'Super_session', 'Super_session_max_risk', 'Super_session_min_risk', 'Session_wt_wrt_corr', \
                     'Shc_session', 'Shc_session_max_risk', 'Shc_session_min_risk', 'Shc_weight']

    filtered_df = final_out.loc[(final_out['Shc_weight'].notnull() & final_out['Shc_weight'] !=0 ), required_cols]
    filtered_df['shortcode'] = list(map(lambda x: x.split('.')[0], filtered_df['Shc_session']))
    filtered_df['session'] =  list(map(lambda x: x.split('.')[1], filtered_df['Shc_session']))
    shortcode_rows = filtered_df[['Date', 'shortcode', 'session', 'Shc_session_max_risk', 'Shc_session_min_risk', 'Shc_weight']]\
                     .rename(columns = {'Shc_session_max_risk':'max_risk', 'Shc_session_min_risk':'min_risk', 'Shc_weight':'risk'})

    session_level_data = filtered_df[['Date', 'Exchange', 'Super_session', 'Super_session_max_risk', 'Super_session_min_risk', 'Session_wt_wrt_corr']].drop_duplicates()
    session_rows = session_level_data.rename(columns= {'Exchange':'shortcode', 'Super_session':'session', 'Super_session_max_risk':'max_risk', 'Super_session_min_risk':'min_risk', 'Session_wt_wrt_corr':'risk'})

    exchange_level_data = filtered_df[['Date', 'Exchange', 'Exchange_max_risk', 'Exchange_min_risk', 'Exchange_wt_wrt_corr']].drop_duplicates()
    exchange_level_data['session'] = 'All'
    exchange_rows = exchange_level_data.rename(columns = {'Exchange':'shortcode', 'Exchange_max_risk':'max_risk', 'Exchange_min_risk':'min_risk', 'Exchange_wt_wrt_corr':'risk'})

    overall_level_data = filtered_df[['Date', 'Overall_risk', 'Overall_wt_wrt_corr']].drop_duplicates()
    overall_level_data['shortcode'] = 'All'
    overall_level_data['session'] = 'All'
    overall_level_data['min_risk'] = overall_level_data['Overall_risk']
    overall_rows = overall_level_data.rename(columns = {'Overall_risk':'max_risk', 'Overall_wt_wrt_corr':'risk'})

    converted_df = pd.concat([shortcode_rows, session_rows, exchange_rows, overall_rows], axis=0)
    converted_df = converted_df[['shortcode', 'session', 'Date', 'risk', 'min_risk', 'max_risk']]
    return converted_df


def remove_super_product(tree_df):
    """
    Drop duplicates at super product level and removes the super product column if present.
    Eg :- BR_WIN_0.US_HP, BR_WIN_0.US_LP, BR_WIN_0.US are same product, so their super product is same.

    :param tree_df: dataframe
    :return:        dataframe
    """

    if "Super_Product" in tree_df.columns.tolist():
        tree_df['Shc_session'] = list(map(lambda x,y: x if x!=None else y, tree_df['Super_Product'], tree_df['Shc_session']))
        tree_df.drop('Super_Product', axis=1, inplace=True)

    tree_df = tree_df.drop_duplicates(['Shc_session', 'Super_session', 'Exchange'])

    return tree_df


def remove_temp_files(work_dir):
    """
    Remove all files in the temp directory. Except the files with risk numbers.
    TODO Remove everything in production

    :param work_dir:    str 
    :return:            None
    """
    file_list = glob.glob(work_dir + '*')
    for file in file_list:
        if os.path.basename(file) != 'risk_temp_out':
            os.system("rm -f " + file)


def check_valid_super_product_names(super_product_list):
    """
    Check if all super products are provided in valid format or not

    :param super_product_list: list
    :return:                   None
    """
    for sp in super_product_list:
        shc_session_list = sp.split('.')
        msg = "Provide super product as <valid_shc>.<valid_session>. Failing for " + sp
        if len(shc_session_list) != 2:
            exit_code_with_message(msg, exit_codes['Invalid_input_file'])
        else:
            shc = shc_session_list[0]
            session = shc_session_list[1]
            if not isvalid_shortcode(shc) or not isvalid_session(session):
                exit_code_with_message(msg, exit_codes['Invalid_input_file'])


def check_validity_super_product(tree_df):
    """
    Checks validity of records of products that needs to combined.
    They should have valid name and hvae same min/max risk across super_product

    :param tree_df: dataframe
    :return:        None
    """
    if "Super_Product" in tree_df.columns.tolist():
        tree_df_super =  tree_df[tree_df['Super_Product'].notnull()].copy()
        tree_df_super.drop(['Shc_session', 'Shc', 'Sub_session'], axis=1, inplace=True)
        tree_df_super_grouped = tree_df.groupby(tree_df_super.columns.tolist()).size().reset_index()
        tree_df_super_grouped_super_product = tree_df_super_grouped.groupby('Super_Product').size().reset_index()
        super_product_list = tree_df_super.drop_duplicates('Super_Product')['Super_Product'].tolist()
        check_valid_super_product_names(super_product_list)
        if max(tree_df_super_grouped_super_product[0].tolist()) > 1:
            msg = "Some products to be combined do not have same min/max risks. Or same super product given across different session/exchange."
            exit_code_with_message(msg, exit_codes['Invalid_input_file'])


def check_super_session_validity(tree_df):
    """
    Check if the super session provided is a valid session or not

    :param tree_df: dataframe
    :return:        None
    """
    session_list = tree_df.drop_duplicates('Super_session')['Super_session'].tolist()
    for session in session_list:
        if not isvalid_session(session):
            msg = " Please provide valid session name. Failing for " + session
            exit_code_with_message(msg, exit_codes['Invalid_input_file'])


def check_exchange_validity(tree_df, input_date):
    """
    Check if the super session provided is a valid session or not

    :param tree_df: dataframe
    :return:        None
    """

    # Get list of all exchanges
    get_exchange_command = execs.execs().get_contract_specs + " ALL " + str(input_date) + " EXCHANGE | cut -d ' ' -f2 | sort | uniq"
    out = subprocess.check_output(get_exchange_command, shell=True)
    all_valid_exchange_list = out.decode('utf-8').strip().split('\n')

    exchange_list = tree_df.drop_duplicates('Exchange')['Exchange'].tolist()
    for exchange in exchange_list:
        if exchange not in all_valid_exchange_list:
            msg = " Please provide valid exchange name. Failing for " + exchange
            exit_code_with_message(msg, exit_codes['Invalid_input_file'])


def check_validtity_of_config_read(tree_df, input_date):
    """
    Check for overall validity of the config which we have read

    :param tree_df:     dataframe
    :return:            None
    """

    check_exchange_validity(tree_df, input_date)
    check_super_session_validity(tree_df)
    check_validity_super_product(tree_df)


def remove_risk_records_or_exit(exchange_runid_data, run_date, write_in_db, overwrite_db_values):
    """
    Exit if records already exists and we do not want to overwrite.
    If we want to overwrite, remove existing records if exists.

    :param exchange_runid_data:     list of list. Each list is of type [exchange, run_id]
    :param run_date:                int
    :param write_in_db:             int 0/1
    :param overwrite_db_values:     int 0/1
    :return:                        None
    """

    if exchange_runid_data != None:
        exchange_list = []
        run_ids = []
        for row in exchange_runid_data:
            exchange_list.append(row[0])
            run_ids.append(row[1])
        if write_in_db and overwrite_db_values:
            print("Overwrite flag set to true. Overwriting already existing records in db.")
            remove_risk_run_ids(convert_list_to_tuple(run_ids))
        elif write_in_db and not overwrite_db_values:
            msg = "For date = " + str(run_date) + ", Run IDs are already present for exchanges " + " ".join(exchange_list) + " .Overwrite flag set to false. Cannot write to DB. Exiting."
            exit_code_with_message(msg, exit_codes['Cannot_write_to_db'])


def parse_hfile(hfile):
    """
    Parses the input risk config file

    :param hfile:       str  
    :return:            dataframe. Each row is at level of exchange, session, shortcode.
                        For exchange and session, the min/max risk numbers will be repeated for certain rows
    """

    if not os.path.isfile(hfile):
        exit_code_with_message("Input file not present", exit_codes["Invalid_input_file"])
    else:
        if os.stat(hfile).st_size == 0:
            exit_code_with_message("Input file empty", exit_codes["Invalid_input_file"])

    tree_list = []
    current_instruction = None
    try:
        with open(hfile, 'r') as cfile:
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
                    if current_instruction == "EXCHANGE":
                        exchange_list = line.split()
                        if len(exchange_list) < 2:
                            exchange_list.append(0)
                        if len(exchange_list) < 3:
                            exchange_list.append(np.inf)
                    elif current_instruction == "SESSION":
                        session_list = line.split()
                        if len(session_list) < 2:
                            session_list.append(0)
                        if len(session_list) < 3:
                            session_list.append(np.inf)
                    elif current_instruction == "SHORTCODE":
                        shortcode_list = line.split()
                        if len(shortcode_list) < 2:
                            shortcode_list.append(0)
                        if len(shortcode_list) < 3:
                            shortcode_list.append(np.inf)
                        list_to_append = exchange_list + session_list + shortcode_list
                        tree_list.append(list_to_append)
    except:
        print(hfile + " not readable")
        raise ValueError(hfile + " not readable.")

    return tree_list


def update_input_date(input_date):
    """
    If the date is weekend, return the next weekday

    :param input_date:      int
    :return:                int
    """

    if (datetime.datetime.strptime(str(input_date), '%Y%m%d').weekday() > 4):
        output_date = calc_next_week_day(input_date, 1)
        print("Provided date is a weekend, updating the input date to next week day which is ", output_date)
        return output_date
    return input_date


def get_json_from_risk_stats(risk_stats, mode=0):
    """
    Get json to be returned for simula

    :param risk_stats:  dataframe
    :param mode:        int. mode = 1 if converting a dataframe made from reading simula format json.
    :return:            json
    """

    riskstats_jsobj = []

    if mode == 0:
        risk_stats.index.name = 'Shc_session'
        risk_stats = risk_stats.reset_index()
        risk_stats['shortcode'] = risk_stats['Shc_session'].apply(lambda x: pd.Series(x.split('.')[0]))
        risk_stats['timeperiod'] = risk_stats['Shc_session'].apply(lambda x: pd.Series(x.split('.')[1]))
        risk_stats.drop('Shc_session', axis=1, inplace=True)

    risk_stats = risk_stats.where(pd.notnull(risk_stats), None)
    risk_stats.sort_values(['Exchange', 'Type'], inplace=True)

    sessions = risk_stats['Session'].unique()
    exchanges = risk_stats['Exchange'].unique()
    sessions.sort()

    record_columns = risk_stats.columns.tolist()
    record_columns.remove('Session')
    for session in sessions:
        sess_riskstats = risk_stats.loc[risk_stats['Session'] == session, record_columns].to_dict(orient='records')
        sess_riskstats_f = {'session': session, 'values': sess_riskstats}
        riskstats_jsobj.append(sess_riskstats_f)

    return riskstats_jsobj


def get_exchange_session_pnl_risk_series(tree_df, real_pnl_series, real_risk_series, list_of_shc_considered, corr_df, risk_factor):

    # Get combined pnl series
    pnl_series_df = pd.DataFrame(index = real_pnl_series['Date'].tolist())
    pnl_concat_list = [pnl_series_df]
    for group in tree_df.groupby(['Exchange','Super_session']):
        shortcode_list = group[1]['Shc_session'].unique().tolist()
        shortcode_list = list(set(shortcode_list).intersection(set(list_of_shc_considered)))
        pnl_series = real_pnl_series.set_index('Date')[shortcode_list].sum(axis=1)
        pnl_concat_list.append(pnl_series.rename(group[0][0] + '.' + group[0][1]))

    pnl_series_df = pd.concat(pnl_concat_list, axis=1).reset_index()
    pnl_series_melt = pd.melt(pnl_series_df, id_vars = ['Date'])
    pnl_series_melt.columns = ['Date', 'Shc_session', 'Combined_Pnl']

    risk_data_melt = pd.melt(real_risk_series[list_of_shc_considered + ['Date']], id_vars=['Date'])
    risk_data_melt.columns = ['Date', 'Shc_session', 'Risk_Factor']
    risk_data_melt = risk_data_melt.merge(tree_df[['Exchange','Super_session','Shc_session']], on = 'Shc_session', how='left')
    risk_data_melt = risk_data_melt.groupby(['Date','Exchange','Super_session']).apply(add_shc_weights_wrt_cov, 'Risk_Factor', 'Combined_Risk', corr_df, 'Shc_session', risk_factor['session'])

    risk_data_combined = risk_data_melt[['Date', 'Exchange','Super_session','Combined_Risk']].drop_duplicates().rename(columns = {'Super_session':'Session'})
    risk_data_combined['Shc_session'] = list(map(lambda x,y: x + '.' + y, risk_data_combined['Exchange'], risk_data_combined['Session']))

    pnl_series_melt = pnl_series_melt.merge(risk_data_combined, on = ['Date', 'Shc_session'], how='left')
    pnl_series_melt['Combined_Pnl_Norm'] = list(map(lambda x,y: x/y if y!=0 and ~np.isnan(y) else np.nan, pnl_series_melt['Combined_Pnl'], pnl_series_melt['Combined_Risk']))

    return pnl_series_melt


def get_pnl_stats_for_json(real_pnl_data, real_pnl_data_norm, run_date, fill_holes, interval_dict,
                           previous_risks, date_stats, debug_mode, tree_df, real_risk_data, weight_vector_dates_to_use):
    """
    Get the dataframe which needs to be converted to json

    :param real_pnl_data:           dataframe
    :param real_pnl_data_norm:      dataframe
    :param run_date:                int
    :param fill_holes:              int
    :param interval_dict:           dict
    :param previous_risks:          dataframe
    :param date_stats:              dataframe
    :param debug_mode:              int
    :return:                        dataframe
    """

    run_date_minus_1 = calc_prev_week_day(run_date)
    list_of_shc_considered = date_stats[date_stats['Shc_wt'].notnull()].index.tolist()

    # Get holes-filled data from the raw pnl data
    real_pnl_series_shc = get_day_data_from_created_data(real_pnl_data, run_date, fill_holes, pd.DataFrame(), debug_mode, True, 300, 0, print_nothing=1)
    real_pnl_series_norm_shc = get_day_data_from_created_data(real_pnl_data_norm, run_date, fill_holes, pd.DataFrame(), debug_mode, True, 300, 0, print_nothing=1)

    list_of_shc_with_enough_data = real_pnl_series_shc.columns.tolist()
    list_of_shc_with_enough_data.remove('Date')
    _, _, corr_df = get_mean_covariance(real_pnl_series_norm_shc[list_of_shc_with_enough_data], weight_vector_dates_to_use)

    # Get risk data for the dates and shortcodes considered
    real_risk_series_shc = real_risk_data.loc[real_risk_data['Date'].isin(real_pnl_series_shc['Date'].unique().tolist()), list_of_shc_considered + ['Date']].copy()

    # Get pnl series and risk numbers for exchange, session here
    ex_session_combined_series = get_exchange_session_pnl_risk_series(tree_df, real_pnl_series_shc, real_risk_series_shc, list_of_shc_considered, corr_df, risk_factor)

    ex_session_list = ex_session_combined_series['Shc_session'].unique().tolist()
    real_pnl_series_ex_session = ex_session_combined_series[['Date', 'Shc_session', 'Combined_Pnl']].pivot(index='Date', columns='Shc_session',
                                                                                   values='Combined_Pnl')
    del (real_pnl_series_ex_session.columns.name)
    real_pnl_series_ex_session = real_pnl_series_ex_session.reset_index()

    real_pnl_series_norm_ex_session = ex_session_combined_series[['Date', 'Shc_session', 'Combined_Pnl_Norm']].pivot(index='Date', columns='Shc_session',
                                                                                   values='Combined_Pnl_Norm')
    del (real_pnl_series_norm_ex_session.columns.name)
    real_pnl_series_norm_ex_session = real_pnl_series_norm_ex_session.reset_index()

    real_pnl_series = real_pnl_series_shc.merge(real_pnl_series_ex_session, how='left', on='Date')
    real_pnl_series_norm = real_pnl_series_norm_shc.merge(real_pnl_series_norm_ex_session, how='left', on='Date')

    risk_stats = pd.DataFrame()
    concat_list = [risk_stats]

    # Get pnl_maxdd for YTD
    first_day_of_year = int(str(run_date)[:4] + '0101')
    stats_real_pnls = get_pnl_stats_for_all_columns(real_pnl_series.loc[real_pnl_series['Date'] >= first_day_of_year,:]).set_index('Shc')
    stats_real_pnls['pnl_maxdd'] = list(map(lambda x, y: np.nan if y == 0 else x / y, stats_real_pnls['Pnl_avg'], stats_real_pnls['Pnl_overdays_DD']))
    pnl_maxdd = stats_real_pnls['pnl_maxdd']
    concat_list.append(pnl_maxdd.rename('PNL_MAXDD_YTD'))

    # Get Pnl_avg, Normalized_pnl_avg for each interval
    intv_datelist = get_intv_to_one_weights_vec(interval_dict)
    num_dates_data_available = real_pnl_series.shape[0]

    for current_interval in np.sort(list(interval_dict.keys())):
        stats_intv_pnls = get_pnl_stats_for_all_columns(real_pnl_series, intv_datelist[current_interval][:num_dates_data_available]).set_index('Shc')
        stats_intv_pnls_norm = get_pnl_stats_for_all_columns(real_pnl_series_norm, intv_datelist[current_interval][:num_dates_data_available]).set_index('Shc')
        concat_list.append(stats_intv_pnls['Pnl_avg'].rename(str(current_interval)+'.PNL_AVG'))
        concat_list.append(stats_intv_pnls_norm['Pnl_avg'].rename(str(current_interval)+'.RISK_NORM_PNL'))

    recent_dates = get_week_day_list_from_day(run_date_minus_1, 10)

    # Get last day Alloc  risk for ex, session
    ex_session_prev_days_risk_list = fetch_risk_numbers_from_db(min(recent_dates), run_date, convert_list_to_tuple(ex_session_list))
    ex_session_prev_days_risk_df = pd.DataFrame(ex_session_prev_days_risk_list, columns = ['Run_id', 'Shortcode', 'Session', 'Date', 'Alloc_Risk'])
    ex_session_prev_days_risk_df['Shc_session'] = list(map(lambda x,y: x + '.' + y, ex_session_prev_days_risk_df['Shortcode'], ex_session_prev_days_risk_df['Session']))

    # Get last day Alloc Risk for shcortcodes
    previous_risks_new_shc = pd.melt(previous_risks.drop('Overall_wt_wrt_corr', axis=1), id_vars = ['Date'])
    previous_risks_new_shc.columns = ['Date', 'Shc_session', 'Alloc_Risk']

    # Join the above two and fetch final numbers
    previous_risks_new = pd.concat([previous_risks_new_shc, ex_session_prev_days_risk_df[['Date', 'Shc_session', 'Alloc_Risk']]], axis=0)
    prev_available_alloc_risk = previous_risks_new[previous_risks_new['Alloc_Risk'].notnull()].sort_values(['Shc_session', 'Date']).drop_duplicates('Shc_session', keep='last').set_index('Shc_session')
    concat_list.append(prev_available_alloc_risk['Alloc_Risk'].rename('PREV_ALLOC_RISK'))
    last_day_alloc_risk = previous_risks_new[previous_risks_new['Date'] == run_date_minus_1][['Alloc_Risk', 'Shc_session']].set_index('Shc_session')
    concat_list.append(last_day_alloc_risk['Alloc_Risk'].rename('LAST_DAY_ALLOC_RISK'))

    # Get last day Actual Risk
    actual_risks_raw_shc = pd.melt(real_risk_series_shc, id_vars = ['Date'])
    actual_risks_raw_shc.columns = ['Date', 'Shc_session', 'Risk_Factor']
    actual_risks_raw = pd.concat([actual_risks_raw_shc, ex_session_combined_series[['Date', 'Shc_session', 'Combined_Risk']].rename(columns = {'Combined_Risk':'Risk_Factor'})], axis=0)
    actual_risk_last_10_days =  actual_risks_raw.loc[actual_risks_raw['Date'].isin(recent_dates), ['Date', 'Shc_session', 'Risk_Factor']]

    # Get Avg risk of last days
    prev_avg_risk = actual_risk_last_10_days.groupby('Shc_session')['Risk_Factor'].mean()
    concat_list.append(prev_avg_risk.rename('AVG_RISK_10_DAYS'))

    # Get risk of last day
    prev_available_actual_risks = actual_risk_last_10_days[actual_risk_last_10_days['Risk_Factor'].notnull()].sort_values(['Shc_session','Date']).drop_duplicates('Shc_session', keep='last').set_index('Shc_session')
    concat_list.append(prev_available_actual_risks['Risk_Factor'].rename('PREV_ACTUAL_RISK'))
    last_day_actual_risk = actual_risk_last_10_days[actual_risk_last_10_days['Date'] == run_date_minus_1][['Risk_Factor', 'Shc_session']].set_index('Shc_session')
    concat_list.append(last_day_actual_risk['Risk_Factor'].rename('LAST_DAY_ACTUAL_RISK'))

    # Get is_live flag. If actual risk for last 3 days available then is_live = 1
    last_3_days_list = get_week_day_list_from_day(run_date_minus_1, 3)
    last_3_days_risk = actual_risk_last_10_days[(actual_risk_last_10_days['Date'].isin(last_3_days_list) & (actual_risk_last_10_days['Risk_Factor'].notnull()))].copy()
    last_3_days_risk['IS_LIVE'] = 1
    is_live_data = last_3_days_risk[['Shc_session','IS_LIVE']].drop_duplicates().set_index('Shc_session')
    concat_list.append(is_live_data)

    # Get allocated risk and weighted sharpe of exchange, session
    wt_sharpe_ex_session = get_pnl_stats_for_all_columns(real_pnl_series_norm_ex_session, weight_vector_dates_to_use[:real_pnl_series_norm_ex_session.shape[0]])[['Shc','Pnl_sharpe']].set_index('Shc')
    allocated_risk_ex_session = date_stats[['Exchange','Session','Session_wt_wrt_corr']].drop_duplicates().rename(columns = {'Session_wt_wrt_corr':'Shc_wt'})
    allocated_risk_ex_session['Shc_session'] = list(map(lambda x,y: x + '.' + y, allocated_risk_ex_session['Exchange'], allocated_risk_ex_session['Session']))
    allocated_risk_ex_session.set_index('Shc_session', inplace=True)
    sharpe_allocated_risk_ex_session = pd.concat([wt_sharpe_ex_session, allocated_risk_ex_session], axis=1).rename(columns = {'Pnl_sharpe':'WEIGHTED_SHARPE','Shc_wt':'ALLOCATED_RISK'})

    # Weighted Sharpe, Allocated Risk, Exchange and Session for shc
    date_stats.rename(columns = {'Pnl_sharpe':'WEIGHTED_SHARPE','Shc_wt':'ALLOCATED_RISK'}, inplace=True)
    date_stats_for_shc_ex_session = pd.concat([date_stats[['Exchange', 'Session', 'ALLOCATED_RISK', 'WEIGHTED_SHARPE']], sharpe_allocated_risk_ex_session], axis=0)
    concat_list.append(date_stats_for_shc_ex_session)

    # Create a type to identify Exchanges,sessions and shortcode. ES = Exchange-Session.
    type_series = pd.Series(['ES']*len(ex_session_list) ,index = ex_session_list)
    concat_list.append(type_series.rename('Type'))

    # Concat all into 1
    risk_stats = pd.concat(concat_list, axis=1)
    # Filter shortcodes which risk allocation considered.
    risk_stats = risk_stats.loc[list_of_shc_considered + ex_session_list]

    risk_stat_cols  = risk_stats.columns.tolist()
    risk_stat_cols.remove('Exchange')
    risk_stat_cols.remove('Session')
    risk_stat_cols.remove('Type')

    # Remove rows with all nan's (exchange and session are always non null, so excluding those)
    nan_rows_flag = risk_stats[risk_stat_cols].isnull().all(axis=1)
    risk_stats_final = risk_stats[~nan_rows_flag]

    # If all important columns are 0/nan then do not show them
    important_risk_columns = ['ALLOCATED_RISK', 'PREV_ACTUAL_RISK', 'PREV_ALLOC_RISK']
    important_cols_nan_or_zero_flag = (risk_stats[important_risk_columns].fillna(0) == 0).all(axis = 1)
    risk_stats_final = risk_stats_final[~important_cols_nan_or_zero_flag]

    # Set IS_LIVE to 0 wherever it is nan (not 1)
    risk_stats_final['IS_LIVE'] = risk_stats_final['IS_LIVE'].fillna(0)

    # Set Type to 'S' (Shortcode) wherever it is nan (not 'ES')
    risk_stats_final['Type'] = risk_stats_final['Type'].fillna('S')

    # Converting nan's to None for making it null in json file
    risk_stats_final = risk_stats_final.where(pd.notnull(risk_stats_final), None)

    # Changing format of Pnl_Avg and Risk_norm_pnl columns. Dropping earlier columns
    for current_interval in np.sort(list(interval_dict.keys())):
        risk_stats_final['INTV: '+str(current_interval)] = list(map(lambda x,y: {'PNL_AVG':x, 'RISK_NORM_PNL':y},
                                                                    risk_stats_final[str(current_interval)+'.PNL_AVG'], risk_stats_final[str(current_interval)+'.RISK_NORM_PNL']))
        risk_stats_final.drop([str(current_interval)+'.PNL_AVG', str(current_interval)+'.RISK_NORM_PNL'], axis=1, inplace=True)

    risk_stats_final.sort_values(['Exchange', 'Type'], inplace=True)

    return risk_stats_final


def read_risk_alloc_simula_json(input_json):

    concat_list = []
    for json_dict in input_json:
        df = pd.read_json(json.dumps(json_dict['values']), orient = 'records')
        df['Session'] = json_dict['session']
        concat_list.append(df)

    out_df = pd.concat(concat_list, axis=0)

    return out_df


def write_or_append_to_json_file(risk_stats_json_file_path, code_output_json, dump_to_json=0):
    """
    Appends json to file if file is not empty else writes it

    :param risk_stats_json_file_path:   str
    :param code_output_json:            list. content of list is json output from the code
    :param dump_to_json:                int. 0 will not write. 1 will overwrite. 2 will append to existing json file.
    :return:                            None
    """

    if dump_to_json != 0:
        print("Output Json file: ", risk_stats_json_file_path)
        new_json = code_output_json[:]
        json_to_write = []
        # if appending, then read and update the json to write
        if dump_to_json == 2:
            if os.path.isfile(risk_stats_json_file_path) and os.stat(risk_stats_json_file_path).st_size != 0:
                with open(risk_stats_json_file_path, 'r') as f:
                    file_present_json =  json.load(f)
                old_df = read_risk_alloc_simula_json(file_present_json)
                new_df = read_risk_alloc_simula_json(new_json)
                final_df = pd.concat([old_df, new_df], axis=0).sort_values(['Exchange', 'Type'])
                json_to_write = get_json_from_risk_stats(final_df, mode=1)
        else:
            json_to_write = new_json

        with open(risk_stats_json_file_path, 'w') as f:
            f.write(json.dumps(json_to_write))


def report_risk_mismatch_of_last_day(risk_stats_df, run_date, perc_risk_diff_to_flag, min_absolute_risk_diff_to_flag):
    """
    Report last days risk allocation numbers if in actual they were ran at a different risk

    :param risk_stats_df:                   dataframe
    :param run_date:                        int
    :param perc_risk_diff_to_flag:          int. 30 for 30% change. Report if %diff is greater than this number.
    :param min_absolute_risk_diff_to_flag:  int. If absolute diff is less than this number, then do not report irrespective of % diff.
    :return:
    """

    required_columns = ['LAST_DAY_ACTUAL_RISK', 'LAST_DAY_ALLOC_RISK']
    risk_stats_df.index.name = 'Shc_session'
    required_df =  risk_stats_df.loc[risk_stats_df['Type']=='S', required_columns].fillna(0)
    required_df = required_df.loc[~(required_df[required_columns] == 0).all(axis = 1), :]

    required_df['Diff'] = list(map(lambda x, y: x - y, required_df['LAST_DAY_ACTUAL_RISK'], required_df['LAST_DAY_ALLOC_RISK']))
    required_df['% Diff_wrt_alloc_risk'] = list(map(lambda x,y: 100*x/y if y!=0  else np.inf, required_df['Diff'], required_df['LAST_DAY_ALLOC_RISK']))
    report_flag = (required_df['% Diff_wrt_alloc_risk'].abs() > perc_risk_diff_to_flag) & (required_df['Diff'].abs() > min_absolute_risk_diff_to_flag)
    final_df = required_df[report_flag][required_columns + ['% Diff_wrt_alloc_risk']].reset_index()

    if not final_df.empty:
          message_string_to_send = ' Risk Mismatch Between Allocated and Actual for date = ' + str(calc_prev_week_day(run_date)) + '\n' + final_df.to_string(index=False)
          print(message_string_to_send)
          # Send slack notifiction
          os.system('/home/pengine/prod/live_execs/send_slack_notification risk-manager DATA ' + '"' + message_string_to_send + '"')


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('-hfile', dest='hfile',help='hierarchy file having exchanges,sessions, shortcodes and risk constraints',
                        type=str, required=True)
    parser.add_argument('-risk', dest='total_risk', help='Total risk to be distributed', type=str, required=True)
    parser.add_argument('-date', dest='run_date', help="run date", type=int, required=True)
    parser.add_argument('-ldays', dest='ldays', help="1 is default. Number of past days from current date (including) for which risk numbers needs to be calculated",
                        type=int, required=False, default=1)
    parser.add_argument('-work_dir', dest='work_dir', help="work dir for temp outputs", type=str, required=False, default=None)
    parser.add_argument('-with_constraint', dest='with_constraint', help="How to run the code. With(default)/without constraints",
                        type=int, required=False, default=1)
    parser.add_argument('-temporal_constraint', dest='temporal_constraint', help="Use temporal constraints(default) or not", type=int, required=False, default=1)
    parser.add_argument('-fill_holes', dest='fill_holes', help="Fill holes or not. 2 for filling with zeros (default). 1 for filling with sim pnl. 0 for not",
                        type=int, required=False, default=2)
    parser.add_argument('-backtest', dest='backtest_mode', help="Flag for running backtest. Default 0",
                        type=int, required=False, default=0)
    parser.add_argument('-write_in_db', dest='write_in_db', help="Write risk valus in DB table or not. Default 0",
                        type=int, required=False, default=0)
    parser.add_argument('-overwrite', dest='overwrite_db_values', help="Overwrite risk valus in DB table or not. Default 0",
                        type=int, required=False, default=0)
    parser.add_argument('-debug_mode', dest='debug_mode', help="Default 0. Set to 1 for more logging", type=int, required=False, default=0)
    parser.add_argument('-dump_json', dest='dump_json', help="Default 0 (do not write to json). 1 overwrite json. 2 append to json.", type=int, required=False, default=0)

    args = parser.parse_args()

    risk_stats_json_file_path = '/media/shared/ephemeral16/RiskAlloc.js'

    # Reading arguments
    input_date = args.run_date
    hfile = args.hfile
    total_risk = float(args.total_risk)
    ldays = args.ldays
    with_constraint = args.with_constraint
    temporal_constraint = args.temporal_constraint
    work_dir = args.work_dir
    fill_holes = args.fill_holes
    backtest_mode = args.backtest_mode
    write_in_db = args.write_in_db
    overwrite_db_values = args.overwrite_db_values
    debug_mode = args.debug_mode
    dump_json = args.dump_json

    # Defining other parameters needed
    # Weights given to days. {20:1.0, 100:0.7} means first 20 days have weight of 1, then 20-100 days have weight of 0.7 and so on.
    interval_dict = {20:1.0, 100:0.7, 300:0.3}
    # This number of past history is used for looking at the pnl performance
    history_days = max(interval_dict.keys())
    # Minimum these number of real pnl data should be present within history days for risk allocation to consider a product
    enough_days_for_real_pnl = 100
    # For shortcode/session, we consider risk as 95%tile as risk. But for exchange,overall we consider 98%tile and 99%tile respectively. This factors are ratio to 95%tile.
    risk_factor  = {"session":1.0, "exchange":1.4, "overall":1.6}
    # In optimization, a regularization factor is used. This is the multiplier for that
    regularization_factor = 0.2
    # If system assigns risk below this number then it is made 0. To avoid system giving a risk of 10/20.
    min_risk_cap = 100
    # Percentile of sim_pnl series assumed as risk. 95%tile is taken
    sim_pnl_series_risk_ptile = 0.95
    # temporal change that can be allowed. increase/decrease numbers are in terms of multipliers to risk.
    # Push risk is :- if 0 risk was allocated for past days. Then bounds are made as (min, min+push_risk)
    temporal_change_allowance_dict = {"increase_from_last_day":1.3, "decrease_from_last_day":0.7,
                                      "increase_from_min_of_large_period":2.0, "decrease_from_max_of_large_period":0.5,
                                      "large_period_length":5, "push_risk": 3000.0}
    # If DI expiry is within these number of days, then do not consider it for risk allocation
    days_before_expiry_to_stop_DI = 120
    # Number of days past risk numbers to be fetched. Need it in displaying average risk allocated on simula
    prev_days_risk_to_fetch = 10
    # Report last days risk allocation numbers if in actual they were ran at a different risk
    # % diff of actual wrt allocated that should be reported
    perc_risk_diff_to_flag = 15
    # If absolute diff is less than this number, then do not report irrespective of % diff.
    min_absolute_risk_diff_to_flag = 1000

    # Creating a weight vector for number of days = max(interval_dict.keys())
    if debug_mode:
        print("\nWeighted Interval dict for days: ", interval_dict)
    weight_vector_dates = get_weight_vector_for_dates(interval_dict)

    # Updating some params or creating work directory
    if backtest_mode:
        print("Backtest mode. Not reading/writing allocated risk numbers from/to db. And not writing json.")
        write_in_db = 0
        overwrite_db_values = 0
        dump_json = 0

    # Setting write_in_db to 1 if overwrite is true for consistency.
    if not write_in_db and overwrite_db_values:
        write_in_db = 1

    # In the input date provided is a weekend, then update the date to next weekday
    input_date = update_input_date(input_date)

    if work_dir == None:
        work_dir = "/media/shared/ephemeral16/" + getpass.getuser() + "/risk_allocation/" + os.path.basename(hfile) + "_" + str(input_date) + "/"
        os.system("mkdir -p " + work_dir)
    else:
        work_dir = work_dir + '/'

    # Parse the config file
    tree_struct = parse_hfile(hfile)

    df_main_columns = ['Exchange', 'Exchange_min_risk','Exchange_max_risk',
                       'Super_session', 'Super_session_min_risk', 'Super_session_max_risk',
                       'Shc_session', 'Shc_session_min_risk', 'Shc_session_max_risk']

    if max([len(i) for i in tree_struct]) == 10:
        df_main_columns.append("Super_Product")

    tree_df = pd.DataFrame(tree_struct, columns=df_main_columns)
    float_cols = ['Exchange_min_risk','Exchange_max_risk', 'Super_session_min_risk', 'Super_session_max_risk','Shc_session_min_risk', 'Shc_session_max_risk']
    tree_df[float_cols] = tree_df[float_cols].astype(float)

    tree_df['Shc'] = list(map(lambda x: x.split('.')[0], tree_df['Shc_session']))
    tree_df['Sub_session'] = list(map(lambda x: x.split('.')[1], tree_df['Shc_session']))
    tree_df['Overall_risk'] = total_risk

    check_validtity_of_config_read(tree_df, input_date)

    # Get days for which the risk output is needed
    all_run_date_list = get_week_day_list_from_day(input_date, ldays)

    # Getting all days real pnl data for plotting or comparision.
    # From current date, fetch data for ldays+history days so that for earliest backtest date also, we have data of history days
    # minimum_past_dates_data_needed = 0 as we need all products here whose data is available
    all_days_real_pnl_data_norm, all_days_real_pnl_data_actual, all_days_real_risk_data =  create_data_for_all_days(tree_df, input_date, debug_mode,
                                                                                           number_of_past_days_to_look=ldays+history_days,
                                                                                           minimum_past_dates_data_needed=0)

    tree_df = remove_super_product(tree_df)

    # Fetch normalized simulated pnl
    if fill_holes == 1:
        if debug_mode:
            print("\nFetching simulated pnl to fill holes in the real pnl values")
        simulated_df = fetch_normalized_simulated_pnl(all_days_real_pnl_data_norm, history_days, sim_pnl_series_risk_ptile, debug_mode)
    else:
        simulated_df = pd.DataFrame()

    all_days_real_pnl_data_norm.to_csv(work_dir + 'real_pnl_data_risk_normalized.csv')
    all_days_real_pnl_data_actual.to_csv(work_dir + 'real_pnl_data_actual.csv')
    simulated_df.to_csv(work_dir + 'simulated_df.csv')

    products_data_available = all_days_real_pnl_data_actual.columns.tolist()
    products_data_available.remove('Date')

    risk_df_list = []
    stats_df_list = []

    # Create empty and keep updating. Later we will fetch it from DB
    prev_weights_cols = all_days_real_pnl_data_norm.columns.tolist() + ['Overall_wt_wrt_corr']
    prev_weights_last = pd.DataFrame(columns = prev_weights_cols)

    # Loop over the dates for which risk output is needed.
    all_run_date_list.sort()
    for run_date in all_run_date_list:

        # Get past 300 days real pnl data. Normalize with risk and fill holes with sim_median pnl (by default, not filling the holes)
        pnl_series_df_pivot = get_day_data_from_created_data(all_days_real_pnl_data_norm, run_date, fill_holes, simulated_df,
                                                             debug_mode, True, history_days, enough_days_for_real_pnl, days_before_expiry_to_stop_DI)
        shc_in_consideration = pnl_series_df_pivot.columns.tolist()
        shc_in_consideration.remove('Date')
        date_pnl_with_all_products = pnl_series_df_pivot['Date'].tolist()
        exchange_list_for_run = list(set(tree_df['Exchange'].tolist()))

        # Updating the date weight vector based on the final set of days that we have.
        if len(date_pnl_with_all_products) <= len(weight_vector_dates):
            weight_vector_dates_to_use = weight_vector_dates[:len(date_pnl_with_all_products)]

        # Unconstrained optimization
        if with_constraint == 0:
            risk_output_for_day = get_risk_output_unconstrained(pnl_series_df_pivot, shc_in_consideration, tree_df,
                                                           run_date, weight_vector_dates_to_use, debug_mode)
            risk_stats_df_merge_keys = ['Exchange', 'Session', 'Shc', 'Date']

        # Getting unconstrainted weights first. Then updating the constraints based on CovM and initial wegihts
        # Finally optimizing the pnl sharpe with all the constraints in place.
        elif with_constraint == 1:

            if not backtest_mode and temporal_constraint:
                if debug_mode:
                    print("Reading risk numbers (shortcode and total) from DB of last days for temporal constraints. ")
                prev_weights_last = read_risk_numbers_from_db(run_date, prev_days_risk_to_fetch,
                                                              shc_in_consideration, exchange_list_for_run, tree_df)

            risk_output_for_day = get_risk_output_constrained(pnl_series_df_pivot, shc_in_consideration,
                                                              tree_df, run_date, weight_vector_dates_to_use,
                                                              all_days_real_pnl_data_norm, all_days_real_pnl_data_actual,
                                                              risk_factor, regularization_factor,  min_risk_cap,
                                                              prev_weights_last, temporal_constraint, temporal_change_allowance_dict,
                                                              backtest_mode, debug_mode, work_dir)
            risk_stats_df_merge_keys = ['Shc', 'Date']
            prev_weights_last = risk_output_for_day[2]

            if write_in_db:
                if debug_mode:
                    print("Trying to write risk values to database ")
                db_friendly_df = convert_to_db_format(risk_output_for_day[0])
                exchange_runid_data = fetch_risk_runID_present(convert_list_to_tuple(exchange_list_for_run), run_date)
                remove_risk_records_or_exit(exchange_runid_data, run_date, write_in_db, overwrite_db_values)
                insert_risk_records_to_db(db_friendly_df.values, total_risk, exchange_list_for_run)
                # insert_update_risk_number_to_db(db_friendly_df.values, overwrite_db_values)

        # Appending a days output to the a list
        risk_df_list.append(risk_output_for_day[0])
        stats_df_list.append(risk_output_for_day[1])

    # Combine different days output
    risk_df = pd.concat(risk_df_list, axis=0, ignore_index=True)
    stats_df = pd.concat(stats_df_list, axis=0, ignore_index=True)

    # Rename few columns to make it consistent
    risk_df.rename(columns={'Super_session': 'Session', 'Shc_session': 'Shc', 'Super_session_weight': 'Session_wt',
                            'Shc_weight': 'Shc_wt', 'Exchange_weight': 'Exchange_wt',
                            'Super_session_min_risk': 'Session_min_risk', 'Super_session_max_risk': 'Session_max_risk',
                            'Shc_session_min_risk': 'Shc_min_risk', 'Shc_session_max_risk': 'Shc_max_risk',
                            }, inplace=True)

    final_out = pd.merge(risk_df, stats_df, on=risk_stats_df_merge_keys, how='left')

    if len(all_run_date_list) > 1:
        #Get and merge cumulative pnl per unit risk numbers
        cumulative_pnl_data = get_cumulative_pnl_data(all_days_real_pnl_data_norm, all_run_date_list, ldays)
        cumulative_pnl_df = pd.melt(cumulative_pnl_data, id_vars=['Date'], var_name='Shc', value_name='Cumulative_pnl')
        final_out = pd.merge(final_out, cumulative_pnl_df, on=['Date','Shc'], how='left')
        cumulative_pnl_data_actual = get_cumulative_pnl_data(all_days_real_pnl_data_actual, all_run_date_list, ldays)
        cumulative_pnl_df_actual = pd.melt(cumulative_pnl_data_actual, id_vars=['Date'], var_name='Shc', value_name='Cumulative_pnl_actual')
        final_out = pd.merge(final_out, cumulative_pnl_df_actual, on=['Date','Shc'], how='left')
    else:
        final_out['Cumulative_pnl'] = 0
        final_out['Cumulative_pnl_actual'] = 0

    final_out['Shc_wt'] = final_out['Shc_wt'].round(0)

    final_cols = ['Exchange', 'Exchange_max_risk', 'Exchange_min_risk', 'Exchange_wt', 'Exchange_wt_wrt_corr', 'Overall_risk', 'Overall_sum_weight', 'Overall_wt_wrt_corr', \
                  'Session_wt_wrt_corr', 'Shc', 'Shc_max_risk', 'Shc_min_risk', 'Shc_wt', 'Session', 'Session_max_risk', 'Session_min_risk', 'Session_wt', \
                  'unconstrained_wts_for_shc', 'PNL_using_risk_provided_by_system', 'Actual_PNL_of_products_assigned_non_zero_risk', \
                  'Actual_PNL_of_all_products_considered', 'Actual_PNL_of_all_products', 'Date', 'Pnl_avg', 'Pnl_std', 'Pnl_sharpe', 'Pnl_overdays_DD', \
                  'Number of Nan', 'Cumulative_pnl', 'Cumulative_pnl_actual', 'system_pnl', 'actual_pnl', 'actual_pnl_norm', \
                  'actual_risk', 'Real_Session_wt_wrt_corr', 'Real_Exchange_wt_wrt_corr', 'Real_Overall_wt_wrt_corr', 'Adjusted_System_PNL', \
                  'Session_sharpe', 'Exchange_sharpe', 'Overall_sharpe']

    final_out = final_out.loc[:,final_cols]

    ofile = work_dir + 'risk_output'

    # Print to a csv file
    final_out.to_csv(ofile, index=False)
    print("\nOutput Risk file: ", ofile)

    if dump_json != 0:
        # Get Shc, Weighted_sharpe, Allocated_risk for the last day
        input_date_stats = final_out.loc[final_out['Date'] == input_date, ['Exchange', 'Session', 'Shc', 'Shc_wt', 'Pnl_sharpe', 'Session_wt_wrt_corr']].set_index('Shc')
        prev_weights_df = prev_weights_last.loc[prev_weights_last['Date'] != input_date,:]

        # Prepare the JSON object for simula
        risk_stats = get_pnl_stats_for_json(all_days_real_pnl_data_actual, all_days_real_pnl_data_norm, input_date,
                                            fill_holes, interval_dict, prev_weights_df, input_date_stats, debug_mode,
                                            tree_df, all_days_real_risk_data, weight_vector_dates_to_use)
        final_json = get_json_from_risk_stats(risk_stats)

        # Write/append json object to a file
        write_or_append_to_json_file(risk_stats_json_file_path, final_json, dump_json)

        # Report last days risk allocation numbers if in actual they were ran at a different risk
        report_risk_mismatch_of_last_day(risk_stats, input_date, perc_risk_diff_to_flag, min_absolute_risk_diff_to_flag)

    print("Output Directory: ", work_dir)
