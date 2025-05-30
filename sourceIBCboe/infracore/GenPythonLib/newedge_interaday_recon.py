from os.path import expanduser
from decimal import *
import subprocess
import os
import time
from time import gmtime, strftime

home = expanduser("~")

INTERADAY_PNL_SCRIPT= home+"/infracore/scripts/ose_pnl_from_broker.sh"
PRINT_PNL_SCRIPT=home+"/infracore/scripts/print_pnl.sh"
BROKER_PNL_FILE=home+"/interaday_recon/broker_pnl_file"
OUR_PNL_FILE=home+"/interaday_recon/our_pnl_file"
directory=home+"/interaday_recon"
RESULT=home+"/interaday_recon/result"

if not os.path.exists(directory):
  os.makedirs(directory)
 
result_file = open(RESULT, 'w+')
result_file.truncate() #emptying contents of file

last_recon_time=0;

with open(BROKER_PNL_FILE,'w+') as f:
  subprocess.Popen([INTERADAY_PNL_SCRIPT, "R"] ,stdout=f, stderr=f)
  f.close()

symbol_vol_pnl_map={}
symbol_vol_pos_map={}
exchange_vol_pnl_map={}

while(True):
  command = PRINT_PNL_SCRIPT+ " R > " + OUR_PNL_FILE + " 2>/dev/null"
  subprocess.call(command, shell=True)
  time.sleep(5)
 #   print "got current pnl from ors trades file."
  
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
      
      if not (symbol in symbol_vol_pnl_map) :
        symbol_vol_pnl_map[symbol] = {}
        symbol_vol_pos_map[symbol] = {}
      
      symbol_vol_pnl_map[symbol][vol]=pnl
      symbol_vol_pos_map[symbol][vol]=pos
      
    elif (len(words) == 5):
      exchange=words[0].strip()
      
      words_2=words[2].split(":")
      pnl=Decimal(words_2[1].strip())
      
      words_2=words[3].split(":")
      vol=Decimal(words_2[1].strip())
      
      if not (exchange in exchange_vol_pnl_map) :
        exchange_vol_pnl_map[exchange] = {}
      
      exchange_vol_pnl_map[exchange][vol]=pnl
  
  file_info = os.stat(BROKER_PNL_FILE)
  last_modified_time=file_info.st_mtime
  if (last_modified_time != last_recon_time):
    
    with open(BROKER_PNL_FILE) as f:
      lines = f.read().splitlines()
      f.close()
    
    with open(BROKER_PNL_FILE, 'w+') as f:  
      f.truncate()
      f.close()
    
    file_info = os.stat(BROKER_PNL_FILE)
    last_modified_time=file_info.st_mtime
    last_recon_time=last_modified_time
    
    result_file.write(strftime("%Y-%m-%d %H:%M:%S", gmtime()))
    result_file.write("\n\n");
    
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
        
        floor_vol=0;
        ceil_vol=0;
        
        if not (symbol in symbol_vol_pnl_map) :
          result_file.write ("SYMBOL_NOT_FOUND "+ symbol+" BROKER VALUES "+ str(pnl) +" "+ str(pos) + " " + str(vol) + "\n")
          print ("SYMBOL_NOT_FOUND "+ symbol+" BROKER VALUES "+str(pnl)+" "+str(pos) + " " + str(vol) + "\n")
          continue;
        
        for vol_key in symbol_vol_pnl_map[symbol] :
          if vol == vol_key :
            ceil_vol = floor_vol = vol;
            break
          if vol > vol_key and vol_key > floor_vol:
            floor_vol = vol_key
          if vol < vol_key and (vol_key < ceil_vol or ceil_vol == 0):
            ceil_vol = vol_key
            
        if floor_vol == 0 or ceil_vol == 0 : 
          result_file.write ("COULD_NOT_MATCH " + symbol + " " + str(pnl) + " " + str(pos) + " " + str(vol) + "\n")
          print ("COULD_NOT_MATCH " + symbol + " " + str(pnl) + " " + str(pos) + " " + str(vol) + "\n")
          
        elif floor_vol == ceil_vol :
          if pnl == symbol_vol_pnl_map[symbol][vol] and pos == symbol_vol_pos_map[symbol][vol] :
            result_file.write ("PERFECT_MATCH " + symbol + " " + str(pnl) + " " + str(pos) + " " + str(vol) + "\n")
            print ("PERFECT_MATCH " + symbol + " " + str(pnl) + " " + str(pos) + " " + str(vol) + "\n")
          else :
            result_file.write ("DISCREPENCY " + symbol + " OUR VALUES " + str(pnl) + " " + str(pos) + " " + str(vol) + "\n")
            result_file.write ("DISCREPENCY "+symbol+" BROKER VALUES "+symbol_vol_pnl_map[symbol][vol]+" "+symbol_vol_pos_map[symbol][vol]+" "+str(vol)+"\n")
        else :
          check_pnl = ((ceil_vol - vol)*symbol_vol_pnl_map[symbol][floor_vol] + (vol - floor_vol)*symbol_vol_pnl_map[symbol][ceil_vol] )/(ceil_vol - floor_vol)
          check_pos = ((ceil_vol - vol)*symbol_vol_pos_map[symbol][floor_vol] + (vol - floor_vol)*symbol_vol_pos_map[symbol][ceil_vol] )/(ceil_vol - floor_vol)
          
          if abs(pnl-check_pnl) < 20 and abs(pos-check_pos) < 5:
            result_file.write ("INTERPOLATION_MATCH " + symbol + " OUR INTERPOLATED VALUES " + " " + str(check_pnl) + " " + str(check_pos) + " " + str(vol) + "\n")
            result_file.write ("INTERPOLATION_MATCH " + symbol + " BROKER VALUES " + " " + str(pnl) + " " + str(pos) + " " + str(vol) + "\n")
          else :
            result_file.write ("DISCREPENCY_INTERAPOLATION " + symbol + " OUR VALUES " + str(pnl) + " " + str(pos) + " " + str(vol) + "\n")
            result_file.write ("DISCREPENCY_INTERAPOLATION " + symbol + " BROKER VALUES " + str(check_pnl) + " " + str(check_pos) + " " + str(vol) + "\n")
        
        result_file.flush()
        print symbol +" " + str(pnl) +" "+ str(pos) + " " + str(vol)
        
      elif (len(words) == 5):
        exchange=words[0].strip()
        
        words_2=words[2].split(":")
        pnl=Decimal(words_2[1].strip())
        
        words_2=words[3].split(":")
        vol=Decimal(words_2[1].strip())
        
        print exchange +" " + str(pnl) + " " + str(vol)
        
    result_file.write("-------------------------------------------------------------------------\n\n");
    result_file.flush()
              
    #for symbol_key in symbol_vol_pnl_map :
     #   print symbol_key
      #  for vol_key in symbol_vol_pnl_map[symbol_key] :
       #   print "volume: " + str(vol_key) + " pnl: " + str(symbol_vol_pnl_map[symbol_key][vol_key]) + " pos: " + str(symbol_vol_pos_map[symbol_key][vol_key])
      
