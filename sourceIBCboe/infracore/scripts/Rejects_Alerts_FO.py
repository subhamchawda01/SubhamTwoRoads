#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import sys
import time
import subprocess
import datetime as dt
import pandas as pd

rejection_map_ = {}
exchange_rejects_map = {}
channel = 'nsehft-rejects'
slack_exec = '/home/pengine/prod/live_execs/send_slack_notification'
os.environ['LD_PRELOAD'] = '/home/dvcinfra/important/libcrypto.so.1.1'
data_source_exch_symbol_file = '/spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt'
batch_cancel_count = 0

def BashExec(command):
    child = subprocess.Popen(['bash', '-c', command],
                             stdout=subprocess.PIPE)
    return child.communicate()[0].decode('UTF-8').rstrip()

def SendSlackAlerts():
    for symbol in rejection_map_:
        line = hostname \
            + ' ALERT : %s Rejections for the symbol %s with reason %s' \
            % (rejection_map_[symbol][0],
               dsexch_symbol_df.loc[symbol.strip()].iloc[0],
               ','.join([':'.join([key, str(val)]) for (key, val) in
               rejection_map_[symbol][1].items()]))
        command = slack_exec + ' ' + channel + ' ' + 'DATA' + ' \'' \
            + line + '\''
        BashExec(command)
    for reason in exchange_rejects_map:
        line = hostname \
            + ' ALERT : Exchange Rejects : %d Reason Code : %d ' \
            % (exchange_rejects_map[reason], reason)
        command = slack_exec + ' ' + channel + ' ' + 'DATA' + ' \'' + line \
            + '\''
        BashExec(command);
    if batch_cancel_count > 0:
        line = hostname \
        + ' ALERT : Batch Cancellation Recieved : %d ' \
            % (batch_cancel_count)
        command = slack_exec + ' ' + channel + ' ' + 'DATA' + ' \'' + line \
            + '\''
        BashExec(command);

def ProcessExchRejects(rejects_file):
    rejects_data = BashExec('grep -B1 "Exch OrderReject" %s'
                            % rejects_file).rstrip().split('\n')
    numLines = len(rejects_data)
    if numLines < 1 or (numLines == 1 and rejects_data[0] == ''):
        return;
    for i in range(0, numLines, 2):
        reason = int(rejects_data[i].split('Reason :')[1])
        if reason in exchange_rejects_map:
            exchange_rejects_map[reason] += 1
        else:
            exchange_rejects_map[reason] = 1
    print('Done with  exchange rejects')

def ProcessORSRejects(rejects_file):
    rejects_data = \
        BashExec('grep -B1 "Current Risk Limit for" %s | grep -v "\-\-"'\
                            % rejects_file).split('\n')
    numLines = len(rejects_data)
    if numLines < 1 or (numLines == 1 and rejects_data[0] == '') : 
      return;
    for i in range(0, numLines, 2):
        symbol = rejects_data[i + 1].split('Current Risk Limit for'
                )[1].split(':')[0]
        reason = rejects_data[i].split('margin_retval = ')[1]
        if symbol in rejection_map_:
            rejection_map_[symbol][0] += 1
            if reason in rejection_map_[symbol][1]:
                rejection_map_[symbol][1][reason] += 1
            else:
                rejection_map_[symbol][1][reason] = 1
        else:
            rejection_map_[symbol] = [1, {reason: 1}]
    print('Done processing ors rejects');

def ProcessBatchCancellation(rejects_file):
    global batch_cancel_count
    batch_cancel_data = \
        BashExec('grep "Received Batch ORDER CANCELLATION" %s | grep -v "\-\-"'\
                            % rejects_file).split('\n')
    numLines = len(batch_cancel_data)
    if numLines < 1 or (numLines == 1 and batch_cancel_data[0] == ''):
        return;
    batch_cancel_count = numLines
    print('Done processing Batch Cancel');

def SlackNotifyRejects():
    BashExec('touch %s' % t_rejects_file)
    while True:
        BashExec('cat %s | egrep -B1 "Current Risk Limit|Exch OrderReject|Received Batch ORDER CANCELLATION"\
                > %s'
                  % (log_file, rejects_file))
        BashExec('diff -u %s %s  | egrep -v "@@|\-\-|\+\+" | grep "\+" > %s'
                 % (t_rejects_file, rejects_file, live_rejects_file))
        ProcessExchRejects(live_rejects_file)
        ProcessORSRejects(live_rejects_file)
        ProcessBatchCancellation(live_rejects_file)
        BashExec('mv %s %s' % (rejects_file, t_rejects_file))
        SendSlackAlerts()
        rejection_map_.clear()
        exchange_rejects_map.clear()
        time.sleep(60)


def GetShortCodeFromDs(input_file, out_file):
    command = '/home/pengine/prod/live_execs/get_shortcode_from_ds ' \
        + str(today) + ' FO ' + input_file + ' ' + out_file
    return BashExec(command)


def LoadDataSourceExchSymbol(file, today):
    df = pd.read_csv(file, names=['DS', 'symbol'],
                     delim_whitespace=True)
    df = df[df['symbol'].apply(lambda x: int(today) <= int(x.split('_'
            )[3]))]
    del df['symbol']
    print(df)
    df.to_csv('/tmp/ds_symbols', header=None, index=None)
    GetShortCodeFromDs('/tmp/ds_symbols', '/tmp/ds_shortcodes')
    df = pd.read_csv('/tmp/ds_shortcodes', names=['DS', 'Symbol'],
                     delim_whitespace=True)
    df = df[df['Symbol'] != 'INVALID']
    df = df.set_index('DS', drop=True)
    return df


today = dt.datetime.today().strftime('%Y%m%d')
hostname = BashExec('hostname')
log_file = BashExec('echo `ps -ef |grep cme_ilink_ors | grep -v grep | \
                     awk \'{print $NF}\'`/log.`date +"%Y%m%d"`')
if not os.path.isfile(log_file):
    print ("Log file doesn't exist : %s" % log_file)
    sys.exit(-1)
t_rejects_file = '/tmp/t_rejects_file' + today
rejects_file = '/tmp/rejects_file' + today
live_rejects_file = '/tmp/live_rejects_file' + today
dsexch_symbol_df = LoadDataSourceExchSymbol(data_source_exch_symbol_file, today)
dsexch_symbol_df.to_csv('sanjeev')
SlackNotifyRejects()
