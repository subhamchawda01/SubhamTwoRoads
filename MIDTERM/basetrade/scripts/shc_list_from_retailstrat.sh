#!/bin/bash

for file in `awk '{if($1=="STRUCTURED_STRATEGYLINE"){print $2}}' $1`; do echo `awk '{if($1=="STRUCTURED_TRADING"){print $2}}' $file` ; done; 
awk '{if($1=="STRATEGYLINE"){print $2}}' $1;
