#!/usr/bin/env python

import sys
import os,smtplib, subprocess
import httplib 
import urllib 
import urllib2 
from bs4 import BeautifulSoup
import datetime 
import csv

is_url_open_ = False 
while not is_url_open_ : 
  try :
    url = 'http://nseindia.com/content/indices/niftymcwb.csv'
    hdr = {'User-Agent' : 'Mozilla/5.0','Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8'}
    req = urllib2.Request(url,headers=hdr)
    response = urllib2.urlopen(req) 
    is_url_open_ = True 
  except urllib2.HTTPError, e :
    is_url_open_ = False 
  except urllib2.URLError, e : 
    is_url_open_ = False 
  except Exception  :
    is_url_open_ = False 
  except :
    is_url_open_ = False 
  
the_page = csv.reader(response)
#print the_page
now = datetime.datetime.now()
YYMMDD=now.strftime("%Y%m%d")
file = open ("/spare/local/tradeinfo/NSE_Files/IndexInfo/NIFTY_" + YYMMDD + ".csv", "wb")
csv_file = csv.writer(file)

for row in the_page:
  if( (row[0] == 'Sr. No') or (row[0].isdigit()) ):
    csv_file.writerow(row)

file.close() 



is_url_open_ = False 
while not is_url_open_ : 
  try :
    url = 'http://www.nseindia.com/content/indices/ind_cnxbanklist.csv'
    hdr = {'User-Agent' : 'Mozilla/5.0','Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8'}
    req = urllib2.Request(url,headers=hdr)
    response = urllib2.urlopen(req) 
    is_url_open_ = True 
  except urllib2.HTTPError, e :
    is_url_open_ = False 
  except urllib2.URLError, e : 
    is_url_open_ = False 
  except Exception  :
    is_url_open_ = False 
  except :
    is_url_open_ = False 
  
the_page = csv.reader(response)
#print the_page
now = datetime.datetime.now()
YYMMDD=now.strftime("%Y%m%d")
file = open ("/spare/local/tradeinfo/NSE_Files/IndexInfo/BANKNIFTY_" + YYMMDD + ".csv", "wb")
csv_file = csv.writer(file)

for row in the_page:
  #if( (row[0] == 'Sr. No') or (row[0].isdigit()) ):
  csv_file.writerow(row)

file.close() 
