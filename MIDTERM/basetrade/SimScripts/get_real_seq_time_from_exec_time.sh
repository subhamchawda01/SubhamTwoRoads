#!/bin/bash

if [ $# -lt 3 ] ; then echo "USAGE: $0 <shc> <dt> <exec_time>"; exit 0; fi;

shc=$1;
dt=$2;
exec_time_=$3;
$HOME/basetrade_install/bin/ors_binary_reader $shc $dt | grep Exec | awk -vet=$exec_time_ '{if($10==et){print $12; exit;}}'
