#!/bin/bash

if [ $# -gt 0 ] ; then
    grep "INDICATOR " $1 | awk '{ if ( $2 * $NF > 0 ) { $2 = "1.00" ; print $0 ; } else { $2 = "1.00" ; $NF = - $NF ; print $0 ; } }' | sort | uniq
fi
