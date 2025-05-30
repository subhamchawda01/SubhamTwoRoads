#!/bin/bash

USAGE="$0 PRODUCT EXCH DATE" ;

if [ $# -ne 3 ] ;
then
    echo $USAGE;
    exit;
fi

LAST_TRD_PRICE_EXEC=$HOME/LiveExec/bin/mds_log_reader

PRODUCT=$1; shift;
EXCH=$1; shift;
YYYYMMDD=$1; shift;

MDS_LOGGED_DATA_DIR=/NAS1/data ;

BMF_NTP_DATA=$MDS_LOGGED_DATA_DIR/NTPLoggedData;
CME_DATA=$MDS_LOGGED_DATA_DIR/CMELoggedData;
EUREX_DATA=$MDS_LOGGED_DATA_DIR/EUREXLoggedData;
MX_DATA=$MDS_LOGGED_DATA_DIR/TMX_FSLoggedData;
LIFFE_DATA=$MDS_LOGGED_DATA_DIR/LIFFELoggedData;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


PRODUCT_DATA_FILE="NONAME";

LAST_TRD_PRICE=0.0;
LOCATION="NA" ;

case $EXCH in
    cme|CME)

        LOCATION=CHI;
        PRODUCT_DATA_FILE=$CME_DATA/$LOCATION/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/$PRODUCT*".gz" ;
        EXCH=CME;

        ;;
    eurex|EUREX)

        LOCATION=FR2;
        PRODUCT_DATA_FILE=$EUREX_DATA/$LOCATION/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/$PRODUCT*".gz" ;
        EXCH=EUREX;

        ;;
    tmx|TMX)

        LOCATION=TOR;
        PRODUCT_DATA_FILE=$MX_DATA/$LOCATION/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/$PRODUCT*".gz" ;
        EXCH=TMX;

        ;;
    bmf|BMF|bmfep|BMFEP|ntp|NTP)

        LOCATION=BRZ;
        PRODUCT_DATA_FILE=$BMF_NTP_DATA/$LOCATION/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/$PRODUCT*".gz" ;
        EXCH=NTP;

        ;;
    *)
        echo You did not chose CME, EUREX, TMX or BMF
        ;;
esac

LAST_TRD_PRICE=`$LAST_TRD_PRICE_EXEC $EXCH $PRODUCT_DATA_FILE | tac | grep "Trade" | head | grep "Trade_Px" | head -1 | awk '{print $2}'` ;

echo $LAST_TRD_PRICE ;
