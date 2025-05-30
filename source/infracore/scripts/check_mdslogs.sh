#!/bin/bash

USAGE="$0 EXCH";

if [ $# -ne 1 ] ;
then
    echo $USAGE;
    exit;
fi

EXCH=$1; shift;

EXCH_SYMBOL_EXEC_=$HOME/LiveExec/bin/get_exchange_symbol

REFDIR=/spare/local/files;

REFFILE="NONAME";
MDSDIR=/spare/local/MDSlogs;
MAIL_FILE=/tmp/reference_mode_products_check.txt

TODAY=`date +"%Y%m%d"` ;
date=`date "+%Y%m%d"`
>$MAIL_FILE ;

case $EXCH in

    cme|CME)

        REFFILE=$REFDIR/$EXCH/cme-ref.txt ;
        PRODUCTLIST=$REFDIR/cme_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/GENERIC/;
        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then


                echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in MDS DIR : " $LOGSDIR  " on: " $date >> $MAIL_FILE ;
            fi

        done

        ;;
    eurex|EUREX)

        REFFILE=$REFDIR/$EXCH/eurex-prod-codes.txt ;
        PRODUCTLIST=$REFDIR/eurex_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/GENERIC/;
        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

                echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in MDS DIR : " $LOGSDIR  " on: " $date >> $MAIL_FILE ;
            fi

        done

        ;;
    tmx|TMX)
        REFFILE=$REFDIR/$EXCH/tmx-prod-codes.txt ;
        PRODUCTLIST=$REFDIR/tmx_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/$EXCH/;
        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then


                echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in MDS DIR : " $LOGSDIR  " on: " $date >> $MAIL_FILE ;
            fi

        done



        ;;
    NTP|ntp)

        REFFILE=$REFDIR/BMF/ntp-ref.txt ;
        PRODUCTLIST=$REFDIR/ntp_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/NTP/
        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi
            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

                echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in MDS DIR : " $LOGSDIR  " on: " $date >> $MAIL_FILE ;
            fi

        done

        ;;
    bmf|BMF|bmfep|BMFEP)

        REFFILE=$REFDIR/$EXCH/bmf-ref.txt ;
        PRODUCTLIST=$REFDIR/bmf_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/$EXCH/
        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

                echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in MDS DIR : " $LOGSDIR  " on: " $date >> $MAIL_FILE ;

            fi

        done

        ;;

    liffe|LIFFE)

        REFFILE=$REFDIR/$EXCH/liffe-ref.txt ;
        PRODUCTLIST=$REFDIR/liffe_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/GENERIC/

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi
            this_exch_symbol_=`echo  "$this_exch_symbol_" | tr ' ' '~'`;
            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

                echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in MDS DIR : " $LOGSDIR  " on: " $date >> $MAIL_FILE ;
            fi

        done

        ;;
    quincy|QUINCY)

        REFFILE=$REFDIR/$EXCH/quincy-ref.txt ;
        PRODUCTLIST=$REFDIR/quincy_ref_products_to_check.txt ;
	LOGSDIR=$MDSDIR/QUINCY/

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

		echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

		continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

		echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in REF FILE : " $REFFILE >> $MAIL_FILE ;

            fi

        done

        ;;
    hkex|HKEX|HK|hk)

        REFFILE=$REFDIR/$EXCH/hkex-ref.txt ;
        PRODUCTLIST=$REFDIR/hkex_ref_products_to_check.txt ;
	LOGSDIR=$MDSDIR/HKEX/

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

		echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

		continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

		echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in REF FILE : " $REFFILE >> $MAIL_FILE ;

            fi

        done

        ;;
    hkomd|HKOMD)
        PRODUCTLIST=$REFDIR/hkomd_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/HKOMD/;
        total_ref_products_=`wc -l $PRODUCTLIST` ;

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then
                echo "NO ENTRY FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in MDS DIR : " $LOGSDIR  " on: " $date >> $MAIL_FILE ;
            fi

        done
        ;;
     RTS_P2|rts_p2)

        REFFILE=$REFDIR/$EXCH/rts_p2-ref.txt ;
        PRODUCTLIST=$REFDIR/rts_p2_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/RTS_P2/

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

                echo "NO MDSlog file FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in LOGSDIR : " $LOGSDIR >> $MAIL_FILE ;

            fi

        done

        ;;
    RTS|rts)

        REFFILE=$REFDIR/$EXCH/rts-ref.txt ;
        PRODUCTLIST=$REFDIR/rts_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/GENERIC/

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

                echo "NO MDSlog file FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in LOGSDIR : " $LOGSDIR >> $MAIL_FILE ;

            fi

        done

        ;;
    MICEX|micex)

        REFFILE=$REFDIR/$EXCH/micex-ref.txt ;
        PRODUCTLIST=$REFDIR/micex_ref_products_to_check.txt ;
        LOGSDIR=$MDSDIR/GENERIC/

        for product in `cat $PRODUCTLIST`
        do

            this_exch_symbol_=`$EXCH_SYMBOL_EXEC_ $product $TODAY` ;

            if [ "X$this_exch_symbol_" == "X" ]
            then

                echo "Couldn't Find Exchange Symbol For Given Shortcode : " $product >> $MAIL_FILE ;

                continue ;

            fi

            product_search_bool_=`ls $LOGSDIR| grep $this_exch_symbol_ | grep $date | wc -l` ;

            if [ $product_search_bool_ -lt 1 ]
            then

                echo "NO MDSlog file FOR SHORTCODE : " $product " EXCH SYMBOL : " $this_exch_symbol_ " in LOGSDIR : " $LOGSDIR >> $MAIL_FILE ;

            fi

        done

        ;;


    *)

        echo You did not chose CME, EUREX, TMX or BMF, LIFFE, HKEX, RTS_P2, RTS, MICEX, QUINCY
        ;;
esac

LINE_COUNT=`wc -l $MAIL_FILE | awk '{print $1}'`

if [ $(($LINE_COUNT)) -gt 0 ]
then

    /bin/mail -s "MDS DATA ALERT" -r "mds_check@`hostname`" "nseall@tworoads.co.in" < $MAIL_FILE
fi

rm -rf $MAIL_FILE ;

