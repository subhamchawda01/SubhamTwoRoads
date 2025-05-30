#!/bin/env bash

if [ $# -lt 1 ] ; then echo "need stdoutfile"; exit; fi

stdoutfile=$1; shift;
if [ -e $stdoutfile ] ; then
    origmodel=`grep ModelFile $stdoutfile| awk '{print $3}'`;
    newmodel=`grep OutputModel $stdoutfile| awk '{print $3}'`;

    if [ $# -gt 0 ] ; then
	$HOME/basetrade/ModelScripts/compare_opt_model.py $origmodel $newmodel $1;
    else
	$HOME/basetrade/ModelScripts/compare_opt_model.py $origmodel $newmodel;
    fi
fi
