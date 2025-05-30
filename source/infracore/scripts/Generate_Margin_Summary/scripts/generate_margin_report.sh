#!/bin/bash

MarginReport () { 
  Margin_FILE=$destpath"daily_margin_file/"$day".index.html"
  > $Margin_FILE ;
  `chmod a+rx $Margin_FILE`
  cat $path"daily_margin_header.txt" | sed 's/Margin Report/Margin Report [ '$day' ]/g' >> $Margin_FILE ;
  echo "<th>Time</th><th>Margin</th></tr></thead><tbody>" >> $Margin_FILE
  marg_file=$file
  [ -f $marg_file ] || continue
  while read line
    do
         sym=`echo $line | cut -d' ' -f 1`
         perInc=`echo $line | cut -d' ' -f 2`

        echo "<tr><td><b>$sym</b></td><td>$perInc</td></tr>" >> $Margin_FILE
    done < $marg_file

  cat $path"daily_margin_footer.txt" >> $Margin_FILE
}

init () {

  path="/home/dvcinfra/important/Generate_Margin_Summary/scripts/";

  destpath="/var/www/html/daily_margin/"
  readpath="/home/dvcinfra/important/Generate_Margin_Summary/gen_margin_data/"
  REPORTING_FILE=$destpath"index.html" ;
  date=`date +\%Y\%m\%d`
  mkdir -p $destpath ;
  >$REPORTING_FILE ;
  `chmod a+r $REPORTING_FILE`
  cat $path"main_report_header.txt" > $REPORTING_FILE ;
  echo "<th>Date</th>" >> $REPORTING_FILE;
  echo "<th>Max Margin</th>" >> $REPORTING_FILE ;
  echo "</tr></thead><tbody>" >> $REPORTING_FILE ;
 
  for file in $readpath/*
  do
    day=$(basename $file);
    #echo $file" " $day
    echo "<tr><td><a href =$despath"daily_margin_file/"$day".index.html" style="color:blue">$day</a></td>" >>$REPORTING_FILE;
      mkdir -p $destpath"/daily_margin_file" ;
          MarginReport
      echo "<td>`sort -k2 -n $file | tail -1 | cut -d' ' -f2`</td>" >> $REPORTING_FILE;
    echo "</tr>" >> $REPORTING_FILE;
  done
  cat $path"main_report_footer.txt" >> $REPORTING_FILE;
}

init $*
