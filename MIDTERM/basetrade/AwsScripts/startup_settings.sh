#!/bin/bash
# Wait till ec2-user creates directories
sleep 10
echo "---------------------------------"
echo `date`
echo "running startup script for worker"

# Create NAS1
ln -s /media/shared/ephemeral1/s3_cache/NAS1/logs /NAS1/logs

# Assuming Worker 15 is always alive
# Syncing /apps from worker 15 to this machine
rsync -raz dvctrader@10.0.1.15:/media/disk-ephemeral1/apps /media/disk-ephemeral1/
echo "Synced /apps"
rsync -raz /mnt/sdf/ec2_globalresults /media/disk-ephemeral1/
echo "Synced ec2_globalresults"
rsync -raz --timeout=60 --exclude=bindebug --exclude=*.a /mnt/sdf/cvquant_install/basetrade/ /media/disk-ephemeral1/cvquant_install/basetrade/
echo "Synced cvquant_install/basetrade"
rsync -raz --timeout=60 --exclude=bindebug --exclude=*.a /mnt/sdf/cvquant_install/dvctrade/bin /media/disk-ephemeral1/cvquant_install/basetrade/
echo "Synced cvquant_install/dvctrade to cvquant_install/basetrade"
rsync -raz --timeout=60 --exclude=bindebug --exclude=*.a /mnt/sdf/cvquant_install/dvccode/bin /media/disk-ephemeral1/cvquant_install/basetrade/
echo "Synced cvquant_install/dvccode to cvquant_install/basetrade"
rsync -raz --timeout=60 --exclude=bindebug --exclude=*.a --delete-after /mnt/sdf/cvquant_install/infracore/ /media/disk-ephemeral1/cvquant_install/infracore/
echo "Synced cvquant_install/infracore"
rsync -raz --timeout=60 --exclude=bindebug --exclude=*.a /mnt/sdf/cvquant_install/dvccode/bin /media/disk-ephemeral1/cvquant_install/infracore/
echo "Synced cvquant_install/dvccode to cvquant_install/infracore"
rsync -raz --timeout=60 --delete-after /mnt/sdf/LiveExec /media/disk-ephemeral1/
echo "Synced LiveExec"

# Creating soft-links for basetrade, infracore execs and LiveExcs

# This part relates to models and strategies and params.
# We maintain everyting in a git repository on New York 11 machine.
# 
#rsync -raz /mnt/sdf/modelling /media/disk-ephemeral1/
cd /media/disk-ephemeral1/
git clone gitolite@ny11:modelling
cd modelling

# We are checking out the master branch of modelling
# This was changed in https://github.com/cvquant/basetrade/pull/1028
# The update here is just to correct the active branch of modelling 
# in the workers (we had shifted to master branch from devmodel, 
# a couple of months back. However, it's not updated in the startup script)
# Also, we have not shifted to DB entirely for configs. This PR is 
# an effort in that direction. I think that after we move entirely to DB, 
# we would still need the modelling repo for the stratwork directory.
git checkout master
echo "Synced modelling"

#
find /media/shared/ -maxdepth 2  -type f
echo "Discovered the size of each mounted disk"

# We are adding some config lines to make ny servers accessible
# Not sure why we need it.
echo "Host 10.23.*.*" >> /home/dvctrader/.ssh/config
echo "ProxyCommand ssh -q dvctrader@10.23.74.51 nc %h %p" >> /home/dvctrader/.ssh/config
echo "Added config lines to make ny-servers accessible"

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
