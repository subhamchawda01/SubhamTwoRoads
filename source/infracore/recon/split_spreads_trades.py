from os.path import expanduser
from decimal import *
import subprocess
import os
import time
from time import gmtime, strftime
import sys

home = expanduser("~")

ORS_TRADE_FILE=sys.argv[1]


symbol_pnl_map={}
symbol_pos_map={}
symbol_vol_map={}
exchange_pnl_map={}
exchange_vol_map={}

xt_in_xtyt_spread = Decimal(sys.argv[2]);
yt_in_xtyt_spread = Decimal(sys.argv[3]);
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

with open(ORS_TRADE_FILE) as f:
  lines = f.read().splitlines()
  f.close()
    
for val in lines:
  words=val.split("\x01");
  if (len(words) == 9):
    symbol=words[0].strip()
    
    side=Decimal(words[1].strip())
    qty=Decimal(words[2].strip())
   
    px=words[3].strip()
    saos=words[4].strip()
    
    var=words[5].strip()
    time=words[6].strip()
    saci=words[7].strip()
    
    if ("XT" in symbol and len(symbol) > 8): 	#xt-yt spread
      symbol2_pos = symbol.rfind("-");
      symbol_1 = symbol[:symbol2_pos];
      symbol_1 = "XT201"+symbol_1[3]+ symbol_to_expiry_month_[symbol_1[2]];
      symbol_2 = symbol[(symbol2_pos+1):];
      symbol_2 = "YT201"+symbol_2[3]+ symbol_to_expiry_month_[symbol_2[2]];
      print (str(symbol_1) + " " + str(1-side) + " " + str(xt_in_xtyt_spread * qty) + " " + str(px) + " " + str(saos) + " "+str(var) + " "+ str(time)  + " "+ str(saci));
      print (str(symbol_2) + " " + str(side) + " " + str(yt_in_xtyt_spread * qty) + " " + str(px) + " " + str(saos) + " "+str(var) + " "+ str(time)  + " "+ str(saci)); 
   
    elif ("BAX" in symbol and len(symbol) > 6) :
      symbol2_pos = symbol.rfind("BAX");
      symbol_1 = symbol[:symbol2_pos];
      symbol_1 = symbol_1[:4] + symbol_1[5:]
      symbol_2 = symbol[symbol2_pos:];
      symbol_2 = symbol_2[:4] + symbol_2[5:]
      
      print (str(symbol_1) + " " + str(side) + " " + str(qty) + " " + str(px) + " " + str(saos) + " "+str(var) + " "+ str(time)  + " "+ str(saci));
      print (str(symbol_2) + " " + str(1-side) + " " + str(qty) + " " + str(px) + " " + str(saos) + " "+str(var) + " "+ str(time)  + " "+ str(saci));         

    elif ("_VX" in symbol) :
      symbol2_pos = symbol.rfind("_");
      symbol_1 = symbol[:symbol2_pos];
      symbol_1 = "VX201"+ symbol_1[3] + symbol_to_expiry_month_[symbol_1[2]];
      symbol_2 = symbol[(symbol2_pos+1):];
      symbol_2 = "VX201"+ symbol_2[3] + symbol_to_expiry_month_[symbol_2[2]];
      print (str(symbol_1) + " " + str(1-side) + " " + str(qty) + " " + str(px) + " " + str(saos) + " "+str(var) + " "+ str(time)  + " "+ str(saci));
      print (str(symbol_2) + " " + str(side) + " " + str(qty) + " " + str(px) + " " + str(saos) + " "+str(var) + " "+ str(time)  + " "+ str(saci));                    

    else :
      print (str(symbol) + " " + str(side) + " " + str(qty) + " " + str(px) + " " + str(saos) + " "+str(var) + " "+ str(time)  + " "+ str(saci));

