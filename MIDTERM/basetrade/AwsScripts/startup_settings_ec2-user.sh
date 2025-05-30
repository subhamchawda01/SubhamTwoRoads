#!/bin/bash
echo "User ec2-user running startup script for worker"

#Ensure that /etc/fstab is perfect for next reboot
cat /etc/fstab | grep -v "comment=cloudconfig" | grep -v "swapfile" > /home/ec2-user/temp-fstab
echo "/dev/sdb  /media/disk-ephemeral1       auto    defaults,nofail,comment=cloudconfig     0       2" >> /home/ec2-user/temp-fstab
echo "/dev/sdc  /media/disk-ephemeral2       auto    defaults,nofail,comment=cloudconfig     0       2" >> /home/ec2-user/temp-fstab
sudo mv /home/ec2-user/temp-fstab /etc/fstab
echo "Corrrected /ec/fstab"
#mount glusterfs first
#sudo gluster peer probe 10.0.1.57
#sleep 60 #to ensure gluster is ready
#sudo mount -t nfs -o defaults,_netdev,mountproto=tcp,vers=3 localhost:/gluster-fs /media/gluster-share

#configure pengine user
sudo useradd -g infra -G infra -u 1012 pengine
sudo mkdir /home/pengine/prod
sudo chmod 775 -R /home/pengine
sudo chown pengine:infra -R /home/pengine
sudo runuser -l dvctrader -c 'rsync -ravz dvctrader@10.23.74.51:/home/pengine/prod/live_configs /home/pengine/prod/'
sudo chown pengine:infra -R /home/pengine
echo "Created pengine user and folders"

sudo mkdir -p /media/disk-ephemeral1/spare/local
sudo rm -f /media/disk-ephemeral1/spare/local/files /media/disk-ephemeral1/spare/local/tradeinfo /media/disk-ephemeral1/spare/local/L1Norms /NAS1/ec2_globalresults /NAS1/SampleData
sudo ln -s /mnt/sdf/spare_local_files /media/disk-ephemeral1/spare/local/files
sudo ln -s /mnt/sdf/spare_local_tradeinfo /media/disk-ephemeral1/spare/local/tradeinfo
sudo ln -s /mnt/sdf/spare_local_L1Norms /media/disk-ephemeral1/spare/local/L1Norms
echo "Created symlinks"

sudo mkdir -p /media/disk-ephemeral1/spare0 /media/disk-ephemeral1/spare1 /media/disk-ephemeral2/spare2 /media/disk-ephemeral2/spare3 /media/disk-ephemeral1/s3_cache/NAS1 /media/disk-ephemeral1/s3_404
sudo mkdir -p /media/disk-ephemeral2/indicatorwork
sudo mkdir -p /media/ephemeral1/basetrade_install /media/ephemeral1/infracore_install /media/disk-ephemeral1/modelling /media/disk-ephemeral1/apps /media/disk-ephemeral1/s3_cache/NAS1
sudo mkdir -p /media/ephemeral1/LiveExec /media/disk-ephemeral1/s3_404 /media/disk-ephemeral1/s3_cache
sudo mkdir -p /media/disk-ephemeral1/spare /media/disk-ephemeral1/spare0 /media/disk-ephemeral1/spare1 /media/disk-ephemeral2/spare2 /media/disk-ephemeral2/spare3
echo "Created directories"

sudo ln -s /media/disk-ephemeral1/ec2_globalresults /NAS1/ec2_globalresults
sudo ln -s /media/shared/ephemeral2/SampleData /NAS1/SampleData

#Change permissions
sudo chown -R dvctrader:infra /media/disk-ephemeral1/ /media/disk-ephemeral2/
sudo chmod 777 -R /media/disk-ephemeral1/ /media/disk-ephemeral2/
echo "Changed permissions"

#Configure swap
sudo dd if=/dev/zero of=/media/disk-ephemeral2/swapfile bs=1M count=16384
sudo chown root:root /media/disk-ephemeral2/swapfile
sudo chmod 600 /media/disk-ephemeral2/swapfile
sudo mkswap /media/disk-ephemeral2/swapfile
sudo swapon /media/disk-ephemeral2/swapfile
sudo echo "/media/disk-ephemeral2/swapfile swap swap defaults 0 0" >> /etc/fstab
echo "swap enabled"
sudo swapon -a

#Mount remote user directories
for user in `ls -l ../ | grep -v total | grep -v dvctrader | grep -v ec2-user | awk '{print $NF}'`; do sudo mount --bind /media/user-accounts/$user /home/$user; done

#For updateing ld config cache
sudo ldconfig
