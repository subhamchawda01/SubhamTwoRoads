#!/bin/bash

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
elif [ $YYYYMMDD = "YESTERDAY" ] ;
then
    YYYYMMDD=$(cat /tmp/YESTERDAY_DATE)
fi

# Link files.
cd /apps/data/MFGlobalTrades/MFGFiles;

ALPES_INVOICE_FILE1="InvoiceDetailed_4505_"$YYYYMMDD".csv";
ALPES_INVOICE_FILE2="InvoiceDetailed_4972_"$YYYYMMDD".csv";
ALPES_INVOICE_FILE3="InvoiceDetailed_4994_"$YYYYMMDD".csv";

ALPES_INVOICE_FILE="InvoiceDetailed_ALL_"$YYYYMMDD".csv";

if [ ! -f $ALPES_INVOICE_FILE1 ] ; then
    ALPES_INVOICE_FILE1="InvoiceDetailled_4505_"$YYYYMMDD".csv";
fi

if [ ! -f $ALPES_INVOICE_FILE2 ] ; then
    ALPES_INVOICE_FILE2="InvoiceDetailled_4972_"$YYYYMMDD".csv";
fi

if [ ! -f $ALPES_INVOICE_FILE3 ] ; then
    ALPES_INVOICE_FILE3="InvoiceDetailled_4994_"$YYYYMMDD".csv";
fi

echo "cat $ALPES_INVOICE_FILE1 $ALPES_INVOICE_FILE2 $ALPES_INVOICE_FILE3 | grep "ISIN_CODE" | sort | uniq > $ALPES_INVOICE_FILE 2>/dev/null" ;

`cat $ALPES_INVOICE_FILE1 $ALPES_INVOICE_FILE2 $ALPES_INVOICE_FILE3 | grep "ISIN_CODE" | sort | uniq > $ALPES_INVOICE_FILE 2>/dev/null`;
`cat $ALPES_INVOICE_FILE1 $ALPES_INVOICE_FILE2 $ALPES_INVOICE_FILE3 | grep -v "ISIN_CODE" >> $ALPES_INVOICE_FILE 2>/dev/null`;

chmod 644 /apps/data/MFGlobalTrades/MFGFiles/*$YYYYMMDD*.csv ;

rsync -avpz $ALPES_INVOICE_FILE dvcinfra@10.23.74.40:/apps/data/MFGlobalTrades/MFGFiles/
