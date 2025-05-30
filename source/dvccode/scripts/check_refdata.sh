#!/bin/bash

USAGE="$0 EXCH";

if [ $# -ne 1 ] ;
then
    echo $USAGE;
    exit;
fi

EXCH=$1; shift;

EXCH_SYMBOL_EXEC_=/home/pengine/prod/live_execs/get_exchange_symbol

REFDIR=/spare/local/files;

REFFILE="NONAME";

MAIL_FILE=/tmp/reference_mode_products_check.txt 

TODAY=`date +"%Y%m%d"` ;

>$MAIL_FILE ;

case $EXCH in

    cme|CME)

        REFFILE=$REFDIR/CMEMDP/cme-ref.txt ;
        PRODUCTLIST=$REFDIR/cme_ref_products_to_check.txt ;

        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do 

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then 
		
		echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

		continue ;

            fi      
            
            product_search_bool_=`grep "$this_exch_symbol_" $REFFILE | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then 

		echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in REF FILE : " $REFFILE >> $MAIL_FILE ;

            fi     

        done
        
        ;;
    csm|CSM)

        REFFILE=$REFDIR/CSM/csm-ref.txt ;
        PRODUCTLIST=$REFDIR/csm_ref_products_to_check.txt ;

        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do 

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then 
		
		echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

		continue ;

            fi      
            
            product_search_bool_=`grep "$this_exch_symbol_" $REFFILE | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then 

		echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in REF FILE : " $REFFILE >> $MAIL_FILE ;

            fi     

        done
        
        ;;
    eurex|EUREX)

        REFFILE=$REFDIR/$EXCH/eobi-prod-codes.txt ;
        PRODUCTLIST=$REFDIR/eurex_ref_products_to_check.txt ;

        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do 

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then 
		
		echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

		continue ;

            fi      
	    
            product_search_bool_=`grep "$this_exch_symbol_" $REFFILE | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then 

		echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in REF FILE : " $REFFILE >> $MAIL_FILE ;

            fi     

        done
	
        ;;
    tmx|TMX)


        ;;
    bmf|BMF|bmfep|BMFEP)

        REFFILE=$REFDIR/$EXCH/ntp-ref.txt ;
        PRODUCTLIST=$REFDIR/ntp_ref_products_to_check.txt ;

        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do 

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then 
		
		echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

		continue ;

            fi      
	    
            product_search_bool_=`grep "$this_exch_symbol_" $REFFILE | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then 

		echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in REF FILE : " $REFFILE >> $MAIL_FILE ;

            fi     

        done
	
        ;;
    liffe|LIFFE)

        REFFILE=$REFDIR/$EXCH/liffe-ref.txt ;
        PRODUCTLIST=$REFDIR/liffe_ref_products_to_check.txt ;

        for product in `cat $PRODUCTLIST`
        do 

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then 
		
		echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

		continue ;

            fi      
	    
            product_search_bool_=`grep "$this_exch_symbol_" $REFFILE | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then 

		echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in REF FILE : " $REFFILE >> $MAIL_FILE ;

            fi     

        done
	
        ;;

       ice|ICE)

        REFFILE=$REFDIR/$EXCH/ice-ref.txt ;
        PRODUCTLIST=$REFDIR/ice_ref_products_to_check.txt ;

        for product in `cat $PRODUCTLIST`
        do 
            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then 
        
        echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

        continue ;

            fi      
        
            product_search_bool_=`grep "$this_exch_symbol_" $REFFILE | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then 

        echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in REF FILE : " $REFFILE >> $MAIL_FILE ;

            fi     

        done
    
        ;;
    *)

        echo You did not chose CME, EUREX, TMX ,BMF, LIFFE or ICE
        ;;
esac

LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then

    /bin/mail -s "REFERENCE-DATA-ALERT" -r "reference_check@`hostname`" "nseall@tworoads.co.in" < $MAIL_FILE
fi

rm -rf $MAIL_FILE ;

