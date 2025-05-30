#!/bin/bash

USAGE="$0 STRATFILENAME DEST_SERVER DEST_ID ";
if [ $# -ne 3 ] ; 
then 
    echo $USAGE;
    exit;
fi


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

STRATFILENAME=$1; shift;
DEST_SERV=$1; shift;
DEST_ID=$1; shift;

~/infracore/ModelScripts/call_inst_strat_prod.pl ~/modelling/strats/*/*/$STRATFILENAME $DEST_SERV $DEST_ID
