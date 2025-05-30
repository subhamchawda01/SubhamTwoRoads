#!/bin/bash

verbose_level=0
USAGE="$0 EXECNAME verbose=0";
if [ $# -lt 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

EXECNAME=$1
shift;
if [ $# -gt 0 ] ; then
    verbose_level=$1
    shift;
fi

LIVE_BIN_DIR=$HOME/LiveExec/bin ;
LIVE_BACK_BIN_DIR=$HOME/LiveExecBackup/bin ;

mkdir -p $LIVE_BIN_DIR ;

# mkdir -p $HOME/LiveExec/bindebug ;
# install -p --backup=t -t $HOME/LiveExec/bindebug/ $HOME/basetrade_install/bindebug/$EXECNAME ;

search_path_list=("$HOME/cvquant_install/dvccode/bin" "$HOME/cvquant_install/dvctrade/bin" "$HOME/cvquant_install/basetrade/bin" "$HOME/basetrade_install/bin") ;

found_exec=0 ;
LIVE_EXECNAME=$LIVE_BIN_DIR/$EXECNAME ;
for search_path in "${search_path_list[@]}"; do   # The quotes are necessary here
    BIN_EXECNAME=$search_path/$EXECNAME ;
    if [ $verbose_level -gt 0 ] ; then
	echo "Searching exec in $search_path/$EXECNAME" ;
    fi
    if [ -d $search_path ] && [ -e $BIN_EXECNAME ] ; then
	need_to_sync=0
	if [ ! -e $LIVE_EXECNAME ] ; then
	    # File not present
	    need_to_sync=1
	else
	    # File present check if same
	    diff -q $LIVE_EXECNAME $BIN_EXECNAME;
	    if [ $? -ne 0 ] ; then
		# In this loop if there is a difference
		need_to_sync=1;
	    fi
	fi
	if [ $need_to_sync -gt 0 ] ; then

	    if [ $USER == "dvctrader" ] || [ $USER == "dvcinfra" ] || [ $USER == "pengine" ]; then
		# If this is a production user then backup as well
		install -p --backup=t -t $LIVE_BACK_BIN_DIR $BIN_EXECNAME ;
	    fi

	    # For all users install to LIVE_BIN_DIR
	    install -p --backup=t -t $LIVE_BIN_DIR $BIN_EXECNAME ;
	    if [ $verbose_level -gt 0 ] ; then
		echo "install -p --backup=t -t $LIVE_BIN_DIR $BIN_EXECNAME" ;
	    fi
	fi
	found_exec=1 ;
	break ; # need to break once found to avoid searching in following directories
    fi
done

if [ $found_exec -lt 1 ] ; then
    HOSTNAME=`hostname -s`;
    echo "On $HOSTNAME missing exec $EXECNAME in all search paths." ;
fi

# # This is to delete old copies
# find $HOME/LiveExec -name *~ -type f -mtime +60 -exec rm -f {} \;
