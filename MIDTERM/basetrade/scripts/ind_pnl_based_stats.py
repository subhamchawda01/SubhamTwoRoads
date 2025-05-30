import os
import sys
import shutil
import numpy as np
import json
import MySQLdb
import time
import math

from grid.client.api import GridClient
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.wf_db_utils.utils import FileExistsWithSize
from PyScripts.generate_dates import get_traindates

conn = MySQLdb.connect(host="52.87.81.158", user='dvcwriter', passwd="f33du5rB", db="PnlBasedStats")
conn.autocommit(True)
cursor = conn.cursor()
GRID_URL = "http://10.1.4.15:5000"


def read_master_ilist(ilist_filename):
    """
    Reads a ilist, and returns a tuple of indicators and their weights
    :param ilist_filename: (str)
    :return: (list(str),list(float))
    """
    indicators = []
    weights = []
    f = open(ilist_filename, 'r')
    for line in f:
        tokens = line.split()
        if len(tokens) > 0 and tokens[0] == "INDICATOR":
            end_indx = len(tokens)
            for j in range(len(tokens)):
                if tokens[j][0] == '#':
                    end_indx = j
                    break
            indicator = ' '.join(x for x in tokens[2:end_indx])
            weights.append(float(tokens[1]))
            indicators.append(indicator)

    f.close()
    return indicators, weights


def get_shc_id(shortcode):
    """
    Fetch the shc_id for a shortcode
    :param shortcode: (str)
    :return: (int)
    """
    sql_query = "SELECT shc_id FROM shortcodes WHERE shortcode=\"%s\"" \
                % shortcode
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return int(data[0][0])
    return -1


def get_tp_id(timez):
    """
    Fetch the tp_id for a time
    :param timez: (str)
    :return: (int)
    """
    sql_query = "SELECT tp_id FROM times WHERE time=\"%s\"" % timez
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return int(data[0][0])
    return -1


def insert_product_in_db(shc_id, start_time_id, end_time_id):
    """
    Inserts product in db 
    :param shc_id: (int)
    :param start_time_id: (int) 
    :param end_time_id: (int)
    :return: 
    """
    sql_query = "SELECT prod_id FROM products WHERE shc_id=%d AND start_time_id=%d AND end_time_id=%d" \
                % (shc_id, start_time_id, end_time_id)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return
    sql_query = "INSERT INTO products(shc_id, start_time_id, end_time_id) VALUES(%d,%d,%d)" \
                % (shc_id, start_time_id, end_time_id)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)


def insert_indicator_in_db(indicator):
    """
    Inserts an indicator in db
    :param indicator: (str)
    :return: 
    """
    sql_query = "SELECT ind_id FROM indicators WHERE indicator=\"%s\"" \
                % indicator
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return
    sql_query = "INSERT INTO indicators(indicator) VALUES(\"%s\")" \
                % indicator
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)


def insert_ind_id_prod_id(ind_id, prod_id):
    """
    Insert ind_id and a prod_id , to create a new entry (prod_ind_id)
    :param ind_id: (int)
    :param prod_id: (int)
    :return: 
    """
    sql_query = "SELECT prod_ind_id FROM prod_indicators WHERE ind_id=%d AND prod_id=%d" \
                % (ind_id, prod_id)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return
    sql_query = "INSERT INTO prod_indicators(prod_id,ind_id) VALUES (%d, %d)" \
                % (prod_id, ind_id)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)


def get_prod_id(shc_id, start_time_id, end_time_id):
    """
    Fetches a prod_id 
    :param shc_id: (int)
    :param start_time_id: (int)
    :param end_time_id: (int)
    :return: (int)
    """
    sql_query = "SELECT prod_id FROM products WHERE shc_id=%d AND start_time_id=%d AND end_time_id=%d" \
                % (shc_id, start_time_id, end_time_id)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return int(data[0][0])
    return -1


def insert_shortcode(shortcode):
    """
    Insert shortcode
    :param shortcode: (str)
    :return: 
    """
    sql_query = "SELECT shc_id FROM shortcodes WHERE shortcode=\"%s\"" % shortcode
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return
    sql_query = "INSERT INTO shortcodes(shortcode) VALUES(\"%s\")" % shortcode
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)


def insert_time(timez):
    """
    Insert time 
    :param timez: (str)
    :return: 
    """
    sql_query = "SELECT tp_id FROM times WHERE time=\"%s\"" % timez
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return
    sql_query = "INSERT INTO times(time) VALUES(\"%s\")" % timez
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)


def insert_product(shortcode, start_time, end_time):
    """
    Insert product 
    :param shortcode: (str)
    :param start_time: (str)
    :param end_time: (str)
    :return: 
    """
    insert_shortcode(shortcode)
    print('Shortcode Inserted')
    insert_time(start_time)
    print('Start time inserted')
    insert_time(end_time)
    print('End time inserted')
    shc_id = get_shc_id(shortcode)
    start_time_id = get_tp_id(start_time)
    end_time_id = get_tp_id(end_time)

    insert_product_in_db(shc_id, start_time_id, end_time_id)


def insert_indicators(indicators):
    """
    Insert list of indicators 
    :param indicators: (list(str))
    :return: 
    """
    ind_ids = []
    for indicator in indicators:
        insert_indicator_in_db(indicator)
        ind_id = get_indicator_id(indicator)
        ind_ids.append(ind_id)

    return ind_ids


def insert_indicator_ids_for_product(ind_ids, prod_id):
    """
    Insert ind_id for prod_id
    :param ind_ids: list(int)
    :param prod_id: (int)
    :return: 
    """
    prod_ind_ids = []
    for ind_id in ind_ids:
        insert_ind_id_prod_id(ind_id, prod_id)
        prod_ind_id = get_prod_ind_id(prod_id, ind_id)
        prod_ind_ids.append(prod_ind_id)

    return prod_ind_ids


def insert_indicators_for_product(shortcode, start_time, end_time, master_ilist):
    """
    Function to insert product, indicators from master ilist and assign in prod_indicators
    :param shortcode: (str)
    :param start_time: (str)
    :param end_time: (str)
    :param master_ilist: (str)
    :return: list(int)
    """
    insert_product(shortcode, start_time, end_time)
    print('Product Inserted')
    shc_id = get_shc_id(shortcode)
    start_time_id = get_tp_id(start_time)
    end_time_id = get_tp_id(end_time)
    product_id = get_prod_id(shc_id, start_time_id, end_time_id)

    indicators, _ = read_master_ilist(master_ilist)
    print(indicators)
    ind_ids = insert_indicators(indicators)
    print('Indicators Inserted')
    prod_ind_ids = insert_indicator_ids_for_product(ind_ids, product_id)

    return prod_ind_ids


def get_prod_ids_for_shortcode(shc):
    prod_ids = []
    sql_query = "select prod_id from products join shortcodes using(shc_id) where shortcode = '{}'".format(shc)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    if len(data) > 0:
        for i in range(len(data)):
            prod_ids.append(int(data[i][0]))
    return prod_ids


def get_indicator_id(indicator):
    """
    Fetch ind_id
    :param indicator: (str) 
    :return: (int)
    """
    sql_query = "SELECT ind_id FROM indicators WHERE indicator=\"%s\"" \
                % indicator
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    cursor.execute(sql_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return int(data[0][0])
    return -1


def get_prod_ind_id(prod_id, ind_id):
    """
    Fetch prod_ind_id for prod_id and ind_id
    :param prod_id: (int)
    :param ind_id: (int)
    :return: (int)
    """
    sql_query = "SELECT prod_ind_id FROM prod_indicators WHERE prod_id=%d AND ind_id=%d" \
                % (prod_id, ind_id)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return int(data[0][0])
    return -1


def get_all_ind_ids_for_prod_id(prod_id):
    """
    Fetch all indicator ids for a prod_id
    :param prod_id: (int)
    :return: list(int)
    """
    ind_ids = []
    sql_query = "SELECT ind_id FROM prod_indicators WHERE prod_id=%d" \
                % prod_id
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    if len(data) > 0:
        for i in range(len(data)):
            ind_ids.append(int(data[i][0]))
    return ind_ids


def get_all_prod_ind_ids_for_prod_id(prod_id):
    """
    Fetch all prod_ind_id for a prod_id
    :param prod_id: (int)
    :return: (list(int))
    """
    prod_ind_ids = []
    sql_query = "SELECT prod_ind_id FROM prod_indicators WHERE prod_id=%d" \
                % prod_id
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    if len(data) > 0:
        for i in range(len(data)):
            prod_ind_ids.append(int(data[i][0]))
    return prod_ind_ids


def get_indicator_ids(indicators):
    """
    Fetch list of ind_id for a list of indicators    
    :param indicators: (list(str))
    :return: list(int)
    """
    ind_ids = []
    for indicator in indicators:
        ind_id = get_indicator_id(indicator)
        ind_ids.append(ind_id)
    return ind_ids


def get_shortcode_from_prod_id(prod_id):
    """
    Fetch shortcode for a prod_id
    :param prod_id: (int)
    :return: (str)
    """
    sql_query = "SELECT shortcodes.shortcode FROM shortcodes, products " \
                "WHERE products.prod_id=%d AND products.shc_id=shortcodes.shc_id" \
                % prod_id
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return data[0][0]
    return ""


def get_start_time_from_prod_id(prod_id):
    """
    Fetch start_time for a prod_id
    :param prod_id: (int)
    :return: (str)
    """
    sql_query = "SELECT times.time FROM times, products " \
                "WHERE products.prod_id=%d AND times.tp_id=products.start_time_id" \
                % prod_id
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return data[0][0]
    return ""


def get_end_time_from_prod_id(prod_id):
    """
    Fetch end_time for a prod_id
    :param prod_id: (int)
    :return: (str)
    """
    sql_query = "SELECT times.time FROM times, products " \
                "WHERE products.prod_id=%d AND times.tp_id=products.end_time_id" \
                % prod_id
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute the command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    if len(data) > 0:
        return data[0][0]
    return ""


def read_day_output(prod_id, ilist, date, output_file):
    """
    Read a day's pnl stats and inserts in db
    :param prod_id: (int)
    :param ilist: (str)
    :param date: (int)
    :param output_file: (str)
    :return:
    """
    shortcode = get_shortcode_from_prod_id(prod_id)
    start_time = get_start_time_from_prod_id(prod_id)
    end_time = get_end_time_from_prod_id(prod_id)

    if shortcode == "" or start_time == "" or end_time == "":
        print("Invalid prod_id %d" % prod_id)
        return

    indicators, _ = read_master_ilist(ilist)

    ind_ids = get_indicator_ids(indicators)

    if not FileExistsWithSize(output_file):
        return
    lines = open(output_file, 'r').read().splitlines()

    indicator_str = " ".join(map(str, ind_ids))
    num_rows = None
    self_term_mean_str = ""
    cross_term_str = ""
    for line_idx in range(len(lines)):
        line = lines[line_idx]
        tokens = line.split()
        if tokens[0] != "PNL_BASED_STATS":
            print('Something wrong with output: ' + output_file +
                  ' for configuration: ' + shortcode + "," +
                  start_time + "," + end_time + "," + ilist + "," + str(date))
            break
        if num_rows is None:
            num_rows = tokens[1]
        self_term_mean_str += tokens[2] + " "
        cross_term_str += " ".join(tokens[4:4+line_idx]) + " " + tokens[3] + "\n"
    if num_rows is None:
        return
    ind_mean_cov_str = indicator_str + "\n" + num_rows + "\n" + self_term_mean_str.strip() \
                       + "\n" + cross_term_str.strip()
    insert_mean_cov(prod_id, date, ind_mean_cov_str)
    #print(prod_id, date)
    #print(ind_mean_cov_str)


def insert_mean_cov(prod_id, date, ind_mean_cov_str):
    """
    Inserts mean_cov_stats of indicators in pnl_based_stats tables's std_list_n_cov_mat col.
    :param prod_id:
    :param date:
    :param ind_mean_cov_str:
    :return:
    """
    sql_query = "INSERT INTO pnl_based_stats(prod_id, date, std_list_n_cov_mat) VALUES(%d, %d, '%s') \
                ON DUPLICATE KEY UPDATE std_list_n_cov_mat = '%s'"\
                % (prod_id, date, ind_mean_cov_str, ind_mean_cov_str)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute commmand %s" % sql_query, e)


def get_indicator_from_ind_id(ind_id):
    """
    Fetch indicator from ind_id
    :param ind_id: (int)
    :return: (str)
    """
    sql_query = "SELECT indicator FROM indicators WHERE ind_id=%d" % ind_id
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute command %s" % sql_query, e)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    return data[0][0]


def create_temp_ilist_for_product(ind_ids, prod_id):
    """
    Creates temp ilist for a prod_id, and list of ind_id
    :param ind_ids: (list(int))
    :param prod_id: (int)
    :return: (str)
    """
    temp_ilist = ""
    shortcode = get_shortcode_from_prod_id(prod_id)
    if shortcode == "":
        return temp_ilist
    temp_ilist = "MODELINIT DEPBASE %s MktSizeWPrice MktSizeWPrice\n" \
                 "MODELMATH LINEAR CHANGE\n" \
                 "INDICATORSTART\n" % shortcode
    for ind_id in ind_ids:
        indicator = get_indicator_from_ind_id(ind_id)
        temp_ilist += "INDICATOR 1.00 " + indicator + "\n"
    temp_ilist += "INDICATOREND"
    return temp_ilist


def check_stats_exists_for_date(prod_id, date):
    """
    Check if all stats exist for all indicators for a prod_id for a date
    :param prod_id: (int)
    :param date: (int)
    :return: (bool)
    """
    all_ind_ids_for_product = get_all_ind_ids_for_prod_id(prod_id)
    mean_cov_str = fetch_mean_cov(prod_id, date)
    if mean_cov_str is None:
        return False
    ind_mean_cov_lines = mean_cov_str.splitlines()
    pnl_stats_ind_ids = list(map(int, ind_mean_cov_lines[0].split(" ")))
    if sorted(all_ind_ids_for_product) != sorted(pnl_stats_ind_ids):
        return False
    num_rows = int(ind_mean_cov_lines[1])
    if num_rows == -1:
        return False
    self_term_mean = np.array(list(map(float, ind_mean_cov_lines[2].split(" "))))
    cross_term_mat = np.array(list(map(lambda x: np.pad(list(map(float, x.split(" "))),
                                                        pad_width=(0, len(ind_mean_cov_lines[3:]) - len(x.split(" "))), \
                                                        mode="constant", constant_values=(0, 0)),
                                       ind_mean_cov_lines[3:])))
    cross_term_mat = cross_term_mat + cross_term_mat.T - np.diag(cross_term_mat.diagonal())

    ## IF ANY of the stats are -1 return False
    if (self_term_mean == -1).any() or (cross_term_mat == -1).any():
        return False

    return True


def generate_stats_for_product(shortcode, start_time, end_time, start_date=None, end_date=None,
                               dates=None, overwrite=False):
    """
    Generate stats for a product given start date and end date or a list of dates
    :param shortcode: 
    :param start_time: 
    :param end_time: 
    :param start_date: 
    :param end_date: 
    :param dates: 
    :param overwrite: 
    :return: 
    """
    if start_date is None and end_date is None and dates is None:
        return
    if dates is None and \
            ((start_date is None and end_date is not None) or (start_date is not None and end_date is None)):
        return
    if dates is None and start_date is not None and end_date is not None:
        dates = get_traindates(min_date=start_date, max_date=end_date)

    shc_id = get_shc_id(shortcode)
    start_time_id = get_tp_id(start_time)
    end_time_id = get_tp_id(end_time)
    prod_id = get_prod_id(shc_id, start_time_id, end_time_id)
    if prod_id == -1:
        print("Product not added. Please add product")
        return
    generate_stats_for_prod_id_date(prod_id, dates, overwrite)


def generate_stats_for_prod_id_date(prod_id, dates, overwrite):
    """

    :param prod_id: 
    :param dates: 
    :param overwrite: 
    :return: 
    """
    dates_to_generate = []
    for date in dates:
        if not overwrite and check_stats_exists_for_date(prod_id, date):
            continue
        dates_to_generate.append(date)

    all_ind_ids_for_product = get_all_ind_ids_for_prod_id(prod_id)

    work_dir = "/media/shared/ephemeral20/pnl_stats/" + str(int(time.time() * 1000))
    if not os.path.exists(work_dir):
        os.makedirs(work_dir)
    else:
        shutil.rmtree(work_dir)
        os.makedirs(work_dir)
    print(work_dir)

    temp_ilist = create_temp_ilist_for_product(all_ind_ids_for_product, prod_id)
    temp_ilist_filename = os.path.join(work_dir, "temp_ilist_filename")
    file_handle = open(temp_ilist_filename, 'w')
    file_handle.write(temp_ilist)
    file_handle.close()

    output_directory = generate_pnl_stats(temp_ilist_filename, prod_id, dates_to_generate)
    datagen_artifacts_directory = os.path.join(output_directory, "artifacts", "datagen")
    for date in dates:
        date_file = os.path.join(datagen_artifacts_directory, str(date) + ".txt")
        if os.path.exists(date_file):
            read_day_output(prod_id, temp_ilist_filename, date, date_file)


def generate_pnl_stats(ilist, prod_id, dates):
    """
    Calls grid for a ilist file, prod_id and list of dates and returns the output directory
    :param ilist: (str)
    :param prod_id: (int)
    :param dates: (list(int))
    :return: (str)
    """
    grid_datagen_json = {'job': "generate_data", 'ilist': ilist, 'msecs': '4000', 'l1events': 'c3', 'trades': '0',
                         'eco_mode': '0', 'start_time': get_start_time_from_prod_id(prod_id),
                         'end_time': get_end_time_from_prod_id(prod_id), 'stats_args': 'PNL_BASED_STATS', 'dates': []}
    for date in dates:
        grid_datagen_json['dates'].append(str(date))

    print(grid_datagen_json)
    work_dir = os.path.dirname(ilist)

    temp_json_file = os.path.join(work_dir, "temp_json_file")
    file_handle = open(temp_json_file, 'w')
    file_handle.write(json.dumps(grid_datagen_json, sort_keys=True, separators=(',', ':'), indent=2))
    file_handle.close()

    grid_client = GridClient(server_url=GRID_URL, username=os.getenv("GRID_USERNAME"),
                             password=os.getenv("GRID_PASSWORD"), grid_artifacts_dir=work_dir)
    output_directory = grid_client.submit_job(json.dumps(grid_datagen_json))
    return output_directory


def fetch_mean_cov(prod_id, date):
    sql_query = "select std_list_n_cov_mat from pnl_based_stats where prod_id = {} and date = {}".format(prod_id, date)
    #print(sql_query)
    try:
        cursor.execute(sql_query)
    except MySQLdb.Error as e:
        print("Could not execute commmand %s" % sql_query, e)
    data = cursor.fetchall()
    if len(data) == 0:
        return None
    data = sql_out_to_str(data)
    return data[0][0]


def fetch_pnl_stats_for_ilist_dates(shortcode, start_time, end_time, ilist, start_date=None, end_date=None, dates=None):
    """

    :param shortcode:
    :param start_time:
    :param end_time:
    :param ilist:
    :param start_date:
    :param end_date:
    :param dates:
    """
    if start_date is None and end_date is None and dates is None:
        return None
    if dates is None and \
            ((start_date is None and end_date is not None) or (start_date is not None and end_date is None)):
        return None
    if dates is None and start_date is not None and end_date is not None:
        dates = get_traindates(min_date=start_date, max_date=end_date)
    shc_id = get_shc_id(shortcode)
    if shc_id == -1:
        print("Shortcode not added. Add Product")
        return None

    start_time_id = get_tp_id(start_time)
    end_time_id = get_tp_id(end_time)
    if start_time_id == -1 or end_time_id == -1:
        print("Start/End time not added. Add Product")
        return None

    prod_id = get_prod_id(shc_id, start_time_id, end_time_id)
    if prod_id == -1:
        print("Product not added.")
        return None

    indicators, _ = read_master_ilist(ilist)
    ind_ids = get_indicator_ids(indicators)
    mean_vec = np.zeros(len(indicators))
    stdev_vec = np.zeros(len(indicators))
    cross_mat = np.zeros((len(indicators), len(indicators)))
    cov_mat = np.zeros((len(indicators), len(indicators)))
    sum_instances = 0

    num_dates = 0
    for date in dates:
        # ind_mean_cov_lines = fetch_mean_cov(prod_id, date).splitlines()
        # if ind_mean_cov_lines is None:
        mean_cov_str = fetch_mean_cov(prod_id, date)
        if mean_cov_str is None:
            continue
        ind_mean_cov_lines = mean_cov_str.splitlines()
        num_dates += 1
        pnl_stats_ind_ids = list(map(int, ind_mean_cov_lines[0].split(" ")))
        if len(set(ind_ids) - set(pnl_stats_ind_ids)) != 0:
            return None
        ## get these indicator indices in the same order as in the indicators list
        indid_idx = [pnl_stats_ind_ids.index(ind_id) for ind_id in ind_ids]
        num_rows = int(ind_mean_cov_lines[1])
        self_term_mean = np.array(list(map(float, ind_mean_cov_lines[2].split(" "))))
        self_term_mean_sel = self_term_mean[indid_idx]
        cross_term_mat = np.array(list(map(lambda x: np.pad(list(map(float, x.split(" "))),
                                                            pad_width=(0, len(ind_mean_cov_lines[3:]) - len(x.split(" "))),\
                                                            mode="constant", constant_values=(0, 0)),
                                           ind_mean_cov_lines[3:])))
        cross_term_mat = cross_term_mat + cross_term_mat.T - np.diag(cross_term_mat.diagonal())
        ix_grid = np.ix_(indid_idx, indid_idx)
        cross_term_mat_sel = cross_term_mat[ix_grid]
        a = 1.0*num_rows/(num_rows+sum_instances)
        mean_vec  = a*self_term_mean_sel + (1.0 - a)*mean_vec
        cross_mat = a*cross_term_mat_sel + (1.0 - a)*cross_mat
        sum_instances+=num_rows

    for i in range(len(indicators)):
        for j in range(len(indicators)):
            cov_mat[i][j] = cross_mat[i][j] - mean_vec[i]*mean_vec[j]

    stdev_vec = cov_mat.diagonal()

    pnl_stats = ""
    pnl_stats += "STDEV " + " ".join(map(str, stdev_vec))
    pnl_stats += "\nCOVARIANCE MATRIX\n"

    # print("STDEV", " ".join(stdev_vec))
    # print("COVARIANCE_MATRIX")
    for i in range(len(indicators)):
        # print(" ".join(map(str, cov_mat[i])))
        pnl_stats += " ".join(map(str, cov_mat[i]))
        pnl_stats += "\n"

    if num_dates >= int(0.7 * len(dates)):
        return pnl_stats
    else:
        return None
