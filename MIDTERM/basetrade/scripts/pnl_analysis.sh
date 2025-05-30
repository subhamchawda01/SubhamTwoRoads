#!/bin/bash
if [ $# -lt 1 ]; then
    echo "<script> SHC/Secname date_str"
    exit;
fi
SHC=$1;

YYYY=2014;
if [ $# -gt 1 ]; then
    YYYY=$2;
fi

EXCH=0;
case "$SHC" in
    TOTAL)
	EXCH=1;
	;;
    MFT_TOTAL)
       EXCH=1;
       ;;
    CME)
	EXCH=1;
	;;
    EUREX)
	EXCH=1;
	;;
    TMX)
	EXCH=1;
	;;
    LIFFE)
	EXCH=1;
	;;
    BMF)
	EXCH=1;
	;;
    OSE)
	EXCH=1;
	;;
    HKEX)
	EXCH=1;
	;;
    RTS)
	EXCH=1;
	;;
    MICEX)
  EXCH=1;
  ;;
    CFE)
	EXCH=1;
	;;      
    ICE)
	EXCH=1;
	;;
    ASX)
  EXCH=1;
  ;;
    NSE)
  EXCH=1;
  ;;
    SGX)
  EXCH=1;
  ;;
    BMFEQ)
	EXCH=2;
	;;
    TOTAL)
	EXCH=1;
	;;
esac

EODdir=/NAS1/data/MFGlobalTrades/EODPnl
if [ ! -d $EODdir ] ; then
    EODdir=/apps/data/MFGlobalTrades/EODPnl ;
    if [ ! -d $EODdir ] ; then 
	echo "$EODdir does not exist";
	exit ;
    fi
fi

grep_command="grep -w $SHC ";
if [ $SHC == "TOTAL" ] ; then
   grep_command="grep -m1 $SHC ";
fi

case $EXCH in
   2)
	for name in $EODdir/ors_equities_pnls_$YYYY*.txt ; do yyyymmdd=`echo $name | cut -c52-59` ; grep $SHC $name | awk -vyyyymmdd=$yyyymmdd 'BEGIN{pnl=0;} { pnl+= $5; } END{ print pnl,yyyymmdd; }' ; done | awk 'BEGIN{maxdd=100;} { sumpnl+= $1; count+=1; if ( highpnl < sumpnl ) { highpnl = sumpnl; } if ( maxdd < ( highpnl - sumpnl ) ) { maxdd = ( highpnl - sumpnl ) ; } printf "%8d: %8d SUM: %8d AVG: %7d DD: %8d AVG_BY_MAXDD: %.2f\n", $2,$1,sumpnl,(sumpnl/count), (highpnl-sumpnl), (sumpnl/maxdd/count); }' 	
	;;

    1)
	for name in $EODdir/ors_pnls_$YYYY*.txt ; do yyyymmdd=`echo $name | cut -c43-50` ; $grep_command $name | replace "LFI  1" "LFI~~1" "LFZ  1" "LFZ~~1" "LFR  1" "LFR~~1" | awk -vyyyymmdd=$yyyymmdd 'BEGIN{pnl=0;} { pnl+= $5; } END{ print pnl,yyyymmdd; }' ; done | awk 'BEGIN{maxdd=100;} { sumpnl+= $1; count+=1; if ( highpnl < sumpnl ) { highpnl = sumpnl; } if ( maxdd < ( highpnl - sumpnl ) ) { maxdd = ( highpnl - sumpnl ) ; } printf "%8d: %8d SUM: %8d AVG: %7d DD: %8d AVG_BY_MAXDD: %.2f\n", $2,$1,sumpnl,(sumpnl/count), (highpnl-sumpnl), (sumpnl/maxdd/count); }' 
	
	;;
    *)
	for name in $EODdir/ors_pnls_$YYYY*.txt ; do 
    yyyymmdd=`echo $name | cut -c43-50` ; 
    newSHC=`~/basetrade_install/bin/get_exchange_symbol "$SHC" $yyyymmdd 2>/dev/null`; 
    searchSHC=$SHC; if [ `echo "$newSHC" | wc -w` -gt 0 ] ; then searchSHC=$newSHC; fi; 
    grep "$searchSHC" $name | replace "LFI  1" "LFI~~1" "LFZ  1" "LFZ~~1" "LFR  1" "LFR~~1" "LFL  1" "LFL~~1" "Z   F" "Z~~F" "I   F" "I~~F" "R   F" "R~~F" "L   F" "L~~F" | awk -vyyyymmdd=$yyyymmdd 'BEGIN{pnl=0;vol=0} { pnl+= $6; vol+=$14 } END{ if(vol>0){print pnl,yyyymmdd;} }' ; 
  done | awk 'BEGIN{maxdd=100;} { sumpnl+= $1; count+=1; if ( highpnl < sumpnl ) { highpnl = sumpnl; } if ( maxdd < ( highpnl - sumpnl ) ) { maxdd = ( highpnl - sumpnl ) ; } printf "%8d: %8d SUM: %8d AVG: %7d DD: %8d AVG_BY_MAXDD: %.2f\n", $2, $1, sumpnl, (sumpnl/count), (highpnl-sumpnl), (sumpnl/maxdd/count); }' 
	;;
esac
