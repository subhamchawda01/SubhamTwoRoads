#!/bin/bash


export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

fl=$1
col=$2
mean=$(awk '{s+=$'$2'}END{print s/NR}' $fl)
cat $fl | awk '{$'$col' =  $'$col'/'$mean'; print $0 }'
