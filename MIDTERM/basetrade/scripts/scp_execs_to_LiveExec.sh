#!/bin/bash

USAGE="$0 EXEC_NAME ... ";

while [ $# -gt 0 ]
do
    exec_name=$1;
    shift;

    if [ -f $HOME/infracore_install/bin/$exec_name ] ;
    then

	for name in sdv-chi-srv11 sdv-chi-srv12 sdv-chi-srv13 sdv-chi-srv14 sdv-fr2-srv11 sdv-fr2-srv12 sdv-fr2-srv13 sdv-fr2-srv14
	do
	    echo $name;
	    rsync -avz $HOME/infracore_install/bin/$exec_name dvcinfra@$name:/home/dvcinfra/infracore_install/bin/
	    ssh dvcinfra@$name "/home/dvcinfra/infracore/scripts/set_live_exec.sh $exec_name"
	done
    fi
done