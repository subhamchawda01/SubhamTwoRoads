#!/usr/bin/python
# -*- coding: utf-8 -*-
import pandas as pd
from time import strptime
from selenium import webdriver
from selenium.webdriver.support.ui import WebDriverWait


def GetDateYYYYMDD(date_):
    date_ = date_.strip()
    date_arr = date_.split(' ')
    month_ = '{0}'.format(str(strptime(date_arr[1], '%b'
                          ).tm_mon).zfill(2))
    date_ = date_arr[2] + '-' + month_ + '-' + date_arr[0]
    return date_


def main():
    earnings_data_frame = pd.DataFrame([])
    bse_eannings_url = \
        'https://www.bseindia.com/corporates/Forth_Results.aspx#'
    options = webdriver.ChromeOptions()
    options.add_argument('--headless')
    options.add_argument('--no-sandbox')
    options.add_argument('--incognito')
    browser = \
        webdriver.Chrome(executable_path='/usr/local/bin/chromedriver',
                         chrome_options=options)
    browser.implicitly_wait(30)
    browser.get(bse_eannings_url)
    next_page = 2
    while True:
        results = \
            browser.find_element_by_id('ContentPlaceHolder1_gvData')
        data_rows = results.find_elements_by_class_name('TTRow')
        print(next_page)
        for row in data_rows:
            earning_date = GetDateYYYYMDD(row.text[-11:])
            ticker_ = row.text.split(' ')[1]
            new_df_ = pd.DataFrame([[ticker_, earning_date]])
            earnings_data_frame = earnings_data_frame.append(new_df_)
        try:
            next_link_ = \
                browser.find_element_by_link_text(str(next_page))
            next_link_.click()
            next_page += 1
        except:
            try:
                next_link_ = results.find_elements_by_link_text('...')
                if len(next_link_) > 2:
                    next_link_[1].click()
                    next_page += 1
                elif len(next_link_) == 2 and next_page == 11:
                    next_link_[0].click()
                    next_page += 1
                elif len(next_link_) == 0:
                    try:
                        next_link_ = \
                            results.find_elements_by_link_text('last')
                        next_link_.click()
                        next_link_ = \
                            results.find_elements_by_link_text(next_page)
                        next_link_.click()
                    except:
                        break
                else:
                    break
            except:
                break
    earnings_data_frame.to_csv('/home/dvcinfra/NSE_Data_processed/Earnings_Data/earnings_dates_new.csv'
                               , sep='\t', index=False, header=False)
    print(earnings_data_frame)


if __name__ == '__main__':
    main()
