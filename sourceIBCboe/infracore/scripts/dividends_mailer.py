#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import time
import jinja2
import trlibs
import subprocess
import pandas as pd
import datetime as dt
from time import strptime
from selenium import webdriver
from selenium.webdriver.support.ui import WebDriverWait
from jinja2 import Environment, FileSystemLoader, Template

me = 'sanjeev.kumar@tworoads-trading.co.in'
you = ['sanjeev.kumar@tworoads-trading.co.in']
you = ['sanjeev.kumar@tworoads-trading.co.in','nseall@tworoads.co.in ', 'smit@tworoads-trading.co.in' ]

HTML_REPORT_TEMPLATE = 'html_report_template.html'
HTML_OUT_FILE = '/home/dvctrader/trash/dividends.html'
HTML_REPORT_TITLE = 'Dividend Report'
url = 'https://www.bseindia.com/corporates/corporate_act.aspx'
tradeinfodir = '/spare/local/tradeinfo/NSE_Files/'
dividents_reports_dir = tradeinfodir + 'DividendsReports/'

def DumpEarnings(df_, df2):
    env = Environment(loader=FileSystemLoader('/home/dvctrader/important/'))
    template = env.get_template(HTML_REPORT_TEMPLATE)
    template_vars = {'stats_table': df_.to_html(), 'stats_table2' : df2.to_html()}
    html = template.render(template_vars)
    file_ = open(HTML_OUT_FILE, 'w')
    file_.write(html)
    file_.flush()
    file_.close()


def SendDividendsReport(df1,df2):
    DumpEarnings(df1,df2);
    subject = 'Dividend Report TRADER13';
    command = \
        '(echo To: "%s" ; echo From: "%s"; echo Subject: "%s"; \
				echo "Content-Type: text/html;";cat %s) | /usr/sbin/sendmail -t' \
        % (' '.join(you), me, subject, HTML_OUT_FILE)
    trlibs.BashExec(command)



def GetDateYYYYMDD(date_):
    date_ = date_.strip()
    date_arr = date_.split(' ')
    month_ = '{0}'.format(str(strptime(date_arr[1], '%b'
                          ).tm_mon).zfill(2))
    date_ = date_arr[2] + '-' + month_ + '-' + date_arr[0]
    return date_

def DownloadDividends():
    options = webdriver.ChromeOptions();
    options.add_argument('--incognito');
    options.add_argument('--no-sandbox');
    options.add_argument('--headless')
    prefs = {'download.default_directory' : dividents_reports_dir + '/'}
    options.add_experimental_option('prefs', prefs)
    browser = webdriver.Chrome(executable_path='/usr/local/bin/chromedriver',options=options);
    browser.implicitly_wait(20)
    browser.get(url);
    link = browser.find_element_by_id('ContentPlaceHolder1_lnkDownload')
    link.click();
    time.sleep(5)

def main():
    date = dt.datetime.today().strftime('%Y%m%d')
    DownloadDividends()
    dividends_file = dividents_reports_dir + '/Corporate_Actions.csv'
    fileday = dt.datetime.fromtimestamp(os.path.getctime(dividends_file)).strftime("%Y%m%d")
    if fileday != date:
        exit(-1)
    df = pd.read_csv(dividends_file, usecols=[1, 3, 4], skiprows = [0],
                     names=['SYMBOL','DATE', 'Purpose'])
    fo_products = [ line.split(',')[1].strip() for line in open(tradeinfodir + '/Lotsizes/fo_mktlots_' + date + '.csv') ]
    df = df[ df['SYMBOL'].isin(fo_products) ].reset_index(drop=True)
    df['DATE'] = df['DATE'].apply(lambda x: GetDateYYYYMDD(x))
    df_filtered = df[df['Purpose'].str.contains('Dividend')];
    other_events = df[~df['Purpose'].str.contains('Dividend')].reset_index(drop=True);
    df_filtered.to_csv('/spare/local/tradeinfo/NSE_Files/DividendsReports/dividends.' + date, index = None, header=None)
    SendDividendsReport(df_filtered, other_events)


if __name__ == '__main__':
    main()
