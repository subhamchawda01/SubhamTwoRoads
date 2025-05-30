#!/usr/bin/env python

import sys
import os,smtplib, subprocess
import httplib 
import urllib 
import urllib2 
from bs4 import BeautifulSoup
import datetime 

is_url_open_ = False 
while not is_url_open_ : 
  try :
    response = urllib2.urlopen('http://pregao-online.bmfbovespa.com.br/indices.aspx') 
    print "ERRor Could not open url file, reloading"
    is_url_open_ = True 
  except urllib2.HTTPError, e :
    is_url_open_ = False 
  except urllib2.URLError, e : 
    is_url_open_ = False 
#  except urllib2.HTTPException, e :
#    is_url_open_ = False 
  except Exception  :
    is_url_open_ = False 
  except :
    is_url_open_ = False 
  
the_page = response.read()
#print the_page
soup=BeautifulSoup(the_page)
now = datetime.datetime.now()
YYMMDD=now.strftime("%Y%m%d")
closing_price_file_name_ = "/spare/local/tradeinfo/IndexInfo/closing_price_" + YYMMDD + ".txt"

file = open ("/spare/local/tradeinfo/IndexInfo/closing_price_" + YYMMDD + ".txt", "w")
lines = soup.find_all( id='ctl00_DefaultContent_GrdCarteiraIndice' )
perc_filename_ = ""
if ( len ( sys.argv ) > 1 ) :
  perc_filename_ = sys.argv[1] 

prod_to_perc_ = {} 
if perc_filename_ :
 read_lines_ = open ( perc_filename_).readlines() 
 for line in read_lines_ :
  line = line.split() 
  if ( len ( line )  > 1 ) :
   prod_to_perc_[line[0]] = float ( line[1] ) / 100.0 

for line in lines :
    tr_ = line.find_all('tr' )
    for row_ in tr_ :
        col_ = row_.find_all ( 'td' )
        if ( len ( col_ ) > 2 )  :
            #print "col_ " +  col_[0].get_text().strip() + " " + col_[2].get_text().strip() + " " + col_[3].get_text().strip() + "\n"
            price_ = float ( col_[2].get_text().strip() .replace( ',', '.'))  
            prod_ = col_[0].get_text().strip()
            upper_limit_ = format  ( price_ + 0.3 * price_ , '.2f' ) 
            lower_limit_ = format ( price_ - 0.3 * price_, '0.2f' ) 
            if prod_ in prod_to_perc_.keys () :
              upper_limit_ = format ( price_ + prod_to_perc_[prod_ ] * price_ , '.2f' )
              lower_limit_ = format ( price_ - prod_to_perc_[prod_] * price_ , '.2f' ) 
            file.write ( col_[0].get_text().strip().replace( ',', '.') + " " +
                        col_[2].get_text().strip() .replace( ',', '.')+ " " +
                        col_[3].get_text().strip().replace( ',', '.') + " " + str ( lower_limit_) +
                        " " + str ( upper_limit_ ) +  "\n" )

exec_cmd_ = "~/basetrade_install/bin/calc_prev_day " + YYMMDD
out = subprocess.Popen ( exec_cmd_, shell=True, stdout=subprocess.PIPE )
prev_YYMMDD = out.communicate()[0].strip()
exec_cmd_ = "~/basetrade_install/bin/get_utc_hhmm_str BRT_1655 " + prev_YYMMDD 
out = subprocess.Popen ( exec_cmd_, shell=True, stdout=subprocess.PIPE ) 
st_time_ = out.communicate() [0].strip()
exec_cmd_ = "~/basetrade_install/bin/get_utc_hhmm_str BRT_1700 " + prev_YYMMDD 
out = subprocess.Popen ( exec_cmd_, shell=True, stdout=subprocess.PIPE ) 
en_time_ = out.communicate() [0].strip()
exec_cmd_ = "~/basetrade_install/bin/get_price_info_for_day BR_IND_0 " + prev_YYMMDD + " " + st_time_  + " "+ en_time_+ " | grep \"\<avg_px_\>\" | awk '{print $4}'"
out = subprocess.Popen( exec_cmd_, shell=True,stdout=subprocess.PIPE )
str_=out.communicate()[0].strip() 
price_ = 0
if str_ :
  price_ = int (  ( float ( str_)/5 ) * 5 )
file.write( "BR_IND_0 " + str( price_) + " 0.0 0.0 0.0 \n" ) 
file.close() 
if ( os.stat( closing_price_file_name_).st_size == 0 ) :
    sender='diwakar@circulumvite.com'
    receiver=['diwakar@circulumvite.com','nseall@tworoads.co.in']
    message= 'Last day closing price file : '+ closing_price_file_name_ + " is Empty. \n" + " is_url_open " + str ( is_url_open_ ) + "\n" 
    message="""From: Diwakar <diwakar@circulumvite.com>
To: Diwakar <diwakar@circulumvite.com>
Subject: Stock Closing price not available 

Last day Clsing price file """ + closing_price_file_name_ + " is Empty.\n is_url_open_ " + str ( is_url_open_ ) + "\n"

    try: 
      smtpobj = smtplib.SMTP ( '10.23.74.51' ) 
      smtpobj.sendmail ( sender, receiver, message ) 
    except :
        print " Error Empty file found and unable to send mail\n"
