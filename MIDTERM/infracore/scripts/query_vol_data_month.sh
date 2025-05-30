if [ $# -lt 2 ] ; then 
echo "usage: $0 MON SHC"; exit ; 
fi

MONTH=$1; shift;
SHC=$1; shift;

SDATE=20120101;
EDATE=20120131;
YYYY=2012;

case $MONTH in
    JAN)
	SDATE=$YYYY"0101";
	EDATE=$YYYY"0131";
	;;
    FEB)
	SDATE=$YYYY"0201";
	EDATE=$YYYY"0229";
	;;
    MAR)
	SDATE=$YYYY"0301";
	EDATE=$YYYY"0331";
	;;
    APR)
	SDATE=$YYYY"0401";
	EDATE=$YYYY"0430";
	;;
    MAY)
	SDATE=$YYYY"0501";
	EDATE=$YYYY"0531";
	;;
    JUN)
	SDATE=$YYYY"0601";
	EDATE=$YYYY"0630";
	;;
    JUL)
	SDATE=$YYYY"0701";
	EDATE=$YYYY"0731";
	;;
    AUG)
	SDATE=$YYYY"0801";
	EDATE=$YYYY"0831";
	;;
    SEP)
	SDATE=$YYYY"0901";
	EDATE=$YYYY"0930";
	;;
    OCT)
	SDATE=$YYYY"1001";
	EDATE=$YYYY"1031";
	;;
    NOV)
	SDATE=$YYYY"1101";
	EDATE=$YYYY"1130";
	;;
    DEC)
	SDATE=$YYYY"1201";
	EDATE=$YYYY"1231";
	;;
esac
	
VALVOLUME=`$HOME/infracore/scripts/query_pnl_data_mult.pl $SDATE $EDATE $SHC | awk -F, '{print $3}' | $HOME/infracore/scripts/sum_mean_calc.pl`;
echo $SHC $MONTH $VALVOLUME; 
