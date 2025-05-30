#!/bin/bash                                                                                                                                                                                                                                 

# USAGE1="$0 DATE"                                                                                                                                                                                                                          

# if [ $# -ne 1 ] ;                                                                                                                                                                                                                         
# then                                                                                                                                                                                                                                      
#     echo $USAGE1;                                                                                                                                                                                                                         
#     echo $EXAMP1;                                                                                                                                                                                                                         
#     exit;                                                                                                                                                                                                                                 
# fi                                                                                                                                                                                                                                        

#DDMMYY=$1;                                                                                                                                                                                                                                 

LIFFE_SPREADS_SYM_EXEC=$HOME/LiveExec/bin/generate_liffe_spreads_symbol_mapping


# if [ "$DDMMYY" == "TODAY" ]                                                                                                                                                                                                               
# then                                                                                                                                                                                                                                      
    DDMMYY=`date +"%d%m%y"` ;
    YYYYMMDD=`date +%Y%m%d` ;
#fi                                                                                                                                                                                                                                         


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

LIFFE_SPREADS_SYM_MAP_OUTFILE=/spare/local/files/LIFFE/liffe-london-spreads-mapping-$YYYYMMDD.txt
LIFFE_SPREADS_SYM_MAP_DEFAULT_OUTFILE=/spare/local/files/LIFFE/liffe-london-spreads-mapping-default.txt

#================== This file contains the London futures and stirs, bunds, packs =========================#                                                                                                                               

LIFFE_FIXML_LONDON_FILE=/spare/local/files/LIFFE/NYSE_LIFFE_REFDATA/nyseliffe_stdata_fin_L_$DDMMYY".xml"

#========================================================================================================#                                                                                                                                  

ERR=""
if [ ! -f $LIFFE_FIXML_LONDON_FILE ]
then

    ERR="Could not find $LIFFE_FIXML_LONDON_FILE to generate the symbol mapping"
else
    $LIFFE_SPREADS_SYM_EXEC $LIFFE_FIXML_LONDON_FILE $LIFFE_SPREADS_SYM_MAP_OUTFILE
    sort -u $LIFFE_SPREADS_SYM_MAP_OUTFILE -o $LIFFE_SPREADS_SYM_MAP_OUTFILE

    if [ ! -f $LIFFE_SPREADS_SYM_MAP_OUTFILE ]
    then
        ERR="Unable to generate the $LIFFE_SPREADS_SYM_MAP_OUTFILE"

    elif [ ! -s $LIFFE_SPREADS_SYM_MAP_OUTFILE ]
    then
        ERR="Empty File Generated for Liffe-London-Spreads-Symbol-Mapping"
    else
        b="Newly added: "`diff $LIFFE_SPREADS_SYM_MAP_OUTFILE.new $LIFFE_SPREADS_SYM_MAP_OUTFILE`
        cat $LIFFE_SPREADS_SYM_MAP_OUTFILE $LIFFE_SPREADS_SYM_MAP_DEFAULT_OUTFILE | sort -u -o $LIFFE_SPREADS_SYM_MAP_DEFAULT_OUTFILE
        a="$b\n"`for i in 0 1 2 3 4 5; do j=$(expr $i + 1); sp=SP_LFI${i}_LFI${j}; echo "\t$sp ==> $(~/LiveExec/bin/get_exchange_symbol $sp $YYYYMMDD)\n"; done`
    fi
fi
#echo -e $a;                                                                                                                                                                                                                                

if [ ! -z "$ERR" ]; then echo $ERR | /bin/mail -s "LIFFE Spreads Symbol Generation Failed" -r "liffe-london-spreads-refdata" "rahul@tworoads.co.in"; fi #"nseall@tworoads.co.in" ; fi
if [ ! -z "$a" ]; then echo -e $a | /bin/mail -s "LIFFE Spreads Symbols" -r "liffe-london-spreads-refdata" "rahul@tworoads.co.in"; fi #"nseall@tworoads.co.in" ;  exit; fi
~/infracore/scripts/sync_file_to_all_machines.pl $LIFFE_SPREADS_SYM_MAP_OUTFILE
~/infracore/scripts/sync_file_to_all_machines.pl $LIFFE_SPREADS_SYM_MAP_DEFAULT_OUTFILE

