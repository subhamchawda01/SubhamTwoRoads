#!/bin/bash 

SPLITTER_EXEC=/home/dvcinfra/nse_currency_data_splitter
MERGER_EXEC=/home/dvcinfra/nse_data_merger

usage () {

  echo "<Usage> <CurrencyDataOrdersFile> <CurrencyDataTradesFile> <Date>" ;
  echo "<Example> /NAS1/data/NSEData/CD/2015/Mar/CDS_Orders_31032015.DAT.gz /NAS1/data/NSEData/CD/2015/Mar/CDS_Trades_31032015.DAT.gz 20150331" ; 
  exit ; 

}

generate_data () {

  $SPLITTER_EXEC $* >/dev/null 2>/dev/null

  for files in `find /home/dvcinfra/NSECDTradeData -type f | grep $3`; do  fname=`echo $files | awk -F"/home/dvcinfra/NSECDTradeData" '{print $2}'`;  cname=`echo $files | awk -F"/" '{print $NF}'` ; $MERGER_EXEC /home/dvcinfra/NSECDOrderData$fname /home/dvcinfra/NSECDTradeData$fname ; mv /home/dvcinfra/MERGED_OUT /home/dvcinfra/NSECurrencyMergedData/$cname ; done

  gzip /home/dvcinfra/NSECurrencyMergedData/* >/dev/null 2>/dev/null ; 

}

main () {

  if [ $# -lt 3 ] ;
  then

     usage ;

  else 
     
     generate_data $* 

  fi 

}

main $* ; 
