#!/bin/bash

## takes a file and finds the minimum at colum $1. ignore lines with less fields
f=$1
awk '{ if (NR==1) {min=$'$f'; max=$'$f'}
if(min>$'$f') min=$'$f';
if(max<$'$f') max=$'$f';
}END{print min, max, $'$f'}' $2