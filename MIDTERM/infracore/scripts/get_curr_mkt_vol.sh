
NUM_TOKENS=`echo -n $1 | wc -w`;

if [ $NUM_TOKENS -lt 2 ] ;
then

  cat /home/dvcinfra/trades/ALL_MKT_VOLUMES | grep "$1" | awk '{ print $21}' | head -2 | tail -1
else
  cat /home/dvcinfra/trades/ALL_MKT_VOLUMES | grep "$1" | awk '{ print $22}' | head -2 | tail -1
fi
