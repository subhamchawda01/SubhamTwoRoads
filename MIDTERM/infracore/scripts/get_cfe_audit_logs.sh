#!/bin/bash

# This script was made when NAS servers were not accessiable. So order binary files should be given as input to calculate audit logs.
# Logs for required date have to be copied into FOLDER/YYMMDD/ from /spare/local/ORSBCAST/CFE and PATHS have to be changed accordingly.
# Mode O for orders and T for trades.
# EXPIRY_CONFIG contains expiry dates of products. Each line in this file should have space seperated Symbol and its expiry data.
# TRADE_CONFIG contains map between branch_seq and (Trade_high, Trade_low, Order_high, Order_low). All of them in same order seperated by space.
# TRADE_CONFIG can be generated from drop_copy logs

FORMAT="script YYYYMMDD O/T"
if [ $# -lt 2 ] ;
then
  echo $FORMAT;
  exit 0;
fi

YYYYMMDD=$1;
MODE=$2;
OUT_FILE=$2_OUT_FILE;
SCRIPT=/home/dvcinfra/infracore_install/bin/print_trades_for_cfe_audit_trail
EXPIRY_CONFIG=/home/dvcinfra/sasidhar/cfe/expiry_config
BIN_FILES_PATH=/home/dvcinfra/sasidhar/cfe/$YYYYMMDD
TRADE_CONFIG=/home/dvcinfra/sasidhar/cfe/$YYYYMMDD/trade_id_config
cd $BIN_FILES_PATH

bin_files=( `find ./ -name "*VX*$YYYYMMDD*"` )

> $OUT_FILE

for i in "${bin_files[@]}"
do
  echo "Getting logs from $i"
  $SCRIPT "VX_0" $YYYYMMDD $MODE $EXPIRY_CONFIG $TRADE_CONFIG $BIN_FILES_PATH/$i >> $OUT_FILE
done


