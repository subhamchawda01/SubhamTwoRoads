
import numpy as np
import pandas as pd
from scipy.optimize import minimize

from statistics_function import get_pnl_stats_for_all_columns, get_mean_covariance
from constraints_related_functions import get_constraints_optimization_format
from general_util_functions import exit_code_with_message
from risk_specific_functions_variables import exit_codes

def get_weights_and_combined_pnl_series(pnl_series_df, entity, weight_vector_for_dates, debug_mode):
    """
    Finds unconstrained optimal weights for pnl series passed and computes the pnl series with combination of these weights

    :param pnl_series_df:               dataframe. Date and pnl series of entity 
    :param entity:                      str. Entity may be shortcode,session or exchange
    :param weight_vector_for_dates:     list
    :return: 

    entity_wts:                         zip object of two lists (entity, weight)
    pnl_series_for_session:             series. Weigthed combined pnl series
    pnl_stats_df:                       dataframe. stats dataframe
    """

    pnl_stats_df = get_pnl_stats_for_all_columns(pnl_series_df, weight_vector_for_dates)
    print_stats = 0
    if debug_mode:
        print_stats = 1
    else:
        if entity != 'Shc':
            print_stats = 1
    if print_stats == 1:
        print("\nSummary Stats for " + entity)
        print(pnl_stats_df)

    list_of_entities = pnl_series_df.columns.tolist()
    list_of_entities.remove('Date')
    mean_entity, cov_entity, _ = get_mean_covariance(pnl_series_df[list_of_entities], weight_vector_for_dates)

    weights = get_weight_unconstrained_sharpe_optim(list_of_entities, mean_entity, cov_entity)
    entity_wts = zip(list_of_entities, weights)
    combined_sharpe = np.dot(np.array(mean_entity), weights)/np.sqrt(np.dot(np.dot(weights, cov_entity.values), weights))

    if debug_mode:
        print("Entities: ", " ".join(list_of_entities))
        print("Weights: ", " ".join(list(map(str,np.round(weights,2)))))
        print("Combined Sharpe: ", round(combined_sharpe,3))

    pnl_series_for_session = np.nansum(pnl_series_df[list_of_entities ]*weights ,axis=1)

    return entity_wts, pnl_series_for_session, pnl_stats_df


def get_weight_unconstrained_sharpe_optim(list_of_entities, mean_pnl, covM, regularization_constant=0.2):
    """
    Get unconstrained optimal weights 

    :param list_of_entities:                list
    :param mean_pnl:                        pandas series
    :param covM:                            dataframe
    :param regularization_constant:         float
    :return:                                list. Optimal weights with sum of weights = 1
    """

    # Get the target funtion to minimize
    covMatrix = covM.loc[list_of_entities, list_of_entities].values
    target_func = list(- 1 *np.array(mean_pnl.loc[list_of_entities].tolist()))
    target_func_formulation = lambda x : np.dot(np.array(target_func) ,x ) /np.sqrt \
        (np.dot(np.dot(x.T ,covMatrix) ,x)) + regularization_constant* np.sum(x ** 2) / (np.sum(abs(x)) ** 2)

    initial_wt = [1.0 / len(list_of_entities)] * len(list_of_entities)
    bounds_tuple = ((0, None),) * len(list_of_entities)
    res = minimize(target_func_formulation, initial_wt, method='L-BFGS-B', tol=1e-12, bounds=bounds_tuple,
                   options={"disp": False})

    wt = res.x.tolist()
    sum_weights = np.sum(wt)
    wt2 = [i / sum_weights for i in wt]

    return wt2


def get_weight_constrained_sharpe_optim(mean_pnl, covM, constraint_mat_df, risk_factor, total_risk,
                                        regularization_constant, temporal_constraints, push_risk, backtest_mode):
    """
    Get optimal weight for constrained optimization. Report them and print final sharpe value as well

    :param mean_pnl:                    pandas series      
    :param covM:                        dataframe
    :param constraint_mat_df:           dataframe
    :param risk_factor:                 dictionary of risk factors of session,exchange and overall firm level
    :param total_risk:                  float
    :param regularization_constant:     float
    :param temporal_constraints:        dataframe
    :param push_risk:                   float
    :param backtest_mode:               int 1/0
    :return:                            list of list. list of [shortcode, weight]
    """
    # Minimize target_func*x with subject to A_ub * x <= b_ub and A_eq * x = b_eq
    all_cols = constraint_mat_df.columns.tolist()
    all_shcs = all_cols.copy()
    all_shcs.remove('Min_value')
    all_shcs.remove('Type')
    all_shcs.remove('Session')
    all_shcs.remove('Exchange')
    mean_pnl = mean_pnl[all_shcs]
    covM = covM.loc[all_shcs, all_shcs]

    # Get the target funtion to minimize
    covMatrix = covM.values
    target_func = list(-1 * np.array(mean_pnl[all_shcs].tolist()))
    target_func_formulation = lambda x: np.dot(np.array(target_func), x) / np.sqrt(
        np.dot(np.dot(x.T, covMatrix), x)) + regularization_constant * np.sum(x ** 2) / (np.sum(abs(x)) ** 2)

    # Get final constrains and optimize
    final_cons, final_bound_tuple, max_const = get_constraints_optimization_format(all_shcs, constraint_mat_df, temporal_constraints,
                                                                                   risk_factor, total_risk, push_risk)

    wts_out = optim_wts_over_random_initial_weights(all_shcs, target_func_formulation, final_cons, final_bound_tuple, backtest_mode)

    # Bring weights back to normal scale
    wts_out2 = np.array(wts_out) * max_const

    print("Shortcodes: ", " ".join(all_shcs))
    print("Final Weights: ", " ".join(list(map(str, np.round(wts_out2, 0)))))
    print("Final Combined Sharpe: ", round(-1 * np.dot(np.array(target_func), wts_out2) / np.sqrt(np.dot(np.dot(wts_out2.T, covMatrix), wts_out2)),3))

    # Return a list shortcode and corresponding optimal weight
    return list(zip(all_shcs, wts_out2))


def optim_wts_over_random_initial_weights(all_shcs, target_func_formulation, final_cons, final_bound_tuple, backtest_mode):
    """
    Iterate over random 100 intilizations and return the optimal weights amongst them

    :param all_shcs:                    list
    :param target_func_formulation:     lambda function
    :param final_cons:                  dictionary of constraints
    :param final_bound_tuple:           tuple of bounds
    :param backtest_mode:               int 1/0
    :return:                            list
    """
    wts = []
    initial_wts = []
    # Loop over few number of random initialization in (0,1) to and get the output
    for j in range(0, 100):
        # print(j)
        initial_wt = np.random.rand(1, len(all_shcs))
        res = minimize(target_func_formulation, initial_wt, constraints=final_cons, bounds=final_bound_tuple,
                       method='SLSQP',
                       tol=1e-12, options={"disp": False})

        if res.message == 'Inequality constraints incompatible':
            if backtest_mode:
                print(res.message.upper())
                wts_out2 = np.array([0] * len(all_shcs))
                return wts_out2
            else:
                exit_code_with_message(res.message.upper(), exit_codes["Optimal_weights_not_found"])

        wts.append(res.x.tolist() + [res.fun, res.success])
        initial_wts.append(initial_wt.tolist()[0] + [res.fun, res.success])

    # Combine all runs
    wts_df = pd.DataFrame(wts, columns=all_shcs + ['Function_value', 'Success'])
    initial_wts_df = pd.DataFrame(initial_wts, columns=all_shcs + ['Function_value', 'Success'])

    wts_df = wts_df[wts_df['Success'] == True]
    initial_wts_df = initial_wts_df[initial_wts_df['Success'] == True]

    if wts_df.shape[0] == 0:
        if backtest_mode:
            print("Optimizer failed for all random initilizations".upper())
            wts_out2 = np.array([0] * len(all_shcs))
            return wts_out2
        else:
            exit_code_with_message("Optimizer failed for all random initilizations".upper(), exit_codes["Optimal_weights_not_found"])

    # Sort by the function value
    wts_df.sort_values('Function_value', inplace=True)
    initial_wts_df.sort_values('Function_value', inplace=True)

    # Pick the wts for which the function value is minimum
    wts_out = wts_df.iloc[0, :][all_shcs].tolist()

    return wts_out


