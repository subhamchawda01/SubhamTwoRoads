#!/bin/bash

export IFS=";"
if [ $# -ne 1 ];
then 
    echo "$0 DATE";
    exit;
fi
YYYYMMDD=$1;

if [ $YYYYMMDD = "TODAY" ] ;
then
    YYYYMMDD=$(date "+%Y%m%d")
fi

EXEC=$HOME/basetrade_install/bin/get_avg_l1ord_on_day
DEST_DIR=/spare/local/tradeinfo/L1OrderInfo/
if [ ! -d $DEST_DIR ];
then
    mkdir -p $DEST_DIR 
fi
id_=`date +%s%n`;

List_of_symbols="VX_0;VX_1;VX_2;VX_3;VX_4;VX_5;ABEV3;ALLL3;RUMO3;BBAS3;BBDC3;BBDC4;BBSE3;BRAP4;BRFS3;BRKM5;BRML3;BRPR3;BVMF3;CCRO3;CESP6;CIEL3;CMIG4;CPFE3;CPLE6;CRUZ3;CSAN3;CSNA3;CTIP3;CYRE3;DTEX3;ECOR3;ELET3;ELET6;ELPL4;EMBR3;ENBR3;ESTC3;EVEN3;FIBR3;GFSA3;GGBR4;GOAU4;GOLL4;HGTX3;HYPE3;ITSA4;ITUB4;JBSS3;KLBN11;KROT3;LAME4;LIGT3;LREN3;MRFG3;MRVE3;NATU3;OIBR4;PCAR4;PDGR3;PETR3;PETR4;POMO4;QUAL3;RENT3;RLOG3;RSID3;SANB11;SBSP3;SUZB5;TBLE3;TIMP3;UGPA3;USIM5;VALE3;VALE5;VIVT4;SMLE3;GE_0;GE_1;GE_2;GE_3;GE_4;GE_5;GE_6;GE_7;GE_8;GE_9;GE_10;GE_11;GE_12;GE_13;GE_14;GE_15;GE_16;BOVA11;FGBM_0;FGBS_0;ZT_0;ZN_0;ZF_0;"

for symbol in $List_of_symbols;
do
    symbol_file_=$DEST_DIR"/"$symbol;

    if [ -e $symbol_file_ ]; then
      grep $YYYYMMDD $symbol_file_ >/dev/null 2>/dev/null;
      if [ $? -eq 1 ]; then
      echo $symbol
        echo "Adding avg l1 size for $symbol for $YYYYMMDD";
        $EXEC $symbol $YYYYMMDD 800 1600 | awk -v d="$YYYYMMDD" '{ if($3 != 0) print $1" "d" "$3;}' |  cat - $DEST_DIR"/"$symbol | sort -nk2 -r | awk 'BEGIN{nextdate = 20991212}{if(nextdate > $2){print $1" "$2" "$3}; nextdate = $2}' > $DEST_DIR"/temp_$symbol$id_";
        mv $DEST_DIR"/temp_$symbol$id_" $DEST_DIR"/"$symbol
      fi;
    else
      echo "Adding avg l1 size for $symbol for $YYYYMMDD";
      $EXEC $symbol $YYYYMMDD 800 1600 | awk -v d="$YYYYMMDD" '{ if($3 != 0) print $1" "d" "$3;}'| sort -nk2 -r | awk 'BEGIN{nextdate = 20991212}{if(nextdate > $2){print $1" "$2" "$3}; nextdate = $2}' >> $symbol_file_;
   fi; 
done

$HOME/basetrade/scripts/sync_dir_to_all_machines.pl $DEST_DIR;
