#!/bin/bash
# Wait till ec2-user creates directories
sleep 10
echo "---------------------------------"
echo `date`
echo "running startup script for worker"

#Create NAS1
ln -s /media/shared/ephemeral1/s3_cache/NAS1/logs /NAS1/logs

# Assuming Worker 15 is always alive
#rsync -raz dvctrader@10.0.1.15:/media/disk-ephemeral1/apps /media/disk-ephemeral1/
echo "Synced /apps"
#rsync -raz /mnt/sdf/ec2_globalresults /media/disk-ephemeral1/
echo "Synced ec2_globalresults"
#rsync -raz /mnt/sdf/basetrade_install /media/disk-ephemeral1/
echo "Synced basetrade_install"
#rsync -raz /mnt/sdf/LiveExec /media/disk-ephemeral1/
echo "Synced LiveExec"
#rsync -raz /mnt/sdf/infracore_install /media/disk-ephemeral1/
echo "Synced infracore_install"
ln -s /media/shared/ephemeral2/SampleData /NAS1/SampleData
ln -s /NAS1/SampleData /spare/local/Features
#rsync -raz /mnt/sdf/modelling /media/disk-ephemeral1/
#cd /media/disk-ephemeral1/
#git clone gitolite@ny11:modelling
#cd modelling
#git checkout devmodel
echo "Synced modelling"
find /media/shared/ -maxdepth 2  -type f
echo "Discovered the size of each mounted disk"
echo "Instance is ready for scheduling"
#new_ip=`ifconfig | grep "inet addr" | grep "10.0.1" | awk '{print $2}' | cut -d':' -f2`
#new_ip+=" 32"
#ssh -o StrictHostKeyChecking=no 10.0.0.11 << APPEND_SCRIPT
#  echo $new_ip >> /home/dvctrader/AWSScheduler/instance_cores.txt
#APPEND_SCRIPT
#echo "random-id running nil $new_ip" >> /mnt/sdf/JOBS/all_instances.txt
echo "Started scheduling processes"
echo "---------------------------------"
echo "---------------------------------"
