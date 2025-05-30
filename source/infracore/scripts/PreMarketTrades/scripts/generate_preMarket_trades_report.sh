#!/bin/bash

PreMarketReport () { 
  Margin_FILE=$destpath"daily_perMarket_trades_file/"$day".index.html"
  > $Margin_FILE ;
  `chmod a+rx $Margin_FILE`
  cat $path"daily_header.txt" | sed 's/PreMarket Report/PreMarket Report [ '$day' ]/g' >> $Margin_FILE ;
  echo "<th>Symbol</th><th>Movement in %</th><th>Trade Price in Premarket</th><th>Closing Price</th></tr></thead><tbody>" >> $Margin_FILE
  marg_file=$file
  [ -f $marg_file ] || continue
  while read line
    do
         sym=`echo $line | cut -d' ' -f 1`
         f2=`echo $line | cut -d' ' -f 2`
         f3=`echo $line | cut -d' ' -f 3`
         f4=`echo $line | cut -d' ' -f 4`

        echo "<tr><td><b>$sym</b></td><td>$f2</td><td>$f3</td><td>$f4</td></tr>" >> $Margin_FILE
    done < $marg_file

  cat $path"daily_footer.txt" >> $Margin_FILE
}

init () {

  path="/home/dvcinfra/important/PreMarketTrades/scripts/";

  destpath="/var/www/html/PreMarketTrades/"
  readpath="/home/dvcinfra/important/PreMarketTrades/Product_Ratio/"
  REPORTING_FILE=$destpath"index.html" ;
  date=`date +\%Y\%m\%d`
  mkdir -p $destpath ;
  >$REPORTING_FILE ;
  `chmod a+r $REPORTING_FILE`
  cat $path"main_report_header.txt" > $REPORTING_FILE ;
  echo "<th>Date</th>" >> $REPORTING_FILE;
  echo "<th>No of Symbols</th>" >> $REPORTING_FILE ;
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;
 
  for file in $readpath/*
  do
    day=$(basename $file);
    echo "<tr><td><a href ="daily_perMarket_trades_file/"$day".index.html" style="color:blue">$day</a></td>" >>$REPORTING_FILE;
    echo "<td>`wc -l $file | cut -d' ' -f1`</td>" >> $REPORTING_FILE;
    echo "</tr>" >> $REPORTING_FILE;
    [[ $day != $date ]] && [[ -f $destpath"daily_perMarket_trades_file/"$day".index.html" ]] && { continue;}
      mkdir -p $destpath"/daily_perMarket_trades_file" ;
    echo $file" " $day
          PreMarketReport
  done
  cat $path"main_report_footer.txt" >> $REPORTING_FILE;
}

init $*
