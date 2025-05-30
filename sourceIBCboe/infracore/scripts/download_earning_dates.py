#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import time
import trlibs
import subprocess
import pandas as pd
import datetime as dt
from time import strptime
from selenium import webdriver
from selenium.webdriver.support.ui import WebDriverWait

tradeinfodir = '/spare/local/tradeinfo/NSE_Files'
earnings_file_dir = tradeinfodir + '/EarningsReports'
bse_eannings_url = 'https://www.bseindia.com/corporates/Forth_Results.aspx#';

def GetDateYYYYMDD(date_):
    date_ = date_.strip()
    date_arr = date_.split(' ')
    month_ = '{0}'.format(str(strptime(date_arr[1], '%b'
                          ).tm_mon).zfill(2))
    date_ = date_arr[2] + '-' + month_ + '-' + date_arr[0]
    return date_

def DownloadEarningsReports():
    options = webdriver.ChromeOptions();
    options.add_argument('--incognito');
    options.add_argument('--no-sandbox');
    prefs = {'download.default_directory' : earnings_file_dir + '/'}
    options.add_experimental_option('prefs', prefs)
    options.add_argument('--headless')
    browser = webdriver.Chrome(executable_path='/usr/local/bin/chromedriver',options=options);
    browser.implicitly_wait(20)
    browser.get(bse_eannings_url);
    link = browser.find_element_by_id('ContentPlaceHolder1_lnkDownload')
    link.click();
    time.sleep(10)

def main():
    today = dt.datetime.today().strftime('%Y%m%d')
    DownloadEarningsReports();
    earnings_file = earnings_file_dir + '/Results.csv'
    fileday = dt.datetime.fromtimestamp(os.path.getctime(earnings_file)).strftime("%Y%m%d")
    if fileday != today:
        exit(-1)
    df = pd.read_csv(earnings_file, usecols=[1, 3], skiprows = [0],
                     names=['SYMBOL','DATE'])
    fo_products = [ line.split(',')[1].strip() for line in open(tradeinfodir + '/Lotsizes/fo_mktlots_' + today + '.csv') ]
    df = df[ df['SYMBOL'].isin(fo_products) ].reset_index(drop=True)
    df['DATE'] = df['DATE'].apply(lambda x: GetDateYYYYMDD(x))
    df = df.sort_values(by = ['DATE'])
    df.to_csv(earnings_file_dir + '/earnings.' + today ,
            header=None, sep='\t', index=None)

if __name__ == '__main__':
    main()
