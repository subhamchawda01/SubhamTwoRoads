#!/bin/bash
date_=$1
print_usage_and_exit () {
    echo "$0 YYYYMMDD" ;  
    exit ; 
} 

FTP_HOST='ftp.connect2nse.com'


fetch_stock_security () {

ftp -np $FTP_HOST <<SCRIPT
  user "ftpguest" "FTPGUEST" 
  cd Common
  cd NTNEAT
  binary 
  get security.gz
  quit 
SCRIPT

}


#Main 


FTP_DIR="/home/dvctrader/usarraf/"

cd $FTP_DIR

fetch_stock_security;

scp security.gz dvctrader@10.23.227.83:/spare/local/security${date_}.gz
scp security.gz dvctrader@10.23.227.82:/spare/local/security${date_}.gz
scp security.gz dvctrader@10.23.227.81:/spare/local/security${date_}.gz

#scp -P 22781 security.gz dvctrader@202.189.245.205:/spare/local/security${date_}.gz
#scp -P 22782 security.gz dvctrader@202.189.245.205:/spare/local/security${date_}.gz
#scp -P 22783 security.gz dvctrader@202.189.245.205:/spare/local/security${date_}.gz
# If today is a CD holiday, we look for the last working day for CD and copy the files from that date to Today's dated files

#scp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt dvctrader@10.23.227.62:/spare/local/tradeinfo/NSE_Files/
#scp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day" dvcinfra@10.23.227.62:/spare/local/tradeinfo/NSE_Files/ContractFiles/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_"$next_working_day"_contracts.txt dvcinfra@10.23.227.62:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_"$next_working_day"_contracts.txt dvcinfra@10.23.227.62:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt dvcinfra@10.23.227.62:/spare/local/tradeinfo/NSE_Files/RefData/

#rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.227.83:/spare/local/tradeinfo --delete-after
#rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.230.15:/spare/local/tradeinfo --delete-after
#rsync -ravz --timeout=60 /spare/local/tradeinfo/NSE_Files 10.23.230.16:/spare/local/tradeinfo --delete-after
#scp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt dvctrader@10.23.227.61:/spare/local/tradeinfo/NSE_Files/
#scp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day" dvcinfra@10.23.227.61:/spare/local/tradeinfo/NSE_Files/ContractFiles/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_"$next_working_day"_contracts.txt dvcinfra@10.23.227.61:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_"$next_working_day"_contracts.txt dvcinfra@10.23.227.61:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt dvcinfra@10.23.227.61:/spare/local/tradeinfo/NSE_Files/RefData/

#scp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt dvctrader@10.23.227.62:/spare/local/tradeinfo/NSE_Files/
#scp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day" dvcinfra@10.23.227.62:/spare/local/tradeinfo/NSE_Files/ContractFiles/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_"$next_working_day"_contracts.txt dvcinfra@10.23.227.62:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_"$next_working_day"_contracts.txt dvcinfra@10.23.227.62:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt dvcinfra@10.23.227.62:/spare/local/tradeinfo/NSE_Files/RefData/

#scp /spare/local/tradeinfo/NSE_Files/datasource_exchsymbol.txt dvctrader@10.23.227.61:/spare/local/tradeinfo/NSE_Files/
#scp /spare/local/tradeinfo/NSE_Files/ContractFiles/nse_contracts."$next_working_day" dvcinfra@10.23.227.61:/spare/local/tradeinfo/NSE_Files/ContractFiles/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_fo_"$next_working_day"_contracts.txt dvcinfra@10.23.227.61:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_cd_"$next_working_day"_contracts.txt dvcinfra@10.23.227.61:/spare/local/tradeinfo/NSE_Files/RefData/
#scp /spare/local/tradeinfo/NSE_Files/RefData/nse_eq_"$next_working_day"_contracts.txt dvcinfra@10.23.227.61:/spare/local/tradeinfo/NSE_Files/RefData/


