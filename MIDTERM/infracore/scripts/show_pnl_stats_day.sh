
TRADELOGDIR=/spare/local/logs/tradelogs;
YYYYMMDD=`date +%Y%m%d`;
if [ $# -gt 0 ] ; then
    YYYYMMDD=$1; shift;
fi

for TRADESFILENAME in $TRADELOGDIR/trades.$YYYYMMDD.* ; 
do 
    if [ -e $TRADESFILENAME ] ; then
	echo "------- $TRADESFILENAME --------"; 
	~/infracore/ModelScripts/get_pnl_stats.pl $TRADESFILENAME ; 
    fi
done
