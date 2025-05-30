#!/usr/bin/python

import sys
import smtplib
import socket

num_arg = len(sys.argv)
if num_arg != 3:
   print("Scripts trade_file_dir date")
   sys.exit(0)

trade_dir = str(sys.argv[1])
trade_date = str(sys.argv[2])
file_path=trade_dir+'/trades.'+trade_date
message = ""
position = {}
for line in open(file_path, 'r'):
    column = line.split('\x01')
    product = column[0]
    buy_sel = int(column[1])
    size = int(column[2])
    if product in position:
       if buy_sel == 0:
          position[product] = int(position[product]) + size
       else:
          position[product] = int(position[product]) - size
    else:
       if buy_sel == 0:
          position[product] = size
       else:
          position[product] = -size
count = 0  
for key in position:
    if  position[key] != 0: 
        print('{:>20}'.format(key)+" " + '{:>50}'.format(str(position[key])))
        message = message+'{:>20}'.format(key)+" " + '{:>50}'.format(str(position[key]))+'\n'
        count = count + 1

body=message
hostname = socket.gethostname()
if count != 0:
   message = 'Subject: {}\n\n{}'.format("Open Position Date:  " + trade_date + "   HostName: " + hostname, body)
else:
   message = 'Subject: {}\n\n{}'.format("Open Position Date:  " + trade_date + "   HostName: " + hostname, "No postion open")

sender = 'dvc-infra@tworoads.co.in'
receivers = ['rahul.kumar@tworoads.co.in', 'shantanu.kedia@tworoads.co.in', 'ravi.parikh@tworoads.co.in', 'uttkarsh.sarraf@tworoads.co.in']
try:
   smtpObj = smtplib.SMTP('localhost')
   smtpObj.sendmail(sender, receivers, message)
except SMTPException:
   print ("Error: unable to send email")
