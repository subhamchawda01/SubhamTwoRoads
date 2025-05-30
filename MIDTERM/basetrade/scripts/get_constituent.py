#!/usr/bin/env python

import sys
import os
import http.client
import urllib.request
import urllib.parse
import urllib.error
import urllib.request
import urllib.error
import urllib.parse
from bs4 import BeautifulSoup
import datetime

is_url_open_ = False
while not is_url_open_:
    try:
        response = urllib.request.urlopen(
            'http://www.bmfbovespa.com.br/indices/ResumoCarteiraTeorica.aspx?Indice=Ibovespa&idioma=en-us')
        is_url_open_ = True
    except urllib.error.HTTPError as e:
        is_url_open_ = False
    except urllib.error.URLError as e:
        is_url_open_ = False
    except Exception:
        is_url_open_ = False
    except:
        is_url_open_ = False

the_page = response.read()
# print the_page
soup = BeautifulSoup(the_page)
now = datetime.datetime.now()
YYMMDD = now.strftime("%Y%m%d")
stock_list_filename_ = "/spare/local/tradeinfo/IndexInfo/stock_list_" + YYMMDD + ".txt"
file = open(stock_list_filename_, "w")
for line in soup.find_all('tr'):
    this_line_ = ""
    index_ = 0
    if (line.get('disabled') == 'disabled'):
        continue
    for td_ in line.find_all('td'):
        if not td_:
            break
        if (index_ == 1 or index_ == 2):
            index_ += 1
            continue
        span = td_.find_all('span')
        for val in span:
            if val:
                this_str_ = val.get_text().strip().replace(',', '').replace(' ', '')
                this_line_ = this_line_ + " " + this_str_
        index_ += 1
    if this_line_:
        # print this_line_
        file.write(this_line_.strip() + "\n")
file.close()
if (os.stat(stock_list_filename_).st_size == 0):
    sender = 'diwakar@circulumvite.com'
    receiver = ['diwakar@circulumvite.com']
    message = 'ERROR index constituent file : ' + stock_list_filename_ + \
        " is Empty. \n" + " is_url_open " + str(is_url_open_) + "\n"
    message = """From: Diwakar <diwakar@circulumvite.com>
To: Diwakar <diwakar@circulumvite.com>
Subject: ERROR index constitunts not available

Today's Constituent List not available """ + stock_list_filename_ + " is Empty.\n is_url_open_ " + str(is_url_open_) + "\n"

    try:
        smtpobj = smtplib.SMTP('10.23.74.51')
        smtpobj.sendmail(sender, receiver, message)
    except:
        print(" Error Empty file found and unable to send mail\n")
