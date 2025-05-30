#!/bin/bash
if [ $# -lt 1 ]; then echo "usage: "$0" config-file"; exit 1 ; fi

t_config_file=$1
t_run_date_=`date +\%Y\%m\%d`

SCRIPTS_DIR=$HOME/LiveExec/scripts
$SCRIPTS_DIR/optimize_risk_html.pl $t_config_file $t_run_date_
