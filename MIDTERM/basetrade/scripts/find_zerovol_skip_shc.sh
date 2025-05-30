#!/bin/bash

USAGE="$0 0vol_perc_threshold";
if [ $# -lt 1 ] ;
then
  echo $USAGE;
  exit;
fi

ZEROVOL_PERC_THRESH=$1; shift;

/home/dvctrader/basetrade/pylib/mysql_fetch_rows.py -c "select shortcode, avg(perc) as avg_perc from (select t.configid, t1.cnt / t.cnt * 100 as perc from (select configid, count(*) as cnt from wf_results where vol = 0 and date > 20160101 group by configid) t1 inner join (select configid, count(*) as cnt from wf_results where date > 20160101 group by configid) t on t.configid = t1.configid) PP inner join wf_configs on PP.configid = wf_configs.configid where type='N' and strat_type='Regular' group by shortcode order by avg_perc desc" | tail -n +3 | awk '{if ($2>40) {print $1}}'


