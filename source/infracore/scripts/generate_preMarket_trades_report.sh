#!/bin/bash

print_msg_and_exit () {
  echo $* ;
  exit ;
}

PreMarketTradesReport () { 
  PerDayTrade_FILE=$destpath"daily_perMarket_trades_file/"$day".index.html"
  > $PerDayTrade_FILE ;
  `chmod a+rx $PerDayTrade_FILE`
  cat $path"daily_trades_report_header.txt" | sed 's/PreMarket Trades Reports/PreMarket Reports [ '$day' ] /g' >> $PerDayTrade_FILE ;
  echo "<th>Symbol</th><th>Movement in % </th><th>Trade Price in PreMarket</th><th>Closing Price</th></tr></thead><tbody>" >> $PerDayTrade_FILE
  trades_data_file=$file
  [ -f $trades_data_file ] || continue
  while read line
    do
         sym=`echo $line | cut -d' ' -f 1`
         perInc=`echo $line | cut -d' ' -f 2`
         tradePrice=`echo $line | cut -d' ' -f 3`
	 closePrice=`echo $line | cut -d' ' -f 4`
	 
        echo "<tr><td><b>$sym</b></td><td>$perInc</td><td>$tradePrice</td><td>$closePrice</tr>" >> $PerDayTrade_FILE
	done < $trades_data_file

  cat $path"daily_trades_report_footer.txt" >> $PerDayTrade_FILE
}

init () {

  path="/home/dvcinfra/important/PreMarketTrades/scripts/";

  destpath="/var/www/html/PreMarketTrades/"
  readpath="/home/dvcinfra/important/PreMarketTrades/Product_Ratio/"
  computedDate=$path"already_computed"
  REPORTING_FILE=$destpath"index.html" ;
  date=`date +\%Y\%m\%d`
  mkdir -p $destpath ;
  >$REPORTING_FILE ;
  `chmod a+r $REPORTING_FILE`
  cat $path"main_report_header.txt" > $REPORTING_FILE ;
  echo "<th>Date</th>" >> $REPORTING_FILE;
  echo "<th>No of Symbols</th>" >> $REPORTING_FILE ;
#  echo "<th>Max increase in %</th>" >> $REPORTING_FILE ;
#  echo "<th>Max decrease in %</th>" >> $REPORTING_FILE ;
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;
 
  for file in $readpath/*
  do
    day=$(basename $file);
    #echo $file" " $day
    echo "<tr><td><a href =$despath"daily_perMarket_trades_file/"$day".index.html" style="color:blue">$day</a></td>" >>$REPORTING_FILE;
      mkdir -p $destpath"/daily_perMarket_trades_file" ;
      if ! grep -Fxq $day $computedDate
      then 
          PreMarketTradesReport
          echo "For date :" $day
          echo $day >>$computedDate
      elif [[ $day == $date ]]; then
          PreMarketTradesReport
          echo "Computing Todays For date :" $day
          echo $day >>$computedDate
      fi
      echo "<td>`wc -l $file | cut -d' ' -f1`</td>" >> $REPORTING_FILE;
#      maxval=`less $file | sort -k2 -n | tail -n1| cut -d' ' -f2`
#      minval=`less $file | sort -k2 -n | head -n1| cut -d' ' -f2`
#      echo "<td>$maxval</td>" >> $REPORTING_FILE;
#      echo "<td>$minval</td>" >> $REPORTING_FILE;
    echo "</tr>" >> $REPORTING_FILE;
  done
  cat $path"main_report_footer.txt" >> $REPORTING_FILE;
}

init $*
