
import sys, os
import MySQLdb

sys.path.append(os.path.expanduser('~/basetrade/'))
from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.utils.date_utils import calc_prev_week_day


def get_start_end_query_ids(pick_strat_cname_tuple):
    """
    Return start and end query id's from Pick strat config

    pick_strat_cname_tuple:     tuple of configs           

    returns:                    list of list with records


    """
    cursor = connection().cursor()
    search_query = ("Select config_name, start_queryid, end_queryid from PickstratConfig where config_name in %s;" %
                    (pick_strat_cname_tuple,))
    #print(search_query)
    # execute the SQL query using execute() method.
    cursor.execute(search_query)
    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    # print the rows

    if len(data) > 0:
        return data

    return None


def get_risk_numbers_for_configs_from_db(config_tuple, run_date, history_days):
    """
    Return min_pnl series of last 300 days for list of configs
    

    config_tuple:       tuple of configs           

    run_date:           int
                        date (including) before which the 300 days data needs to be fetched

    history_days:       int. Days to look at for calculating risk

    returns:            list of list with records


    """


    min_date = calc_prev_week_day(run_date, history_days)

    cursor = connection().cursor()
    search_query = (
     "Select configid, min_pnl from wf_results where configid in %s and date <= %s and date >= %s and pnl <> 0 and vol <> 0;" %
    (config_tuple, run_date, min_date))
    #print(search_query)
    # execute the SQL query using execute() method.
    cursor.execute(search_query)
    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    # print the rows

    if len(data) > 0:
        return data

    return None


def fetch_real_pnl_series(max_date, min_date, shortcode_tuple, session_tuple):
    """

    Returns the Real pnl data for set of shortcode, session and dates

    max_date:           int
                        date (including) before which the latest data needs to be fetched

    max_date:           int
                        date (including) after which the latest data needs to be fetched

    shortcode_tuple:    tuple of shortcodes to fetch for 

    session_tuple:      tuple of sessions to fetch for

    returns:            list of list with records


    """

    cursor = connection().cursor()
    search_query = (
    "SELECT shortcode, session, date, pnl, volume FROM RealPNLS WHERE shortcode in %s and session in %s and date <= %d and date >= %d;" %
    (shortcode_tuple, session_tuple, max_date, min_date))
    # execute the SQL query using execute() method.
    cursor.execute(search_query)
    # fetch all of the rows from the query
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    # print the rows

    if len(data) > 0:
        return data

    return None


def get_min_date_for_which_risk_available(min_date, config_name_tuple):
    """
    
    Sub_query to find minimum of (max_dates of all shortcodes for which an entry is present in pick strats before min date)

    min_date:           int

    config_name_tuple:  tuple of Pick strat config names to fetch records for 

    returns:            int 

    """

    cursor = connection().cursor()
    sub_query = "select min(max_date) as min_max_date from \
                (select max(date) as max_date from PickstratRecords as pr, PickstratConfig as pc \
                where pr.config_id = pc.config_id and date < '%s' and config_name in %s group by pr.config_id) as sq1;" % \
                (min_date, config_name_tuple)

    # print(sub_query)
    cursor.execute(sub_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)

    return_date = None
    if len(data) > 0:
        return_date = int(data[0][0])

    if return_date == None:
        return_date = int(min_date)

    return return_date


def fetch_maxloss_numbers(min_date, max_date, config_name_tuple):
    """

    min_date:           int
                        date (including) after which the records needs to fetched

    max_date:           int
                        date (including) before which the latest data needs to be fetched

    config_name_tuple:  tuple of Pick strat config names to fetch records for 

    returns:            list of list with required records 


    """
    cursor = connection().cursor()
    search_query = ("SELECT config_name, date, global_maxloss, sum_maxlosses, num_queries, time FROM PickstratConfig, PickstratRecords \
                    WHERE PickstratRecords.config_id = PickstratConfig.config_id and date >= '%s' and date <= '%s' and config_name in %s;" % \
                    (min_date, max_date, config_name_tuple))
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


def get_simulated_pnl(shc, start_end_time_tuple, date_list, type='N', strat_type='Regular'):
    """

    Returns sim pnl for shc, start_end_time list and date list 

    shc:                        str
         
    start_end_time_tuple :      tuple of start end combinations to look for the shc 

    date_list :                 list of dates

    returns:                    list of list having configid, date and sim pnl


    """
    cursor = connection().cursor()
    search_query = (
        "Select wf_results.configid, date, pnl from wf_configs, wf_results where wf_configs.configid = wf_results.configid and \
        shortcode = '%s' and CONCAT(start_time, '-', end_time) in %s and date in %s and type = '%s' and strat_type = '%s' and pnl <> 0 and vol <> 0" %
        (shc, start_end_time_tuple, date_list, type, strat_type))
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


def insert_update_risk_number_to_db(data_to_insert, overwrite=False):
    """
    Inserts/updates the entries

    :param data_to_insert:  list of list. Each list is a row that needs to be inserted/updated
    :return:                None
    """

    sql_select = "SELECT * from RiskAllocated where shortcode = '%s' and session = '%s' and date = '%s' ; "
    insert_query = "INSERT into RiskAllocated (shortcode, session, date, risk, min_constraint, max_constraint ) Values ('%s', '%s', '%s', '%s', '%s', '%s') ; "
    update_query = "UPDATE RiskAllocated SET risk = '%s', min_constraint = '%s', max_constraint = '%s' WHERE shortcode = '%s' and session = '%s' and date = '%s' ; "
    cursor = connection().cursor()
    for row in data_to_insert:
        [shortcode, session, date, risk, min_const, max_const] = [i for i in row]
        cursor.execute(sql_select%(shortcode, session, date))
        data = cursor.fetchall()
        data = sql_out_to_str(data)
        if len(data) > 0:
            if overwrite:
                try:
                    #print(update_query%(risk, min_const, max_const, shortcode, session, date))
                    cursor.execute(update_query%(risk, min_const, max_const, shortcode, session, date))
                except MySQLdb.Error as e:
                    print(("Could not execute the command %s " % (update_query%(risk, min_const, max_const, shortcode, session, date)), e))
        else:
            try:
                #print(insert_query%(shortcode, session, date, risk, min_const, max_const))
                cursor.execute(insert_query%(shortcode, session, date, risk, min_const, max_const))
            except MySQLdb.Error as e:
                print(("Could not execute the command %s " % (insert_query%(shortcode, session, date, risk, min_const, max_const)), e))


def insert_risk_records_to_db(data_to_insert, target_risk, exchange_list):
    """
    Inserts entries to db. This assumes we have already called remove_risk_run_ids in the code before coming here. So just inserting records.

    :param data_to_insert:  list of list. Each list is a row that needs to be inserted/updated
    :param target_risk:     float
    :param exchange_list:   list of exchanges considered in this run
    :return:
    """

    cursor = connection().cursor()
    insert_query_runid_table = "INSERT into RiskRunIds (target_risk, exchanges) values ('%s', '%s'); "%(target_risk, ",".join(exchange_list))
    select_last_id = "SELECT LAST_INSERT_ID();"
    try:
        #print(insert_query_runid_table)
        cursor.execute(insert_query_runid_table)
    except MySQLdb.Error as e:
        print(("Could not execute the command %s " % (insert_query_runid_table), e))
        exit(125)

    # Fetch the last inserted id
    cursor.execute(select_last_id)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        run_id = int(data[0][0])

    insert_query = "INSERT into RiskAllocated (run_id, shortcode, session, date, risk, min_constraint, max_constraint ) Values ('%s','%s', '%s', '%s', '%s', '%s', '%s') ; "
    cursor = connection().cursor()
    for row in data_to_insert:
        [shortcode, session, date, risk, min_const, max_const] = [i for i in row]
        try:
            #print(insert_query%(run_id, shortcode, session, date, risk, min_const, max_const))
            cursor.execute(insert_query%(run_id, shortcode, session, date, risk, min_const, max_const))
        except MySQLdb.Error as e:
            print(("Could not execute the command %s " % (insert_query%(run_id, shortcode, session, date, risk, min_const, max_const)), e))



def fetch_risk_numbers_from_db(min_date, max_date, shc_session_to_filter_tuple):
    """
    Fetch all risk numbers between two dates

    :param min_date:                int
    :param max_date:                int
    shc_session_to_filter_tuple:    tuple
    :return:                        list of list
    """

    cursor = connection().cursor()
    sql_select = "SELECT run_id, shortcode, session, date, risk from RiskAllocated where date > '%s' and date < '%s' and CONCAT(shortcode, '.', session) in %s; "\
                 %(min_date, max_date, shc_session_to_filter_tuple)
    cursor.execute(sql_select)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return data

    return None


def fetch_risk_runID_present(exchange_tuple, run_date):
    """
    If an run for any exchange was done on run_date earlier than this, then fetch run_ids corresponding to those

    :param exchange_tuple:  tuple of exchanges
    :param run_date:        int
    :return:                list of list.  [exchange, run_id]
    """

    cursor = connection().cursor()
    sql_select = "SELECT distinct shortcode, run_id from RiskAllocated where date = '%s' and shortcode in %s;" % (run_date, exchange_tuple)
    #print(sql_select)
    cursor.execute(sql_select)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return data

    return None


def remove_risk_run_ids(run_ids):
    """
    Remove records for the given run_id from db

    :param run_ids:   tuple
    :return:
    """

    cursor = connection().cursor()
    delete_query = "DELETE from RiskRunIds where run_id in %s"%(run_ids,)
    try:
        #print(delete_query)
        cursor.execute(delete_query)
    except MySQLdb.Error as e:
        print(("Could not execute the command %s "%(delete_query), e))