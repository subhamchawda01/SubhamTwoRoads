#!/bin/bash

awk '{ if($1=="STRUCTURED_STRATEGYLINE"){print $3;} }' $1;
awk '{ if($1=="STRATEGYLINE"){print $8} }' $1;
