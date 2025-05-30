########################################################################
#								       #
# Description: This script is used to create files for each product    #
#	      for getting price information for day for each interval  #
#	      like min price, max price, etc			       #
#								       #
# Usage: The data created will be useful to compare two product	       #
#								       #
# command: eachProductDataGeneration.py interval start_time end_time   #
#								       #
#      								       #				    
########################################################################


#!/usr/bin/env python

import sys
import os,smtplib, subprocess
from datetime import datetime, timedelta

if ( len(sys.argv) != 4 ) :
  print("Usage eachProductDataGeneration.py interval start_time end_time")
  exit()

interval = sys.argv[1]
starttime = datetime.strptime(sys.argv[2], '%I:%M')
endtime = datetime.strptime(sys.argv[3], '%I:%M')

date1= datetime(2018,11,9)
date2= datetime(2019,1,30)

while date1 <= date2 :
  if (date1.weekday() == 5 or date1.weekday() == 6) :
    date1=date1+timedelta(days=1)
    continue
  else :
    with open('/home/dvctrader/hardik/ProductDetails/shortcodes.txt') as ShortCodes :
      for shortcode in ShortCodes :
        sc=shortcode[:len(shortcode)-1]
        start_time = date1.replace(hour=starttime.hour, minute=starttime.minute)
        end_time = date1.replace(hour=endtime.hour, minute=endtime.minute)
        mm= str(date1.month) if date1.month>9 else "0"+str(date1.month)
        dd= str(date1.day) if date1.day>9 else "0"+str(date1.day)
        yyyymmdd = str(date1.year)+mm+dd
        directory = "/NAS4/hardik/ProductDetails/"+yyyymmdd
        if not os.path.exists(directory) :
          os.makedirs(directory)
       
        tempcmd = "~/get_price_info_for_day " + sc + " " + yyyymmdd + " 330 1000" 
        tempout = subprocess.Popen(tempcmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        tempstr_= str(tempout.stdout.read())
        tempstderr_= str(tempout.stderr.read())
     
        if (tempstr_.find('does not exist, returning') != -1 or tempstderr_.find('kGetContractSpecificationMissingCode') != -1 or tempstderr_.find('does not exist, returning') != -1) :
          continue
        file = open("/NAS4/hardik/ProductDetails/" + yyyymmdd + "/" + sc + "." + yyyymmdd, "w+")
        file.write("Time min_px_ max_px_ px_range_ avg_px_ px_stdev_ total_volume_ total_trades_ total_buy_volume_ total_sell_volume_ volume_weighted_avg_px_ volume_weighted_px_stdev_ avg_l1_size_ min_trd_sz_ max_trd_sz_ avg_trd_sz_ min_spread_ max_spread_ avg_spread_\n")
        file.close()
     
        while start_time < end_time :
          t1=str(start_time.hour)+str(start_time.minute)
          start_time=start_time + timedelta(minutes=int(interval))
          t2=str(start_time.hour)+str(start_time.minute)
          cmd = "echo " + str(start_time.hour) + ":" + str(start_time.minute) + ": `~/get_price_info_for_day " + sc + " " + yyyymmdd + " " + t1 + " " + t2 + " | awk '{print $4}' | tr '\n' ' '` >> /NAS4/hardik/ProductDetails/" + yyyymmdd + "/" + sc + "." + yyyymmdd
          cmd2= "proc_count=`ps -ef |grep get_price_info_for_day | grep -v grep | wc -l` ;while [ $proc_count -ge 30 ] ; do sleep 1;proc_count=`ps -ef |grep get_price_info_for_day | grep -v grep | wc -l` ;done"
          out = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
          out2 = subprocess.Popen(cmd2, shell=True, stdout=subprocess.PIPE)
	  
          out2.wait()
    date1=date1+timedelta(days=1) 

print("out\n")

