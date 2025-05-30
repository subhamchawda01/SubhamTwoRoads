#!/usr/bin/python
# -*- coding: utf-8 -*-
import pandas as pd
import datetime as dt
from ftplib import FTP
import subprocess
from jinja2 import Environment, FileSystemLoader, Template
me = 'sanjeev.kumar@tworoads-trading.co.in'
infra = ['sanjeev.kumar@tworoads-trading.co.in', 'raghunandan.sharma@tworoads-trading.co.in', 'hardik.dhakate@tworoads-trading.co.in', 'subham.chawda@tworoads-trading.co.in']
all_infra = ['sanjeev.kumar@tworoads-trading.co.in', 'raghunandan.sharma@tworoads-trading.co.in', 'hardik.dhakate@tworoads-trading.co.in', 'subham.chawda@tworoads-trading.co.in', 'nseall@tworoads.co.in']
#me = 'subham.chawda@tworoads-trading.co.in'
#you = ['subham.chawda@tworoads-trading.co.in']

HTML_REPORT_TITLE1 = "FO : STREAM INFO DIFFERENCE"
HTML_REPORT_TITLE2 = "FO : STREAM INFO REMOVED"
HTML_REPORT_TITLE3 = "FO : STREAM INFO ADDED"
HTML_REPORT_TEMPLATE = 'position_report_template.html'
HTML_OUT_FILE = '/home/dvctrader/trash/stream_diff_reportfo.html'
#HTML_OUT_FILE = '/home/subham/subham/stream_diff_reportfo.html'


def BashExec(command):
    return subprocess.check_output(['bash', '-c',
                                   command]).decode('UTF-8')


def DumpDiff(df_, df_old_, df_new_):
    env = Environment(loader=FileSystemLoader('/home/dvctrader/important/'))
#    env = Environment(loader=FileSystemLoader('/home/subham/subham/'))
    template = env.get_template(HTML_REPORT_TEMPLATE)
    template_vars = {'title1': HTML_REPORT_TITLE1 ,
                     'stats_table1': df_.to_html(),
                     'title2': HTML_REPORT_TITLE2 ,
                     'stats_table2': df_old_.to_html(),
                     'title3': HTML_REPORT_TITLE3 ,
                     'stats_table3': df_new_.to_html() }
    html_out = template.render(template_vars)
    file_ = open(HTML_OUT_FILE, 'w')
    file_.write(html_out)
    file_.flush()
    file_.close()


def SendDiffReport(df_, df_old_, df_new_):
    if (not df_.empty) or (not df_new_.empty) or (not df_new_.empty):
        subject = '**********ALERT : FO NSE stream info difference**********'
        DumpDiff(df_, df_old_, df_new_)
        command = \
            '(echo To: "%s" ; echo From: "%s"; echo Subject: "%s"; echo "Content-Type: text/html;";cat %s) | /usr/sbin/sendmail -t' \
            % (' '.join(all_infra), me, subject, HTML_OUT_FILE)
        BashExec(command)


def getStreamDifference(filename1_, filename2_):
    df1 = pd.read_csv(filename1_, skiprows=1, header=None)
    df1.columns = ['A','STREAM_NO','C','D','SYMBOL','E','F','TYPE']
    df1 = df1[ df1['TYPE'] == 'XX' ]
    df1 = df1[['STREAM_NO', 'SYMBOL']]
    df1 = df1.drop_duplicates(subset='SYMBOL',keep='first')
    df2 = pd.read_csv(filename2_, header=None, skiprows=1)
    df2.columns = ['A','STREAM_NO','C','D','SYMBOL','E','F','TYPE']
    df2 = df2[ df2['TYPE'] == 'XX' ]
    df2 = df2[['STREAM_NO', 'SYMBOL']]
    df2 = df2.drop_duplicates(subset='SYMBOL',keep='first')
    old_stream = df1.merge(df2, on=['SYMBOL'], how = 'outer' , indicator=True, suffixes=['_NOW','_PREV']).loc[lambda x : x['_merge']=='right_only']
    new_stream = df1.merge(df2, on=['SYMBOL'], how = 'outer' , indicator=True, suffixes=['_NOW','_PREV']).loc[lambda x : x['_merge']=='left_only']
    stream_diff = df1.merge(df2, on=['SYMBOL'], suffixes=['_NOW', '_PREV'])

    old_stream = old_stream.astype({"STREAM_NO_PREV": int})
    new_stream = new_stream.astype({"STREAM_NO_NOW": int})
    stream_diff = stream_diff[stream_diff['STREAM_NO_PREV']
                              != stream_diff['STREAM_NO_NOW']]

    stream_diff = stream_diff.sort_values(by=['SYMBOL'])
    old_stream = old_stream.sort_values(by=['SYMBOL'])
    new_stream = new_stream.sort_values(by=['SYMBOL'])

    stream_diff.reset_index(drop=True, inplace=True)
    old_stream.reset_index(drop=True, inplace=True)
    new_stream.reset_index(drop=True, inplace=True)

    stream_diff = stream_diff[['SYMBOL','STREAM_NO_PREV','STREAM_NO_NOW']]
    old_stream = old_stream[['SYMBOL','STREAM_NO_PREV']]
    new_stream = new_stream[['SYMBOL','STREAM_NO_NOW']]

    return stream_diff, old_stream, new_stream;


def downloadFileFromFtp(ftp_host,ftp_username,ftp_passwd,ftp_filename):
    try:
        ftp = FTP(ftp_host)
        ftp.login(ftp_username, ftp_passwd)
        ftp.set_pasv(True)
        ftp.cwd('faoftp')
        ftp.cwd('faocommon')
        ftp.cwd('tbt_masters')
        ftp.retrbinary('RETR fo_contract_stream_info.csv',
                       open(ftp_filename, 'wb').write)
    except:
        subject = 'FO : FAILED DOWNLOADING CONTRACT STREAM INFO 5.26'
        command = \
            '(echo To : "%s" ; echo From: "%s"; echo Subject : "%s";) | /usr/sbin/sendmail -t' \
            % (' '.join(infra), me, subject)
        BashExec(command)


def main():
    date = dt.datetime.today().strftime('%Y%m%d')
    current_time = dt.datetime.today().strftime('%Y-%d-%d %H:%M:%S')
    ftp_host = 'ftp.connect2nse.com'
    ftp_username = 'FAOGUEST'
    ftp_passwd = 'FAOGUEST'
    stream_info_dir = '/spare/local/tradeinfo/NSE_Files/StreamInfo/Derivative/'
#    stream_info_dir = '/home/subham/subham/stream_check_script/'
    ftp_filename = stream_info_dir + 'fo_contract_stream_info.csv'
    old_ftp_filename = stream_info_dir \
        + 'fo_contract_stream_info.csv_old'
    downloadFileFromFtp(ftp_host, ftp_username, ftp_passwd,
                        ftp_filename)
    ftp_file_new = open(ftp_filename, "r").readline().split(",")
    ftp_file_old = open(old_ftp_filename, "r").readline().split(",")

    if ftp_file_new[0] != ftp_file_old[0]:
      print (ftp_file_new[0] , ftp_file_old[0])
      subject = 'INFO : FO STREAM FILE UPDATED'
      command = 'echo "file : %s" | mailx -s "%s : %s" %s' \
               % (old_ftp_filename, subject, current_time,' '.join(infra))
      BashExec(command)

      stream_diff, old_stream, new_stream = getStreamDifference(ftp_filename, old_ftp_filename)
      print (stream_diff)
      print (old_stream)
      print (new_stream)
      if (not stream_diff.empty) or (not old_stream.empty) or (not new_stream.empty):
          print("stream difference")
          DumpDiff(stream_diff, old_stream, new_stream)
          SendDiffReport(stream_diff, old_stream, new_stream)
      BashExec('mv ' + old_ftp_filename + ' ' + old_ftp_filename + '_'  + date)
      BashExec('mv ' + ftp_filename + ' ' + ftp_filename + '_old')
    else:
      BashExec('rm -rf ' + ftp_filename)


if __name__ == '__main__':
    main()
