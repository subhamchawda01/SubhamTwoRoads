#!/usr/bin/env python
"""
Script to generate statistics for a portfolio
"""

import os
import sys
import subprocess
import argparse
import statistics
import pandas as pd
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText


sys.path.append(os.path.expanduser('~/basetrade/'))
from get_config_n_uts_for_queryid_n_date import get_date_to_config_to_uts_map_for_queries_n_dates
from walkforward.definitions import execs
from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str
from walkforward.utils.get_list_of_dates import get_list_of_dates
from pylib.pnl_modelling_utils.load_config_date_pnl_map import get_config_uts_n_date_pnl


home_directory_ = os.getenv("HOME")
home_directory_ = home_directory_.strip()


def read_a_config_(config_):
    shortcode_ = ""
    folder_time_ = ""
    session_ = config_.split('.')[1]
    maxloss_ = 0
    uts_ = 0
    startid_ = 0
    endid_ = 0
    current_instruction = None
    pick_strat_config_name_ = config_.split('/')[-1]

    fetch_query = "select config_id from PickstratConfig where config_name = '" + pick_strat_config_name_ + "';"

    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    configid_ = (data[0][0])

    fetch_query = "select computed_oml from PickstratRecords where config_id =  " + str(
        configid_) + " ORDER BY date DESC limit 1;"

    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    computed_oml_ = (data[0][0])

    try:
        with open(config_, 'r') as cfile:
            for line in cfile:
                if not line.strip():
                    current_instruction = None
                    continue

                if line[0] == '#':
                    continue
                line = line.strip()
                if current_instruction is None:
                    current_instruction = line
                else:
                    if current_instruction == "SHORTCODE":
                        shortcode_ = line
                    if current_instruction == "GLOBAL_MAX_LOSS":
                        maxloss_ = int(line)
                    if current_instruction == "TOTAL_SIZE_TO_RUN":
                        uts_ += int(line)
                    if current_instruction == "FOLDER_TIME_PERIODS":
                        folder_time_ = line
                    if current_instruction == "PROD_QUERY_START_ID":
                        startid_ = line
                    if current_instruction == "PROD_QUERY_STOP_ID":
                        endid_ = line
    except:
        print(config_ + " not readable")
        raise ValueError(config_ + " not readable.")

    config_dict = {"shc": str(shortcode_), "folder_time": str(folder_time_), "session": str(session_),
                   "computed_oml": str(computed_oml_),
                   "uts": str(uts_), "query_start_id": str(startid_), "query_end_id": str(endid_)}
    return config_dict


def get_lastday_simreal_(product_to_config_dict_, date_):
    YYYY_ = date_[0:4]
    MM_ = date_[4:6]
    DD_ = date_[6:8]
    simreal_map_ = {}

    for product_ in product_to_config_dict_:
        config_dict_ = product_to_config_dict_[product_]
        startid_ = int(config_dict_["query_start_id"])
        endid_ = int(config_dict_["query_end_id"])
        simtotalpnl_ = 0
        realtotalpnl_ = 0
        simtotalvol_ = 0
        realtotalvol_ = 0
        for query in range(startid_, endid_ + 1):
            logfile_ = "/NAS1/logs/QueryLogs/" + str(YYYY_) + "/" + str(MM_) + \
                "/" + str(DD_) + "/log." + date_ + "." + str(query) + ".gz"
            tradefile_ = "/NAS1/logs/QueryTrades/" + \
                str(YYYY_) + "/" + str(MM_) + "/" + str(DD_) + "/trades." + date_ + "." + str(query)
            if(os.path.exists(tradefile_)):
                statinfo = os.stat(tradefile_)
                if(statinfo.st_size > 0):

                    cmd = "env NO_PLOT_SIMREAL=0 " + execs.execs().run_accurate_sim_real + " " + date_ + " " + str(query) + \
                        " " + "| grep -e \" REALRESULT \" -e \" SIMRESULT \" "
                    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
                    out, err = proc.communicate()
                    out = out.decode('utf-8')
                    out.rstrip('\n')
                    this_array_ = out.split('\n')
                    for j in range(len(this_array_) - 1):
                        temp_ = (this_array_[j]).split(' ')
                        if (temp_[1] == "REALRESULT"):
                            realtotalpnl_ += int(temp_[2])
                            realtotalvol_ += int(temp_[3])
                        if (temp_[1] == "SIMRESULT"):
                            simtotalpnl_ += int(temp_[2])
                            simtotalvol_ += int(temp_[3])

        simreal_map_[product_] = [str(simtotalpnl_), str(realtotalpnl_), str(simtotalvol_), str(realtotalvol_)]
    return simreal_map_


def get_dated_utsscaled_sim_pnls(shc_, query_start_id_, query_end_id_, end_date_, num_days_):
    dates_list = get_list_of_dates(shc_, num_days_, end_date_)
    dates_list.sort()
    start_date = dates_list[0]

    date_to_config_to_maps = get_date_to_config_to_uts_map_for_queries_n_dates(query_start_id_, query_end_id_,
                                                                               dates_list)
    date_to_config_to_uts_map = date_to_config_to_maps[0]
    date_to_config_to_pnl_map = date_to_config_to_maps[1]

    folder_times_N = os.path.join(execs.execs().modelling_dvc, "wf_strats", shc_)
    folder_times_S = os.path.join(execs.execs().modelling_dvc, "wf_staged_strats", shc_)

    config_uts_n_date_pnl_map = get_config_uts_n_date_pnl(shc_, folder_times_N, start_date, end_date_, " ")

    config_uts_n_date_pnl_map_S = get_config_uts_n_date_pnl(shc_, folder_times_S, start_date, end_date_, " ")

    config_uts_n_date_pnl_map.update(config_uts_n_date_pnl_map_S)
    date_pnl_map = {}
    for dt in dates_list:
        if dt in date_to_config_to_uts_map:
            for real_config in date_to_config_to_uts_map[dt]:
                if real_config in config_uts_n_date_pnl_map:
                    if dt in config_uts_n_date_pnl_map[real_config]:
                        sim_uts = config_uts_n_date_pnl_map[real_config]["sim_uts"]
                        real_uts = float(date_to_config_to_uts_map[dt][real_config])
                        real_pnl = int(date_to_config_to_pnl_map[dt][real_config])
                        if real_pnl != 0:
                            if dt not in date_pnl_map:
                                date_pnl_map[dt] = real_pnl
                            else:
                                date_pnl_map[dt] += real_pnl
                            continue
                        uts_scaling_param = real_uts / sim_uts
                        uts_scaled_pnl = uts_scaling_param * config_uts_n_date_pnl_map[real_config][dt]
                        if dt not in date_pnl_map:
                            date_pnl_map[dt] = uts_scaled_pnl
                        else:
                            date_pnl_map[dt] += uts_scaled_pnl
                    else:
                        real_uts = float(date_to_config_to_uts_map[dt][real_config])
                        real_pnl = int(date_to_config_to_pnl_map[dt][real_config])
                        if real_pnl != 0:
                            if dt not in date_pnl_map:
                                date_pnl_map[dt] = real_pnl
                            else:
                                date_pnl_map[dt] += real_pnl
                else:
                    real_uts = float(date_to_config_to_uts_map[dt][real_config])
                    real_pnl = int(date_to_config_to_pnl_map[dt][real_config])
                    if real_pnl != 0:
                        if dt not in date_pnl_map:
                            date_pnl_map[dt] = real_pnl
                        else:
                            date_pnl_map[dt] += real_pnl
    return date_pnl_map


def get_accurate_sim_history_(product_to_config_dict_, date_, history_):
    """

    :param product_to_config_dict_: 
    :param date_: 
    :param history_: 
    :return: 
    """
    sim_history_stats_ = {}
    for product in product_to_config_dict_:
        config_dict = product_to_config_dict_[product]
        shc = config_dict["shc"]
        query_start_id = config_dict["query_start_id"]
        query_end_id = config_dict["query_end_id"]
        num_days = max(list(map(int, history_)))

        date_pnl_list = get_dated_utsscaled_sim_pnls(shc, query_start_id, query_end_id, date_, num_days)

        sim_avg_array_ = []
        pnl_average = 0
        for hist_ in history_:
            dates_list_hist = get_list_of_dates(shc, hist_, date_)
            hist_pnl = 0
            day_count = 0
            for dt in dates_list_hist:
                if dt in date_pnl_list:
                    hist_pnl += date_pnl_list[dt]
                    day_count += 1
            if day_count > 0:
                pnl_average = int(float(hist_pnl) / day_count)
            if day_count < 0.7 * len(dates_list_hist):
                sim_avg_array_.append(str(pnl_average) + "(" + str(day_count) + ")")
            else:
                sim_avg_array_.append(str(pnl_average))
            sim_history_stats_[product] = sim_avg_array_

    return sim_history_stats_


def get_real_history_(product_to_config_dict_, date_, history_):
    """

    :param product_to_config_dict_: 
    :param date_: 
    :param history_: 
    :return: 
    """
    real_history_stats_ = {}
    for product_ in product_to_config_dict_:
        config_dict_ = product_to_config_dict_[product_]
        shortcode_ = config_dict_["shc"]
        session_ = config_dict_["session"]
        avg_array_ = []
        for hist_ in history_:
            dates_list_hist = get_list_of_dates(shortcode_, hist_, date_)
            startdate_ = dates_list_hist[-1]
            enddate_ = dates_list_hist[0]

            pnl_average_ = 0
            vol_average_ = 0
            days_ = 0

            fetch_query = "select pnl,date from RealPNLS where shortcode = '" + shortcode_ + "' and session = '" + \
                          session_ + "' and date >= " + str(startdate_) + " and date <= " + str(enddate_) + \
                          " and pnl is not NULL;"
            cursor = connection().cursor()
            cursor.execute(fetch_query)
            data = cursor.fetchall()
            data = sql_out_to_str(data)

            for rows in data:
                pnl = rows[0]
                date = rows[1]
                if (pnl != None):
                    pnl_average_ += int(pnl)
                    days_ += 1
            if (days_ > 0):
                pnl_average_ = int(float(pnl_average_) / days_)
            if (days_ < 0.7 * int(hist_)):
                avg_array_.append(str(pnl_average_) + "(" + str(days_) + ")")
            else:
                avg_array_.append(str(pnl_average_))
        real_history_stats_[product_] = avg_array_

    return (real_history_stats_)


def read_product_configs_(product_to_config_map):
    """

    :param product_to_config_map: 
    :return: 
    """
    product_to_config_dict_ = {}
    for product_ in product_to_config_map.keys():
        config = product_to_config_map[product_]
        config_dict_temp = read_a_config_(config)
        product_to_config_dict_[product_] = config_dict_temp

    return product_to_config_dict_


def make_mail_matrix_(shistory_, rhistory_, product_to_config_dict_, lastday_simreal_, simhistory_, realhistory_, product_session_, tag_):
    """

    :param shistory_: 
    :param rhistory_: 
    :param product_to_config_dict_: 
    :param lastday_simreal_: 
    :param simhistory_: 
    :param realhistory_: 
    :return: 
    """
    # header_ = [" ","L_SPNL", "L_RPNL", "L_SVOL", "L_RVOL", "M_LOSS"]
    header_ = [tag_, " Last Day Sim Pnl ", " Last Day Real Pnl ", " Last Day Sim Vol ", " Last Day Real Vol ",
               " Optimal Maxloss "]
    for sim_hist in shistory_:
        string_ = "" + sim_hist + " days expectation from pool"
        header_.append(string_)
    for real_hist in rhistory_:
        string_ = "" + real_hist + " days Real Pnl "
        header_.append(string_)
    mail_array_ = [header_]

    for product_ in product_session_:
        config_dict_ = product_to_config_dict_[product_]

        temp_array_ = [product_]
        temp_array_.extend(lastday_simreal_[product_])
        temp_array_.append(config_dict_["computed_oml"])
        temp_array_.extend(simhistory_[product_])
        temp_array_.extend(realhistory_[product_])
        mail_array_.append(temp_array_)

    print(mail_array_)
    total_array_ = ["TOTAL"]
    for i in range(1, len(mail_array_[0]), 1):
        temp = 0
        for j in range(1, len(mail_array_), 1):
            if "(" in mail_array_[j][i]:
                avg_by_removing_days_in_bracket = mail_array_[j][i].strip().split("(")[0]
                temp += int(avg_by_removing_days_in_bracket)
            else:
                temp += int(mail_array_[j][i])
        total_array_.append(str(temp))

    mail_array_.append(total_array_)
    return mail_array_


def print_systemetic_(mail_array_):
    """

    :param mail_array_: 
    :return: 
    """
    for product_ in range(len(mail_array_)):
        for i in range(len(mail_array_[product_])):
            sys.stdout.write("%10s " % mail_array_[product_][i])
            # sys.stdout.write(mail_array_[product_][i] + "\t")
        print("")


def send_mail_combined_(matrix1_, matrix2_, matrix3_, portfolio_, mailsender_, mailreceiver_):
    """

        :param matrix_: 
        :param portfolio_: 
        :param mailsender_: 
        :param mailreceiver_: 
        :return: 
        """
    me = mailsender_
    you = ",".join(mailreceiver_)

    msg = MIMEMultipart('alternative')
    msg['Subject'] = portfolio_
    msg['From'] = me
    msg['To'] = you

    text = "Table\n"
    part1 = MIMEText(text, 'plain')
    msg.attach(part1)

    mail_body = ""

    mail_body += "<p> <font font-weight = \"bold\" size = \"4\" color=\"Black\"> Session wise performance summary <br>"

    if(len(matrix1_) > 2):
        mail_body += "<table border = \"2\">"
        mail_body += "\n"
        for k in range(len(matrix1_)):
            mail_body += "<tr>\n"
            for j in range(len(matrix1_[k])):
                if(k == 0 or k == (len(matrix1_) - 1)):
                    mail_body += "<td align=\"center\"><font font-weight = \"bold\" size = \"4\" color=\"Black\">" + \
                        matrix1_[k][j] + "</font></td>\n"
                else:
                    mail_body += "<td align=\"center\"><font font-weight = \"bold\" size = \"3\" color=\"darkblue\">" + \
                        matrix1_[k][j] + "</font></td>\n"
            mail_body += "</tr>\n"
        mail_body += "</table>\n"
        mail_body += "</p>"
        mail_body += "<p> <br>"

    if(len(matrix2_) > 2):
        mail_body += "<table border = \"2\">"
        mail_body += "\n"
        for k in range(0, len(matrix2_)):
            mail_body += "<tr>\n"
            for j in range(len(matrix2_[k])):
                if(k == 0 or k == (len(matrix2_) - 1)):
                    mail_body += "<td align=\"center\"><font font-weight = \"bold\" size = \"4\" color=\"Black\">" + \
                        matrix2_[k][j] + "</font></td>\n"
                else:
                    mail_body += "<td align=\"center\"><font font-weight = \"bold\" size = \"3\" color=\"darkblue\">" + \
                        matrix2_[k][j] + "</font></td>\n"
            mail_body += "</tr>\n"
        mail_body += "</table>\n"
        mail_body += "</p>"
        mail_body += "<p>  <br>"

    if(len(matrix3_) > 2):
        mail_body += "<table border = \"2\">"
        mail_body += "\n"
        for k in range(0, len(matrix3_)):
            mail_body += "<tr>\n"
            for j in range(len(matrix3_[k])):
                if(k == 0 or k == (len(matrix3_) - 1)):
                    mail_body += "<td align=\"center\"><font font-weight = \"bold\" size = \"4\" color=\"Black\">" + \
                        matrix3_[k][j] + "</font></td>\n"
                else:
                    mail_body += "<td align=\"center\"><font font-weight = \"bold\" size = \"3\" color=\"darkblue\">" + \
                        matrix3_[k][j] + "</font></td>\n"
            mail_body += "</tr>\n"
        mail_body += "</table>\n"
        mail_body += "</p>"

    msg.attach(MIMEText(mail_body, 'html'))

    #html_ = df.to_html()
    # msg.attach(mail_body)
    s = smtplib.SMTP('localhost')
    s.sendmail(me, you, msg.as_string())
    s.quit()


def highlight_last_row(s):
    """

    :param s: string 
    :return: the highlighted string
    """
    return ['background-color: #FF0000' if i == 0 else '' for i in range(len(s))]


def read_params_from_config(config_file, date_):

    current_instruction = None
    portfolio_ = ""
    current_instruction = None
    portfolio_ = ""
    as_product_to_config_map = {}
    eu_product_to_config_map = {}
    us_product_to_config_map = {}
    mailsender_ = ""
    mailreceiver_ = []
    history_ = []

    as_product_session_ = []
    eu_product_session_ = []
    us_product_session_ = []

    try:
        with open(config_file, 'r') as cfile:
            for line in cfile:
                if not line.strip():
                    current_instruction = None
                    continue

                if line[0] == '#':
                    continue
                line = line.strip()
                if current_instruction is None:
                    current_instruction = line
                else:
                    if current_instruction == "PORTFOLIO":
                        portfolio_ = line
                    if current_instruction == "PRODUCTS_AS":
                        words = line.split(' ')
                        product = words[0]
                        pick_strats_config_file = words[1]
                        as_product_to_config_map[product] = pick_strats_config_file
                        as_product_session_.append(words[0])
                    if current_instruction == "PRODUCTS_EU":
                        words = line.split(' ')
                        product = words[0]
                        pick_strats_config_file = words[1]
                        eu_product_to_config_map[product] = pick_strats_config_file
                        eu_product_session_.append(words[0])
                    if current_instruction == "PRODUCTS_US":
                        words = line.split(' ')
                        product = words[0]
                        pick_strats_config_file = words[1]
                        us_product_to_config_map[product] = pick_strats_config_file
                        us_product_session_.append(words[0])
                    if current_instruction == "MAILFROM":
                        mailsender_ = line
                    if current_instruction == "MAILTO":
                        mailreceiver_.append(line)
                    if current_instruction == "REALHISTORY":
                        rhistory_ = line.split(' ')
                    if current_instruction == "SIMHISTORY":
                        shistory_ = line.split(' ')

    except:
        print(config_file + " not readable")
        raise ValueError(config_file + " not readable.")

    as_product_to_config_dict = read_product_configs_(as_product_to_config_map)
    as_lastday_simreal_ = get_lastday_simreal_(as_product_to_config_dict, date_)
    as_simhistory_ = get_accurate_sim_history_(as_product_to_config_dict, date_, shistory_)
    as_realhistory_ = get_real_history_(as_product_to_config_dict, date_, rhistory_)
    as_mail_matrix_ = make_mail_matrix_(shistory_, rhistory_, as_product_to_config_dict,
                                        as_lastday_simreal_, as_simhistory_, as_realhistory_, as_product_session_, "AS")

    eu_product_to_config_dict = read_product_configs_(eu_product_to_config_map)
    eu_lastday_simreal_ = get_lastday_simreal_(eu_product_to_config_dict, date_)
    eu_simhistory_ = get_accurate_sim_history_(eu_product_to_config_dict, date_, shistory_)
    eu_realhistory_ = get_real_history_(eu_product_to_config_dict, date_, rhistory_)
    eu_mail_matrix_ = make_mail_matrix_(shistory_, rhistory_, eu_product_to_config_dict,
                                        eu_lastday_simreal_, eu_simhistory_, eu_realhistory_, eu_product_session_, "EU")

    us_product_to_config_dict = read_product_configs_(us_product_to_config_map)
    us_lastday_simreal_ = get_lastday_simreal_(us_product_to_config_dict, date_)
    us_simhistory_ = get_accurate_sim_history_(us_product_to_config_dict, date_, shistory_)
    us_realhistory_ = get_real_history_(us_product_to_config_dict, date_, rhistory_)
    us_mail_matrix_ = make_mail_matrix_(shistory_, rhistory_, us_product_to_config_dict,
                                        us_lastday_simreal_, us_simhistory_, us_realhistory_, us_product_session_, "US")

    print_systemetic_(as_mail_matrix_)
    print("")
    print_systemetic_(eu_mail_matrix_)
    print("")
    print_systemetic_(us_mail_matrix_)
    print("")

    portfolio_ = portfolio_ + " portfolio report " + date_
    send_mail_combined_(as_mail_matrix_, eu_mail_matrix_, us_mail_matrix_, portfolio_, mailsender_, mailreceiver_)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', dest='configpath', help="config defining the portfolio", type=str, required=True)
    parser.add_argument('-d', dest='date', help="date in YYYYMMDD", type=str, required=True)
    args = parser.parse_args()
    read_params_from_config(args.configpath, args.date)
