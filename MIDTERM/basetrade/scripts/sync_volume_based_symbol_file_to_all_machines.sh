#!/bin/bash

USAGE="$0  YYYYMMDD";
USAGE_DESC=" sync all *YYYYMMDD* from /spare/local/VolumeBasedSymbol/ to all machines";

if [ $# -ne 1 ];
then 
    echo $USAGE
    echo $USAGE_DESC
    exit;
fi

YYYYMMDD=$1;
if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

echo $YYYYMMDD
for file in /spare/local/VolumeBasedSymbol/*$YYYYMMDD* ;
do 
    rsync -avz  --quiet $file dvcinfra@10.1.3.12:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@10.1.3.13:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@10.1.3.14:/spare/local/VolumeBasedSymbol/
#    rsync -avz  --quiet $file dvcinfra@10.1.3.15:/spare/local/VolumeBasedSymbol/
#    rsync -avz  --quiet $file dvcinfra@10.1.3.16:/spare/local/VolumeBasedSymbol/

    rsync -avz  --quiet $file dvcinfra@10.2.3.11:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@10.2.3.12:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@10.2.3.13:/spare/local/VolumeBasedSymbol/ 
#    rsync -avz  --quiet $file dvcinfra@10.2.3.14:/spare/local/VolumeBasedSymbol/

    rsync -avz  --quiet $file dvcinfra@90.226.13.131:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@90.226.13.132:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@90.226.13.133:/spare/local/VolumeBasedSymbol/
#    rsync -avz  --quiet $file dvcinfra@90.226.13.134:/spare/local/VolumeBasedSymbol/

    rsync -avz  --quiet $file dvcinfra@10.23.182.51:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@10.23.182.52:/spare/local/VolumeBasedSymbol/

    rsync -avz  --quiet $file dvcinfra@10.5.3.11:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@10.5.3.12:/spare/local/VolumeBasedSymbol/
    rsync -avz  --quiet $file dvcinfra@10.5.3.13:/spare/local/VolumeBasedSymbol/

#    echo $file " synced to all servers";
done