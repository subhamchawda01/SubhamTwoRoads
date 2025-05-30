#!/usr/bin/env python
# -*- coding: utf-8 -*-

import requests
import sys

import json
import urllib3
urllib3.disable_warnings()

# Constant
input_date=sys.argv[1]

url = 'https://efs.euronext.com/api/authenticate' 
headers = {'content-type': 'application/json', 'Accept': 'application/json'}
json_={"username":"P_4225_U_Gaurav_Chakravorty", "password":"EACS2PD#c"}
r = requests.post(url, json=json_, headers=headers, verify=False)

print r.status_code
if r.status_code != 200:
    raise RuntimeError('Error during authentication: exiting')

#use cookie and access tokent to download file
cookie_ = r.headers['Set-Cookie']
content = r.json()
access_token = content["access_token"]

#download the commodity standing data file
commodity_file_url = 'https://efs.euronext.com/api/files/OptiqMDG/Production/Commodities/Current/OptiqMDG_Production_DerivativesStandingDataFile_Commodities_'+str(input_date)+'.xml'
headers_ = {'Authorization': 'Bearer %s' % access_token, 'cookie': cookie_}
r = requests.get(commodity_file_url, headers=headers_)

print r.status_code
if r.status_code != 200:
    raise RuntimeError('Could not download commodity standing data file: exiting')

with open("/spare/local/files/LIFFE/commodity_std_data_file.xml", "w") as file:
    file.write(r.content)

#download the future standing data file
future_std_data_file_url = 'https://efs.euronext.com/api/files/OptiqMDG/Production/Futures/Current/OptiqMDG_Production_DerivativesStandingDataFile_Futures_'+str(input_date)+'.xml'
headers_ = {'Authorization': 'Bearer %s' % access_token, 'cookie': cookie_}
r = requests.get(future_std_data_file_url, headers=headers_)

print r.status_code 
if r.status_code != 200:
    raise RuntimeError('Could not download future standing data file: exiting')

with open("/spare/local/files/LIFFE/future_std_data_file.xml", "w") as file:
    file.write(r.content)



