#!/bin/bash

#usage: script <File_a> <No_of_months_to_run_the_pair> <startdate> <enddate> <statstartdate> <no_of_pairs_per_tranche>
N=$2;
M=$N;
((M--));

id=$(echo $1 | awk -F '/' '{print $(NF)}' | awk -F '_' '{print $(NF)}')
echo "$N parallel tranches will be created..."
for i in $(seq 0 $M);do
    sd=$(date +%Y%m%d -d "$3 + $i month");
    echo "Creating input for tranche$i...";
    while [ $sd -lt $4 ];do
        startdate=$sd;
        enddate=$(date +%Y%m%d -d "$startdate + $N month");
        enddate=$(date +%Y%m%d -d "$enddate - 1 day");

        for sf in `cat $1| awk -v "SD=$sd" -F'_' '{if($3==SD) print $1"_"$2}'|grep -v DLF|grep -v IBREALEST|grep -v HDIL| head -n"$6"|tr ['&'] '~'|awk -v"ID=$id" -v"N=$N" -v"SD=$sd" '{print $0"_"ID"_"N"_"SD}'`;do
            f="/spare1/ssingh/test/MT_SPRD_NAV_SERIES/""$sf"
            
            #truncating the head of file
            cutoff=`date +%s -d $startdate`;
            cat $nf| awk -v "CO=$cutoff" '($1 > CO)' > tmp;
            rm $nf;
            mv tmp $nf;
            
            echo $startdate" "$enddate" "$nf >> input_$i;
        done;
        sd=$(date +%Y%m%d -d "$sd + $N month");
    done;
    echo "Processing tranche$i..."
    $HOME/basetrade/hftrap/Scripts/get_consolidated_nav.pl input_$i output_$i
    rm input_$i
    echo "output_$i">>outputpaths
done;

python3 $HOME/basetrade/hftrap/Scripts/combine_overlapping_navs.py --file outputpaths --title "$id"_"$N"M_"$6"Pair_Tranche --statstart $5

#cleaning up
rm outputpaths;
for i in $(seq 0 $M);do
    rm output_$i
done;

