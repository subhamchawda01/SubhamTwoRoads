##!/usr/bin/env python 

import os
import sys
import argparse
import base64
import http.client
import json
import time
from datetime import datetime

def get_seq_no(next_seq_no_):
#generate seq no
  seq_no_str_ = ''
  for i in range(7 - len(next_seq_no_)):
    seq_no_str_ += '0'
  seq_no_str_ += next_seq_no_
  return seq_no_str_

def get_base64(message):
#  message = "16022021112133333:123456"
  message_bytes = message.encode('ascii')
  base64_bytes = base64.b64encode(message_bytes)
  base64_message = base64_bytes.decode('ascii')

  return base64_message

def get_token(usr_pass_comb_, nonce_ddmmyyyyhhmmssSSS_):
  
  base64_usr_pass = get_base64(usr_pass_comb_)
  print("base64: " + base64_usr_pass)
  conn = http.client.HTTPSConnection("www.connect2nse.com")
  payload = 'grant_type=client_credentials'
  headers = {
      'Content-Type': 'application/x-www-form-urlencoded',
      'nonce': nonce_ddmmyyyyhhmmssSSS_,
      'Authorization': 'Basic ' + base64_usr_pass,
  }
  print("\n\nHEADER: ")

  for key in headers:
    print(key + " : " + headers[key])

  conn.request("POST", "/token", payload, headers)
  res = conn.getresponse()
  print("TOKEN STATUS: " + str(res.status) + " REASON: " + str(res.reason))
  data = res.read()
  print(data.decode("utf-8"))
  return json.loads(data.decode("utf-8"))

def get_tradeexport_file(date_, base64_nonce_, next_seq_no_, token_, trade_inquiry_str_, cm_fo_):
  is_start_ = True
  next_msg_seq_no = "0"

  #current time:
  curr_time_str_ = datetime.now().strftime("%H%M") 
  curr_time_secs = time.time()
#
  comb_filename = "C:/Users/tworoads/Desktop/Postman_Position_File/" + cm_fo_ + "/Trade_export_" + cm_fo_ + "_" + date_ + "_" + curr_time_str_ + "_comb.txt"
  comb_file = open(comb_filename,"w+")
  while is_start_:
    member_code_ = "90044"
    seq_no_str_ = get_seq_no(next_seq_no_)
    next_seq_no_ = str(int(next_seq_no_) + 1)
    #next_msg_seq_no = str(int(next_msg_seq_no) + 1)
    print("############\n\nSeq_str: " + seq_no_str_)
    msgId_ = member_code_ + date_ + seq_no_str_
    print("msgId: " + msgId_)
    conn = http.client.HTTPSConnection("www.connect2nse.com")
    payload = "{ \"version\": \"1.0\",\r\n\"data\": {\r\n\"msgId\": \"" + msgId_ + "\",\r\n\"dataFormat\": \"CSV:CSV\",\r\n\"tradesInquiry\": \"" + next_msg_seq_no + ",ALL,,\"\r\n}\r\n}"
    headers = {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
        'nonce': base64_nonce_,
        'Authorization': 'Bearer ' + token_
    }
    print("PayLOAD: \n" + payload)
    print("header_Authorization: " + headers['Authorization'])
    conn.request("POST", trade_inquiry_str_, payload, headers)
    res = conn.getresponse()
    data = res.read()
    response_str_ = data.decode("utf-8")
    #
    strt_index = 0
    is_msgId_found = False
    while response_str_.find('}}', strt_index) != -1 :
      find_ = response_str_.find('}}', strt_index)
      find_t = find_ + 2
      if msgId_ == json.loads(response_str_[strt_index:find_t])['data']['msgId']:
        is_msgId_found = True
        print("FIND DONE")
        break
      strt_index = find_t
    response_json_ = json.loads(response_str_[strt_index:find_t])
    status = response_json_['status']
    message_code = response_json_['messages']['code']
    print("msgId, status, msg_code :: " + msgId_ + ", " + status + ", " + message_code)
    if is_msgId_found == False or status != "success" or message_code != "01010000":
      print("END")
      break
    trade_enquiry = response_json_['data']['tradesInquiry']
    #
    fnd_seq_index = trade_enquiry.find(',,,')
    info_end_index = trade_enquiry.find('^')
    seq_entries = trade_enquiry[fnd_seq_index:info_end_index]
    seq_find = seq_entries.find(',',3)
    print("trade_inq_info: " + seq_entries)
    strt = 3
    next_msg_seq_no = seq_entries[strt:seq_find]
    print("Next seq_str: " + next_msg_seq_no)
#    
    print("seq_str: " + next_msg_seq_no + " seq_find: " + str(seq_find))
    no_of_entries = int(seq_entries[seq_find+1:])
    print("no_of_entries: " + str(no_of_entries))

    if no_of_entries <= 0 :
      print("END no_of_entries: " + str(no_of_entries))
      break
   
    #
    trade_details = trade_enquiry[info_end_index+1:].replace('^',"\n")
#
    lst_find = trade_enquiry.rfind('^')
    print("last_find: " + str(lst_find))
    last_entry = trade_enquiry[lst_find+1:]

    time_find1 = last_entry.find(',')
    time_find2 = last_entry.find(',',time_find1 + 1)
    time_find3 = last_entry.find(',',time_find2 + 1)
    time_find4 = last_entry.find(',',time_find3 + 1)
    if time_find3 == -1 or time_find4 == -1 :
       print("invalid")
       
       break
    lst_entry_time = int(last_entry[time_find3+1:time_find4]) / 65536 + 315513000
    print("time: " + str(lst_entry_time))
    print("last_entry: " + last_entry)


    #
    filename = "C:/Users/tworoads/Desktop/Postman_Position_File/" + cm_fo_ + "/Trade_export_" + cm_fo_ + "_" + date_ + "_" + seq_no_str_ + "_" + next_msg_seq_no + "_" + curr_time_str_ + ".txt"
    file1 = open(filename,"w+")
    file1.write(trade_details)
    comb_file.write(trade_details)
    comb_file.write('\n')
    file1.close()
    if curr_time_secs < lst_entry_time :
      print("time extended")
      is_start_ = False
      break
    print("Wait 30 secs")
    time.sleep(30)
    #
  
  comb_file.close()

#



def main():

#  strt_index = 0
#  response_str_='{"status": "success","messages": {"code": "01010000"},"data": {"msgId": "90044202102160000002","tradesInquiry": "6,20210216,,,2066393,0000^376065,1,2021021675018658,85061409177600,3045,125,41165,1,1300000000186524,1,43965,2,90044,90044,,2,6001,1297934100,1,0,N,,0,560049001011000,4128,N,SBIN,EQ,STATE BANK OF INDIA,EQ,90044,,,,,,,,^376510,1,2021021675018914,85061409177600,3499,88,67110,1,1300000000187772,1,43740,2,90044,90044,,2,6001,1297934100,1,0,N,,0,560049001009000,4128,N,TATASTEEL,EQ,TATA STEEL LIMITED,EQ,90044,,,,,,,,"}}'
##  response_str_="{\"status\": \"success\",\"messages\": {\"code\": \"01010000\"},\"data\": {\"msgId\": \"90044202102160000002\",\"tradesInquiry\": \"6,20210216,,,2066393,10000^376065,1,2021021675018658,85061409177600,3045,125,41165,1,1300000000186524,1,43965,2,90044,90044,,2,6001,1297934100,1,0,N,,0,560049001011000,4128,N,SBIN,EQ,STATE BANK OF INDIA,EQ,90044,,,,,,,,^376510,1,2021021675018914,85061409177600,3499,88,67110,1,1300000000187772,1,43740,2,90044,90044,,2,6001,1297934100,1,0,N,,0,560049001009000,4128,N,TATASTEEL,EQ,TATA STEEL LIMITED,EQ,90044,,,,,,,,\"}}"
#  find_ = response_str_.find('}}', strt_index)
#
#  find_t = find_ + 2
#  print("find: " + str(find_t) + " len: " + str(len(response_str_)))
#  response_json_ = json.loads(response_str_[strt_index:find_t])
#  status = response_json_['status']
#  message_code = response_json_['messages']['code']
#  msgId_ = json.loads(response_str_[strt_index:find_t])['data']['msgId']
#  print("msgId, status, msg_code :: " + msgId_ + ", " + status + ", " + message_code)
#
#  trade_enquiry = response_json_['data']['tradesInquiry']
#  fnd_seq_index = trade_enquiry.find(',,,')
#  info_end_index = trade_enquiry.find('^')
#  seq_entries = trade_enquiry[fnd_seq_index:info_end_index]
#  seq_find = seq_entries.find(',',3)
#  print("trade_inq_info: " + seq_entries)
#  strt = 3
#  next_msg_seq_no = seq_entries[strt:seq_find]
#  print("seq_str: " + next_msg_seq_no + " seq_find: " + str(seq_find))
#  no_of_entries = int(seq_entries[seq_find+1:])
#  print("no_of_entries: " + str(no_of_entries))
#
#  if no_of_entries <= 0 :
#    print("ERROR no_of_entries: " + str(no_of_entries))
#    #break
#
#  trade_details = trade_enquiry[info_end_index+1:].replace('^',"\n")
#  lst_find = trade_enquiry.rfind('^')
#  print("last_find: " + str(lst_find))
#  last_entry = trade_enquiry[lst_find+1:]
#
#  time_find1 = last_entry.find(',')
#  time_find2 = last_entry.find(',',time_find1 + 1)
#  time_find3 = last_entry.find(',',time_find2 + 1)
#  time_find4 = last_entry.find(',',time_find3 + 1)
#  if time_find3 == -1 or time_find4 == -1 :
#     print("invalid")
#  lst_entry_time = int(last_entry[time_find3+1:time_find4]) / 65536 + 315532800
#  print("time: " + str(lst_entry_time))
#  print("last_entry: " + last_entry)
#
#   
#
#  filename = "/tmp/trade_file"
#  file1 = open(filename,"w+")
#  file1.write(trade_details)
#  file1.write('\n')
#  file1.close()
#  input('press enter to exit...')
#
#  exit(0)

  n = len(sys.argv)
  if n < 3 :
    print("USAGE: script CM/FO next_seq_no")
    exit(0)
  
  username_ = "5cc6c8d2b5c54183a1b3e89ecb14a9d6"
  password_ = "1d588087aae54dc5af3fe3616f87d6a6"
  usr_pass_comb_ = username_ + ':' + password_
  cm_fo_ = sys.argv[1]
  trade_inquiry_str_ = ''
  if cm_fo_ == "CM":
    trade_inquiry_str_ = "/notis-cm/trades-inquiry"
  else:
    trade_inquiry_str_ = "/inquiry-fo/trades-inquiry"

#  date_ = str(sys.argv[2])
#  hhmm_ = str(sys.argv[3])
  curr_time = datetime.now()
  mnth = str(curr_time.month) if curr_time.month>9 else str('0' + str(curr_time.month))
  day = str(curr_time.day) if curr_time.day>9 else str('0' + str(curr_time.day))
  
  date_ = str(curr_time.year) + mnth + day
  
  HH = str(curr_time.hour) if curr_time.hour>9 else str('0' + str(curr_time.hour))
  MM = str(curr_time.minute) if curr_time.minute>9 else str('0' + str(curr_time.minute))
  hhmm_ = HH + MM
  
  print('date ' + date_ + ' HHMM: ' + hhmm_)
  
  next_seq_no_ = sys.argv[2]

  print("INQUIRY: " + trade_inquiry_str_)
 
  nonce_ = date_ + hhmm_ + "00000:123456"
  print("date_: " + date_ + "hhmm: " + hhmm_)
  print("nonce: " + nonce_)
  base64_nonce_ = get_base64(nonce_)

  token_key_value = get_token(usr_pass_comb_, base64_nonce_)
  token_ = token_key_value['access_token']
  print("TOKEN: " + token_)

  #get Trade export file
  get_tradeexport_file(date_, base64_nonce_, next_seq_no_, token_, trade_inquiry_str_, cm_fo_)
  input('press enter to exit...')

if __name__ == "__main__":
  main()
