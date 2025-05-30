#! /bin/bash
#
# created on: 2024-10-22 16:22:54
# processname:
# Author:
#
### BEGIN INIT INFO
# Provides:
# description:
### END INIT INFO

import subprocess
import csv
import os
import sys
import time
import shutil
import argparse
import re
import json
from datetime import datetime, timedelta, date
import pandas as pd

# import yfinance as yf

from ibapi.client import *
from ibapi.wrapper import *
from ibapi.contract import Contract

# File PATHs
bhav_info_file_ = "" # Will be given as input
datasource_exchsymbol_file_ = "" # Will be given as input

RUN_DATE="/home/pengine/prod/live_scripts/get_next_trading_day.sh"

# VARIABLEs
app = None
today_ = date.today().strftime("%Y%m%d")
today=""
# Run the subprocess and capture the output
result = subprocess.run(
    [RUN_DATE, today_],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    universal_newlines=True  # Use this instead of text=True in Python 3.6
)

# Check for successful execution
if result.returncode == 0:
  date_str = result.stdout.strip()

  try:
    date_obj = datetime.strptime(date_str, "%Y%m%d").date()
    today = date_obj.strftime("%Y%m%d")
    ## print(f"Date object: {today}")
  except ValueError as e:
    print(f"Date parsing failed: {e}")
else:
  print(f"Subprocess failed: {result.stderr}")

print(f"============= Fetch Running For {today} =============")

YYYYMM = date.today().strftime("%Y%m")
MMYY = date.today().strftime("%m%y")
DDMM = date.today().strftime("%d%m")
current_date = datetime.now()
next_month_date = current_date + timedelta(days=31)
N_YYYYMM = next_month_date.strftime("%Y%m")

### YYYYMM="202501"
### MMYY="0125"
# When want to run on custom date.
### today = "20241209"
### MMYY = "1224"
### DDMM = "0912"
### current_date = datetime.now()
### next_month_date = current_date + timedelta(days=31)
### N_MMYY = next_month_date.strftime("%m%y")
spot_price = 0
hightest_offset = 0
current_offset = 0

bhav_info_file_path = f"/spare/local/tradeinfo/CBOE_Files/BhavCopy/fo/{MMYY}/{DDMM}fo_0000.md"
refdata_file_path = f"/spare/local/tradeinfo/CBOE_Files/RefData/cboe_fo_{today}_contracts.txt"
datasource_file_path = f"/spare/local/tradeinfo/CBOE_Files/datasource_exchsymbol.txt"
contract_file_path = f"/spare/local/tradeinfo/CBOE_Files/ContractFiles/cboe_contracts.{today}"
contract_info_df = pd.DataFrame(columns=['condID', 'optType', 'optClass', 'expiry', 'strike', 'right'])

# For SPOT
OEBU_FILE="/var/www/html/oebu_info_ibkr.json"
OEBU_MACHINE="10.23.5.101"
LOCAL_OEBU_FILE="/home/dvcinfra/trash/oebu_info_ibkr.json"
LOCAL_OEBU_DIR="/home/dvcinfra/trash/"

# For SYNC
SYNC_SCRIPT="/home/pengine/prod/live_scripts/sync_cboe_fetch_files.sh"

class TradeApp(EWrapper, EClient): 
    def __init__(self): 
      EClient.__init__(self, self) 

    def contractDetails(self, reqId: int, contractDetails: ContractDetails):
      global contract_info_df
      condID = contractDetails.contract.conId
      optType = "IDXOPT" if contractDetails.contract.secType == "OPT" else optType
      optClass = contractDetails.contract.tradingClass
      expiry = contractDetails.contract.lastTradeDate
      strike = int(contractDetails.contract.strike)
      right = "CE" if contractDetails.contract.right == "C" else ("PE" if contractDetails.contract.right == "P" else None)
      # print(condID, optType, optClass, expiry, strike, right, sep=',', end=',\n')

      row = {
          'condID': condID,
          'optType': optType,
          'optClass': optClass,
          'expiry': expiry,
          'strike': strike,
          'right': right
      }

      contract_info_df = pd.concat([contract_info_df, pd.DataFrame([row])], ignore_index=True)

    def contractDetailsEnd(self, reqId: int):
      # Disconnect on 2nd months request
      if reqId == 25:
        self.disconnect()
      ## print(contract_info_df)

      ## Handling Exception
      ## Sometimes TWS API returns contracts of previous
      ## day which we cannot include in contract file since
      ## it messes up shortcode mapping.
      valid_contracts = []
      expired_contracts = []

      for index, row in contract_info_df.iterrows():
        expiry = int(row["expiry"])  # Ensure expiry is treated as an integer
        if expiry < int(today):
          expired_contracts.append(row)
        else:
          valid_contracts.append(row)

      # Write valid contracts to main file
      with open(f"{bhav_info_file_path}", "w") as file:
        for row in valid_contracts:
          file.write(",".join(map(str, row)) + ",\n")

      # Write expired contracts to a separate file
      expired_file_path = f"{bhav_info_file_path}.expired"
      with open(expired_file_path, "w") as file:
        for row in expired_contracts:
          file.write(",".join(map(str, row)) + ",\n")

      ### with open(f"{bhav_info_file_path}", "w") as file:
      ###   for index, row in contract_info_df.iterrows():
      ###     file.write(",".join(map(str, row)) + ",\n")

      shutil.copy(bhav_info_file_path, refdata_file_path)
      print(f"Contract info fetched and file created as {bhav_info_file_path}")


def create_contract(symbol, secType, currency, exchange, last_trade_date):
    contract = Contract()
    contract.symbol = symbol
    contract.secType = secType
    contract.currency = currency
    contract.exchange = exchange
    ## contract.strike = 6495
    contract.lastTradeDateOrContractMonth = last_trade_date
    ## contract.lastTradeDateOrContractMonth = 202412
    return contract

def generate_bhav_and_ref_file():
  app = TradeApp()      
  # app.connect("10.23.5.102", 7497, clientId=522)
  # app.connect("10.23.5.36", 7497, clientId=522)
  # app.connect("10.23.5.29", 7497, clientId=522)
  # app.connect("10.23.5.101", 7793, clientId=522)

  # Live Trading.
  app.connect("10.23.5.102", 7793, clientId=522)

  # Paper Trading.
  ## app.connect("10.23.5.102", 7497, clientId=522)

  time.sleep(1)

  # Create contracts
  contract_1 = create_contract("SPX", "OPT", "USD", "CBOE", f"{YYYYMM}")
  contract_2 = create_contract("SPX", "OPT", "USD", "CBOE", f"{N_YYYYMM}")
  
 #  contract_1 = create_contract("SPX", "OPT", "USD", "CBOE", f"{202505}")
  # contract_2 = create_contract("SPX", "OPT", "USD", "CBOE", f"{202505}")

  # Request contract details
  ## print(f"Request 1: {contract_1.lastTradeDateOrContractMonth}")
  app.reqContractDetails(21, contract_1)
  
  ## print(f"Request 2: {contract_2.lastTradeDateOrContractMonth}")
  app.reqContractDetails(25, contract_2)
  
  # Start app
  app.run()

def generate_datasource_exchsymbol():
  global hightest_offset
  global current_offset

  # For datasource_exchsymbol.txt
  bhav_info_file_ = bhav_info_file_path # Bhavcopy
  datasource_exchsymbol_file_ = datasource_file_path # datasource_

  check_file(bhav_info_file_)
  check_file(datasource_exchsymbol_file_)

  # Load the current datasource_exchsymbol_file_ into a df.
  try:
    datasource_exchsymbol = pd.read_csv(datasource_exchsymbol_file_, header=None, sep=' ')
  except Exception as e:
    print(f"Error loading {datasource_exchsymbol_file_} into DF: {e}")

  # Get highest offset
  for indd, row in datasource_exchsymbol.iterrows(): 
    exch_symbol, datasource_symbol = row
    current_offset = int(exch_symbol[4:])

    if current_offset > hightest_offset:
      hightest_offset = current_offset

  # Load the contract_info_file_ into a df.
  try:
    contract_info = pd.read_csv(bhav_info_file_, header=None) 
    contract_info = contract_info.iloc[:, :-1]
  except Exception as e:
    print(f"Error loading {bhav_info_file_} into DF: {e}")
    sys.exit(1)

  df = pd.DataFrame()

  # incrementing pre hand, so inside loop
  # we can do after printing.
  hightest_offset = hightest_offset + 1

  # Start iterating df:contract_info 
  for index, row in contract_info.iterrows():
    conid, opt_type, underlying, expiry, strike, right = row
    symbol=f"CBOE_{underlying}_{right}_{expiry}_{strike}.00"

    second_column = datasource_exchsymbol.iloc[:, 1]
    if symbol not in second_column.values:
      df = pd.concat([df, pd.DataFrame([[f"CBOE{hightest_offset}", f"CBOE_{underlying}_{right}_{expiry}_{strike}.00"]])], ignore_index=True)
      hightest_offset = hightest_offset + 1

  # Append the df into current datasource_exchsymbol.txt file.
  print("Appending this df to datasource file...")
  print("Taking dated backup of current file...")
  datasource_exchsymbol_file_bkp = f"{datasource_exchsymbol_file_}_"
  
  # Make copy of the current file first.
  shutil.copy(datasource_exchsymbol_file_, datasource_exchsymbol_file_bkp)
  
  # Adding data to datasource_exchsymbol.txt
  df.to_csv(datasource_exchsymbol_file_, mode='a', index=False, header=False, sep=' ')
  print("datasource_exchsymbol.txt updated")

def get_spx_spot_oebu_and_gen_con_file():
  # Sync OEBU info file to get SPOT value
  rsync_cmd = ["rsync", "-rav", "--progress", f"dvcinfra@{OEBU_MACHINE}:{OEBU_FILE}", f"{LOCAL_OEBU_DIR}"]
  result = subprocess.run(rsync_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

  if result.returncode != 0:
    print("Rsync failed!", result.stderr)
    sys.exit(1)
  
  try:
    with open(LOCAL_OEBU_FILE, "r") as f:
      content = f.read()
    # Remove "jsonstr=" and anything before the list
    match = re.search(r'=\s*(\[.*\])', content)
    if not match:
      return "No valid JSON list found after assignment."
    json_data = match.group(1).strip()
    # Parse JSON list
    data = json.loads(json_data)
    # Extract TotalPnl from the first dictionary
    if data and isinstance(data, list) and "SPOT_SPX" in data[0]:
      spot_price = float(data[0]["SPOT_SPX"])
      print(f"SPOT Price: {spot_price}") 
    else:
      return "Key 'TotalPnl' not found"

  except FileNotFoundError:
    return f"File '{file_path}' not found"
  except json.JSONDecodeError as e:
    return f"Invalid JSON format: {e}"

  print(f"Current SPX Spot Price: {spot_price}")

  print(f"Generating Contract File: {contract_file_path}")
  # Load the file into a DataFrame
  df = pd.read_csv(bhav_info_file_path, header=None, names=["optID", "optType", "optClass", "expiry", "strike", "class", "col7"])
  
  # Filter unique rows based on col3 and col4
  unique_df = df.drop_duplicates(subset=["optClass", "expiry"])
  
  # Sort the DataFrame by col3 and col4
  sorted_df = unique_df.sort_values(by=["optClass", "expiry"])
  
  # Create a new DataFrame with the desired output
  output_df = sorted_df[["optType", "optClass", "expiry"]].copy()
  output_df["prev_close"] = spot_price
  output_df["lot"] = 1
  output_df["min_tick"] = 0.05
  output_df["num_itm"] = 400
  output_df["step_value"] = 5.00
  
  output_df.to_csv(contract_file_path, index=False, header=False, sep=" ")
  check_file(contract_file_path)
  print(f"Contract File Generated: {contract_file_path}")

# Check if file exists, if not send mail.
def check_file(file):
  if os.path.exists(file):
    print(f"{file} exists.")
    pass
  else:
    print(f"{file} does not exist.")
    sys.exit(1)  # Exit the script with a non-zero status

if __name__ == "__main__":
  print(f"Runnging CBOE Fetch For {today}")
  app = None

  generate_bhav_and_ref_file()
  generate_datasource_exchsymbol()
  get_spx_spot_oebu_and_gen_con_file()
 
  print("Files generated...")

  print("Running sync script...")
  subprocess.run([SYNC_SCRIPT, today])
  print("Syncing done...")

  print("Script finished")
