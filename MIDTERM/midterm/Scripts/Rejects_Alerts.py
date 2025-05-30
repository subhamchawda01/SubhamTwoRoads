#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import sys
import time
import subprocess
import datetime as dt


'''
    Code is dependent on exec logic log file,
    any change in th format of the log file
    would make this script to break
'''

os.environ['LD_LIBRARY_PATH'] = '/opt/glibc-2.14/lib'
rej_reason = {
    0: 'ORSOrderAllowed',
    1: 'ORSRejectSecurityNotFound',
    2: 'ORSRejectMarginCheckFailedOrderSizes',
    3: 'ORSRejectMarginCheckFailedMaxPosition',
    4: 'ORSRejectMarginCheckFailedWorstCasePosition',
    5: 'ExchOrderReject',
    6: 'SendOrderRejectNotMinOrderSizeMultiple',
    7: 'ORSRejectMarginCheckFailedMaxLiveOrders',
    8: 'ORSRejectSelfTradeCheck',
    9: 'ORSRejectThrottleLimitReached',
    10: 'ORSRejectMarketClosed',
    11: 'ORSRejectNewOrdersDisabled',
    12: 'ORSRejectFailedPriceCheck',
    13: 'ORSRejectETIAlgoTaggingFailure',
    14: 'ORSRejectFOKSendFailed',
    15: 'ORSRejectFailedIOCOrder',
    16: 'ORSRejectMarginCheckFailedBidNotionalValue',
    17: 'ORSRejectMarginCheckFailedAskNotionalValue',
    18: 'ORSRejectNSEComplianceFailure',
    19: 'TAPRejectThrottleLimitReached',
    20: 'ORSRejectModifyOrdersDisabled',
    21: 'ORSRejectMarginCheckFailedSumQueryWorstCasePosition',
    22: 'ORSRejectFailedPnlCheck',
    23: 'ORSRejectFailedGrossMarginCheck',
    24: 'ORSRejectFailedNetMarginCheck',
    25: 'ORSRejectICEAlgoTaggingFailure',
    }


channel = 'midterm-order-rejects'
slack_exec = '/home/pengine/prod/live_execs/send_slack_notification'
trade_info_dir = '/spare/local/tradeinfo/NSE_Files/'
exec_logic_logs_dir_ = '/spare/local/logs/alllogs/MediumTerm/'

rejection_dic_ = {}
limit_reached_symbols_ = set([])
securities_under_ban = set([])
sec_uder_physical_ = set([])
YYYYMMDD = dt.datetime.today().strftime('%Y%m%d')

'''
   we have disabled these products permantently,
   filter these symbols alert
'''

disabled_products_ = ['DHFL','RELINFRA','RELCAPITAL']

def BashExec(command):
    child = subprocess.Popen(['bash', '-c', command],stdout=subprocess.PIPE)
    return child.communicate()[0].decode('UTF-8').rstrip()

def UpdateFile(message):
	f = open("/tmp/order_msg_details_"+ YYYYMMDD, "a")
	f.write(message+"\n");
	f.close()

def LoadSecUnderBan(file):
    global securities_under_ban
    try:
        sec_ban_file_stream = open(file, mode = 'r')
        for line in sec_ban_file_stream:
            securities_under_ban.add(line.rstrip())
    except FileNotFoundError:
        message = 'Unable to find sec ban file'
        command = slack_exec + ' ' + channel + ' DATA ' +\
            ' \'' + message + '\''
        BashExec(command)
    except:
        sec_ban_file_stream.close()

def LoadSecUnderPhysical(file):
    global sec_uder_physical_
    try:
        sec_uder_physical_file_stream = open(file, mode = 'r')
        for line in sec_uder_physical_file_stream:
            sec_uder_physical_.add(line.rstrip())
    except FileNotFoundError:
        message = 'Unable to physical settlement products file'
        command = slack_exec + ' ' + channel + ' DATA  \'' \
        + message + '\''
        BashExec(command)
    except EOFError:
        sec_uder_physical_file_stream.close()

def GetRejectionReasonString(rejection_code_):
    if rejection_code_ in rej_reason:
        return rej_reason[rejection_code_]
    return 'Unknown Reason'



def SlackNotifyRejects(exec_name_):
    for symbol in rejection_dic_:
        symbol_status=""
        symbol_split_list = symbol.split('_')
        base_name = symbol.split('_')[1];
        if base_name in disabled_products_:
            continue;
        if base_name in sec_uder_physical_:
            symbol_status = '[ PHYSICAL ]'
        if base_name in securities_under_ban:
            symbol_status += '[ BANNED ]'
        message = "ALERT from Exec %s: %s Rejections for the symbol %s with reason %s" \
        		%(exec_name_,rejection_dic_[symbol][0], (symbol + symbol_status) ,','.join([':'.\
        		join([key,str(val)]) for key,val in rejection_dic_[symbol][1].items()]))
        command = slack_exec + ' ' + channel + ' DATA \'' + message + '\''
        BashExec(command)
        UpdateFile(message)
    for symbol in limit_reached_symbols_:
        message = "ALERT from Exec %s: Max orders limit reached for %s"\
            %(exec_name_,symbol)
        command = slack_exec + ' ' + channel + ' DATA \'' + message + '\''
        BashExec(command)
        UpdateFile(message)

def ProcessORSRejects(file):
    all_rejects_string = BashExec('cat %s | grep \"Rej\"' %(file))
    if all_rejects_string == '' :
        return
    rejects_list = all_rejects_string.split('\n')
    for item in rejects_list:
        item_split_ = item.strip().rstrip().split(' ')
        rej_no = int(item_split_[13].strip())
        #if the reject is throttle, then we don't  send an alert
        if rej_no == 9:
            continue;
        symbol = item_split_[19]
        rejection_reason = GetRejectionReasonString(rej_no)
        rejection_reason += ' '+item_split_[14]+' '+item_split_[15]+' '+item_split_[16]
        if symbol in rejection_dic_:
            rejection_dic_[symbol][0]+=1
            if rejection_reason in rejection_dic_[symbol][1]:
                rejection_dic_[symbol][1][rejection_reason] += 1
            else:
                rejection_dic_[symbol][1][rejection_reason] = 1
        else:
            rejection_dic_[symbol] = [1,{rejection_reason : 1}]

def ProcessMaxOrderLimitReached(file):
    all_rejects_string = BashExec('cat %s | grep \"Max orders limit\"' %(file))
    if all_rejects_string == '' :
        return
    rejects_list = all_rejects_string.rstrip().split('\n')
    for item in rejects_list:
        if item.split(' ')[8].rstrip() != '':
            limit_reached_symbols_.add(item.split(' ')[8].rstrip())

def SendRejectionAlert(log_file, exec_name_, date_):
    while True:
        rejects_file_ = '/tmp/rejects_file.' + exec_name_ + '.' + date_
        t_rejects_file_ = '/tmp/t_rejects_file.' + exec_name_ + '.' + date_
        live_rejects_file = '/tmp/live_rejects.' + exec_name_ + '.' + date_
        BashExec('touch %s' %(t_rejects_file_))
        BashExec('cat %s | egrep \"Rej|Max orders limit\" > %s' %(log_file,rejects_file_))
        BashExec('diff -u %s %s | egrep -v \"@|\\+\\+|\\-\\-\" \
            | grep "\\-" > %s '%(rejects_file_, t_rejects_file_, live_rejects_file))
        ProcessORSRejects(live_rejects_file)
        ProcessMaxOrderLimitReached(live_rejects_file)
        BashExec('mv %s %s '%(rejects_file_, t_rejects_file_))
        BashExec('true>%s' %(live_rejects_file))
        SlackNotifyRejects(exec_name_)
        limit_reached_symbols_.clear()
        rejection_dic_.clear()
        time.sleep(60)

def main():
    if len(sys.argv) != 2:
        print('USAGE : %s STRATNAME'%(sys.argv[0]))
        sys.exit(0)
    exec_name_ = sys.argv[1]
    date_yyyymmdd_ = dt.datetime.today().strftime('%Y%m%d')
    dblog_file_ =  exec_logic_logs_dir_ + exec_name_ + '_execlogic_dbglog.' + date_yyyymmdd_
    sec_ban_file = trade_info_dir + 'SecuritiesUnderBan/fo_secban_' + date_yyyymmdd_ + '.csv'
    contract_file_ = trade_info_dir + 'ContractFiles/nse_contracts.' + date_yyyymmdd_
    sec_uder_physical_file = trade_info_dir + 'SecuritiesUnderPhysicalSettlement/fo_securities_under_physical_settlement.csv'
    nearest_expiry = BashExec('grep IDXFUT %s | grep BANKNIFTY | awk \'{print $NF}\' | sort | head -n1'%(contract_file_))
    message = 'Initializing ' + exec_name_ + ' exec rejects alerts script'
    command = slack_exec +  ' ' + channel + ' DATA \'' + message + '\''
    BashExec(command)
    if date_yyyymmdd_ == nearest_expiry:
        LoadSecUnderPhysical(sec_uder_physical_file)
    LoadSecUnderBan(sec_ban_file)
    SendRejectionAlert(dblog_file_, exec_name_, date_yyyymmdd_)
if __name__ == '__main__':
    main()
