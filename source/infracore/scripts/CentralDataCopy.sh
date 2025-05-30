#!/bin/bash

USAGE1="$0 EXCHSOURCE DATE"
EXAMP1="$0 EUREX 20140130"

if [ $# -ne 2 ] ;
then
    echo $USAGE1;
    echo $EXAMP1;
    exit;
fi

SOURCE=$1 ; shift ;
YYYYMMDD=$1 ; shift ;

if [ "$SOURCE" == "AFL" ] 
then 
    ssh -n -f 10.23.74.54 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh AFLASH NY4 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
fi

#Only copying control msgs for NSE for now
if [ "$SOURCE" == "NSE" ]
then
    ssh -n -f 10.23.115.61 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL NSE '$YYYYMMDD' >/dev/null 2>/dev/null &" &
fi

if [ "$SOURCE" == "HK" ] 
then 
    ssh -n -f 10.152.224.145 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh HONGKONG HK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.152.224.145 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh HKOMD HK '$YYYYMMDD' >/dev/null 2>/dev/null &" & 
    ssh -n -f 10.152.224.145 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh HKOMDCPF HK '$YYYYMMDD' >/dev/null 2>/dev/null &" & 
    ssh -n -f 10.152.224.145 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL HK '$YYYYMMDD' >/dev/null 2>/dev/null &" & 
    ssh -n -f 10.152.224.145 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME HK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.152.224.145 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed HK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.152.224.145 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh OSE_L1 HK '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.152.224.146 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh HKEX HK '$YYYYMMDD' >/dev/null 2>/dev/null &" & 
fi

if [ "$SOURCE" == "TOK" ] 
then
    ssh -n -f 10.134.210.182 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh OSE TOK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.134.210.182 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed TOK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.134.210.182 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME TOK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.134.210.182 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh HKOMDCPF TOK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.134.210.182 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh OSEPriceFeed TOK '$YYYYMMDD' >/dev/null 2>/dev/null &" &   #OSEPriceFeed ( usually exchange ) here is a hack
    ssh -n -f 10.134.210.182 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh OSEOrderFeed TOK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.134.210.182 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL TOK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.134.210.182 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE TOK '$YYYYMMDD' >/dev/null 2>/dev/null &" &
fi

#CME 
if [ "$SOURCE" = "CHI" ] 
then 
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh CME CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
   
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh LIFFE CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh NTP CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE_FOD CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE_PL CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh OSEPriceFeed CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh HKOMDCPF CHI '$YYYYMMDD' >/dev/null 2>/dev/null &" &

#ssh -n -f 10.23.82.55 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh Raw CHI '$YYYYMMDD' > /spare/local/MDSlogs/Raw/copy_logs 2>&1 &" &
fi

if [ "$SOURCE" = "CME" ] 
then 
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.182.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME TOR '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.142.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME CRT '$YYYYMMDD' >/dev/null 2>/dev/null &" &
fi

## Now the order doesn't matter much. Even though the DAG of trading locations doesn't exist here, we will try to follow the order that minimizes damange!!

if [ "$SOURCE" == "BSL" ] 
then 
    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh LIFFE BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.52.52 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh LIFFE BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh LIFFE FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh LIFFE BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh LIFFE MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.74.54 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh LIFFE NY4 '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.74.54 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE NY4 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE_FOD BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE_FOD FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.74.61 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE_FOD CFE '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE_PL BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE_PL FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.74.61 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE_PL CFE '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.52.52 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CHIX BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CHIX_L1 BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" & 
    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh QUINCY BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed BSL '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.52.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh Raw BSL '$YYYYMMDD' > /spare/local/MDSlogs/Raw/copy_logs 2>&1 &" &
fi

#EUREX
if [ "$SOURCE" == "FR2" ] 
then 
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh EUREX FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBI FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EUREX_NTA FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" & 
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.74.61 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed CFE '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh QUINCY FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CSM FR2 '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.102.53 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh Raw FR2 '$YYYYMMDD' > /spare/local/MDSlogs/Raw/copy_logs 2>&1 &" &
fi

if [ "$SOURCE" == "MOS" ] 
then
    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh RTS MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.241.2 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh MICEX MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh RTS MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.12 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh RTS BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh MICEX MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.12 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh MICEX BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh RTS_P2 MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh RTSCombined MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EBS MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 172.18.244.107 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh QUINCY MOS '$YYYYMMDD' >/dev/null 2>/dev/null &" &
fi

if [ "$SOURCE" == "TOR" ]
then
    ssh -n -f 10.23.182.51 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh TMX TOR '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.182.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh TMX TOR '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.182.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL TOR '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.182.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed TOR '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.182.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ICE TOR '$YYYYMMDD' >/dev/null 2>/dev/null &" &
fi

if [ "$SOURCE" == "BRZ" ]
then
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh BMF BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL  BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh NTP  BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.142.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh NTP CRT '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh NTP_ORD  BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh PUMA BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.23.13 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh QUINCY BRZ '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.23.12 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh Raw BRZ '$YYYYMMDD' > /spare/local/MDSlogs/Raw/copy_logs 2>&1 &" &
fi

if [ "$SOURCE" == "CFE" ] 
then 
    ssh -n -f 10.23.74.61 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh CFE CFE '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.74.61 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL CFE '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.74.61 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CSM CFE '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.74.54 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CSM NY4 '$YYYYMMDD' >/dev/null 2>/dev/null &" &

    ssh -n -f 10.23.74.61 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME CFE '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.74.61 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh QUINCY CFE '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.142.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh QUINCY CRT '$YYYYMMDD' >/dev/null 2>/dev/null &" &
fi

if [ "$SOURCE" == "SYD" ]
then
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/ors_binary_log_backup.sh ASX SYD '$YYYYMMDD' >/dev/null 2>/dev/null &" &
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh Raw SYD '$YYYYMMDD' > /spare/local/MDSlogs/Raw/copy_logs 2>&1 &" &
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ASXPF SYD '$YYYYMMDD' > /dev/null 2>&1 &" &
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh ASX SYD '$YYYYMMDD' > /dev/null 2>&1 &" &
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CONTROL SYD '$YYYYMMDD' > /dev/null 2>&1 &" &
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh CME SYD '$YYYYMMDD' > /dev/null 2>&1 &" &
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh EOBIPriceFeed SYD '$YYYYMMDD' > /dev/null 2>&1 &" &
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh OSEPriceFeed SYD '$YYYYMMDD' > /dev/null 2>&1 &" &
    ssh -n -f 10.23.43.51 "/home/dvcinfra/LiveExec/scripts/mds_log_backup.sh HKOMDCPF SYD '$YYYYMMDD' > /dev/null 2>&1 &" &
fi
