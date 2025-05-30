
import sys, os
import numpy as np
import pandas as pd

from risk_specific_functions_variables import *

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.utils.date_utils import calc_prev_week_day


def assign_min_risk_to_products(unconst_risk_df):
    """
    Assign risk to products which have less data but trader has given a minimum risk to it.
    Minimum risk is provided to such products. Min/max risk at higher levels are updated considering this assignment 
    
    :param unconst_risk_df:         pandas dataframe 
    :return: 
    
    unconst_risk_df_useful_new:     pandas dataframe
                                    Dataframe with updated min max risk for session, exchange and overall level
    
    min_risk_shc_weights:           list of list           
                                    List of [shortcode, minimum risk assigned]
    
    """
    unconst_risk_df_useful = unconst_risk_df[unconst_risk_df['unconstrained_wts_for_shc'].isnull() == False].copy()
    min_risk_no_data_risk_df = unconst_risk_df[(unconst_risk_df['unconstrained_wts_for_shc'].isnull() == True) &
    (unconst_risk_df['Shc_session_min_risk'] != 0)].copy()

    if min_risk_no_data_risk_df.shape[0] == 0:
        return unconst_risk_df_useful, []

    orig_cols = unconst_risk_df.columns.tolist()

    min_risk_shc_weights = min_risk_no_data_risk_df[['Shc_session', 'Shc_session_min_risk']].values.tolist()
    min_risk_no_data_risk_df = min_risk_no_data_risk_df.groupby(['Exchange', 'Super_session']).apply(add_shc_weights,
                                                                                                     'Shc_session_min_risk',
                                                                                                     'Session_risk_to_substract')
    min_risk_no_data_risk_df = min_risk_no_data_risk_df.groupby('Exchange').apply(add_shc_weights,
                                                                                  'Shc_session_min_risk',
                                                                                  'Exchange_risk_to_substract')
    min_risk_no_data_risk_df = min_risk_no_data_risk_df.groupby('Overall_risk').apply(add_shc_weights,
                                                                                      'Shc_session_min_risk',
                                                                                      'Overall_risk_to_substract')

    unconst_risk_df_useful_new = unconst_risk_df_useful.merge(
        min_risk_no_data_risk_df[['Exchange', 'Super_session', 'Session_risk_to_substract']], how='left',
        on=['Exchange', 'Super_session'])
    unconst_risk_df_useful_new = unconst_risk_df_useful_new.merge(
        min_risk_no_data_risk_df[['Exchange', 'Exchange_risk_to_substract']].drop_duplicates(), how='left',
        on='Exchange')
    unconst_risk_df_useful_new = unconst_risk_df_useful_new.merge(
        min_risk_no_data_risk_df[['Overall_risk', 'Overall_risk_to_substract']].drop_duplicates(), how='left',
        on='Overall_risk')

    unconst_risk_df_useful_new[
        ['Session_risk_to_substract', 'Exchange_risk_to_substract', 'Overall_risk_to_substract']] = \
    unconst_risk_df_useful_new[
        ['Session_risk_to_substract', 'Exchange_risk_to_substract', 'Overall_risk_to_substract']].fillna(0)

    # 'Exchange_min_risk','Exchange_max_risk','Super_session_min_risk', 'Super_session_max_risk','Shc_session_min_risk', 'Shc_session_max_risk'

    unconst_risk_df_useful_new['New_Super_session_min_risk'] = list(
        map(lambda x, y: max(0, x - y), unconst_risk_df_useful_new['Super_session_min_risk'],
            unconst_risk_df_useful_new['Session_risk_to_substract']))
    unconst_risk_df_useful_new['New_Super_session_max_risk'] = list(map(lambda x, y: max(0, x - y),
                                                                        unconst_risk_df_useful_new[
                                                                            'Super_session_max_risk'],
                                                                        unconst_risk_df_useful_new[
                                                                            'Session_risk_to_substract']))
    unconst_risk_df_useful_new['New_Exchange_min_risk'] = list(map(lambda x, y: max(0, x - y),
                                                                   unconst_risk_df_useful_new['Exchange_min_risk'],
                                                                   unconst_risk_df_useful_new[
                                                                       'Exchange_risk_to_substract']))
    unconst_risk_df_useful_new['New_Exchange_max_risk'] = list(map(lambda x, y: max(0, x - y),
                                                                   unconst_risk_df_useful_new['Exchange_max_risk'],
                                                                   unconst_risk_df_useful_new[
                                                                       'Exchange_risk_to_substract']))
    unconst_risk_df_useful_new['New_Overall_risk'] = list(map(lambda x, y: max(0, x - y),
                                                              unconst_risk_df_useful_new['Overall_risk'],
                                                              unconst_risk_df_useful_new['Overall_risk_to_substract']))

    old_cols_list = ['Super_session_min_risk', 'Super_session_max_risk', 'Exchange_min_risk', 'Exchange_max_risk',
                     'Overall_risk']
    new_cols_list = ['New_Super_session_min_risk', 'New_Super_session_max_risk', 'New_Exchange_min_risk',
                     'New_Exchange_max_risk', 'New_Overall_risk']
    rename_col_dict = dict(zip(new_cols_list, old_cols_list))
    rename_col_dict_reverse = dict(zip(old_cols_list, new_cols_list))

    cols_to_use = [i if i not in old_cols_list else rename_col_dict_reverse[i] for i in orig_cols]

    unconst_risk_df_useful_new = unconst_risk_df_useful_new[cols_to_use]
    unconst_risk_df_useful_new.rename(columns=rename_col_dict, inplace=True)

    return unconst_risk_df_useful_new, min_risk_shc_weights


def create_constraint_matrix(unconst_risk_df, total_risk, covM, risk_factor, debug_mode):
    """
    
    Create constraints based on the Min/Max risk at each node level, total constant risk to be taken and the covariance
    
    :param unconst_risk_df:     pandas dataframe with the min,max constraints at each level. 
                                Rows are at exchange, session, shortcode level. Min/Max risk of exchange/session are repeated for repeating values
    :param total_risk:          int  
    :param covM:                dataframe with covariance values (from function get_mean_covariance)
    :param risk_factor:         dictionary of risk factors of session,exchange and overall firm level
    :return:                    dataframe with constraints 
                                
    
    """
    unconst_risk_df_useful = unconst_risk_df[unconst_risk_df['unconstrained_wts_for_shc'].isnull() == False].copy()

    constraints_df = get_max_min_constraint_for_shcs(unconst_risk_df_useful)
    constraints_df['Session'] = ""
    constraints_df['Exchange'] = ""

    exchange_list = set(unconst_risk_df_useful['Exchange'].tolist())
    session_factor = risk_factor['session']
    exchange_factor = risk_factor['exchange']
    overall_factor = risk_factor['overall']
    if len(exchange_list) == 1:
        overall_factor = exchange_factor
    for exchange in exchange_list:

        super_session_list = set(
            unconst_risk_df_useful[unconst_risk_df_useful['Exchange'] == exchange]['Super_session'].tolist())
        # Add session level constraint only if more than 1 session are present
        if len(super_session_list) > 1:
            for super_session in super_session_list:
                # Session level constraints
                add_sum_constraints(constraints_df, unconst_risk_df_useful, total_risk, covM, debug_mode, exchange, super_session, session_factor)

        # Exchange level constraints
        # if len(super_session_list) > 1:
        add_sum_constraints(constraints_df, unconst_risk_df_useful, total_risk, covM, debug_mode, exchange, "All", exchange_factor)

    # Firm level constraints
    add_sum_constraints(constraints_df, unconst_risk_df_useful, total_risk, covM, debug_mode, "All", "All", overall_factor)

    check_high_level_constraints(constraints_df)

    return constraints_df


def check_high_level_constraints(constraints_df):
    """
    Given the constraints matrix, check if lower level constraints satify the higher node min/max constraints 
    Update the total risk constraint if sum of exchanges max risk < total risk value 
    
    :param constraints_df:      dataframe
    :return:                    Nothing. Updates input dataframe, printing any concerns that may be there
    """
    list_of_exchanges = list(set(
        constraints_df[(constraints_df['Exchange'] != "") & (constraints_df['Type'] == 'Exchange')][
            'Exchange'].tolist()))

    # Session and exchange level checks
    exchange_for_which_sum_session_max_are_less = []
    for ex in list_of_exchanges:
        sessions = list(set(constraints_df[(constraints_df['Exchange'] == ex) & (constraints_df['Type'] == 'Session')][
                                'Session'].tolist()))
        max_risk_sessions = constraints_df[
            (constraints_df['Type'] == 'Session') & (constraints_df['Session'].isin(sessions)) & (
            constraints_df['Min_value'] < 0) & (constraints_df['Exchange'].isin([ex]))]['Min_value'].tolist()
        min_ex_risk = constraints_df[
            (constraints_df['Type'] == 'Exchange') & (constraints_df['Exchange'].isin([ex])) & (
            constraints_df['Min_value'] >= 0)]['Min_value'].tolist()
        max_ex_risk = constraints_df[
            (constraints_df['Type'] == 'Exchange') & (constraints_df['Exchange'].isin([ex])) & (
            constraints_df['Min_value'] < 0)]['Min_value'].tolist()
        if min_ex_risk[0] > -1 * np.sum(max_risk_sessions) and len(max_risk_sessions) > 1:
            print(
            "Sum of sessions max risk is less than exchange min risk for exchange = ", ex, ". Constraints might fail.")
        # Checking if max of all sessions can match max of exchange.
        # Will come in handy in knowing what constraints will fail if all exchanges needs to be given max risk.
        if -1 * max_ex_risk[0] > -1 * np.sum(max_risk_sessions) and len(max_risk_sessions) > 1:
            exchange_for_which_sum_session_max_are_less.append(ex)

    compare_with = "Exchange"
    compare_list = list_of_exchanges

    # Check for overall risk
    max_risk_to_compare_with_list = constraints_df[
        (constraints_df['Type'] == compare_with) & (constraints_df[compare_with].isin(compare_list)) & (
        constraints_df['Min_value'] < 0)]['Min_value'].tolist()
    max_risk_to_compare_with = -1 * np.sum(max_risk_to_compare_with_list)
    overall_risk = constraints_df[(constraints_df['Type'] == 'Overall') & (constraints_df['Min_value'] > 0)][
        'Min_value'].tolist()
    if overall_risk > max_risk_to_compare_with:
        print("Sum of exchange max risks is less than overall risk provided.")
        print("Setting overall risk to ", max_risk_to_compare_with, " from earlier ", overall_risk,
              " for satisfying constraints.")
        constraints_df.loc[(constraints_df['Type'] == 'Overall') & (
        constraints_df['Min_value'] > 0), 'Min_value'] = max_risk_to_compare_with
        constraints_df.loc[(constraints_df['Type'] == 'Overall') & (
        constraints_df['Min_value'] < 0), 'Min_value'] = -1 * max_risk_to_compare_with
        if len(exchange_for_which_sum_session_max_are_less) > 0:
            print("Sum of session max risk do not reach exchange max risk for exchanges = ",
                  exchange_for_which_sum_session_max_are_less, ". Constraints might fail.")


def add_sum_constraints(constraints, unconst_risk_df, total_risk, covM, debug_mode, exchange='All', super_session='All', adjustment_factor=1.0):
    """
    Adds constraints on sum of shortcode risk for a certain session,exchange or exchange or at overall level 
    i.e.  min_risk < w1 + w2 + w3 ... < max_risk for a products under a certain level
    
    :param constraints:            dataframe (with shortcode min/max constraints already in palce)
    :param unconst_risk_df:        dataframe (min/max risk for all level and unconstrained weights learned)
    :param total_risk:             int
    :param covM:                   dataframe
    :param exchange:               str
    :param super_session:          str
    :return:                       Nothing. Adds constraint on sum of risk for particular level to constraints
    """
    all_shcs = constraints.columns.tolist()
    all_shcs.remove('Min_value')
    all_shcs.remove('Type')
    all_shcs.remove('Session')
    all_shcs.remove('Exchange')

    if exchange == 'All':
        type = 'Overall'
        df = unconst_risk_df.copy()
        constrained_shcs = df['Shc_session'].tolist()
        min_risk = total_risk
        max_risk = total_risk
    elif super_session == 'All':
        type = 'Exchange'
        df = unconst_risk_df[unconst_risk_df['Exchange'] == exchange].copy()
        constrained_shcs = df['Shc_session'].tolist()
        min_risk = df['Exchange_min_risk'].iloc[0]
        max_risk = df['Exchange_max_risk'].iloc[0]
    else:
        type = 'Session'
        df = unconst_risk_df[
            (unconst_risk_df['Exchange'] == exchange) & (unconst_risk_df['Super_session'] == super_session)]
        constrained_shcs = df['Shc_session'].tolist()
        min_risk = df['Super_session_min_risk'].iloc[0]
        max_risk = df['Super_session_max_risk'].iloc[0]

    covM_constrained = covM.loc[constrained_shcs, constrained_shcs]
    wts = np.array(df['unconstrained_wts_for_shc'].tolist())
    scaling_factor = (1/adjustment_factor) * np.dot(wts.T, np.sqrt(np.diagonal(covM_constrained))) / np.sqrt(
        np.dot(wts.T, covM_constrained).dot(wts))
    if np.isnan(scaling_factor) or scaling_factor < 1: scaling_factor = 1
    if debug_mode:
        print("Scaling Factor for", type, exchange, super_session, round(scaling_factor,2))
    # scaling_factor = 1

    new_index = constraints.shape[0]
    new_constraint = []
    for shc in all_shcs:
        if shc in constrained_shcs:
            new_constraint.append(1)
        else:
            new_constraint.append(0)

    constraints.loc[new_index, all_shcs] = new_constraint[0] if len(new_constraint) == 1 else new_constraint
    constraints.loc[new_index, 'Min_value'] = min_risk * scaling_factor
    constraints.loc[new_index, 'Type'] = type
    constraints.loc[new_index, 'Session'] = super_session
    constraints.loc[new_index, 'Exchange'] = exchange

    new_constraint = list(-1 * np.array(new_constraint))
    new_index = constraints.shape[0]
    constraints.loc[new_index, all_shcs] = new_constraint[0] if len(new_constraint) == 1 else new_constraint
    constraints.loc[new_index, 'Min_value'] = -1 * max_risk * scaling_factor
    constraints.loc[new_index, 'Type'] = type
    constraints.loc[new_index, 'Session'] = super_session
    constraints.loc[new_index, 'Exchange'] = exchange

    constraints.reset_index(drop=True, inplace=True)


def get_max_min_constraint_for_shcs(tree_risk_df):
    """
    Creates min,max constraints at shortcode level.
    
    :param tree_risk_df:    dataframe with min/max risk for shortcodes, session, exchange and overall level
    :return: 
    """
    tree_df_exchange_super_session = tree_risk_df.copy()

    shc_session_list = list(tree_df_exchange_super_session['Shc_session'].tolist())
    min_risk_numbers = tree_df_exchange_super_session['Shc_session_min_risk'].tolist()

    # min_risk_numbers = list(np.array(min_risk_numbers) + 0.01)

    tree_df_exchange_super_session.loc[:, 'Shc_session_max_risk'] = -1 * tree_df_exchange_super_session[
        'Shc_session_max_risk']
    max_risk_numbers = tree_df_exchange_super_session['Shc_session_max_risk'].tolist()

    number_of_weights = len(shc_session_list)
    ui = np.zeros((2 * number_of_weights, number_of_weights))
    for i in range(number_of_weights):
        ui[2 * i, i] = 1
        ui[2 * i + 1, i] = -1

    ci = [None] * (2 * number_of_weights)
    ci[::2] = min_risk_numbers
    ci[1::2] = max_risk_numbers
    ci = np.asarray(ci).reshape(-1, 1)

    ui_ci = np.concatenate((ui, ci), axis=1)

    constraints = pd.DataFrame(data=ui_ci, columns=shc_session_list + ['Min_value'])
    constraints.loc[:, 'Type'] = 'Shortcode'

    constraints.reset_index(drop=True, inplace=True)

    return constraints


def create_temporal_constraint(prev_weights, current_date, temporal_change_allowance):
    """
    Create temporal constraints for each shortcode

    :param prev_weights:                dataframe with prev_days weights
                                        Prev weights has columns date, columns with shortcode names, overall_risk
    :param current_date:                int
    :param temporal_change_allowance:   dict
    :return:                            dataframe with 3 columns. Shortcode, temporal_min, temporal_max
    """

    increase_from_last_day = temporal_change_allowance["increase_from_last_day"]
    decrease_from_last_day = temporal_change_allowance["decrease_from_last_day"]
    increase_from_min_of_large_period = temporal_change_allowance["increase_from_min_of_large_period"]
    decrease_from_max_of_large_period = temporal_change_allowance["decrease_from_max_of_large_period"]
    large_period_length = temporal_change_allowance["large_period_length"]

    # Fetching last 5 days if available
    prev_weights_norm = prev_weights[
        (prev_weights['Date'] < current_date) & (prev_weights['Date'] >= calc_prev_week_day(current_date, large_period_length))].fillna(0)
    prev_weights_norm = prev_weights_norm[prev_weights_norm['Overall_wt_wrt_corr'] != 0]
    if prev_weights_norm.shape[0] == 0:
        return pd.DataFrame()
    else:
        shortcode_list = prev_weights.columns.tolist()
        shortcode_list.remove('Date')
        shortcode_list.remove('Overall_wt_wrt_corr')

        # Normalizing the weights
        prev_weights_norm.iloc[:, 1:] = prev_weights_norm.iloc[:, 1:].div(prev_weights_norm['Overall_wt_wrt_corr'], axis=0)

        # Get weights for last day
        prev_weights_norm_last_day = \
        prev_weights_norm[prev_weights_norm['Date'] == calc_prev_week_day(current_date, 1)][shortcode_list]
        if prev_weights_norm_last_day.shape[0] == 0:
            prev_weights_norm_last_day = pd.DataFrame([[0] * len(shortcode_list)], columns=shortcode_list)
        prev_weights_norm_last_day.index = ['wt_last_day']

        # Get min weights of last 5 days
        prev_weights_last_5_days_min = pd.DataFrame(columns=shortcode_list)
        prev_weights_last_5_days_min.loc[0, shortcode_list] = prev_weights_norm[shortcode_list].min(axis=0).values
        prev_weights_last_5_days_min.index = ['min_last_5_days']

        # Get max weights of last 5 days
        prev_weights_last_5_days_max = pd.DataFrame(columns=shortcode_list)
        prev_weights_last_5_days_max.loc[0, shortcode_list] = prev_weights_norm[shortcode_list].max(axis=0).values
        prev_weights_last_5_days_max.index = ['max_last_5_days']

        prev_weights_final = pd.concat(
            [prev_weights_norm_last_day, prev_weights_last_5_days_min, prev_weights_last_5_days_max]).T
        prev_weights_final['temporal_min'] = list(
            map(lambda x, y: max(decrease_from_last_day * x, decrease_from_max_of_large_period * y), prev_weights_final['wt_last_day'],
                prev_weights_final['max_last_5_days']))
        prev_weights_final['temporal_max'] = list(
            map(lambda x, y: min(increase_from_last_day * x, increase_from_min_of_large_period * y) if x != 0 else increase_from_min_of_large_period * y, prev_weights_final['wt_last_day'],
                prev_weights_final['min_last_5_days']))

        prev_weights_final = prev_weights_final.round(4)
        prev_weights_final = prev_weights_final[['temporal_min', 'temporal_max']].reset_index()
        prev_weights_final.rename(columns={'index': 'Shortcode'}, inplace=True)
        return prev_weights_final


def get_constraints_optimization_format(all_shcs, constraint_mat_df, temporal_constraints, risk_factor, total_risk, push_risk):
    """
    Combine input constraints with temporal constraints. 
    Normalize constraints dividing with max value of constraint available to bring constraints in (0,1) and make sum of risks ~ 1
    Convert constraints to format which minimize uses. 
    
    :param all_shcs:                list. all_shcs in consideration
    :param constraint_mat_df:       dataframe. min/max constraints matrix of each level
    :param temporal_constraints:    dataframe. temporal constraints
    :param risk_factor:             dictionary of risk factors of session,exchange and overall firm level
    :param total_risk:              float
    :param push_risk:               float
    :return:   
                         
    final_cons:                     dictionary of constraints
    final_bound_tuple:              tuple of lower and upper bounds
    max_const:                      float. Normalization value. Used to bring risk back to correct scale
    """

    # Get A_ub and b_ub and the bounds of individual weights
    constraint_mat = -1 * constraint_mat_df[constraint_mat_df['Type'].isin(['Exchange', 'Session'])][all_shcs].values
    mat_upper_bound = list(-1 * np.array(constraint_mat_df[constraint_mat_df['Type'].isin(['Exchange', 'Session'])]['Min_value'].tolist()))
    bounds_list = list(-1 * np.array(constraint_mat_df[constraint_mat_df['Type'].isin(['Shortcode'])]['Min_value'].tolist()))
    ind_lower_bound = list(-1*np.array(bounds_list[::2]))
    ind_upper_bound = bounds_list[1::2]
    ind_bounds = tuple(zip(ind_lower_bound, ind_upper_bound))

    max_const = mat_upper_bound[-1]

    # Get equality constraints if present
    cons_equality = {}
    if 'Overall' in set(constraint_mat_df['Type'].tolist()):
        overall_df = constraint_mat_df[constraint_mat_df['Type'].isin(['Overall'])][all_shcs + ['Min_value']].copy()
        constraint_mat_equal = overall_df[overall_df['Min_value'] > 0][all_shcs].values
        mat_equality_const = overall_df[overall_df['Min_value'] > 0]['Min_value'].tolist()
        max_const = mat_equality_const[-1]
        mat_equality_const2 = list(np.array(mat_equality_const) / max_const)
        cons_equality = {'type': 'eq', 'fun': lambda x: mat_equality_const2 - np.dot(constraint_mat_equal, x)}

    # Normalize constraints and bounds to bring between 0,1. Some weights upper bound may be more than 1.
    # Divide by the total risk of firm so that weights add up to 1.
    mat_upper_bound2 = list(np.array(mat_upper_bound) / max_const)
    abc = np.array(ind_bounds) / max_const
    ind_bounds_norm = tuple(tuple(i) for i in abc.tolist())
    cons2 = ({'type': 'ineq', 'fun': lambda x: mat_upper_bound2 - np.dot(constraint_mat, x)})

    if cons_equality != {}:
        final_cons =  tuple([cons2, cons_equality])
    else:
        final_cons = cons2

    # Update bounds using temporal constraints
    push_risk_fraction = round(push_risk/max_const,4)
    if temporal_constraints.empty:
        final_bound_tuple = ind_bounds_norm
    else:
        temporal_regularization_factor = total_risk/max_const
        temporal_constraints_filled = temporal_constraints.fillna(0)
        temporal_constraints_filled.loc[:,['temporal_min','temporal_max']] *= temporal_regularization_factor
        final_bound_list = []
        i = 0
        for min_max_tuple in ind_bounds_norm:
            shc = all_shcs[i]
            i = i + 1
            temporal_min = temporal_constraints_filled[temporal_constraints_filled['Shortcode'] == shc]['temporal_min'].tolist()
            temporal_max = temporal_constraints_filled[temporal_constraints_filled['Shortcode'] == shc]['temporal_max'].tolist()

            if (len(temporal_min) == 0 and len(temporal_max) == 0):
                final_min = min_max_tuple[0]
                final_max = min_max_tuple[1]
            else:
                if (temporal_min[0] == 0 and temporal_max[0] == 0) :
                    final_min = min_max_tuple[0]
                    final_max = min(final_min + push_risk_fraction, min_max_tuple[1])
                else:
                    # Final_min Final_max should be tighter than input provided
                    final_min = min(max(temporal_min[0], min_max_tuple[0]), min_max_tuple[1])
                    final_max = max(min(temporal_max[0], min_max_tuple[1]), min_max_tuple[0])
                    if final_max <= final_min:
                        final_max = min(final_min + push_risk_fraction, min_max_tuple[1])

            final_bound_list.append((final_min, final_max))

        final_bound_tuple = tuple(final_bound_list)

    return final_cons, final_bound_tuple, max_const

