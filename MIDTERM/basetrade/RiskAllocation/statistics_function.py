
import numpy as np
import pandas as pd


def get_over_days_DD(pnl_series):
    """
    Get over the days drawdown
    
    :param pnl_series:      pandas series 
    :return:                int
    """

    cumulative_pnl = 0
    max_pnl = 0
    drawdown = 0
    for pnl_value in pnl_series:
        if ~np.isnan(pnl_value):
            cumulative_pnl += pnl_value
            max_pnl = max(max_pnl, cumulative_pnl)
            drawdown = max(drawdown, max_pnl - cumulative_pnl)
    return drawdown


def get_pnl_stats(pnl_series, weights=None):
    """
    Return weighted avg, std, sharpe, over days drawdown and number of nulls for a particluar series
    
    :param pnl_series:  pandas series 
    :param weights:     list
    :return:            list having the stats of series
    """

    indices = ~np.isnan(pnl_series)
    if len(pnl_series[indices]) == 0:
        return [np.nan, np.nan, np.nan, np.nan, len(pnl_series)]

    if weights is None:
        weights = [1] * len(pnl_series)
    wt = pd.Series(weights)
    wt.index = pnl_series.index

    pnl_avg = pnl_std = pnl_sharpe = np.nan
    if wt[indices].sum() != 0:
        pnl_avg = np.average(pnl_series[indices], weights=wt[indices])
        pnl_std = np.sqrt(np.average(((pnl_series - pnl_avg)**2)[indices], weights=wt[indices]))
        pnl_sharpe = np.nan if pnl_std == 0 else pnl_avg / pnl_std

    pnl_overdays_DD =  get_over_days_DD(pnl_series)
    nan_values = len(pnl_series)- len(pnl_series[indices])
    return [pnl_avg, pnl_std, pnl_sharpe, pnl_overdays_DD, nan_values]


def get_pnl_stats_for_all_columns(df, weights=None):
    """
    Calls get_pnl_stats for all columns (except Date col) of a dataframe
    
    :param df:          dataframe
    :param weights:     list
    :return:            a dataframe with stats of a input column in a row 
    """
    out_df = pd.DataFrame(columns = ['Shc', 'Pnl_avg', 'Pnl_std', 'Pnl_sharpe', 'Pnl_overdays_DD', 'Number of Nan'])
    for col in df.columns.tolist():
        if col == 'Date':
            continue
        out_df.loc[out_df.shape[0]] = [col] + get_pnl_stats(df[col], weights)
    return out_df[['Shc', 'Pnl_avg', 'Pnl_std', 'Pnl_sharpe', 'Pnl_overdays_DD']]


def m(x, w):
    """
    Weighted Mean
    
    :param x:   np array 
    :param w:   np array
    :return:    int
    """
    return np.average(x, weights=w)


def cov(x, y, w):
    """
    Weighted Covariance
        
    :param x:   np array 
    :param y:   np array
    :param w:   np array
    :return:    int

    """
    return np.sum(w * (x - m(x, w)) * (y - m(y, w))) / np.sum(w)


def corr(x, y, w):
    """
    Weighted Correlation

    :param x:   np array 
    :param y:   np array
    :param w:   np array
    :return:    int

    """
    return cov(x, y, w) / np.sqrt(cov(x, x, w) * cov(y, y, w))


def get_mean_covariance(pnl_series_df_pivot, weight_vector_dates_to_use):
    """
    Returns weighted mean, cov and corr removing null values for a dataframe with columns as pnl series of shortcodes
    Cov and corr is computed pairwise, removing common nulls of the pair only
    
    :param pnl_series_df_pivot:             df with columns as pnl series of different products    
    :param weight_vector_dates_to_use:      list
    :return: 
    
    mean_df:    pandas series
                Mean of every column
    cov_df:     pandas dataframe
                Index and columns as shortcodes (columns of input df), values as covariance
    corr_df:    pandas dataframe
                Index and columns as shortcodes (columns of input df), values as correlation
    """

    shcs_list = pnl_series_df_pivot.columns.tolist()
    # Get weighted mean removing nan values
    data = pnl_series_df_pivot.values.T
    cleaned_data = np.ma.masked_array(data, np.isnan(data))
    average = np.ma.average(cleaned_data, axis=1, weights=weight_vector_dates_to_use).filled(np.nan)
    mean_df = pd.Series(average)
    mean_df.index = shcs_list
    num_shc = data.shape[0]

    cov_matrix = np.zeros((num_shc, num_shc))
    corr_matrix = np.zeros((num_shc, num_shc))
    # Get weighted covariance removing nan values
    for i in range(0,num_shc):
        for j in range(i,num_shc):
            corr_value = cov_value = 0
            i_mean = data[i, :]
            j_mean = data[j, :]
            non_null_index = ~np.isnan(i_mean) & ~np.isnan(j_mean)
            i_final = i_mean[non_null_index]
            j_final = j_mean[non_null_index]
            weight_final = [weight_vector_dates_to_use[k] for k in range(0, len(weight_vector_dates_to_use)) if non_null_index[k]]
            if len(weight_final)*len(i_final)*len(j_final) != 0:
                corr_value = max(0, corr(i_final, j_final, weight_final))
                cov_value = max(0, cov(i_final, j_final, weight_final))
            corr_matrix[i,j] = corr_matrix[j,i] =  corr_value
            cov_matrix[i,j] = cov_matrix[j,i] =  cov_value
    cov_df = pd.DataFrame(cov_matrix, columns=shcs_list, index=shcs_list)
    corr_df = pd.DataFrame(corr_matrix, columns=shcs_list, index=shcs_list)

    return mean_df, cov_df, corr_df