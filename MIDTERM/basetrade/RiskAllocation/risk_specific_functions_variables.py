
# Keep all the variables/functions which any risk module may need use but is specific to risk allocation

import numpy as np

exit_codes = {'Invalid_input_parameter':100, 'Invalid_input_file':101, 'Cannot_write_to_db':102,
              'Discrepancy_in_earlier_and_current_run':103, "Optimal_weights_not_found":104}

def add_shc_weights(grp, col_name_to_sum, col_name_to_create):
    """
    compute sum of weights/risk (or any other column) for a group and create a new column
    """
    grp[col_name_to_create] = grp[col_name_to_sum].sum()
    return grp


def add_shc_weights_wrt_cov(grp, col_name_to_sum, col_name_to_create, corr_df, shc_col_name, factor=1.0):
    """
    compute trans(wt).Corr.wt for a group to get overall risk and create a new column.
    Product which are assigned min risk are also handled.
    Final risk = Sum(Min Risk assigned products) +  trans(risk).Corr.risk (of risk obtianed by sharpe optimization)
    """
    wt_array = np.array(grp[col_name_to_sum].tolist())
    if len(wt_array) == 1:
        grp[col_name_to_create] = wt_array[0]
        return grp

    indices = ~np.isnan(wt_array)
    wt = wt_array[indices]
    array_shc = np.array(grp[shc_col_name].tolist())
    list_shc = list(array_shc[indices])

    if len(set(list_shc).intersection(set(corr_df.columns.tolist()))) != 0:
        # Get corr matrix
        corr_df2 = corr_df.loc[list_shc, list_shc]

        # If corr is not available, it means that enough data was not present for that shortcode
        kl = corr_df2.isnull().all() == True
        min_risk_products = kl[kl == True].index.tolist()

        min_risk_indices = [list_shc.index(i) for i in min_risk_products]
        wt_min_risk = wt[min_risk_indices]

        # Get data for shortcodes which have correlation present.
        non_min_risk_indices = [list_shc.index(i) for i in list_shc if i not in min_risk_products]
        list_shc2 = list(array_shc[indices][non_min_risk_indices])
        wt2 = wt[non_min_risk_indices]
        corr_df3 = corr_df.loc[list_shc2, list_shc2]
        covM = corr_df3.values

        risk = np.sqrt(np.dot(np.dot(wt2.T, covM), wt2)) * factor
        # Add min risk of min_riks_products to the corr adjusted risk
        grp[col_name_to_create] = risk + np.sum(wt_min_risk)
    else:
        grp[col_name_to_create] = np.sum(wt_array)

    return grp


def get_sharpe_of_group(grp, col_name_to_sum, col_name_to_create, cov_df, mean_series, shc_col_name):

    list_shc = grp[grp[col_name_to_sum].notnull()][shc_col_name].tolist()
    intersection_sh_list = list(set(list_shc).intersection(set(cov_df.columns.tolist())))
    if len(intersection_sh_list) != 0:
        wt_to_use = [grp.loc[grp[shc_col_name] == i, col_name_to_sum].tolist()[0] for i in intersection_sh_list]
        mean_ser = mean_series.loc[intersection_sh_list]
        covM = cov_df.loc[intersection_sh_list, intersection_sh_list].values
        sharpe = np.dot(np.array(mean_ser), wt_to_use ) /np.sqrt (np.dot(np.dot(wt_to_use, covM), wt_to_use))
    else:
        sharpe = np.nan

    grp[col_name_to_create] = sharpe

    return grp
