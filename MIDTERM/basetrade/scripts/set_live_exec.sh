#!/bin/bash

USAGE="$0 EXECNAME ";
if [ $# -ne 1 ] ; 
then 
    echo $USAGE;
    exit;
fi

EXECNAME=$1
shift;

mkdir -p $HOME/LiveExec/bin ;

# mkdir -p $HOME/LiveExec/bindebug ;
# install -p --backup=t -t $HOME/LiveExec/bindebug/ $HOME/basetrade_install/bindebug/$EXECNAME

search_path_list=("$HOME/cvquant_install/dvccode/bin" "$HOME/cvquant_install/dvctrade/bin" "$HOME/cvquant_install/basetrade/bin" "$HOME/basetrade_install/bin") ;

found_exec=0
for search_path in "${search_path_list[@]}"; do   # The quotes are necessary here
    if [ -e $search_path/$EXECNAME ] ; then
	echo "install -p --backup=t -t $HOME/LiveExec/bin/ $search_path/$EXECNAME"
	found_exec=1
    fi
done

if [ $found_exec -lt 1 ] then
    HOSTNAME=`hostname -s`;
    echo "On $HOSTNAME missing exec $EXECNAME in all search paths." ;
fi

# # This is to delete old copies
# find $HOME/LiveExec -name *~ -type f -mtime +60 -exec rm -f {} \;
