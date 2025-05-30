#!/bin/bash

if [ $# -lt 1 ] ; then echo "USAGE: <script> <MM/DD/YYYY_HH:MM:SS>"; exit ; fi;

echo $1 | replace "_" " " ":" "" | awk '{print $2/100, $1}' | awk -F"/" '{print $1, $2, $3}' | awk '{print $4$2$3, $1}'
