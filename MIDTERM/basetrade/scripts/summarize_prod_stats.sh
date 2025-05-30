if [ $# -lt 5 ]; then echo "USAGE: $0 shc startdate enddate starttime endtime\n"; exit; fi;

shc=$1;
sd=$2;
ed=$3;
st=$4;
et=$5;

echo AVGVOL: `~/basetrade/scripts/get_avg_volume_in_timeperiod.pl $shc $sd $ed $st $et | tail -n1 | awk '{print $(NF-1), $(NF)}'`;
echo AVGL1EVENTS: `~/basetrade/scripts/get_avg_l1_events_in_timeperiod.pl $shc $sd $ed $st $et | tail -n1 | awk '{print $(NF-1), $(NF)}'`;
echo AVGL1SIZE: `~/basetrade/scripts/get_avg_l1_size_in_timeperiod.pl $shc $sd $ed $st $et | tail -n1 | awk '{print $(NF-1), $(NF)}'`
echo AVGNUMTRD: `~/basetrade/scripts/get_avg_num_trades_in_timeperiod.pl $shc $sd $ed $st $et | tail -n1 | awk '{print $(NF-1), $(NF)}'`
