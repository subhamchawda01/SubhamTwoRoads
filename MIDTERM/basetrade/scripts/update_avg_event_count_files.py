#!/usr/bin/env python
import sys
import subprocess
import os
import os.path
from datetime import (timedelta, date)
HOME = os.environ['HOME']
LIVE_BIN_DIR = HOME + "/basetrade_install/bin/"
DATA_DIR = "/home/dvctrader/archit/datageninfo/"

#SHORTCODELIST = ['ZT_0','ZF_0']
SHORTCODELIST = ['ZT_0', 'ZF_0', 'ZN_0', 'ZB_0', 'UB_0', '6A_0', '6B_0', '6E_0', '6J_0', '6M_0', '6N_0', '6S_0', 'CL_0', 'GC_0', 'ES_0', 'YM_0', 'NQ_0', 'CGB_0', 'BAX_0', 'BAX_1', 'BAX_2', 'BAX_3', 'BAX_4', 'BAX_5', 'BAX_6', 'SXF_0', 'FGBS_0', 'FGBM_0', 'FGBL_0', 'FGBX_0', 'FESX_0', 'FDAX_0', 'FSMI_0', 'FESB_0', 'FBTP_0', 'FOAT_0', 'FSTB_0', 'FXXP_0', 'BR_DOL_0', 'BR_WDO_0', 'BR_IND_0',
                 'BR_WIN_0', 'DI1F15', 'DI1F16', 'DI1F17', 'DI1F18', 'DI1F19', 'DI1F20', 'DI1N14', 'DI1N15', 'DI1N16', 'DI1N17', 'DI1N18', 'DI1J15', 'DI1J16', 'DI1J17', 'JFFCE_0', 'KFFTI_0', 'LFR_0', 'LFZ_0', 'LFI_0', 'LFI_1', 'LFI_2', 'LFI_3', 'LFI_4', 'LFI_5', 'LFL_0', 'LFL_1', 'LFL_2', 'LFL_3', 'LFL_4', 'LFL_5', 'HHI_0', 'HSI_0', 'MCH_0', 'MHI_0', 'NKM_0', 'NK_0', 'RI_0', 'Si_0', 'USD000UTSTOM']
history = 30


def GetAvgL1EventAndTradeCountPerSecForShortcode(t_shortcode_, t_history):
    date_ = date.today()
    delta_ = timedelta(days=-1)
    sum_l1events_count_ = 0.0
    sum_trade_count_ = 0.0
    count_ = 0
    total_count_ = 0
    while count_ < t_history:
        if(total_count_ > 5 * t_history):
            print("Incomplete history for %s" % (t_shortcode_))
            if(count_ > 1):
                return (sum_l1events_count_ / count_), (sum_trade_count_ / count_)
            else:
                return 0.0, 0.0
        date_ = date_ + delta_
        #l1events_exec_cmd_ = "%s %s %s | grep -e L1 -e TRADE" % (log_exec_, t_shortcode_, date_.isoformat().replace('-',''))
        l1events_exec_cmd_ = "%s SIM %s %s 2>/dev/null | wc -l" % (
            LIVE_BIN_DIR + "mkt_trade_logger", t_shortcode_, date_.isoformat().replace('-', ''))
        l1events_exec_cmd_out_ = subprocess.getstatusoutput(l1events_exec_cmd_)
        trade_exec_cmd_ = "%s SIM %s %s 2>/dev/null | grep OnTradePrint" % (
            LIVE_BIN_DIR + "mkt_trade_logger", t_shortcode_, date_.isoformat().replace('-', ''))
        trade_exec_cmd_out_ = subprocess.getstatusoutput(trade_exec_cmd_)
        if((int(l1events_exec_cmd_out_[0]) == 0) and (int(trade_exec_cmd_out_[0]) == 0)):
            #l1events_logs_ = l1events_exec_cmd_out_[1].strip().split('\n')
            l1events_count_ = int(l1events_exec_cmd_out_[1].strip().split()[0])
            if(l1events_count_ < 10):
                continue
            trade_logs_ = trade_exec_cmd_out_[1].strip().split('\n')
            if(len(trade_logs_) < 10):
                continue
            time_diff_ = (float(trade_logs_[-1].split()[0]) - float(trade_logs_[0].split()[0]))
            if(time_diff_ < 1.0):
                continue
            sum_l1events_count_ += l1events_count_ / time_diff_
            sum_trade_count_ += len(trade_logs_) / time_diff_
            print(t_shortcode_, date_.isoformat().replace('-', ''), sum_l1events_count_, sum_trade_count_)
            count_ += 1
        total_count_ += 1

    return (sum_l1events_count_ / t_history), (sum_trade_count_ / t_history)


def __main__():

    #"USAGE : $0 [shortcode_list_file_]"

    old_file_ = DATA_DIR + "avg_l1events_trade_per_sec"
    old_count_map_ = {}
    if(os.path.isfile(old_file_)):
        OLDFILE = open(old_file_, 'r')
        lines_ = OLDFILE.readlines()
        for line_ in lines_:
            line_ = line_.strip().split()
            try:
                old_count_map_[line_[0]] = [float(line_[1]), float(line_[2])]
            except:
                print("invalid line : %s in %s" % (line_, old_file_))
        OLDFILE.close()

    shortcode_list_ = SHORTCODELIST
    if(len(sys.argv) > 1):
        shortcode_list_file_ = sys.argv[1]
        if(os.path.isfile(shortcode_list_file_)):
            SHORTCODE_FILE_HANDLE = open(shortcode_list_file_, 'r')
            shortcode_list_ = [line_.strip() for line_ in SHORTCODE_FILE_HANDLE.readlines()]
            SHORTCODE_FILE_HANDLE.close()
        else:
            print("shorcode_list_file %s does not exist" % (shortcode_list_file_))
            exit()

    for shortcode_ in shortcode_list_:
        (avg_l1events_, avg_trade_) = GetAvgL1EventAndTradeCountPerSecForShortcode(shortcode_, history)
        orig_avg_l1events_ = 0.0
        orig_avg_trade_ = 0.0
        if(shortcode_ in old_count_map_):
            orig_avg_l1events_ = old_count_map_[shortcode_][0]
            orig_avg_trade_ = old_count_map_[shortcode_][1]

        if(avg_l1events_ <= 0.0):
            avg_l1events_ = orig_avg_l1events_
        if(avg_trade_ <= 0.0):
            avg_trade_ = orig_avg_trade_

        if((avg_l1events_ <= 0.0) or (avg_trade_ <= 0.0)):
            print("WARNING: not updating counts for %s due to unstable values" % (shortcode_))
            continue
        old_count_map_[shortcode_] = [avg_l1events_, avg_trade_]
        #NEWFILE.write("%s %s %s\n" % ( shortcode_, "{0:.6f}".format(avg_l1events_),  "{0:.6f}".format(avg_trade_)))
        print("UPDATE:", shortcode_, "{0:.6f}".format(avg_l1events_),  "{0:.6f}".format(
            avg_trade_), "OLDVALUES:", orig_avg_l1events_, orig_avg_trade_)

    new_file_ = DATA_DIR + "avg_l1events_trade_per_sec_new"
    NEWFILE = open(new_file_, 'w', 1)

    for shortcode_ in old_count_map_:
        avg_l1events_ = old_count_map_[shortcode_][0]
        avg_trade_ = old_count_map_[shortcode_][1]
        NEWFILE.write("%s %s %s\n" % (shortcode_, "{0:.6f}".format(avg_l1events_),  "{0:.6f}".format(avg_trade_)))

    NEWFILE.close()

    exec_cmd_ = "mv %s %s" % (new_file_, old_file_)
    print(exec_cmd_)
    os.system(exec_cmd_)
    exec_cmd_ = "scp %s %s:%s" % (old_file_, "10.23.74.51", DATA_DIR)
    print(exec_cmd_)
    os.system(exec_cmd_)
    exec_cmd_ = "scp %s %s:%s" % (old_file_, "10.23.74.52", DATA_DIR)
    print(exec_cmd_)
    os.system(exec_cmd_)
    exec_cmd_ = "scp %s %s:%s" % (old_file_, "10.23.74.53", DATA_DIR)
    print(exec_cmd_)
    os.system(exec_cmd_)
    exec_cmd_ = "scp %s %s:%s" % (old_file_, "10.23.74.54", DATA_DIR)
    print(exec_cmd_)
    os.system(exec_cmd_)
    exec_cmd_ = "scp %s %s:%s" % (old_file_, "10.23.74.55", DATA_DIR)
    print(exec_cmd_)
    os.system(exec_cmd_)
    exec_cmd_ = "scp %s %s:%s" % (old_file_, "10.23.142.51", DATA_DIR)
    print(exec_cmd_)
    os.system(exec_cmd_)


__main__()
