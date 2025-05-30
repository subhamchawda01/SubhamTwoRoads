#!/bin/bash

USAGE="USAGE: $0 exec_list_file user[dvcinfra/dvctrader]";

if [ $# -lt 2 ]
then
  echo $USAGE;
  exit;
fi

if [ "$2" != "dvctrader" ] && [ "$2" != "dvcinfra" ]
then
  echo $2;
  echo $USAGE;
  exit;
fi

PROD_UPDATE_SCRIPT=$HOME/infracore_install/scripts/update_production_exec_prod.pl

# Using YYYYMMDD_PID as the backup exec name
YYYYMMDD=`date +%Y%m%d`
BackupName=$YYYYMMDD"_"$$;

for exec in `cat $1`;
do
  if [ -f $exec ]
  then
    BASE_EXEC=`basename $exec`;
    CMD="$PROD_UPDATE_SCRIPT $2 $exec LiveExec/bin/$BASE_EXEC $BackupName $BASE_EXEC-Update-$YYYYMMDD";
    echo $CMD;
    `$CMD >/dev/null 2>&1` ;
  else
    echo "ERROR: $exec does not exist.";
  fi
done
