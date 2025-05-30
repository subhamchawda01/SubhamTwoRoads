#!/bin/bash

if [ $# -gt 0 ]; then
    for i in "$@"; do
        for loc in `grep -v "10.0.0." /mnt/sdf/JOBS/all_instances.txt | grep running | awk '{print $NF}'`;
        do
            dir=`dirname $i`
            ssh -o ConnectTimeout=60 $loc "mkdir -p $dir"
            rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" /mnt/sdf/$i $loc:$i;
            done
    done
    exit $?;
fi

echo "Updating local exec directories on workers";
#Updating local exec directories on workers
for loc in `grep -v "10.0.0." /mnt/sdf/JOBS/all_instances.txt | grep running | awk '{print $NF}'`;
do
    echo $loc;
    rsync -ravz --timeout=60 --exclude=bindebug -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"  --exclude=*.a /mnt/sdf/cvquant_install/basetrade/ $loc:/media/ephemeral1/cvquant_install/basetrade/;
    rsync -ravz --timeout=60 --exclude=bindebug -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --exclude=*.a /mnt/sdf/cvquant_install/dvctrade/bin $loc:/media/ephemeral1/cvquant_install/basetrade/;
    rsync -ravz --timeout=60 --exclude=bindebug -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --exclude=*.a /mnt/sdf/cvquant_install/dvccode/bin $loc:/media/ephemeral1/cvquant_install/basetrade/;
    rsync -ravz --timeout=60 --exclude=bindebug -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --delete-after --exclude=*.a /mnt/sdf/cvquant_install/infracore/ $loc:/media/ephemeral1/cvquant_install/infracore/;
    rsync -ravz --timeout=60 --exclude=bindebug -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --exclude=*.a /mnt/sdf/cvquant_install/dvccode/bin $loc:/media/ephemeral1/cvquant_install/infracore/;
    rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --delete-after /mnt/sdf/LiveExec $loc:/media/ephemeral1/;
done

#Updating autoscaling folder
rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" /mnt/sdf/dvccode/scripts/datainfra/celeryFiles ec2-user@52.91.139.132:;

#Updating EODPnl files on workers
for loc in `grep -v "10.0.0." /mnt/sdf/JOBS/all_instances.txt | grep running | awk '{print $NF}'`;
do
    rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" /apps/data/MFGlobalTrades $loc:/apps/data/;
done

#for loc in `cat /mnt/sdf/JOBS/all_instances.txt | grep running | grep -v "10.0.0.11" | awk '{ print $NF; }'` ; do 
    #ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $loc 'hostname ; ls -R /mnt/sdf/basetrade/ | wc -l; ls -R /mnt/sdf/infracore/ | wc -l; ls -R /mnt/sdf/spare_local_* | wc -l; ~/basetrade/scripts/setup_modelling_live_execs.sh;';    #ls to update execs at workers, nfs has a issue
#done

for loc in `cat /mnt/sdf/JOBS/all_instances.txt | grep running | grep -v "10.0.0.11" | awk '{ print $NF; }'` ;
do
    ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60 $loc 'hostname ; md5sum /home/dvctrader/basetrade_install/bin/sim_strategy /home/dvctrader/LiveExec/bin/sim_strategy'; 
done

#Sync pengine configs
for loc in `cat /mnt/sdf/JOBS/all_instances.txt | grep running | grep -v "10.0.0.11" | awk '{ print $NF; }'` ;
do
    rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --delete-after /mnt/sdf/pengine/prod/live_configs/. $loc:/home/pengine/prod/live_configs; 
done
