#!/bin/bash

if [ $# -lt 2 ] ; then echo "USAGE: $0 <shc> <date>" ; exit 0; fi;

~/basetrade_install/bin/ors_binary_reader $1 $2 | awk '{ s=$2; if($14=="Conf"){c+=1;} else if($14=="Exec" && !($16 in Ex)){e+=1; Ex[$16]=1;} } END{print s, e/c, e ,c;}';
