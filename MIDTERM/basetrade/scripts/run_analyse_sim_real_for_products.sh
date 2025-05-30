#!/bin/bash
USAGE="$0 YYYYMMDD [ YESTERDAY ]";
prev_day_exec="$HOME/basetrade_install/bin/calc_prev_week_day ";

if [ $# -ne 1 ] ;
then 
  echo $USAGE;
  exit;
fi

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

export GCC_4_9_ROOT=/apps/gcc_versions/gcc-4_9_install
if [ -d $GCC_4_9_ROOT ] ; then
   export PATH=$GCC_4_9_ROOT/bin:$PATH;
   export LD_LIBRARY_PATH=$GCC_4_9_ROOT/lib64:$LD_LIBRARY_PATH ;
fi



date=$1;
if [ $date == "YESTERDAY" ] ;
then 
  date_cmd_="$prev_day_exec `date +%Y%m%d`";
  date=`$date_cmd_`;
fi
analyse_scrpit_=$HOME/basetrade/ModelScripts/analyse_sim_real_for_product.pl;
TEMP_MAIL_FILE=/tmp/sim_summary_mail.txt;
TEMP_MAIL_BODY=/tmp/sim_summary_body.txt;
>$TEMP_MAIL_FILE;
>$TEMP_MAIL_BODY;
echo "From: nseall@tworoads.co.in " >> $TEMP_MAIL_FILE;
echo "To: diwakar\@circulumvite.com " >> $TEMP_MAIL_FILE;
echo "To: hardik\@circulumvite.com " >> $TEMP_MAIL_FILE;
echo "To: nseall@tworoads.co.in " >> $TEMP_MAIL_FILE;
echo "Subject: Sim Summary " >> $TEMP_MAIL_FILE;
echo "X-Mailer: htmlmail 1.0 " >> $TEMP_MAIL_FILE;
echo "Mime-Version: 1.0 " >> $TEMP_MAIL_FILE;
echo "Content-Type: text/html; charset=US-ASCII " >> $TEMP_MAIL_FILE;

echo "<html><body> " >> $TEMP_MAIL_FILE;
for SHORTCODE in `cat /spare/local/tradeinfo/curr_trade_prod_list.txt`;
#for SHORTCODE in LFR_0 
do
for time_peroid in US_MORN_DAY EU_MORN_DAY EUS_MORN_DAY AS_MORN_DAY
do

echo "$analyse_scrpit_ $SHORTCODE $date $time_peroid"
out=`$analyse_scrpit_ $SHORTCODE $date $time_peroid INVALIDFILE 1 0.35 0.22 0.22 0.1| grep "====\|SIMRESULT\|REALRESULT\|PNL\|CHECK"`;

if [[ X"$out" == "X" ]] ;
then
	echo " not run yesterday..";
else
  echo "============================== $SHORTCODE : $time_peroid ===================================<br/>" >> $TEMP_MAIL_BODY ;
  echo "$out" | tr '\n' '~' | sed 's/~/<br\/>/g' >>  $TEMP_MAIL_BODY;
  echo "===========================================================================================<br/>" >> $TEMP_MAIL_BODY;
fi

prod_to_check_=`echo "$out"| tr '\n' '~' | sed 's/~/<br\/>/g' | grep "CHECK"`;

if  [[ X"$prod_to_check_" == "X" ]] ;
then 
	echo " Sim real is ok .." ;
else
    bad_sim_prods_=$bad_sim_prods_" $SHORTCODE<$time_peroid> " ;
fi

done
done


if [[ X"$bad_sim_prods_" == "X" ]] ;
then 
	echo " no bad sim real products found";	
else 
	echo "Check Sim for these products: $bad_sim_prods_ <br/> <br/> " >>$TEMP_MAIL_FILE;
fi

cat $TEMP_MAIL_BODY >> $TEMP_MAIL_FILE;

echo "</body></html> " >> $TEMP_MAIL_FILE;

#/bin/mail -s "Sim Summary Last Day" -r "DVCTech@circulumvite" "nseall@tworoads.co.in" < $TEMP_MAIL_FILE ;
cat $TEMP_MAIL_FILE;
cat $TEMP_MAIL_FILE | /usr/sbin/sendmail -it ;

rm -rf $TEMP_MAIL_FILE
rm -rf $TEMP_MAIL_BODY

