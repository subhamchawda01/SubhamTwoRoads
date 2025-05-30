from os.path import expanduser
from decimal import *
import subprocess
import os
import time
from time import gmtime, strftime
import sys

home = expanduser("~")

BROKER_COMM_FILE=sys.argv[1]
OUR_COMM_FILE=sys.argv[2]
BROKER_NAME=sys.argv[3]
DATE=sys.argv[4]

symbol_broker_commission_map={}
symbol_commission_map={}

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

with open(OUR_COMM_FILE) as f:
  lines = f.read().splitlines()
  f.close()
    
for val in lines:
  words=val.split("|");
  if (len(words) == 2):
    symbol=words[0].strip()
    comm=Decimal(words[1].strip())
        
    if not (symbol in symbol_commission_map) :
      symbol_commission_map[symbol] = {}
    
    symbol_commission_map[symbol]=comm

#not sure how commissions will be divided in case of spreads      
bax_commish_first_year=Decimal("0.11")
bax_commish_later_year=Decimal("0.05")
bax_fixed_commish=Decimal("0.15")
numerator=Decimal("0")
denominator=Decimal("1")
for key in symbol_commission_map:
  if ("BAX" in key and len(key) > 6) :
    symbol2_pos = key.rfind("BAX");
    symbol_1 = key[:symbol2_pos];
    symbol_1 = symbol_1[:4] + symbol_1[5:]
    symbol_2 = key[symbol2_pos:];
    symbol_2 = symbol_2[:4] + symbol_2[5:]
    #get shortcode for symbol1
    p = subprocess.Popen(["/home/dvcinfra/basetrade_install/bin/get_shortcode_for_symbol", str(symbol_1), str(DATE)], stdout=subprocess.PIPE)
    shc_1, err = p.communicate()
    expiry_1=Decimal(shc_1.split("_")[1].strip());
    p = subprocess.Popen(["/home/dvcinfra/basetrade_install/bin/get_shortcode_for_symbol", str(symbol_2), str(DATE)], stdout=subprocess.PIPE)
    shc_2, err = p.communicate();
    expiry_2=Decimal(shc_2.split("_")[1].strip());
    if (expiry_1 < 4):
      numerator=bax_commish_first_year
    else:
      numerator=bax_commish_later_year

    if (expiry_2 < 4): 
      denominator=bax_commish_first_year
    else:
      denominator=bax_commish_later_year

    ratio=(numerator+bax_fixed_commish)/(denominator+bax_fixed_commish)
    
    if not (symbol_2 in symbol_commission_map):
      symbol_commission_map[symbol_2] = 0

    symbol2_commish=symbol_commission_map[key]/(ratio+1);
    symbol_commission_map[symbol_2]+= (symbol2_commish);
    
    if not (symbol_1 in symbol_commission_map):
      symbol_commission_map[symbol_1] = 0

    symbol_commission_map[symbol_1]+= (symbol2_commish*ratio);
      
  if ("_VX" in key) :
    symbol2_pos = key.rfind("_");
    symbol_1 = key[:symbol2_pos];
    symbol_1 = "VX201"+ symbol_1[3] + symbol_to_expiry_month_[symbol_1[2]];
    symbol_2 = key[(symbol2_pos+1):];
    symbol_2 = "VX201"+ symbol_2[3] + symbol_to_expiry_month_[symbol_2[2]];
    
    if not (symbol_1 in symbol_commission_map):
      symbol_commission_map[symbol_1] = 0

    symbol_commission_map[symbol_1]+= symbol_commission_map[key]/2;

    if not (symbol_2 in symbol_commission_map):
      symbol_commission_map[symbol_2] = 0

    symbol_commission_map[symbol_2]+= symbol_commission_map[key]/2;
   
with open(BROKER_COMM_FILE) as f:

  lines = f.read().splitlines()
  f.close()

for val in lines:
  words=val.split("|");
  if (len(words) == 2):
    symbol=words[0].strip()
    comm=Decimal(words[1].strip())
        
    if not (symbol in symbol_broker_commission_map) :
      symbol_broker_commission_map[symbol] = 0
    
    symbol_broker_commission_map[symbol]+= comm

for key in symbol_broker_commission_map:
  if (key in "USD000TODTOM") :
      if ("USD000UTSTOM" in symbol_broker_commission_map) :
         symbol_broker_commission_map["USD000UTSTOM"]+= ( symbol_broker_commission_map[key]/200);
      else :
	 symbol_broker_commission_map["USD000UTSTOM"] = ( symbol_broker_commission_map[key]/200);

      if ("USD000000TOD" in symbol_broker_commission_map) :
         symbol_broker_commission_map["USD000000TOD"]+= (symbol_broker_commission_map[key]/200);
      else :
	 symbol_broker_commission_map["USD000000TOD"] =  ( symbol_broker_commission_map[key]/200);

#print "------------------------------------------------------------"  
print ("{0:<20} {1:<20} {2:<20} {3:<20}".format('SYMBOL',BROKER_NAME+'_Commission',' ORS_Commission','Diff (OC - BC)') +"\n")

#TODO: assumption broker splits VX spreads
for symbol in symbol_broker_commission_map:
    comm=abs(symbol_broker_commission_map[symbol]);
    if not (symbol in symbol_commission_map) :
      print ("{0:<20} {1:<20} {2:<20}".format(symbol,"{0:.3f}".format(comm),'SYMBOL_NOT_PRESENT_IN_ORS'))
    
    else :
    #elif ("BAX" in symbol) :
      print ("{0:<20} {1:<20} {2:<20} {3:<20}".format(symbol,"{0:.3f}".format(comm),"{0:.3f}".format(symbol_commission_map[symbol]),
            "{0:.3f}".format(symbol_commission_map[symbol] - comm)))

print ("\n")

#print ("\nSymbols not present in broker's file:\n")
#print ("{:<20} {1:<20}".format('SYMBOL','ORS Commission') + "\n")
#for key in symbol_commission_map:
#  if not (key in symbol_broker_commission_map) :
#    print ("{0:<20} {1:<20}".format(key,str(symbol_commission_map[key])))   
