#!/bin/bash

#since the script fetches web page from fxstreet, works as long as the webpage currency rates table format is not changed

USAGE1="$0 FILE FILENAME"
EXAMPLE1="$0 FILE /spare/local/files/currency_conversion_list.txt"

USAGE2="$0 CURRENCY1 CURRENCY2"
EXAMPLE2="$0 EUR USD"

USAGE3="$0 FILE FILENAME SYNC2ALL"
EXAMPLE3="$0 FILE /spare/local/files/currency_conversion_list.txt SYNC2ALL"

if [ $# -lt 2 ] ;
then
    echo "USAGE : ";
    echo $USAGE1;
    echo $EXAMPLE1;
    echo $USAGE2;
    echo $EXAMPLE2;
    echo $USAGE3;
    echo $EXAMPLE3;
    exit;
fi

MAJOR_CURR=/tmp/major_currency_rates.xml

wget -O $MAJOR_CURR "http://www.ecb.int/stats/eurofxref/eurofxref-daily.xml" 2>/dev/null

TODAY=$(date "+%Y%m%d");
OUTFILE=/spare/local/files/CurrencyData/currency_rates_$TODAY".txt_N";

YESTERDAY=`cat /tmp/YESTERDAY_DATE`
YESTERDAY_RATES=/spare/local/files/CurrencyData/currency_rates_$YESTERDAY".txt";
YESTERDAY_RATES_N=/spare/local/files/CurrencyData/currency_rates_$YESTERDAY".txt_N";
YESTERDAY_RATES_INFRA_INSTALL=$HOME/infracore_install/SysInfo/CurrencyInfo/currency_info_$YESTERDAY.txt;

EUR_US=`grep USD $MAJOR_CURR | awk -F"rate=" '{print $2}' | tr "'" " " | awk '{print $1}'`;

if [[ ! "$EUR_US" =~ ^[0-9]+([.][0-9]+)?$ ]]
then
    rm $OUTFILE
    /bin/mail -s "CurrencyRates: Webpage format error" -r "currencyinfo@ny11" "nseall@tworoads.co.in" < "Currency file not generated for $TODAY"
    exit ;
fi

if [ $1 = "FILE" ]
then

    if [ -f $OUTFILE ]
    then
	rm $OUTFILE
	touch $OUTFILE
    else
	touch $OUTFILE
    fi

    for i in `cat $2`;
    do

	curr1=`echo $i | awk -F":" '{print $1}'`
	curr2=`echo $i | awk -F":" '{print $2}'`

	rev=0 ;

	if [ $curr1 = "USD" ] 
	then 

	    nondolcurr=$curr2 ;

	else 

	    nondolcurr=$curr1 ;
	    rev=1 ;

	fi 

        EUR_NONDOL=`grep $nondolcurr $MAJOR_CURR | awk -F"rate=" '{print $2}' | tr "'" " " | awk '{print $1}'`;  
        
        #Have to handle special case since we ECB is source    
        if [ "$nondolcurr" = "EUR" ]; then

            EUR_NONDOL="1.0";

        fi

        if [[ "$EUR_NONDOL" =~ ^[0-9]+([.][0-9]+)?$ ]]
        then
	    if [ $rev -lt 1 ] 
	    then

		echo $curr1$curr2 `echo $EUR_NONDOL | awk -v denom=$EUR_US '{printf "%f\n",($1/denom)}'` >> $OUTFILE  ;

	    else

		echo $curr1$curr2 `echo $EUR_NONDOL | awk -v num=$EUR_US '{printf "%f\n",(num/$1)}'` >> $OUTFILE  ;

	    fi

	else
	    rm $OUTFILE
            /bin/mail -s "CurrencyRates: Webpage format error" -r "currencyinfo@ny11" "nseall@tworoads.co.in" < "Currency file not generated for $TODAY"
	    exit ;

	fi

    done ;

    echo $OUTFILE

    if [ ! -z $3 ];
    then
        if [ $3 = "SYNC2ALL" ]
        then
            if [ -f $YESTERDAY_RATES_N ]
            then
                mv $YESTERDAY_RATES_N $YESTERDAY_RATES
                cp $YESTERDAY_RATES $YESTERDAY_RATES_INFRA_INSTALL
                chmod 666 $YESTERDAY_RATES_INFRA_INSTALL #permissions may get altered with cp

                $HOME/infracore/scripts/sync_file_to_all_machines.pl $YESTERDAY_RATES
                $HOME/infracore/scripts/sync_file_to_all_machines.pl $YESTERDAY_RATES_INFRA_INSTALL

                /bin/mail -s "CurrencyRates" -r "currencyinfo@ny11" "nseall@tworoads.co.in" < $OUTFILE
            fi
        fi
    fi
else

    curr1=$1
    curr2=$2

    rev=0 ;

    if [ $curr1 = "USD" ] 
    then 

	nondolcurr=$curr2 ;

    else 

	nondolcurr=$curr1 ;
	rev=1 ;

    fi 

    #Have to handle special case since we ECB is source    
    if [ "$nondolcurr" = "EUR" ]; then

        EUR_NONDOL="1.0";

    fi
    
    EUR_NONDOL=`grep $nondolcurr $MAJOR_CURR | awk -F"rate=" '{print $2}' | tr "'" " " | awk '{print $1}'`;  

    if [[ "$EUR_NONDOL" =~ ^[0-9]+([.][0-9]+)?$ ]]
    then
        if [ $rev -lt 1 ] 
        then

	    echo $curr1$curr2 `echo $EUR_NONDOL | awk -v denom=$EUR_US '{printf "%f\n",($1/denom)}'` >> $OUTFILE  ;

        else

	    echo $curr1$curr2 `echo $EUR_NONDOL | awk -v num=$EUR_US '{printf "%f\n",(num/$1)}'` >> $OUTFILE  ;

        fi
    fi

fi

chmod 666 $OUTFILE
rm $MAJOR_CURR

