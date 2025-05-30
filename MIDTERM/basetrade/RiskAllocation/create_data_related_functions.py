
import numpy as np
import pandas as pd
import subprocess
import datetime
import glob, re

from sql_functions import *
from general_util_functions import *
from risk_specific_functions_variables import *

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.utils.date_utils import calc_prev_week_day, calc_next_week_day, get_dates_between, get_week_day_list_from_day


def get_num_queries_ran_data(date_list, pick_strat_cname_list):
    """
    Returns number of queries ran for a list of pick strat configs on list of days
    
    :param date_list:               list
    :param pick_strat_cname_list:   list
    :return:                        dataframe
    """

    out_data = pd.DataFrame(columns = ['Date', 'Shc_session', 'Num_queries_ran'])
    # fetch start end query
    start_end_query_id_data = get_start_end_query_ids(convert_list_to_tuple(pick_strat_cname_list))
    if start_end_query_id_data == None:
        print('Not able to fetch records from table PickstratConfig')

    # Loop for each date
    for curr_dt in date_list:
        dt = str(curr_dt)
        yr = dt[:4]
        mth = dt[4:6]
        day = dt[6:]
        # Trades file location
        log_file_path = os.path.join('/NAS1/logs/QueryTrades', yr, mth, day, 'trades.' + dt + '.')
        file_list = glob.glob(log_file_path + '*')
        for row in start_end_query_id_data:
            cname = row[0]
            start_query_id = row[1]
            end_query_id = row[2]
            if end_query_id < start_query_id:
                end_query_id = np.ceil(1.0*start_query_id/10)*10-1
            counter = 0
            for query_id in range(int(start_query_id), int(end_query_id) + 1):
                if log_file_path + str(query_id) in file_list:
                    # Multiple queries can run in a single trade file. count unique number from trades file
                    cmd = "cat " + log_file_path + str(query_id)  +" | awk '{if(NF > 9 && ($2==\"OPEN\" || $2 ==\"FLAT\")) {print $3}}' | sort | uniq -c | wc -l"
                    #TODO One case still needs to handled is that if 2 queries are ran, but 1 query trades 0 volume, then only 1 query is present in trades file not 2.
                    abc = subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
                    out, err = abc.communicate()
                    if out is not None:
                        out = out.decode('utf-8')
                    if err is not None:
                        err = err.decode('utf-8')
                    if len(err) > 0:
                        raise ValueError("Error in running command ", cmd)
                    out = out.strip().splitlines()
                    counter = counter + int(out[0])

            out_data.loc[out_data.shape[0]] = [curr_dt, cname, counter ]

    out_data['Shc_session'] = out_data['Shc_session'].apply(lambda x: x[:-4])  # Removing .txt from name
    return out_data


def get_median_configs_for_date(sim_pnl_df_pivot):
    """
    Fetch config with median sim pnl for each date
    
    :param sim_pnl_df_pivot:     dataframe. Index as Config , columns as dates and values as sim_pnl 
    :return:  
    
    date_median_config_dict:    dict. Key as date and value as median config name
    date_median_pnl_dict:       dict. Key as date and value as pnl of median config
    """


    date_median_config_dict = {}
    date_median_pnl_dict = {}

    list_of_dates = sim_pnl_df_pivot.columns.tolist()
    for col in list_of_dates:
        #pick only non missing values for day
        series = sim_pnl_df_pivot[~sim_pnl_df_pivot[col].isnull()][col]
        length = len(series)
        median_value = int(length/2)
        # Index will be -1 because list is 0 indexed but least can be 0.
        median_index = max(0, median_value-1)
        sorted_series = series.sort_values()
        median_config = sorted_series.index.tolist()[median_index]
        median_pnl = sorted_series[median_config]
        date_median_config_dict[col] = median_config
        date_median_pnl_dict[col] = median_pnl

    return date_median_config_dict, date_median_pnl_dict


def get_risk_numbers_for_configs(config_list, history_days, sim_pnl_series_risk_ptile, run_date):
    """
    Get 95 percentile min_pnl of last 300 days for list of configs. 
    This 95 percentile min_pnl is defined as risk for the sim pnl.
    
    :param config_list:                 list
    :param history_days:                int. Days to look at for calculating risk
    :param sim_pnl_series_risk_ptile    float. Percentile value of min pnl series considered as risk
    :param run_date:                    int
    :return:                            dict. Key as config_id and value as risk numbers
    """

    config_tuple = convert_list_to_tuple(config_list)

    data = get_risk_numbers_for_configs_from_db(config_tuple, run_date, history_days)
    df = pd.DataFrame(data, columns = ['config_id', 'min_pnl'])
    df['min_pnl'] = -1*df['min_pnl']
    df = df.groupby('config_id').quantile(sim_pnl_series_risk_ptile)

    median_config_risk_dict = df.to_dict()['min_pnl']

    return median_config_risk_dict


# Create data related functions

def create_data_for_all_days(tree_df, run_date, debug_mode, number_of_past_days_to_look=300, minimum_past_dates_data_needed=100):
    """
    Fetch real pnl,risk and create normalized data for all days that needs to be considered. backtest days included
    
    :param tree_df:                             dataframe. 
    :param run_date:                            int
    :param number_of_past_days_to_look:         int. past days to look at. 
                                                If backtesting for 200 days then, this argument should be 300(history_days)+200(backtest_days)
    :param minimum_past_dates_data_needed:      int. if number of days real pnl available for a product < this, then do not fetch data for that product
    :return: 
    
    pnl_series_df_pivot_final:                  dataframe. Date X Shortcode. Values as risk normalized real pnl 
    pnl_series_df_pivot_actual:                 dataframe. Date X Shortcode. Values as real pnl
    """

    min_date = calc_prev_week_day(run_date, number_of_past_days_to_look)
    print("\n  Preparing data ")
    if debug_mode:
        print("\n", number_of_past_days_to_look, " Date range : ", min_date, run_date)

    all_dates_to_look_at = get_week_day_list_from_day(run_date, number_of_past_days_to_look+1)

    unique_shc_session_comb = tree_df['Shc_session'].drop_duplicates().tolist()
    unique_shcs = tree_df['Shc'].drop_duplicates().tolist()
    unique_sessions = tree_df['Sub_session'].drop_duplicates().tolist()

    pnl_series_columns = ['Shc', 'Sub_session', 'Date', 'Pnl', 'Volume']
    pnl_series_data = fetch_real_pnl_series(run_date, min_date, convert_list_to_tuple(unique_shcs),
                                            convert_list_to_tuple(unique_sessions))
    pnl_series_df = pd.DataFrame(pnl_series_data, columns=pnl_series_columns)
    pnl_series_df['Shc_session'] = list(
        map(lambda x, y: str(x) + '.' + str(y), pnl_series_df['Shc'], pnl_series_df['Sub_session']))

    # Filtering Data
    # Filter unwanted rows out of the sql output
    pnl_series_df = pnl_series_df[pnl_series_df['Shc_session'].isin(unique_shc_session_comb)]
    if debug_mode:
        print("Number of shortcodes = ", len(unique_shc_session_comb))
        print("Original Size from DB = ", pnl_series_df.shape)

    shc_session_pnl_counts = pnl_series_df.groupby('Shc_session')['Pnl'].count()
    shc_session_with_enough_days = shc_session_pnl_counts[shc_session_pnl_counts > minimum_past_dates_data_needed].index.tolist()

    # Filter rows which have less than 300 days in total
    pnl_series_df = pnl_series_df[pnl_series_df['Shc_session'].isin(shc_session_with_enough_days)]
    if debug_mode:
        print("Size after removing products with <= ", minimum_past_dates_data_needed," days of real pnl available", pnl_series_df.shape)
        print("Number of shortcodes left = ", len(shc_session_with_enough_days), '\n')

    # Keep only required columns
    pnl_series_df = pnl_series_df[['Date', 'Shc_session', 'Pnl']]

    pick_strat_config_name_list = [i + '.txt' for i in shc_session_with_enough_days]

    # Get max loss numbers and merge with pnl numbers
    max_loss_df_full = get_risk_numbers_for_real(pick_strat_config_name_list, all_dates_to_look_at, debug_mode)
    pnl_series_df = pd.merge(pnl_series_df, max_loss_df_full, on=['Date', 'Shc_session'], how='left')

    # Get number of queries that actually ran on a day instead of assuming that every query installed actually ran.
    if debug_mode:
        print("Fetching number of queries that actually ran for a day and shortcode ")
    query_ran_data = get_num_queries_ran_data(all_dates_to_look_at, pick_strat_config_name_list)
    pnl_series_df = pnl_series_df.merge(query_ran_data, on=['Date', 'Shc_session'], how='left')
    #pnl_series_df['Num_queries_ran'] = pnl_series_df['Num_queries']

    pnl_series_df = combine_same_products(pnl_series_df, tree_df)
    pnl_series_df = scale_risk_numbers_for_real(pnl_series_df)

    # Normalize pnl wrt to max loss numbers. Scaling to unit risk
    pnl_series_df['Norm_Pnl'] = list(map(lambda x, y: max(-1, x/y) if (~np.isnan(y) and y != 0 and ~np.isnan(x)) else np.nan,
            pnl_series_df['Pnl'], pnl_series_df['Risk_Factor']))

    #print("\nPivoting Data")
    pnl_series_df_pivot = pnl_series_df[['Date', 'Shc_session', 'Norm_Pnl']].pivot(index='Date', columns='Shc_session',
                                                                                   values='Norm_Pnl')
    del (pnl_series_df_pivot.columns.name)
    pnl_series_df_pivot = pnl_series_df_pivot.reset_index()
    if debug_mode:
        print("\nFinal size of dataset = ", pnl_series_df_pivot.shape)
        print("Dates, Shortcodes = ", pnl_series_df_pivot.shape[0], pnl_series_df_pivot.shape[1] - 1),
        print("Total Elements = ", pnl_series_df_pivot.shape[0] * (pnl_series_df_pivot.shape[1] - 1), " Nan Values = ",
              pnl_series_df_pivot.isnull().sum().sum(), '\n')


    pnl_series_df_pivot_actual = pnl_series_df[['Date', 'Shc_session', 'Pnl']].pivot(index='Date', columns='Shc_session',
                                                                                   values='Pnl')
    del (pnl_series_df_pivot_actual.columns.name)
    pnl_series_df_pivot_actual = pnl_series_df_pivot_actual.reset_index()

    pnl_series_df_pivot_final = pnl_series_df_pivot.copy()

    risk_series_df_pivot = pnl_series_df[['Date', 'Shc_session', 'Risk_Factor']].pivot(index='Date', columns='Shc_session',
                                                                                   values='Risk_Factor')
    del (risk_series_df_pivot.columns.name)
    risk_series_df_pivot = risk_series_df_pivot.reset_index()

    pnl_series_df_pivot_final.sort_values('Date', ascending=False, inplace=True)
    pnl_series_df_pivot_actual.sort_values('Date', ascending=False, inplace=True)
    risk_series_df_pivot.sort_values('Date', ascending=False, inplace=True)

    return pnl_series_df_pivot_final, pnl_series_df_pivot_actual, risk_series_df_pivot


def get_risk_numbers_for_real(pick_strat_config_name_list, all_dates_to_look_at, debug_mode):
    """
    Get global max loss, sum of max loss, num queries for pick strat config names for all dates
    
    :param pick_strat_config_name_list:     list
    :param all_dates_to_look_at:            list
    :return:                                dataframe. Row is unique by shortcode, date
    """
    if debug_mode:
        print("Fetching maxloss numbers for each day and shortcode ")
    min_date_for_max_loss = min(all_dates_to_look_at)
    max_date_for_max_loss = max(all_dates_to_look_at)

    updated_min_date_for_max_loss = get_min_date_for_which_risk_available(min_date_for_max_loss, convert_list_to_tuple(pick_strat_config_name_list))

    max_loss_data = fetch_maxloss_numbers(updated_min_date_for_max_loss, max_date_for_max_loss,
                                          convert_list_to_tuple(pick_strat_config_name_list))
    max_loss_df = pd.DataFrame(max_loss_data,
                               columns=['Shc_session', 'Date', 'Global_maxloss', 'Sum_maxloss', 'Num_queries', 'Time'])

    max_loss_df['Date_prev'] = max_loss_df['Date']

    # If the time of the install is later than UTC 1700, then most probably the query is installed for next day rather than the current day
    max_loss_df['Date'] = list(
        map(lambda x, y: calc_next_week_day(x, 1) if y > 1700 else x, max_loss_df['Date'], max_loss_df['Time']))

    # Updating the time to 0001 in case it was installed for next day, so that we can pick the latest time of installation for a particular day
    max_loss_df['Time_v2'] = list(map(lambda x: 1 if x > 1700 else x, max_loss_df['Time']))

    # Above will change anything installed on Friday night to Saturday whereas it was intended for Monday
    # If anything was installed on Friday Night/Saturday/Sunday, then it is intended for Monday, so change the date according to weekday.
    max_loss_df['Date_weekday'] = max_loss_df['Date'].apply(
        lambda x: datetime.datetime.strptime(str(x), '%Y%m%d').weekday())  # 0 for Monday and 6 for Sunday
    max_loss_df['Date'] = list(map(lambda x, y: calc_next_week_day(x, 7 - y) if 7 - y <= 2 else x, max_loss_df['Date'],
                                   max_loss_df['Date_weekday']))

    # Also, updating the time to 0001, so that we can pick the latest time of installation for a particular day if another installation was done on Monday
    max_loss_df['Time_v3'] = list(
        map(lambda x, y: 1 if 7 - y <= 2 else x, max_loss_df['Time_v2'], max_loss_df['Date_weekday']))

    # For a date and shortcode, pick the latest time for which the pick strat was installed
    max_loss_df.sort_values(by=['Shc_session', 'Date', 'Time_v3','Date_prev', 'Time'], inplace=True)
    max_loss_df_2 = max_loss_df.drop_duplicates(['Shc_session', 'Date'], keep='last').copy()
    max_loss_df_2.drop(['Time', 'Time_v2', 'Time_v3', 'Date_prev', 'Date_weekday'], axis=1, inplace=True)

    all_valid_risk_dates = get_dates_between(updated_min_date_for_max_loss, max_date_for_max_loss)

    all_combinations = [[x, y] for x in all_valid_risk_dates for y in pick_strat_config_name_list]
    all_dates_session_df = pd.DataFrame(data=all_combinations, columns=['Date', 'Shc_session'])

    max_loss_df_full = pd.merge(all_dates_session_df, max_loss_df_2, on=['Date', 'Shc_session'], how='left')
    max_loss_df_full.sort_values(['Shc_session', 'Date'], inplace=True)

    # Fill null in Max loss number. Fill by previous valid number for a shc. This is because pick strat is not ran everyday and thus there are blanks in between
    # ffill is filling with previous available value which is correct way of doing
    # Initial entries (if null), still remain null, so filling them by next valid entry. Done by using bfill. This is NOT CORRECT. NEEDS TO BE CHANGED LATER. Commenting now.
    max_loss_df_full['Global_maxloss'] = max_loss_df_full.groupby('Shc_session')['Global_maxloss'].fillna(method='ffill') #.fillna(method='bfill')
    max_loss_df_full['Sum_maxloss'] = max_loss_df_full.groupby('Shc_session')['Sum_maxloss'].fillna(method='ffill') #.fillna(method='bfill')
    max_loss_df_full['Num_queries'] = max_loss_df_full.groupby('Shc_session')['Num_queries'].fillna(method='ffill') #.fillna(method='bfill')

    # Removing .txt from name
    max_loss_df_full['Shc_session'] = max_loss_df_full['Shc_session'].apply(lambda x: x[:-4])

    return max_loss_df_full


def scale_risk_numbers_for_real(pnl_series_df):
    """
    Normalize risk by the number of queries that ran on that day

    :param pnl_series_df:   dataframe
    :return:                dataframe
    """

    pnl_series_df['Global_maxloss'] = list(
        map(lambda x, y, z: x * z / y if (y != 0 and ~np.isnan(y)) else x, pnl_series_df['Global_maxloss'],
            pnl_series_df['Num_queries'], pnl_series_df['Num_queries_ran']))
    pnl_series_df['Sum_maxloss'] = list(
        map(lambda x, y, z: x * z / y if (y != 0 and ~np.isnan(y)) else x, pnl_series_df['Sum_maxloss'],
            pnl_series_df['Num_queries'], pnl_series_df['Num_queries_ran']))

    pnl_series_df[['Global_maxloss', 'Sum_maxloss']] = pnl_series_df[['Global_maxloss','Sum_maxloss']].fillna(0)

    pnl_series_df['Risk_Factor'] = list(
        map(lambda y, z: np.nan if (y==0 and z==0) else (min(y, z) if (min(y, z) != 0) else (z if y==0 else y)),
            pnl_series_df['Global_maxloss'], pnl_series_df['Sum_maxloss']))

    return pnl_series_df


def get_day_data_from_created_data(all_days_real_pnl_data_norm, run_date, fill_holes, simulated_pnl_df, debug_mode,
                                   bias_to_account=True, number_of_past_days_to_look=300, minimum_past_dates_data_needed=100,
                                   days_before_expiry_to_stop_DI = 120, days_to_consider_for_sim_real_bias=30, print_nothing=0):
    """
    From data for all days, fetch relevant data for the run date. 
    Fill holes in real pnl
    
    :param all_days_real_pnl_data_norm:         dataframe. Date X Shortcode. Values as risk normalized real pnl 
    :param run_date:                            int
    :param fill_holes:                          int 0,1,2. 1 for filling with sim pnl. 2 for filling with zeros
    :param simulated_pnl_df:                    dataframe. Simulated risk normalized pnl for all days. Empty for not filling
    :param bias_to_account:                     int. 0,1. sim_real bias to take into account
    :param number_of_past_days_to_look:         int. Number of past days to look at
    :param minimum_past_dates_data_needed:      int. if number of days real pnl available for a product < this, then do not consider that product for risk allocation
    :param days_before_expiry_to_stop_DI:       int. If DI expiry is within these number of days, then do not consider it for risk allocation
    :param days_to_consider_for_sim_real_bias:  int. These number of days are taken while accounting for sim real bias
    :return:                                    dataframe. Date X Shortcode. Values as risk normalized pnl (may be holes filled)
    """

    if debug_mode:
        print_nothing = 0

    min_date = calc_prev_week_day(run_date, number_of_past_days_to_look)
    if not print_nothing:
        print("\n  Fetching data for date : ", run_date, '\n')
        print(number_of_past_days_to_look, " Date range : ", min_date, calc_prev_week_day(run_date))

    all_dates_to_look_at = get_week_day_list_from_day(calc_prev_week_day(run_date), number_of_past_days_to_look)

    dates_filtered_df = all_days_real_pnl_data_norm[all_days_real_pnl_data_norm['Date'].isin(all_dates_to_look_at)].copy()
    dates_filtered_df.set_index('Date', inplace=True)
    all_shcs = dates_filtered_df.columns.tolist()
    shc_session_with_enough_days = dates_filtered_df.loc[:, dates_filtered_df.notnull().sum() > minimum_past_dates_data_needed].columns.tolist()

    # This step is just valid for DI's. It will remove all the products which are about to expire in next 120 days.
    DI_products = list(filter(lambda x: re.search(r'^DI', x), shc_session_with_enough_days))
    #print(DI_products)
    for shc_session in DI_products:
        shc = shc_session.split(".")[0]
        cmd = "~/basetrade_install/bin/get_di_term " + shc +  " " + str(run_date)
        abc = subprocess.Popen(cmd, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        out, err = abc.communicate()
        if out is not None:
            out = out.decode('utf-8')
        if err is not None:
            err = err.decode('utf-8')
        if len(err) > 0:
            raise ValueError("Error in running command ", cmd)
        out = out.strip().splitlines()[0]
        if float(out) < days_before_expiry_to_stop_DI:
            if not print_nothing:
                print(shc_session, " removed because of expiry date within ", days_before_expiry_to_stop_DI, " days")
            shc_session_with_enough_days.remove(shc_session)

    shc_removed_list = list(set(all_shcs)-set(shc_session_with_enough_days))
    if not print_nothing and len(shc_removed_list) != 0:
        print("Shortcodes removed due to less data: ", shc_removed_list)

    real_output_df = dates_filtered_df[shc_session_with_enough_days].reset_index()
    output_df = real_output_df.copy()

    if fill_holes == 1 and not simulated_pnl_df.empty:
        if debug_mode and not print_nothing:
            print("Filling holes in real with simulated pnl")
        # Fill real pnl data
        # Filter Dates
        simulated_df_corresponding = simulated_pnl_df[simulated_pnl_df['Date'].isin(all_dates_to_look_at)].copy()
        simulated_df_corresponding.set_index('Date', inplace=True)
        # Filter shortcodes
        simulated_df_corresponding = simulated_df_corresponding[shc_session_with_enough_days].reset_index()
        bias_df = real_output_df.subtract(simulated_df_corresponding)
        for shc in shc_session_with_enough_days:
            index_when_30_days_real_pnl_available = real_output_df.shape[0] - 1 - find_first_k_non_nulls(real_output_df[shc].sort_index(axis=0, ascending=False), days_to_consider_for_sim_real_bias)
            date_when_30_days_real_pnl_available = real_output_df.ix[index_when_30_days_real_pnl_available,'Date']
            fill_flag = ((real_output_df['Date'] > date_when_30_days_real_pnl_available) & (real_output_df[shc].isnull()))
            if bias_to_account:
                dates_to_fill = real_output_df[fill_flag]['Date'].tolist()
                for date in dates_to_fill:
                    bias_compute_flag = ((real_output_df['Date'] < date) & (real_output_df[shc].notnull()))
                    bias = np.array(bias_df.loc[bias_compute_flag, shc])
                    indices = ~np.isnan(bias)
                    bias_non_null = bias[indices][:days_to_consider_for_sim_real_bias] # limit to last 30 values
                    if len(bias_non_null) == 0:  avg_bias = 0
                    else: avg_bias = np.average(bias_non_null)
                    output_df.loc[output_df['Date'] == date, shc] = simulated_df_corresponding.loc[simulated_df_corresponding['Date'] == date, shc] + avg_bias
            else:
                output_df.loc[fill_flag, shc] = simulated_df_corresponding.loc[fill_flag, shc]

    if fill_holes == 2:
        if debug_mode and not print_nothing:
            print("Filling holes in real with zeros")
        output_df = output_df.fillna(0)

    return output_df


def normalize_simulated_pnl(sim_pnl_df_pivot, shc_session, history_days, sim_pnl_series_risk_ptile, end_date_for_min_pnls):
    """
    Normalize simulated pnl with the risk
    
    :param sim_pnl_df_pivot:            dataframe. Date X Shortcode. Values as sim pnl
    :param shc_session:                 str
    :param end_date_for_min_pnls:       int
    :param history_days:                int. Days to look at for calculating risk
    :param sim_pnl_series_risk_ptile    float. Percentile value of min pnl series considered as risk
    :return:                            dataframe. 2 columns, date and risk normalized sim pnl for particular shc
    """

    # Returns df with two columns names 'Date' and shc_session. shc_session column has risk normalized simulated pnl

    date_median_config_dict, date_median_pnl_dict = get_median_configs_for_date(sim_pnl_df_pivot)

    # For a config, 95% percentile of min_pnl for last 300 days from end_date_for_min_pnls is taken as the risk number
    median_config_risk_dict = dict(get_risk_numbers_for_configs(list(set(date_median_config_dict.values())), history_days, sim_pnl_series_risk_ptile, end_date_for_min_pnls))
    date_norm_sim_pnl = {}
    for k in date_median_pnl_dict.keys():
        risk = median_config_risk_dict.get(date_median_config_dict[k], 0)
        if risk != 0 and ~np.isnan(date_median_pnl_dict[k]):
            date_norm_sim_pnl[k] = max(-1, date_median_pnl_dict[k] / risk)  # Scaling to unit risk
        else:
            date_norm_sim_pnl[k] = np.nan  # date_median_pnl_dict[k]
    temp_df = pd.DataFrame.from_dict(date_norm_sim_pnl, orient='index').reset_index()
    temp_df.columns = ['Date', shc_session]

    return temp_df


def fetch_normalized_simulated_pnl(pnl_series_temp_df_pivot, history_days, sim_pnl_series_risk_ptile, debug_mode):
    """
    Get risk normalized simulated pnl for all dates and shortcodes
    
    :param pnl_series_temp_df_pivot:        dataframe. Date X Shortcode. Values as risk normalized real pnl
    :param history_days:                    int. Days to look at for calculating risk
    :param sim_pnl_series_risk_ptile        float. Percentile value of min pnl series considered as risk
    :return:                                dataframe. Date X Shortcode. Values as risk normalized sim pnl
    """

    list_of_shc_session = pnl_series_temp_df_pivot.columns.tolist()
    list_of_shc_session.remove('Date')

    date_list = pnl_series_temp_df_pivot['Date'].tolist()
    simulated_df = pnl_series_temp_df_pivot[['Date']].copy()
    max_date = np.max(date_list)

    for shc_session in list_of_shc_session:
        shc = shc_session.split('.')[0]
        sub_session = shc_session.split('.')[1]
        start_end_time_list = get_start_end_time_from_pick_strat_config(shc, sub_session)

        if len(date_list) == 0:
            if debug_mode:
                print("No holes for ", shc_session)
            simulated_df[shc_session] = pnl_series_temp_df_pivot[shc_session]
        else:
            if start_end_time_list == None:
                if debug_mode:
                    print("Cannot fill holes for ", shc_session)
                # Make simulated pnl same as the real pnl
                simulated_df[shc_session] = pnl_series_temp_df_pivot[shc_session]
            else:
                # Get simulated pnl from db for a product for all dates
                start_end_time_tuple = convert_list_to_tuple(start_end_time_list)
                data = get_simulated_pnl(shc, start_end_time_tuple, convert_list_to_tuple(date_list))
                sim_pnl_df = pd.DataFrame(data, columns=['Config_id', 'Date', 'Sim_Pnl'])
                sim_pnl_df_pivot = sim_pnl_df[['Config_id', 'Date', 'Sim_Pnl']].pivot(index='Config_id', columns='Date',
                                                                                      values='Sim_Pnl')
                del (sim_pnl_df_pivot.columns.name)

                if sim_pnl_df_pivot.empty:
                    if debug_mode:
                        print("UNEXPECTED: No simulated pnl found for regular pool for ", shc_session, start_end_time_tuple)
                    # Make simulated pnl same as the real pnl
                    simulated_df[shc_session] = pnl_series_temp_df_pivot[shc_session]
                else:
                    # Normalize simulated pnl by risk numbers
                    temp_df = normalize_simulated_pnl(sim_pnl_df_pivot, shc_session, history_days, sim_pnl_series_risk_ptile, max_date)
                    # temp_df has two columns names 'Date' and shc_session. shc_session column has risk normalized simulated pnl
                    simulated_df = simulated_df.merge(temp_df, on='Date', how='left')

    # Make column order same as the original df
    simulated_df = simulated_df[pnl_series_temp_df_pivot.columns.tolist()]
    simulated_df.sort_values('Date', ascending=False, inplace=True)

    return simulated_df


def combine_same_products(pnl_series_df, tree_df):
    """
    If there are certain products which needs to be considered same. Eg:- BR_WIN_0.US_HP, BR_WIN_0.US_LP, BR_WIN_0.US
    Different pick strats were ran for same shortcode giving different names to it but essentially they are same.

    :param pnl_series_df:    dataframe. Date X Shortcode. Values as risk normalized real pnl for all shcs
    :param tree_df:          dataframe. Information fetched from risk config file. Each row is a shortcode
    :return:                 dataframe. Updated after combining products to be considered same
    """
    if "Super_Product" in tree_df.columns.tolist():
        pnl_series_df_new = pnl_series_df.merge(tree_df[['Shc_session','Super_Product']], on = 'Shc_session', how='left')
        pnl_series_df_normal = pnl_series_df_new[pnl_series_df_new['Super_Product'].isnull()].copy()
        pnl_series_df_normal.drop('Super_Product', axis=1, inplace=True)
        pnl_series_df_super = pnl_series_df_new[~pnl_series_df_new['Super_Product'].isnull()]
        agg_dict = {'Global_maxloss': 'min', 'Sum_maxloss':'sum', 'Num_queries':'sum', 'Num_queries_ran': 'sum', 'Pnl': 'sum'}
        pnl_series_df_super_grouped = pnl_series_df_super.groupby(['Date','Super_Product']).agg(agg_dict).reset_index()
        pnl_series_df_super_grouped.rename(columns = {'Super_Product':'Shc_session'}, inplace=True)
        return pd.concat([pnl_series_df_normal, pnl_series_df_super_grouped], axis=0)

    return pnl_series_df


def read_risk_numbers_from_db(run_date, num_days, shc_session_list, exchange_list, tree_df):
    """
    Read previous days risk numbers from db and convert them to ratio form. Ratio of shc risk to exchange risk

    :param run_date:            int
    :param num_days:            int
    :param shc_session_list:    list
    :param exchange_list:       list
    :param tree_df:             datarame
    :return:                    dataframe
    """

    min_date = calc_prev_week_day(run_date, num_days+1)
    if len(exchange_list) == 1:
        list_shc_session_to_filter = shc_session_list + [i + '.All' for i in exchange_list]
    else:
        list_shc_session_to_filter = shc_session_list + ['All.All']

    data = fetch_risk_numbers_from_db(min_date ,run_date, convert_list_to_tuple(list_shc_session_to_filter))
    df_columns = ['Run_id', 'Shortcode', 'Session', 'Date', 'Risk']
    if data == None:
        return pd.DataFrame(columns = ['Date'] + shc_session_list + ['Overall_wt_wrt_corr'])
    else:
        df = pd.DataFrame(data, columns = df_columns)

        df['Shc_session'] = list(map(lambda x,y: x +'.'+ y, df['Shortcode'], df['Session']))
        df_new = df[df['Shc_session'].isin(shc_session_list)]
        run_ids = df_new.groupby(['Run_id','Date']).size().reset_index()
        run_ids_grouped = run_ids.groupby('Date').size().reset_index()
        if run_ids_grouped[run_ids_grouped[0] > 1].shape[0] > 0:
            exit_code_with_message("The exchanges were ran independently for previous days. Cannot decide what to consider the total risk. Exiting",
                                   exit_codes['Discrepancy_in_earlier_and_current_run'])

        df_filtered_overall = run_ids.merge(df[df['Session'] == 'All'], on=['Run_id' ,'Date'], how='left')[['Date', 'Risk']].rename(columns = {'Risk':'Overall_wt_wrt_corr'})
        df_pivot = df_new[['Shc_session', 'Date', 'Risk']].pivot(index='Date', columns='Shc_session', values='Risk')
        del (df_pivot.columns.name)
        df_pivot = df_pivot.reset_index()
        df_pivot = df_pivot.merge(df_filtered_overall, on='Date', how='left')
        return df_pivot
