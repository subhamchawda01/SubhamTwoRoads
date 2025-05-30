#!/bin/bash

#Main 
if [ $# -ne 1 ] ; then
  echo "Called As : NEWSYMBOL[WINPRO]" ;
  exit
fi

SYM=$1
echo "symbol NSE_$SYM"

echo -e "$SYM\t10\t0.5\t1000\t1\t0.5\tPASSONLY\tSTRATEGY" >>/home/dvctrader/LiveConfigs/NSE_SIMPLE_EXEC_PARAMS/mult_param_disp
echo -e "$SYM\t10\t0.5\t1000\t1\t0.5\tPASSONLY\tSTRATEGY" >>/home/dvctrader/LiveConfigs/NSE_SIMPLE_EXEC_PARAMS/mult_param_manual
echo -e "$SYM\t10\t0.5\t1000\t1\t0.5\tPASSONLY\tSTRATEGY" >>/home/dvctrader/LiveConfigs/NSE_SIMPLE_EXEC_PARAMS/mult_param_midterm

echo "ADDDIN TO IND11 /spare/local/files/NSE/MidTermLogs/midterm_data_server_products.txt"
ssh dvcinfra@10.23.227.61 "echo \"$SYM\" >> /spare/local/files/NSE/MidTermLogs/midterm_data_server_products.txt"

echo "ADDING TO IND12"
ssh dvctrader@10.23.227.62 "echo \"$SYM\" >> /spare/local/files/NSE/MidTermLogs/midterm_data_server_products.txt"


echo "ADDING ADDTS to /home/pengine/prod/live_configs/sdv-ind-srv11_addts.cfg"
ssh dvcinfra@10.23.227.61 "echo \"NSE MSFO ADDTRADINGSYMBOL NSE_${SYM}_FUT0 150 150 150 150\" >> /home/pengine/prod/live_configs/sdv-ind-srv11_addts.cfg"
ssh dvcinfra@10.23.227.61 "echo \"NSE MSFO ADDTRADINGSYMBOL NSE_${SYM}_FUT1 150 150 150 150\" >> /home/pengine/prod/live_configs/sdv-ind-srv11_addts.cfg"



