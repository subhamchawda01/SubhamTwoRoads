#!/bin/bash
$HOME/basetrade_install/bin/get_contract_specs NSE_$1_FUT0 $3 > abc
$HOME/basetrade_install/bin/get_contract_specs NSE_$2_FUT0 $3 > def

echo $1 $2 `cat abc|grep LOTSIZE:|awk '{print $2}'` `cat def|grep LOTSIZE|awk '{print $2}'` `cat abc | grep LAST_CLOSE_PRICE:|  awk '{print $2}'` `cat def | grep LAST_CLOSE_PRICE |  awk '{print $2}'`

