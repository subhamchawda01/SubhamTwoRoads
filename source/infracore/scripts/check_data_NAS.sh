#!/bin/bash

USAGE1="$0 LOCATION DATE NAS/S3"
EXAMP1="$0 TOK 20140130 NAS"

if [ $# -lt 2 ];
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

LOC=$1; shift;
YYYYMMDD=$1; shift;
NAS=$1; shift;
MAIL_FILE="/tmp/data_check_"$LOC"_"$YYYYMMDD;

# Empty the file
> $MAIL_FILE;
# echo $MAIL_FILE;

LIST=""
if [ "$LOC" == "BRZ" ]
then
    LIST="NTPLoggedData NTP_ORDLoggedData CMELoggedData QUINCY3LoggedData EOBIPriceFeedLoggedData LIFFELoggedData ICELoggedData PUMALoggedData RTSLoggedData MICEXLoggedData ORSData CONTROLLoggedData RawLoggedData"

elif [ "$LOC" == "FR2" ]
then
    LIST="EUREXLoggedData EOBINewLoggedData EOBIPriceFeedLoggedData LIFFELoggedData ICELoggedData ICE_FODLoggedData ICE_PLLoggedData CMELoggedData CSMLoggedData ORSData CONTROLLoggedData RawLoggedData"
elif [ "$LOC" == "CHI" ]
then
    LIST="CMELoggedData EOBIPriceFeedLoggedData LIFFELoggedData ICELoggedData ICE_FODLoggedData ICE_PLLoggedData OSENewPriceFeedLoggedData HKOMDCPFLoggedData ORSData CONTROLLoggedData RawLoggedData"
elif [ "$LOC" == "BSL" ]
then
    LIST="LIFFELoggedData ICELoggedData ICE_FODLoggedData ICE_PLLoggedData CMELoggedData EOBIPriceFeedLoggedData ORSData CONTROLLoggedData RawLoggedData"
elif [ "$LOC" == "TOR" ]
then
    LIST="CMELoggedData TMX_FSLoggedData ORSData CONTROLLoggedData EOBIPriceFeedLoggedData ICELoggedData"
elif [ "$LOC" == "TOK" ]
then
    LIST="OSENewPriceFeedLoggedData OSELoggedData CMELoggedData HKOMDCPFLoggedData EOBIPriceFeedLoggedData ORSData CONTROLLoggedData ICELoggedData"
elif [ "$LOC" == "HK" ]
then
    LIST="HKOMDLoggedData HKOMDCPFLoggedData OSENew_L1LoggedData CMELoggedData EOBIPriceFeedLoggedData ORSData CONTROLLoggedData"
elif [ "$LOC" == "CFE" ]
then
    LIST="CSMLoggedData ICE_FODLoggedData CMELoggedData ICE_PLLoggedData EOBIPriceFeedLoggedData ORSData CONTROLLoggedData"
elif [ "$LOC" == "NY4" ]
then
    LIST="CSMLoggedData LIFFELoggedData ICELoggedData"
elif [ "$LOC" == "CRT" ]
then
    LIST="CMELoggedData"
elif [ "$LOC" == "MOS" ]
then
    LIST="RTSLoggedData MICEXLoggedData LIFFELoggedData ICELoggedData CMELoggedData EOBIPriceFeedLoggedData EBSLoggedData ORSData CONTROLLoggedData"
else
    echo "$LOC is not a valid argument";
    echo $USAGE1;
    echo $EXAMP1;
    rm $MAIL_FILE;
    exit -1;
fi

for EXCH in $LIST
do
    DIRLOC=/NAS1/data/$EXCH/$LOC/${YYYYMMDD:0:4}/${YYYYMMDD:4:2}/${YYYYMMDD:6:2}/
    if [ -d $DIRLOC ]
    then
        if [ "$NAS" == "S3" ]
        then
            FILECOUNT=`/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd ls s3://s3dvc$DIRLOC | wc -l`;
        else
            FILECOUNT=`ls $DIRLOC | wc -l`;
        fi

        echo "$FILECOUNT $EXCH";
        if [[ ! $FILECOUNT -gt 0 ]]
        then
            echo "0 filecount for $LOC $YYYYMMDD" >> $MAIL_FILE;
            echo "0 filecount for $LOC $YYYYMMDD";
        fi
    else
        echo "Directory $DIRLOC doesn't exist for $LOC $YYYYMMDD" >> $MAIL_FILE;
        echo "Directory $DIRLOC doesn't exist for $LOC $YYYYMMDD";
    fi
done
    
rm $MAIL_FILE;
