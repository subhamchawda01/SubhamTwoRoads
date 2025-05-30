#!/bin/bash

#Input portfolio file
PORTFOLIO_FILENAME=$HOME/PCAInfo/portfolio_inputs;
DATAGEN_INPUTDIR=$HOME/PCAInfo/datagen;
PCAOUT_DIR=$HOME/PCAInfo/pcaout;

if [ $# -lt 1 ] ; 
then 
    echo "USAGE: $0 YYYYMMDD";
    exit;
fi

YYYYMMDD=$1; shift;

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH


FINAL_PCA_FILE=$HOME/PCAInfo/pca_portfolio_stdev_eigen_$YYYYMMDD.txt

if [ -f  $PORTFOLIO_FILENAME ] ;
then
    echo $PORTFOLIO_FILENAME  "exists ";
    PORTFOLIO_NAME=`cat $PORTFOLIO_FILENAME | awk '{print $2;}'`
    #echo $PORTFOLIO_NAME

#clear old file for same date if that exists
    if [ -f $FINAL_PCA_FILE ];
    then 
	rm $FINAL_PCA_FILE
    fi

    for portfolio in $PORTFOLIO_NAME
    do
	echo "Doing for portfolio $portfolio"
	DATAGEN_INPUT_FILENAME_FOR_THIS_PORTFOLIO=$DATAGEN_INPUTDIR/tdata_$portfolio.out.datagen.txt;
	PCAOUT_FILENAME_FOR_THIS_PORTFOLIO=$PCAOUT_DIR/$portfolio.pca.out;
	if [ -f $PCAOUT_FILENAME_FOR_THIS_PORTFOLIO ];
	then
	    rm  $PCAOUT_FILENAME_FOR_THIS_PORTFOLIO
	fi
	
	echo "$HOME/trade_strats/test_svd_with_cv $portfolio $DATAGEN_INPUT_FILENAME_FOR_THIS_PORTFOLIO $PCAOUT_FILENAME_FOR_THIS_PORTFOLIO"
	$HOME/trade_strats/test_svd_with_cv $portfolio $DATAGEN_INPUT_FILENAME_FOR_THIS_PORTFOLIO $PCAOUT_FILENAME_FOR_THIS_PORTFOLIO

	cat $PCAOUT_DIR/$portfolio.pca.out >> $FINAL_PCA_FILE
    #Clear the individual files; E.g UBFUT.pca.out and tdata_UBFUT.out.datagen.txt

	if [ -f $DATAGEN_INPUT_FILENAME_FOR_THIS_PORTFOLIO ] ;
	then 
	    rm $DATAGEN_INPUT_FILENAME_FOR_THIS_PORTFOLIO
	fi
	
    done

fi

for name in $PCAOUT_DIR/*.pca.out ; 
do
    rm -f $name;
done

