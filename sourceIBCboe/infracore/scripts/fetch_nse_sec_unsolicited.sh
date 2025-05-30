#!/bin/bash
TEM_FILE_NAME_XLSX='/spare/local/tradeinfo/NSE_Files/SecuritiesUnderUnsolicitedSMS/List_of_Symbol.xlsx'
TEM_FILE_NAME_CSV='/spare/local/tradeinfo/NSE_Files/SecuritiesUnderUnsolicitedSMS/List_of_Symbol.csv'
FINAL_FILE_NAME='/spare/local/tradeinfo/NSE_Files/SecuritiesUnderUnsolicitedSMS/nse_sec_unsolicited_sms_.csv'

download_symbols() {
	wget --referer https://www.nseindia.com/ --user-agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.8; rv:21.0) Gecko/20100101 Firefox/21.0" -O $TEM_FILE_NAME_XLSX https://www.nseindia.com/List_of_Symbol.xlsx
}

download_symbols
if [ -f $TEM_FILE_NAME_XLSX ]; then
  ssconvert -S $TEM_FILE_NAME_XLSX $TEM_FILE_NAME_CSV
  cat $TEM_FILE_NAME_CSV".0" | awk -F',' '(NR==2){colnum=-1;for(i=1;i<=NF;i++)  if($i=="Symbol") colnum=i; }NR!=1&&NR!=2{print $(colnum)}' > $FINAL_FILE_NAME
  rm $TEM_FILE_NAME_XLSX $TEM_FILE_NAME_CSV 
fi
