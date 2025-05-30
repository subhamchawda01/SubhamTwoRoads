#!/bin/bash

USAGE1="$0 DATE ORSDIR"
EXAMP1="$0 TODAY /spare/local/ORSlogs/HKEX/FITII"

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

YYYYMMDD=$1
ORSDIR=$2

if [ $YYYYMMDD = "TODAY" ]
then

    YYYYMMDD=`date +"%Y%m%d"`

fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


yyyy=`echo ${YYYYMMDD:0:4}` ;
mm=`echo ${YYYYMMDD:4:2}` ;
dd=`echo ${YYYYMMDD:6:2}` ;

DEST_SRV="10.152.224.145"
TRGT_LOC=/apps/data/MFGlobalTrades/AUDIT/$yyyy/$mm/$dd ;

REJ_FILE=$ORSDIR/DVCAPITAL_AuditTrail_HKFE_FITII_$YYYYMMDD ; 
TMP_FILE=/tmp/HK_AUDIT_TEMP ;

#=====  Get Rejects File =================# 

scp $DEST_SRV:$REJ_FILE $TMP_FILE ; 


#===================  If file is empty just create a new file and exit ==================== # 

if [ ! -s $TMP_FILE ] 
then 

   mkdir -p $TRGT_LOC ;
   touch $TRGT_LOC/DVCAPITAL_AuditTrail_HKFE_FITII_$YYYYMMDD ;
   exit ; 

fi 

#===============================  Get Shortcode Symbols - need to replace the product commodity code with exchange symbol ======================== # 

SHORTCODE_SYMBOL_EXEC=$HOME/LiveExec/bin/get_exchange_symbol

HSI_SYMBOL=`$SHORTCODE_SYMBOL_EXEC HSI_0 $YYYYMMDD`
HHI_SYMBOL=`$SHORTCODE_SYMBOL_EXEC HHI_0 $YYYYMMDD`
MHI_SYMBOL=`$SHORTCODE_SYMBOL_EXEC MHI_0 $YYYYMMDD`
MCH_SYMBOL=`$SHORTCODE_SYMBOL_EXEC MCH_0 $YYYYMMDD`

replace "Product : 4002" $HSI_SYMBOL -- $TMP_FILE ; 
replace "Product : 4001" $HHI_SYMBOL -- $TMP_FILE ; 
replace "Product : 4004" $MHI_SYMBOL -- $TMP_FILE ; 
replace "Product : 4008" $MCH_SYMBOL -- $TMP_FILE ; 

cp $TMP_FILE $TRGT_LOC/ ; 

rm -rf $TMP_FILE ;

