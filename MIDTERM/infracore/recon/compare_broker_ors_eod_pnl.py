from os.path import expanduser
from decimal import *
import subprocess
import os
import time
from time import gmtime, strftime
import sys

home = expanduser("~")

BROKER_PNL_FILE=sys.argv[1]
OUR_PNL_FILE=sys.argv[2]
BROKER_NAME=sys.argv[3]
DATE=sys.argv[4]

symbol_pnl_map={}
symbol_pos_map={}
symbol_vol_map={}
exchange_pnl_map={}
exchange_vol_map={}

xt_in_xtyt_spread =10;
yt_in_xtyt_spread =34;
symbol_to_expiry_month_={}
symbol_to_expiry_month_["F"] = "01";
symbol_to_expiry_month_["G"] = "02";
symbol_to_expiry_month_["H"] = "03";
symbol_to_expiry_month_["J"] = "04";
symbol_to_expiry_month_["K"] = "05";
symbol_to_expiry_month_["M"] = "06";
symbol_to_expiry_month_["N"] = "07";
symbol_to_expiry_month_["Q"] = "08";
symbol_to_expiry_month_["U"] = "09";
symbol_to_expiry_month_["V"] = "10";
symbol_to_expiry_month_["X"] = "11";
symbol_to_expiry_month_["Z"] = "12";

with open(OUR_PNL_FILE) as f:
  lines = f.read().splitlines()
  f.close()
    
for val in lines:
  words=val.split("|");
  if (len(words) == 8):
    symbol=words[1].strip()
    
    words_2=words[2].split(":")
    pnl=Decimal(words_2[1].strip())
   
    words_2=words[3].split(":")
    pos=Decimal(words_2[1].strip())
    
    words_2=words[4].split(":")
    vol=Decimal(words_2[1].strip())
    
 #   print str(symbol) + " " + str(pnl) + " " + str(pos) + " " + str(vol)
    
    if not (symbol in symbol_pnl_map) :
      symbol_pnl_map[symbol] = {}
      symbol_pos_map[symbol] = {}
      symbol_vol_map[symbol] = {}
    
    symbol_pnl_map[symbol]=pnl
    symbol_pos_map[symbol]=pos
    symbol_vol_map[symbol]=vol
    
  elif (len(words) == 5):
    exchange=words[0].strip()
    
    words_2=words[2].split(":")
    pnl=Decimal(words_2[1].strip())
    
    words_2=words[3].split(":")
    vol=Decimal(words_2[1].strip())
    
    if not (exchange in exchange_pnl_map) :
      exchange_pnl_map[exchange] = {}
      exchange_vol_map[exchange] = {}
    
    exchange_pnl_map[exchange] = pnl
    exchange_vol_map[exchange] = vol

for key in symbol_pnl_map:
  if ("XT" in key and len(key) > 8): 	#xt-yt spread
    symbol2_pos = key.rfind("-");
    symbol_1 = key[:symbol2_pos];
    symbol_1 = "XT201"+symbol_1[3]+ symbol_to_expiry_month_[symbol_1[2]];
    symbol_2 = key[(symbol2_pos+1):];
    symbol_2 = "YT201"+symbol_2[3]+ symbol_to_expiry_month_[symbol_2[2]];

    if not (symbol_1 in symbol_pos_map):
      symbol_pos_map[symbol_1] = 0
      symbol_vol_map[symbol_1] = 0
      
    symbol_pos_map[symbol_1]-= (xt_in_xtyt_spread * symbol_pos_map[key]);
    symbol_vol_map[symbol_1]+= (xt_in_xtyt_spread * symbol_vol_map[key]);
    
    if not (symbol_2 in symbol_pos_map):
      symbol_pos_map[symbol_2] = 0
      symbol_vol_map[symbol_2] = 0

    symbol_pos_map[symbol_2]+= (yt_in_xtyt_spread * symbol_pos_map[key]);
    symbol_vol_map[symbol_2]+= (yt_in_xtyt_spread * symbol_vol_map[key]);

    exchange_vol_map["ASX"] += ((yt_in_xtyt_spread + xt_in_xtyt_spread -1) * symbol_vol_map[key]);
    
  if ("BAX" in key and len(key) > 6) :
    symbol2_pos = key.rfind("BAX");
    symbol_1 = key[:symbol2_pos];
    symbol_1 = symbol_1[:4] + symbol_1[5:]
    symbol_2 = key[symbol2_pos:];
    symbol_2 = symbol_2[:4] + symbol_2[5:]
    
    if not (symbol_1 in symbol_pos_map):
      symbol_pos_map[symbol_1] = 0
      symbol_vol_map[symbol_1] = 0
      
    symbol_pos_map[symbol_1]+= symbol_pos_map[key];
    symbol_vol_map[symbol_1]+= symbol_vol_map[key];
    
    if not (symbol_2 in symbol_pos_map):
      symbol_pos_map[symbol_2] = 0
      symbol_vol_map[symbol_2] = 0
      
    symbol_pos_map[symbol_2]-= symbol_pos_map[key];
    symbol_vol_map[symbol_2]+= symbol_vol_map[key];
    
    exchange_vol_map["TMX"] += symbol_vol_map[key];
  
  if ("_VX" in key) :
    symbol2_pos = key.rfind("_");
    symbol_1 = key[:symbol2_pos];
    symbol_1 = "VX201"+ symbol_1[3] + symbol_to_expiry_month_[symbol_1[2]];
    symbol_2 = key[(symbol2_pos+1):];
    symbol_2 = "VX201"+ symbol_2[3] + symbol_to_expiry_month_[symbol_2[2]];
    
    if not (symbol_1 in symbol_pos_map):
      symbol_pos_map[symbol_1] = 0
      symbol_vol_map[symbol_1] = 0
      
    symbol_pos_map[symbol_1]-= symbol_pos_map[key];
    symbol_vol_map[symbol_1]+= symbol_vol_map[key];
    
    if not (symbol_2 in symbol_pos_map):
      symbol_pos_map[symbol_2] = 0
      symbol_vol_map[symbol_2] = 0
      
    symbol_pos_map[symbol_2]+= symbol_pos_map[key];
    symbol_vol_map[symbol_2]+= symbol_vol_map[key];
    
    exchange_vol_map["CFE"] += symbol_vol_map[key];

for key in symbol_pos_map:
  if not key in symbol_pnl_map:
    symbol_pnl_map[key]=0;
   
with open(BROKER_PNL_FILE) as f:
  lines = f.read().splitlines()
  f.close()

print ("<tr><th>Symbol/Exchange</th><th>ORS_pnl</th><th>ORS_volume</th><th>"+BROKER_NAME+"_pnl</th><th>"+BROKER_NAME+"_volume</th><th>Date</th></tr>")

for val in lines:
  words=val.split("|");
  if (len(words) == 8):
    symbol=words[1].strip()
     
    words_2=words[2].split(":")
    pnl=Decimal(words_2[1].strip())
    
    words_2=words[3].split(":")
    pos=Decimal(words_2[1].strip())
    
    words_2=words[4].split(":")
    vol=Decimal(words_2[1].strip())
      
    #ignoring overnight positions
    if vol == 0 :
      continue;

    if not (symbol in symbol_pnl_map) :
      print ("<tr bgcolor='#FF0000'><td>"+symbol+"</td><td>"+'not found'+"</td><td>"+'not found'+"</td><td>"+str(pnl)+"</td><td>"+str(vol)+"</td><td>"+DATE+"</td></tr>")
    	
    elif not ((abs(symbol_pnl_map[symbol]-pnl) < 1 or "VX" in symbol or "BAX" in symbol or "XT" in symbol or "YT" in symbol) and symbol_pos_map[symbol] == pos and symbol_vol_map[symbol] == vol) :
      print ("<tr bgcolor='#FF0000'><td>"+symbol+"</td><td>"+str(symbol_pnl_map[symbol])+"</td><td>"+str(symbol_vol_map[symbol])+"</td><td>"+str(pnl)+"</td><td>"+str(vol)+"</td><td>"+DATE+"</td></tr>")
    #elif(symbol_pos_map[symbol] != 0) :
     # print ("Open positions for " + symbol + ": " + str(symbol_pos_map[symbol]));
      
  elif (len(words) == 5):
    exchange=words[0].strip()
    
    words_2=words[2].split(":")
    pnl=Decimal(words_2[1].strip())
    
    words_2=words[3].split(":")
    vol=Decimal(words_2[1].strip())
    
    if vol == 0 or exchange == "TOTAL" :
      continue;
    
    if not (exchange in exchange_pnl_map) :
      print ("<tr bgcolor='#FF0000'><td>"+exchange+"</td><td>"+'not found'+"</td><td>"+'not found'+"</td><td>"+str(pnl)+"</td><td>"+str(vol)+"</td><td>"+DATE+"</td></tr>") 
      
    elif not ((abs(exchange_pnl_map[exchange]-pnl) < 10 or "TMX" in exchange or "ASX" in exchange or "CFE" in exchange) and exchange_vol_map[exchange] == vol) :
      print ("<tr bgcolor='#FF0000'><td>"+exchange+"</td><td>"+str(exchange_pnl_map[exchange])+"</td><td>"+str(exchange_vol_map[exchange])+"</td><td>"+str(pnl)+"</td><td>"+str(vol)+"</td><td>"+DATE+"</td></tr>")
            
    else :
      print ("<tr><td>Exchange_"+exchange+"</td><td>"+str(exchange_pnl_map[exchange])+"</td><td>"+str(exchange_vol_map[exchange])+"</td><td>"+str(pnl)+"</td><td>"+str(vol)+"</td><td>"+DATE+"</td></tr>")

