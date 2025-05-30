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
if [ ! -d /NAS1/data/BMFLoggedData/BMF/ ];
then 
    mkdir -p /NAS1/data/BMFLoggedData/BMF/
fi


cd /NAS1/data/BMFLoggedData/BMF/

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
rsync -avz --delete --rsh='ssh -p22' --exclude '.git'  --exclude 'bin' --quiet dvcinfra@200.143.33.126:/spare/local/MDSlogs/BMF/*$YYYYMMDD /NAS1/data/BMFLoggedData/BMF/$YYYY/$MM/$DD
#gzip all files that are not gzipped yet
find /NAS1/data/BMFLoggedData/ -type f ! -name '*.gz' -exec gzip {} \;
