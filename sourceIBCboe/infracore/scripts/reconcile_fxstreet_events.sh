#!/bin/bash

#TODO currently unused later on can specify time range etc.
USAGE1="$0 ALL"
EXAMP1="$0 ALL"

if [ $# -ne 1 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

TRDHR=$1 ;

#=================Switch To Master==============#

cd $HOME/master/infracore/
git checkout master
git pull

#===============================================#

#=================Events In use, Live=================#

TODAY=`date +"%m%d%Y"`
MERGED_EVENTS_CSV=$HOME/master/infracore/SysInfo/FXStreetEcoReports/fxstreet_$TODAY"_"$TODAY".csv"
TEMP_LIVE_FILE=/tmp/live_fxstreet_economic_events.html

touch $TEMP_LIVE_FILE
>$TEMP_LIVE_FILE

#FX_EXEC=$HOME/master/infracore_install/bin/parse_fxstreet_economic_events 
FX_EXEC=$HOME/master/infracore/scripts/parse_fxstreet_csv.sh
SLACK_EXEC=/home/pengine/prod/live_execs/send_slack_notification
LIVE_EVENTS_CSV=/tmp/live_fxstreet_economic_events.csv

curl 'http://calendar.fxstreet.com/eventdate/?f=csv&v=2&timezone=UTC&rows=undefined&view=current&countrycode=AU%2CCA%2CCN%2CEMU%2CFR%2CDE%2CGR%2CIN%2CIT%2CJP%2CNZ%2CPT%2CES%2CCH%2CUK%2CUS&volatility=0&culture=en-US&columns=CountryCurrency%2CCountdown' -H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8' -H 'Connection: keep-alive' -H 'Accept-Encoding: gzip, deflate, sdch' -H 'Referer: http://www.fxstreet.com/economic-calendar/' -H 'Accept-Language: en-US,en;q=0.8' -H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.91 Safari/537.36' --compressed > $TEMP_LIVE_FILE
#wget -nv -O $TEMP_LIVE_FILE "http://www.fxstreet.com/economic-calendar/"
#wget -O $TEMP_LIVE_FILE --user-agent="Mozilla" http://www.investing.com/economic-calendar/

#count events available from html page
TOTAL_EVENTS=`grep -v "DateTime" $TEMP_LIVE_FILE | wc -l`
#TOTAL_EVENTS=`egrep "fxec_oddRow|fxec_evenRow" $TEMP_LIVE_FILE | wc -l`
#TOTAL_EVENTS=`egrep "event_timestamp" $TEMP_LIVE_FILE | wc -l`

#page not available, send notification and exit

if [ $TOTAL_EVENTS -lt 1 ]
then 

#echo "@"`date +%s` "Total Events Fetched : "$TOTAL_EVENTS | /bin/mail -s "FxStreet WebPage Unavailable, Reconciliation Failed, Will Try Again On Next Run" -r "ravi.parikh@tworoads.co.in" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in" ;
#    echo "@"`date +%s` "Total Events Fetched : "$TOTAL_EVENTS | /bin/mail -s "FxStreet WebPage Unavailable, Reconciliation Failed, Will Try Again in 15 mins..." -r "ravi.parikh@tworoads.co.in" "ravi.parikh@tworoads.co.in" ;
    rm -rf $LIVE_EVENTS_CSV $TEMP_LIVE_FILE ;
    exit ;

fi

#event parser
$FX_EXEC $TEMP_LIVE_FILE $LIVE_EVENTS_CSV

#===============================================#

#=================Filter Out Numbers========================#

CURRENT_EVENTS_FILTERED_FILE=/tmp/inuse_filtered_fxstreet_economic_events.txt
LIVE_EVENTS_FILTERED_FILE=/tmp/live_filtered_fxstreet_economic_events.txt

cat $MERGED_EVENTS_CSV | grep -v "country" | awk -F"\",\"" '{print $1 "#" $2 "#" $3 "#" $7}' | tr -dc "[:alnum:][:space:]#" | sort > $CURRENT_EVENTS_FILTERED_FILE
cat $LIVE_EVENTS_CSV | grep -v "country" | awk -F"\",\"" '{print $1 "#" $2 "#" $3 "#" $7}' | tr -dc "[:alnum:][:space:]#" | sort > $LIVE_EVENTS_FILTERED_FILE

#===========================================================#

#=================Alert On Failed Reconciliation================#

MAIL_FILE=/home/pengine/fxstreet_mail_notes/events_reconciliation_mail_file.txt
SLACK_FILE=/home/pengine/fxstreet_mail_notes/events_reconciliation_mail_file_slack.txt
EVENTS_ADDED_FILE=/home/pengine/fxstreet_mail_notes/events_added_$TODAY".txt"
EVENTS_REMOVED_FILE=/home/pengine/fxstreet_mail_notes/events_removed_$TODAY".txt"
EVENTS_CHANGE_FILE=/home/pengine/fxstreet_mail_notes/events_change_$TODAY".txt"

TEMP_ADDED_FILE=/tmp/fxstreet_added_files_temp.txt
TEMP_REMOVED_FILE=/tmp/fxstreet_removed_files_temp.txt

diff -wu $CURRENT_EVENTS_FILTERED_FILE $LIVE_EVENTS_FILTERED_FILE > $MAIL_FILE

LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then
    
    cat $MAIL_FILE | grep "^+" | grep -v "^++" | sed 's/+//' | tr -dc "[:alnum:][:space:][:punct:]" > $TEMP_ADDED_FILE 
    cat $MAIL_FILE | grep "^-" | grep -v "^--" | sed 's/-//' | tr -dc "[:alnum:][:space:][:punct:]" > $TEMP_REMOVED_FILE
    >$MAIL_FILE
    
    for i in `cat $TEMP_ADDED_FILE | tr ' ' '~' | grep -v "tmp"`
    do 
	
	pattern=`echo $i | sed 's/[!@%^*]//g' | tr '~' ' ' | awk -F"#" '{print $1 ".*" $3 ".*" }'`;
	match=`grep "$pattern" $TEMP_REMOVED_FILE | wc -l `;
	pattern_with_time=`echo $i | sed 's/[!@%^*]//g' | tr '~' ' ' | awk -F"#" '{print $1 ".*" $2 ".*" $3 ".*" $4}'`;

	if [ $match -gt 0 ] 
	then 

            counter=`grep "$pattern_with_time" $EVENTS_CHANGE_FILE | wc -l` ;

            if [ "$TRDHR" == "EU" ]
            then

                severity=`echo $i | tr '~' ' '| nawk -F"#" '{print $NF}' | awk -F"\"" '{print $1}'` ;

                if [ "$severity" == "3" ]
                then

                    echo "Event Change : " `grep "$pattern" $TEMP_ADDED_FILE` " From " `grep "$pattern" $TEMP_REMOVED_FILE` >> $EVENTS_CHANGE_FILE ;
                    continue ;

                fi

            fi


            if [ $counter -lt 1 ]
            then 
	        echo "DEBUG: $pattern_with_time not found in $EVENTS_CHANGE_FILE" >> /home/pengine/events_debug_log;
                echo "Event Change : " `grep "$pattern" $TEMP_ADDED_FILE` " From " `grep "$pattern" $TEMP_REMOVED_FILE` >> $EVENTS_CHANGE_FILE ;
		echo "Event Change : " `grep "$pattern" $TEMP_ADDED_FILE` " From " `grep "$pattern" $TEMP_REMOVED_FILE` >> $MAIL_FILE ;
            fi
	    
	else
            counter=`grep "$pattern" $EVENTS_ADDED_FILE | wc -l` ;
            str=`echo $i | sed 's/[!@%^*]//g' | tr '~' ' '`
            if [ "$TRDHR" == "EU" ]
            then 

		severity=`echo $i | tr '~' ' '| nawk -F"#" '{print $NF}' | awk -F"\"" '{print $1}'` ;

		if [ "$severity" == 1 ]
		then 

                    # Add events to mail file, don't report 
		    echo "New Event Added : " "$str" >> $EVENTS_ADDED_FILE ;
                    continue ;

		fi     

	    fi
	    
            if [ $counter -lt 1 ]
            then 
		echo "New Event Added : " "$str" >> $EVENTS_ADDED_FILE ;
		echo "New Event Added : " "$str" >> $MAIL_FILE ;  
            fi

	fi 
	
    done


    for i in `cat $TEMP_REMOVED_FILE | tr ' ' '~' | grep -v "tmp"`
    do 
	
	pattern=`echo $i | sed 's/[!@%^*]//g' | tr '~' ' ' | awk -F"#" '{print $1 ".*" $3 }'`;
	match=`grep "$pattern" $TEMP_ADDED_FILE | wc -l `;

	if [ $match -lt 1 ] 
	then 

            counter=`grep "$pattern" $EVENTS_REMOVED_FILE | wc -l` ;
            #Current Hour (hh)
            cur_hour=`date +%H` ;
            event_hour=`echo $i | sed 's/[!@%^*]//g' | tr '~' ' ' | awk -F"#" '{print $2 }' | awk '{print $2}' | awk -F":" '{print $1}'` ;
            #Dont consider removed events which are already done
            if [ $counter -lt 1 ] && [ "$event_hour" -ge "$cur_hour" ];
            then 
                temp_str=`echo $i | tr '~' ' '`
		echo "Event Removed : " "$temp_str" >> $EVENTS_REMOVED_FILE ;
		echo "Event Removed : " "$temp_str" >> $MAIL_FILE ;  
            fi

	fi 
	
    done

    LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'` ;

    if [ $(($LINE_COUNT)) -gt 0 ]
    then
      #Send a mail
#/bin/mail -s "FxStreetEventsAlert" -r "nseall@tworoads.co.in" "nseall@tworoads.co.in" < $MAIL_FILE
      #Send a slack notification
      $SLACK_EXEC events FILE $MAIL_FILE
      cat $MAIL_FILE
    fi

else 

    echo "Events Match"

fi

#===========================================================#

#rm -rf $TEMP_LIVE_FILE
rm -rf $LIVE_EVENTS_CSV
#rm -rf $MAIL_FILE
#rm -rf $CURRENT_EVENTS_FILTERED_FILE
#rm -rf $LIVE_EVENTS_FILTERED_FILE
#rm -rf $TEMP_ADDED_FILE $TEMP_REMOVED_FILE
