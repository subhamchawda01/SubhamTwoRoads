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


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


wget -O /tmp/major_currency_rates.html "http://www.fxstreet.com/rates-charts/currency-rates/" 2>/dev/null
wget -O /tmp/south_america_currency_rates.html "http://www.fxstreet.com/rates-charts/currency-rates/?id=usdollar3%3busdars%3busdbrl%3busdclp%3busdcop%3busduyu%3busdgyd" 2>/dev/null

TODAY=$(date "+%Y%m%d");
OUTFILE=/spare/local/files/CurrencyData/currency_rates_$TODAY".txt_N";

YESTERDAY=`cat /tmp/YESTERDAY_DATE`
YESTERDAY_RATES=/spare/local/files/CurrencyData/currency_rates_$YESTERDAY".txt";
YESTERDAY_RATES_N=/spare/local/files/CurrencyData/currency_rates_$YESTERDAY".txt_N";

MAJOR_CURR=/tmp/major_currency_rates.html
SOUTH_AME=/tmp/south_america_currency_rates.html

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

	if [ $curr1 = "BRL" ] || [ $curr2 = "BRL" ] ;
	then

	    if [[ `cat $SOUTH_AME | grep ">$curr1\/$curr2<" -A1 | grep -v ">$curr1\/$curr2<" | tr '-' '\n' | head -1 | tr '<' '\n' | tr '>' '\n' | head -3 | tail -1` =~ ^[0-9]+([.][0-9]+)?$ ]]
	    then
		echo $curr1$curr2 `cat $SOUTH_AME | grep ">$curr1\/$curr2<" -A1 | grep -v ">$curr1\/$curr2<" | tr '-' '\n' | head -1 | tr '<' '\n' | tr '>' '\n' | head -3 | tail -1` >> $OUTFILE

	    else
		rm $OUTFILE
		exit ;

	    fi

	else

	    if [[ `cat $MAJOR_CURR| grep ">$curr1\/$curr2<" -A1 | grep -v ">$curr1\/$curr2<" | tr '-' '\n' | head -1 | tr '<' '\n' | tr '>' '\n' | head -3 | tail -1` =~ ^[0-9]+([.][0-9]+)?$ ]]
	    then
		echo $curr1$curr2 `cat $MAJOR_CURR | grep ">$curr1\/$curr2<" -A1 | grep -v ">$curr1\/$curr2<" | tr '-' '\n' | head -1 | tr '<' '\n' | tr '>' '\n' | head -3 | tail -1` >> $OUTFILE

	    else
		rm $OUTFILE
		exit ;

	    fi

	fi

    done ;

    if [ ! -z $3 ]; 
    then 
	
	if [ $3 = "SYNC2ALL" ] 
	then
	    
	    host_to_scp=(

		10.23.199.51
		10.23.199.52
		10.23.199.53
		10.23.199.54
		10.23.199.55
		10.23.142.51
		10.23.196.51
		10.23.196.52
		10.23.196.53
		10.23.196.54
		10.23.200.51
		10.23.200.52
		10.23.200.53
		10.23.200.54
		10.23.182.51 #sdv-tor-srv11
		10.23.182.52 #sdv-tor-srv12  
		10.23.23.11 #sdv-bmf-srv11  
		10.23.23.12 #sdv-bmf-srv12 
		10.220.40.1 #sdv-bmf-srv13


	    )

	    if [ -f $YESTERDAY_RATES_N ]
	    then 
		
		mv $YESTERDAY_RATES_N $YESTERDAY_RATES
		
		for host in "${host_to_scp[@]}"
		do
		    scp $YESTERDAY_RATES dvcinfra@$host:/home/dvcinfra/infracore_install/SysInfo/CurrencyInfo/currency_info_$YESTERDAY.txt;
		    scp $YESTERDAY_RATES dvcinfra@$host:/spare/local/files/CurrencyData/currency_rates_$YESTERDAY.txt;
		done
	    fi
	    
	    /bin/mail -s "CurrencyRates" -r "currencyinfo@ny11" "nseall@tworoads.co.in" < $OUTFILE

	fi

    fi

else

    curr1=$1
    curr2=$2

    if [ $curr1 = "BRL" ] || [ $curr2 = "BRL" ] ;
    then

	if [[ `cat $SOUTH_AME | grep ">$curr1\/$curr2<" -A1 | grep -v ">$curr1\/$curr2<" | tr '-' '\n' | head -1 | tr '<' '\n' | tr '>' '\n' | head -3 | tail -1` =~ ^[0-9]+([.][0-9]+)?$ ]]
	then
	    cat $SOUTH_AME | grep ">$curr1\/$curr2<" -A1 | grep -v ">$curr1\/$curr2<" | tr '-' '\n' | head -1 | tr '<' '\n' | tr '>' '\n' | head -3 | tail -1

	else
	    echo "Error"
	    exit ;

	fi

    else

	if [[ `cat $MAJOR_CURR| grep ">$curr1\/$curr2<" -A1 | grep -v ">$curr1\/$curr2<" | tr '-' '\n' | head -1 | tr '<' '\n' | tr '>' '\n' | head -3 | tail -1` =~ ^[0-9]+([.][0-9]+)?$ ]]
	then
	    cat $MAJOR_CURR | grep ">$curr1\/$curr2<" -A1 | grep -v ">$curr1\/$curr2<" | tr '-' '\n' | head -1 | tr '<' '\n' | tr '>' '\n' | head -3 | tail -1

	else
	    echo "Error"
	    exit ;

	fi

    fi

fi

rm $SOUTH_AME
rm $MAJOR_CURR

