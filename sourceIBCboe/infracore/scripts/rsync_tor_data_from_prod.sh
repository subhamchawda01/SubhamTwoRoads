#!/bin/bash

USER="dvcinfra"
USAGE="$0 YYYYMMDD";

if [ $# -ne 1 ] ;
then
    echo $USAGE;
    exit;
fi


#EXCHANGE=$1;
#CURRENT_LOCATION=$2;
YYYYMMDD=$1;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

#Brazil Server VPN
#make directory if it doesnot exist
if [ ! -d /NAS1/data/TMX_FSLoggedData/NY4/ ];
then 
    mkdir -p /NAS1/data/TMX_FSLoggedData/NY4/
fi


cd /NAS1/data/TMX_FSLoggedData/NY4/

if [ $? != 0 ] ; then exit $? ; fi

# Create the needed directory structure on the server
if [ ! -d ${YYYYMMDD:0:4} ] ; then mkdir ${YYYYMMDD:0:4} ; fi

cd ${YYYYMMDD:0:4} # YYYY/
if [ ! -d ${YYYYMMDD:4:2} ] ; then mkdir ${YYYYMMDD:4:2} ; fi
cd ${YYYYMMDD:4:2} # MM/
if [ ! -d ${YYYYMMDD:6:2} ] ; then mkdir ${YYYYMMDD:6:2} ; fi
cd ${YYYYMMDD:6:2} # DD/

YYYY=${YYYYMMDD:0:4}
MM=${YYYYMMDD:4:2}
DD=${YYYYMMDD:6:2}
echo $YYYY $MM $DD
rsync -avz --delete --rsh='ssh -p22' --exclude '.git'  --exclude 'bin' --quiet dvcinfra@38.64.128.227:/spare/local/MDSlogs/TMX/*$YYYYMMDD /NAS1/data/TMX_FSLoggedData/NY4/$YYYY/$MM/$DD
