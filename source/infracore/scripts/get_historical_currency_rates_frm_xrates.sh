#!/bin/bash

#since the script fetches web page from xrates.com, works as long as the webpage currency rates table format is not changed

USAGE1="$0 FILE FILENAME DATE"
EXAMPLE1="$0 FILE /spare/local/files/currency_conversion_list.txt 20130510"

USAGE2="$0 CURRENCY1 CURRENCY2 DATE"
EXAMPLE2="$0 EUR USD 20130510"

USAGE3="$0 FILE FILENAME DATE SYNC2ALL"
EXAMPLE3="$0 FILE /spare/local/files/currency_conversion_list.txt 20130510 SYNC2ALL"

if [ $# -lt 3 ] ;
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

TODAY=$3

YYYY=${TODAY:0:4}
MM=${TODAY:4:2}
DD=${TODAY:6:2}

wget -O /tmp/major_currency_rates_historical.html "http://www.x-rates.com/historical/?from=USD&amount=1.00&date="$YYYY"-"$MM"-"$DD 2>/dev/null

OUTFILE=/spare/local/files/CurrencyData/currency_rates_$TODAY".txt_N";

YESTERDAY=$(date -d $TODAY)
YESTERDAY=$(date -d "$YESTERDAY-1 day" +%Y%m%d)

YESTERDAY_RATES=/spare/local/files/CurrencyData/currency_rates_$YESTERDAY".txt";
YESTERDAY_RATES_N=/spare/local/files/CurrencyData/currency_rates_$YESTERDAY".txt_N";
YESTERDAY_RATES_INFRA_INSTALL=$HOME/infracore_install/SysInfo/CurrencyInfo/currency_info_$YESTERDAY.txt;

MAJOR_CURR=/tmp/major_currency_rates_historical.html

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

	if [[ `grep $nondolcurr $MAJOR_CURR | grep rtRates | grep "from=USD" | tail -1 | cut -d";" -f2 | cut -d">" -f2 | cut -d"<" -f1` =~ ^[0-9]+([.][0-9]+)?$ ]]
	then

	    if [ $rev -lt 1 ] 
	    then

		echo $curr1$curr2 `grep $nondolcurr $MAJOR_CURR | grep rtRates | grep "from=USD" | tail -1 | cut -d";" -f2 | cut -d">" -f2 | cut -d"<" -f1` >> $OUTFILE  ;

	    else

		echo $curr1$curr2 `grep $nondolcurr $MAJOR_CURR | grep rtRates | grep "from=USD" | tail -1 | cut -d";" -f2 | cut -d">" -f2 | cut -d"<" -f1 | awk '{print 1/$1}'` >> $OUTFILE ;

	    fi

	else
	    rm $OUTFILE
	    exit ;

	fi

    done ;

    echo $OUTFILE

    if [ ! -z $4 ]; 
    then 
        if [ $4 = "SYNC2ALL" ] 
        then
            if [ -f $YESTERDAY_RATES_N ]
            then 
                mv $YESTERDAY_RATES_N $YESTERDAY_RATES
                chmod 666 $YESTERDAY_RATES ; 
                cp $YESTERDAY_RATES $YESTERDAY_RATES_INFRA_INSTALL;

                $HOME/infracore/scripts/sync_file_to_all_machines.pl $YESTERDAY_RATES
                $HOME/infracore/scripts/sync_file_to_all_machines.pl $YESTERDAY_RATES_INFRA_INSTALL
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

    if [[ `grep $nondolcurr $MAJOR_CURR | grep rtRates | grep "from=USD" | tail -1 | cut -d";" -f2 | cut -d">" -f2 | cut -d"<" -f1` =~ ^[0-9]+([.][0-9]+)?$ ]]
    then

	if [ $rev -lt 1 ] 
	then

            echo $curr1$curr2 `grep $nondolcurr $MAJOR_CURR | grep rtRates | grep "from=USD" | tail -1 | cut -d";" -f2 | cut -d">" -f2 | cut -d"<" -f1` ;

	else

            echo $curr1$curr2 `grep $nondolcurr $MAJOR_CURR | grep rtRates | grep "from=USD" | tail -1 | cut -d";" -f2 | cut -d">" -f2 | cut -d"<" -f1 | awk '{print 1/$1}'` ;

	fi

    fi

fi

rm $MAJOR_CURR

