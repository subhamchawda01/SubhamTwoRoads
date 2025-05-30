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

sys.path.append(os.path.expanduser('~/basetrade/'))

import MySQLdb
from walkforward.wf_db_utils.db_handles import connection
from walkforward.utils.sql_str_converter import sql_out_to_str

import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText



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
    #print (fetch_query)
    cursor = connection().cursor()
    cursor.execute(fetch_query)
    data = cursor.fetchall()
    data = sql_out_to_str(data)
    configid_ =  (data[0][0])
    
    fetch_query = "select computed_oml from PickstratRecords where config_id =  " + str(configid_) + " ORDER BY date DESC limit 1;"
    #print (fetch_query)
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
    
    ret = [str(shortcode_), str(folder_time_), str(session_), str(computed_oml_), str(uts_), str(startid_), str(endid_)]
    return ret



def get_sim_history_(details_, date_, history_):
    sim_history_stats_ = []

    for product_ in range(len(details_)):
        med_array_ = []
        med_array_.append(details_[product_][0])
        uts_to_scale_ = int(details_[product_][4])
        shortcode_   =  (details_[product_][0])
        folder_time_ =  (details_[product_][1])
        summarize_script_ = home_directory_ + "/basetrade_install/bin/summarize_strategy_results"
        wf_strat_location_ = home_directory_ + "/modelling/wf_strats/" + shortcode_ + "/" + folder_time_ + "/"

        
        for hist_ in range(len(history_)):
            script_ = home_directory_ + "/cvquant_install/basetrade/bin/calc_prev_week_day"
            cmd1_ = script_ + " " + date_ + " " + history_[hist_]
            proc = subprocess.Popen(cmd1_, stdout = subprocess.PIPE, stderr = subprocess.PIPE,shell=True)
            out, err = proc.communicate()
            out = out.decode('utf-8')
            startdate_ = out.strip()
  
            shortcode_   =  (details_[product_][0])
            folder_time_ =  (details_[product_][1])
            cmd_ = summarize_script_ + " " + shortcode_ + " " + wf_strat_location_ + " DB " + startdate_ + " " + date_ + " IF kCNAPnlSharpeAverage 0 IF 0 | grep STATIS | awk '{print $2}'"
            proc = subprocess.Popen(cmd_, stdout = subprocess.PIPE, stderr = subprocess.PIPE,shell=True)
            out, err = proc.communicate()
            out = out.decode('utf-8')
            out = out.strip()
            arrayout_ = out.split('\n')
            arrayout_ = list(map(int, arrayout_))
            if(hist_ == 0):
                cmd_ = summarize_script_ + " " + shortcode_ + " " + wf_strat_location_ + " DB " + startdate_ + " " + date_ + " IF kCNAPnlSharpeAverage 0 IF 0 | grep STATIS | awk '{print $27}' | head -n1"
                proc = subprocess.Popen(cmd_, stdout = subprocess.PIPE, stderr = subprocess.PIPE,shell=True)
                out, err = proc.communicate()
                out = out.decode('utf-8')
                out = out.strip()
                this_pool_uts_ = float(out)

            med_array_.append(str(int(statistics.median(arrayout_) * uts_to_scale_ /  this_pool_uts_)))
        sim_history_stats_.append(med_array_)
    return sim_history_stats_

def get_lastday_simreal_(details_, date_):
    YYYY_ = date_[0:4]
    MM_ = date_[4:6]
    DD_ = date_[6:8]
    simreal_matrix_ = []

    for product_ in range(len(details_)):
                
        startid_ = int(details_[product_][5]  )
        endid_ = int(details_[product_][6])
        simtotalpnl_  = 0
        realtotalpnl_  = 0
        simtotalvol_  = 0
        realtotalvol_  = 0
        for i in range(startid_ , endid_+1):
            logfile_ = "/NAS1/logs/QueryLogs/" + str(YYYY_) + "/" + str(MM_) + "/" + str(DD_) + "/log." + date_ + "." + str(i) + ".gz"
            tradefile_ = "/NAS1/logs/QueryTrades/" + str(YYYY_) + "/" + str(MM_) + "/" + str(DD_) + "/trades." + date_ + "." + str(i)
            if(os.path.exists(tradefile_)):
                statinfo = os.stat(tradefile_)
                if(statinfo.st_size > 0 ):
                    #print (logfile_)
                    #print (tradefile_)
                    cmd = "env NO_PLOT_SIMREAL=0 " + home_directory_ + "/basetrade/scripts/run_accurate_sim_real.pl " + date_ + " " +   str(i)  + " " + "| grep -e \" REALRESULT \" -e \" SIMRESULT \" "
                    proc = subprocess.Popen(cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE,shell=True)
                    out, err = proc.communicate()
                    out = out.decode('utf-8')
                    out.rstrip('\n')
                    this_array_ = out.split('\n')
                    for j in range(len(this_array_) -1 ):
                        temp_ = (this_array_[j]).split(' ')
                        if (temp_[1] == "REALRESULT"):
                            realtotalpnl_ += int(temp_[2])
                            realtotalvol_ += int(temp_[3])
                        if (temp_[1] == "SIMRESULT"):
                            simtotalpnl_ += int(temp_[2])
                            simtotalvol_ += int(temp_[3])
                    #print (this_array_)
        simreal_matrix_.append([str(details_[product_][0]), str(int(simtotalpnl_)), str(int(realtotalpnl_)), str(int(simtotalvol_)), str(int(realtotalvol_))])
    return simreal_matrix_

def get_real_history_(details_, date_, history_):
    real_history_stats_ = []
    for product_ in range(len(details_)):
        avg_array_ = []
        avg_array_.append(details_[product_][0])
        for hist_ in range(len(history_)):
            shortcode_ = details_[product_][0]
            session_ = details_[product_][2]

            script_ = home_directory_ + "/cvquant_install/basetrade/bin/calc_prev_week_day"
            cmd1_ = script_ + " " + date_ + " " + history_[hist_]
            proc = subprocess.Popen(cmd1_, stdout = subprocess.PIPE, stderr = subprocess.PIPE,shell=True)
            out, err = proc.communicate()
            out = out.decode('utf-8')
            startdate_ = out.strip()
            enddate_ = date_


            pnl_average_ = 0
            vol_average_ = 0
            days_ = 0
            #fetch_query = "select date,pnl,volume from RealPNLS where shortcode = '" + shortcode_ + "' and session = '" + session_ + "' and volume != 'NULL' and date >= " + str(startdate_) +  " and date <= " + str(enddate_) + ";"
            fetch_query = "select pnl,volume from RealPNLS where shortcode = '" + shortcode_ + "' and session = '" + session_ + "' and date >= " + str(startdate_) +  " and date <= " + str(enddate_) + ";"
            #print (fetch_query)
            cursor = connection().cursor()
            cursor.execute(fetch_query)
            data = cursor.fetchall()
            data = sql_out_to_str(data)
            for i in range(len(data)):
                if( str(data[i][0]) != "None" ):
                    pnl_average_ += int(data[i][0])
                    vol_average_ += int(data[i][1])
                    days_ += 1
            if(days_ > 0):
                pnl_average_ = pnl_average_ / (days_ * 1.0)
                vol_average_ = vol_average_ / (days_ * 1.0)
            avg_array_.append(str(int(pnl_average_)))
        real_history_stats_.append(avg_array_)
    return (real_history_stats_)


def read_product_configs_(config_map_):
    matrix_ = []
    for i in range(len(config_map_)):
        temp_arr_ = read_a_config_(config_map_[i][1])
        matrix_.append(temp_arr_)
    return matrix_


def make_mail_matrix_(shistory_, rhistory_, details_, lastday_simreal_, simhistory_, realhistory_, product_session_):
    #header_ = [" ","L_SPNL", "L_RPNL", "L_SVOL", "L_RVOL", "M_LOSS"]
    header_ = [" "," Last Day Sim Pnl ", " Last Day Real Pnl ", " Last Day Sim Vol ", " Last Day Real Vol ", " Optimal Maxloss "]
    for i in range(len(shistory_)):
        string_ = "" +  shistory_[i] + " days expectation from pool"
        header_.append(string_)
    for i in range(len(rhistory_)):
        string_ = "" +  rhistory_[i] + " days Real Pnl "
        header_.append(string_)
    mail_array_ = [header_]
    
    for product_ in range(len(details_)):
        temp_array_ = [str(product_session_[product_])]
        temp_array_.extend(lastday_simreal_[product_][1:])
        temp_array_.append(details_[product_][3])
        temp_array_.extend(simhistory_[product_][1:])
        temp_array_.extend(realhistory_[product_][1:])
        mail_array_.append(temp_array_)

    total_array_ = ["TOTAL"]
    for i in range(1, len(mail_array_[0]), 1):
        temp = 0
        for j in range(1, len(mail_array_), 1):
            temp += int(mail_array_[j][i])
        total_array_.append(str(temp))

    mail_array_.append(total_array_)
    return mail_array_


def print_systemetic_(mail_array_):
    for product_ in range(len(mail_array_)):
        for i in range(len(mail_array_[product_])):
            sys.stdout.write("%10s " % mail_array_[product_][i])
            #sys.stdout.write(mail_array_[product_][i] + "\t")
        print("")


def send_mail_(matrix_, portfolio_, mailsender_, mailreceiver_):
    me = mailsender_
    you = ""
    for i in range(len(mailreceiver_)):
        you = you + mailreceiver_[i]
        if(i != len(mailreceiver_) - 1):
            you = you + ","
    msg = MIMEMultipart('alternative')
    msg['Subject'] = portfolio_
    msg['From'] = me
    msg['To'] = you

    text = "Table\n"
    part1 = MIMEText(text, 'plain')
    msg.attach(part1)
    df = pd.DataFrame(matrix_)
    s = df.style.apply(highlight_last_row)

    

    mail_body = ""
    mail_body += "<p> Portfolio performance summary <br>"

    mail_body += "<table border = \"2\">" 
    mail_body +=  "\n";
    for k in range(len(matrix_)):
        mail_body +=  "<tr>\n"
        for j in range(len(matrix_[k])):
            if(k == 0 or k == (len(matrix_) - 1) ):
                mail_body += "<td align=\"center\"><font font-weight = \"bold\" size = \"4\" color=\"Red\">" + matrix_[k][j] + "</font></td>\n"
            else:
                mail_body += "<td align=\"center\"><font font-weight = \"bold\" size = \"3\" color=\"darkblue\">" + matrix_[k][j] + "</font></td>\n"
        mail_body += "</tr>\n"
    mail_body += "</table>\n"

    mail_body += "</p>"
    #print (mail_body)
    msg.attach(MIMEText(mail_body, 'html'))

    #html_ = df.to_html()
    #msg.attach(mail_body)
    s = smtplib.SMTP('localhost')
    s.sendmail(me, you, msg.as_string())
    s.quit()

def highlight_last_row(s):
    return ['background-color: #FF0000' if i==0 else '' for i in range(len(s))]


def read_params_from_config(config_file, date_):

    current_instruction = None
    portfolio_ = ""
    config_map_ = []
    mailsender_ = ""
    mailreceiver_ = []
    history_ = []
    product_session_ = []

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
                    if current_instruction == "PRODUCTS":
                        words = line.split(' ')
                        words1 = [words[0], words[1]]
                        config_map_.append(words1)
                        product_session_.append(words[0])
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
    
    #matrix_ = [[1,1,1,1,1],[2,2,2,2,2],[3,3,3,3,3]]
    details_ = read_product_configs_(config_map_)
    #print (details_)
    
    lastday_simreal_ = get_lastday_simreal_(details_, date_)
    #print (lastday_simreal_)

    simhistory_ = get_sim_history_(details_, date_, shistory_)
    #print (simhistory_)

    realhistory_ = get_real_history_(details_, date_, rhistory_)
    #print (realhistory_)

    mail_matrix_ = make_mail_matrix_(shistory_, rhistory_, details_, lastday_simreal_, simhistory_, realhistory_, product_session_)
    print_systemetic_(mail_matrix_)
    portfolio_ = portfolio_ + " portfolio report " + date_
    send_mail_(mail_matrix_, portfolio_, mailsender_, mailreceiver_)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', dest='configpath', help="config defining the portfolio", type=str, required=True)
    parser.add_argument('-d', dest='date', help="date in YYYYMMDD", type=str, required=True)
    args = parser.parse_args()
    read_params_from_config(args.configpath, args.date)



