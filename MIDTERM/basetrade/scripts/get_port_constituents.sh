#!/bin/bash

a=/spare/local/tradeinfo/PCAInfo; 
port_inp=`ls -ltr  $a | grep portfolio_inputs| awk '{print $NF}' | tail -n1`
eigen_fl=`ls -ltr  $a | grep portfolio_stdev | awk '{print $NF}' | tail -n1`
#echo $eigen_fl;
grep -w $1 $a/$port_inp; 
grep -w $1 $a/$eigen_fl; 