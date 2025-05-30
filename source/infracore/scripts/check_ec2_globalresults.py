#!/usr/bin/python
#Script to check result computation status for ALL or AS_MORN products
from os import listdir
import time
import os
from subprocess import Popen, PIPE, STDOUT
import sys
import glob
import smtplib
from datetime import datetime, timedelta

#Parse input
if len(sys.argv) < 2:
 print "USAGE: <script> ALL/AS_MORN/product [date] S/N"
 exit(1)

#AS_MORN or ALL
period=sys.argv[1]

#Get yesterday's result dir
today_str=datetime.strftime(datetime.now()-timedelta(1), "%Y%m%d")

#Fetch and initialize product list first
if period == "AS_MORN":
 cmd = "ssh 54.208.92.178 awk \\'{ print \$3}\\' /home/dvctrader/AWSScheduler/queues/globresult_parallel_recent_AS_MORN | sort | uniq | grep -vwf /home/pengine/prod/live_configs/result_check_exceptions.cfg | tr '\n' ' '"
 #We'll check for AS_MORN the same day
 today_str=datetime.strftime(datetime.now(), "%Y%m%d")
elif period == "ALL":
 cmd = "ssh 54.208.92.178 awk \\'{ print \$3}\\' /home/dvctrader/AWSScheduler/queues/globresult_parallel_recent | sort | uniq | grep -vwf /home/pengine/prod/live_configs/result_check_exceptions.cfg | tr '\n' ' '"
else:
 cmd = "echo "+period

#Take date as optional argument
if len(sys.argv) > 2:
 today_str=sys.argv[2]

staged=False
if len(sys.argv) > 3:
 staged=(sys.argv[3]=='S')

listed_products=str(Popen(cmd, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True).stdout.read())
output_path=""
if not staged:
 output_path="/home/dvctrader/trash/ec2_globalresults_status.txt"
else:
 output_path="/home/dvctrader/trash/ec2_staged_globalresults_status.txt"

opfile=open(output_path,'w')
products=filter(lambda x: len(x)>0, listed_products.split(" "))
incomplete_products=[]

for product in products:
 product=product.strip()
 #count number of strats in the pool
 strat_dir="/home/dvctrader/modelling/strats/"+product+"/"
 if staged:
    strat_dir="/home/dvctrader/modelling/staged_strats/"+product+"/"
 num_of_strategies=0
 for root, dirs, files in os.walk(strat_dir):
  num_of_strategies+=len(files)
 if num_of_strategies <= 0:
  continue
 path="/home/dvctrader/ec2_globalresults/"+product+"/"+today_str[0:4]+"/"+today_str[4:6]+"/"+today_str[6:8]+"/results_database.txt"
 if staged:
    path="/home/dvctrader/ec2_staged_globalresults/"+product+"/"+today_str[0:4]+"/"+today_str[4:6]+"/"+today_str[6:8]+"/results_database.txt"

 num_of_executed_strategies=0
 if os.path.isfile(path):
  with open(path) as f:
   num_of_executed_strategies=len(f.readlines())
 percentage=float(float(num_of_executed_strategies)/float(num_of_strategies))*100

 print percentage
 
 #Using 80% as the cutoff for "incomplete"
 if percentage < 80.0:
  incomplete_products.append(product)
  opfile.write(product+": Total_strats: "+str(num_of_strategies)+", Percent_complete: "+str(percentage)+" %\n")
  opfile.flush()

if len(incomplete_products) > 0 and (period == "AS_MORN" or period == "ALL"):
 mail_id="nseall@tworoads.co.in"
 result_str="EC2 globalresults incomplete for "+today_str+" : "+(", ".join(incomplete_products))
 mail_cmd="/bin/mail -s \""+result_str+"\" -r "+mail_id+" "+mail_id+" < "+output_path
 if not staged:
  print str(Popen(mail_cmd, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True).stdout.read())


