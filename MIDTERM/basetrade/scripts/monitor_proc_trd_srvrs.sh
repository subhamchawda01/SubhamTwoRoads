#!/bin/bash

# Get the execs to probe for.
ORS_EXEC=cme_ilink_ors;
FIXFAST_EXEC=fixfast-mds_MCAST;
MDSLOGGER_EXEC=mds_logger_MCAST;
TMXD_EXEC=TMX-mds;
TMXATR_EXEC=TMXATR;

EMAILMESSAGE="/tmp/s_email_msg.txt"

for (( i = 0; i < 45; i++ ))
do
    OLD_RETVAL[$i]=0; # Assume in the beginning that no process is running.
done;

while [ true ]
do
## chi srvrs
    RETVAL[0]=$(ssh dvcinfra@sdv-chi-srv11 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[1]=$(ssh dvcinfra@sdv-chi-srv11 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[2]=$(ssh dvcinfra@sdv-chi-srv11 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[3]=$(ssh dvcinfra@sdv-chi-srv11 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[4]=$(ssh dvcinfra@sdv-chi-srv11 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

    RETVAL[5]=$(ssh dvcinfra@sdv-chi-srv12 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[6]=$(ssh dvcinfra@sdv-chi-srv12 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[7]=$(ssh dvcinfra@sdv-chi-srv12 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[8]=$(ssh dvcinfra@sdv-chi-srv12 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[9]=$(ssh dvcinfra@sdv-chi-srv12 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

    RETVAL[10]=$(ssh dvcinfra@sdv-chi-srv13 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[11]=$(ssh dvcinfra@sdv-chi-srv13 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[12]=$(ssh dvcinfra@sdv-chi-srv13 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[13]=$(ssh dvcinfra@sdv-chi-srv13 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[14]=$(ssh dvcinfra@sdv-chi-srv13 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

    RETVAL[15]=$(ssh dvcinfra@sdv-chi-srv14 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[16]=$(ssh dvcinfra@sdv-chi-srv14 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[17]=$(ssh dvcinfra@sdv-chi-srv14 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[18]=$(ssh dvcinfra@sdv-chi-srv14 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[19]=$(ssh dvcinfra@sdv-chi-srv14 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

## fr2 srvrs

    RETVAL[20]=$(ssh dvcinfra@sdv-fr2-srv11 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[21]=$(ssh dvcinfra@sdv-fr2-srv11 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[22]=$(ssh dvcinfra@sdv-fr2-srv11 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[23]=$(ssh dvcinfra@sdv-fr2-srv11 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[24]=$(ssh dvcinfra@sdv-fr2-srv11 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

    RETVAL[25]=$(ssh dvcinfra@sdv-fr2-srv12 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[26]=$(ssh dvcinfra@sdv-fr2-srv12 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[27]=$(ssh dvcinfra@sdv-fr2-srv12 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[28]=$(ssh dvcinfra@sdv-fr2-srv12 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[29]=$(ssh dvcinfra@sdv-fr2-srv12 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

    RETVAL[30]=$(ssh dvcinfra@sdv-fr2-srv13 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[31]=$(ssh dvcinfra@sdv-fr2-srv13 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[32]=$(ssh dvcinfra@sdv-fr2-srv13 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[33]=$(ssh dvcinfra@sdv-fr2-srv13 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[34]=$(ssh dvcinfra@sdv-fr2-srv13 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

    RETVAL[35]=$(ssh dvcinfra@sdv-fr2-srv14 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[36]=$(ssh dvcinfra@sdv-fr2-srv14 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[37]=$(ssh dvcinfra@sdv-fr2-srv14 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[38]=$(ssh dvcinfra@sdv-fr2-srv14 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[39]=$(ssh dvcinfra@sdv-fr2-srv14 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

## tor srvrs

    RETVAL[40]=$(ssh dvcinfra@38.64.128.227 'ps -efH' | grep cme_ilink_ors | grep -v grep | wc -l);
    RETVAL[41]=$(ssh dvcinfra@38.64.128.227 'ps -efH' | grep fixfast-mds_MCAST | grep -v grep | wc -l);
    RETVAL[42]=$(ssh dvcinfra@38.64.128.227 'ps -efH' | grep mds_logger_MCAST | grep -v grep | wc -l);
    RETVAL[43]=$(ssh dvcinfra@38.64.128.227 'ps -efH' | grep TMX-mds | grep -v grep | wc -l);
    RETVAL[44]=$(ssh dvcinfra@38.64.128.227 'ps -efH' | grep TMXATR | grep -v grep | wc -l);

    echo -e "Here is a quick run down on the processes on our trading servers:\n\n" > $EMAILMESSAGE;
    CHANGED=0; # Assume nothing changed.

    # Loop through all ret vals and see what changed from last time.
    for (( i = 0; i < 45; i++ ))
    do
	case $i in
	    0)
		echo -e "---------------------------------------------------------------------\nchi srv 11:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    5)
		echo -e "---------------------------------------------------------------------\nchi srv 12:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    10)
		echo -e "---------------------------------------------------------------------\nchi srv 13:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    15)
		echo -e "---------------------------------------------------------------------\nchi srv 14:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    20)
		echo -e "---------------------------------------------------------------------\nfr2 srv 11:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    25)
		echo -e "---------------------------------------------------------------------\nfr2 srv 12:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    30)
		echo -e "---------------------------------------------------------------------\nfr2 srv 13:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    35)
		echo -e "---------------------------------------------------------------------\nfr2 srv 14:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    40)
		echo -e "---------------------------------------------------------------------\ntor srv 11:\n---------------------------------------------------------------------\n" >> $EMAILMESSAGE;
		;;
	    *)
		;;
	esac

	if [ ${RETVAL[$i]} != ${OLD_RETVAL[$i]} ] ;
	then
	    OLD_RETVAL[$i]=${RETVAL[$i]}; # This is the current state.

	    CHANGED=1; # Send an email, something changed.

	    i_MOD_5=$(($i % 5));

	    if [ $i_MOD_5 = 0 ] ; then
		if [ ${RETVAL[$i]} = 1 ] ; then echo "cme_ilink_ors just STARTED" >> $EMAILMESSAGE; fi;
		if [ ${RETVAL[$i]} = 0 ] ; then echo "cme_ilink_ors just STOPPED" >> $EMAILMESSAGE; fi;
	    fi;
	    if [ $i_MOD_5 = 1 ] ; then
		if [ ${RETVAL[$i]} = 1 ] ; then echo "fixfast-mds_MCAST just STARTED" >> $EMAILMESSAGE; fi;
		if [ ${RETVAL[$i]} = 0 ] ; then echo "fixfast-mds_MCAST just STOPPED" >> $EMAILMESSAGE; fi;
	    fi;
	    if [ $i_MOD_5 = 2 ] ; then
		if [ ${RETVAL[$i]} = 1 ] ; then echo "mds_logger_MCAST just STARTED" >> $EMAILMESSAGE; fi;
		if [ ${RETVAL[$i]} = 0 ] ; then echo "mds_logger_MCAST just STOPPED" >> $EMAILMESSAGE; fi;
	    fi;
	    if [ $i_MOD_5 = 3 ] ; then
		if [ ${RETVAL[$i]} = 1 ] ; then echo "TMX-mds just STARTED" >> $EMAILMESSAGE; fi;
		if [ ${RETVAL[$i]} = 0 ] ; then echo "TMX-mds just STOPPED" >> $EMAILMESSAGE; fi;
	    fi;
	    if [ $i_MOD_5 = 4 ] ; then
		if [ ${RETVAL[$i]} = 1 ] ; then echo "TMXATR just STARTED" >> $EMAILMESSAGE; fi;
		if [ ${RETVAL[$i]} = 0 ] ; then echo "TMXATR just STOPPED" >> $EMAILMESSAGE; fi;
	    fi;
	fi
    done;

    if [ $CHANGED = 1 ] ; then
	SUBJECT="Processes on Trading servers";
	EMAIL="nseall@tworoads.co.in";

	echo -e "\n\n Someone do something\n\n- S\n" >> $EMAILMESSAGE;

	/bin/mail -s "$SUBJECT" "$EMAIL" < $EMAILMESSAGE
    fi;

    sleep 900;
done;