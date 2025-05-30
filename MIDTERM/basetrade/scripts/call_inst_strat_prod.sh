#!/bin/bash

USAGE="$0 STRATFILENAME DEST_SERVER DEST_ID ";
if [ $# -ne 3 ] ; 
then 
    echo $USAGE;
    exit;
fi

STRATFILENAME=$1; shift;
DEST_SERV=$1; shift;
DEST_ID=$1; shift;

~/basetrade/ModelScripts/call_inst_strat_prod.pl ~/modelling/strats/*/*/$STRATFILENAME $DEST_SERV $DEST_ID
