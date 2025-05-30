#!/bin/bash



SHORTCODE_SYMBOL_EXEC=$HOME/LiveExec/bin/get_exchange_symbol
CALC_NEXT_WEEK_DAY_EXEC=$HOME/LiveExec/bin/calc_next_week_day

#============================================================================================#
TODAY=`date +"%Y%m%d"`;
NEXT_DAY=`$CALC_NEXT_WEEK_DAY_EXEC $TODAY 1`;
EMAIL="Gabriel.Paiva@brasilplural.com,Rodrigo.Haluska@brasilplural.com,nseall@tworoads.co.in"
TEMP_MAIL_FILE="/tmp/mail_roll_file"
#============================= Get Symbol =====================================#

is_today_holiday=`/home/dvcinfra/infracore_install/bin/holiday_manager EXCHANGE BMF $TODAY T`;
is_next_day_holiday=`/home/dvcinfra/infracore_install/bin/holiday_manager EXCHANGE BMF $NEXT_DAY T`;

if [ $is_today_holiday = "1" ] || [ $is_next_day_holiday = "1" ] ; then
     exit;
fi

SHORTCODE="BR_WIN_0";
SYMBOL_CURRENT=`$SHORTCODE_SYMBOL_EXEC $SHORTCODE $TODAY | tr ' ' '~'` ;
SYMBOL_NEXT=`$SHORTCODE_SYMBOL_EXEC $SHORTCODE $NEXT_DAY | tr ' ' '~'` ;

if [ $SYMBOL_CURRENT != $SYMBOL_NEXT ]
then
     echo "$SYMBOL_NEXT from $SYMBOL_CURRENT. Please adjust limits accordingly. This is an automated email, please report errors to nseall@tworoads.co.in" >> $TEMP_MAIL_FILE;
     SUBJECT="Potential rollover for WIN/IND on $NEXT_DAY";
     /bin/mail -s "$SUBJECT" -r "dvcinfra@ny11" "$EMAIL" < $TEMP_MAIL_FILE;    
     `rm $TEMP_MAIL_FILE`;
     exit ;
fi


SHORTCODE="BR_WDO_0";
SYMBOL_CURRENT=`$SHORTCODE_SYMBOL_EXEC $SHORTCODE $TODAY | tr ' ' '~'` ;
SYMBOL_NEXT=`$SHORTCODE_SYMBOL_EXEC $SHORTCODE $NEXT_DAY | tr ' ' '~'` ;

if [ $SYMBOL_CURRENT != $SYMBOL_NEXT ]
then
     echo "$SYMBOL_NEXT from $SYMBOL_CURRENT. Please adjust limits accordingly. This is an automated email, please report errors to nseall@tworoads.co.in" >> $TEMP_MAIL_FILE;
     SUBJECT="Potential rollover for WDO/DOL on $NEXT_DAY";
     /bin/mail -s "$SUBJECT" -r "dvcinfra@ny11" "$EMAIL" < $TEMP_MAIL_FILE;    
     `rm $TEMP_MAIL_FILE`;
     exit ;
fi
