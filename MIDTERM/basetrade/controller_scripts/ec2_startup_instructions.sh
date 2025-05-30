#!/bin/bash

#start NFS client
#sudo mount -t nfs 10.0.0.11:/mnt/sdf /mnt/sdf

#sudo usermod -u 503 dvctrader

#cleanup links and directories
sudo rm -rf /s3_cache /NAS1 /s3_404 /spare /media/ephemeral1/* /media/ephemeral0/* /media/ephemeral2/*
sudo chown -R dvctrader:infra /media/ephemeral2 /media/ephemeral1 /media/ephemeral0
sudo chmod -R 777 /media/ephemeral2 /media/ephemeral1 /media/ephemeral0

sudo mkdir -p /media/ephemeral0/s3_cache/NAS1 /media/ephemeral0/s3_404 /media/ephemeral1/spare/local
sudo ln -s /media/ephemeral0/s3_cache /s3_cache
sudo ln -s /media/ephemeral0/s3_cache/NAS1 /NAS1
sudo ln -s /media/ephemeral0/s3_404 /s3_404
sudo ln -s /media/ephemeral1/spare /spare

sudo mkdir -p /media/ephemeral2/indicatorwork
sudo rm -rf /home/dvctrader/indicatorwork
sudo ln -s /media/ephemeral2/indicatorwork /home/dvctrader/indicatorwork
sudo ln -s /media/shared/ephemeral2/SampleData /NAS1/SampleData
sudo ln -s /media/shared/ephemeral2/SampleData /home/dvctrader/SampleData

sudo ln -s /mnt/sdf/spare_local_files /media/ephemeral1/spare/local/files
sudo ln -s /mnt/sdf/spare_local_tradeinfo /media/ephemeral1/spare/local/tradeinfo
sudo ln -s /mnt/sdf/spare_local_L1Norms /media/ephemeral1/spare/local/L1Norms
sudo ln -s /NAS1/SampleData /media/ephemeral1/spare/local/Features

#sudo ln -s /mnt/sdf/ec2_globalresults /media/ephemeral0/s3_cache/NAS1/ec2_globalresults
sudo ln -s /NAS1/ec2_staged_globalresults ~/ec2_staged_globalresults
sudo ln -s /NAS1/ec2_globalresults ~/ec2_globalresults
sudo rm -rf /home/dvctrader/LiveExec /home/dvctrader/tradeinfo
sudo ln -s /mnt/sdf/LiveExec /home/dvctrader/LiveExec

sudo mkdir -p  /media/ephemeral0/s3_cache/NAS1/data/ORSData/
sudo ln -s /mnt/sdf/SEQCONF /media/ephemeral0/s3_cache/NAS1/data/ORSData/SEQCONF
sudo ln -s /mnt/sdf/CXLCONF /media/ephemeral0/s3_cache/NAS1/data/ORSData/CXLCONF
sudo ln -s /mnt/sdf/CONFUPDATE /media/ephemeral0/s3_cache/NAS1/data/ORSData/CONFUPDATE

sudo chown -R dvctrader:infra /s3_cache /NAS1 /s3_404 /spare /media/ephemeral2 /media/ephemeral1 /media/ephemeral0 /media/ephemeral3

sudo mv /apps /media/ephemeral0/  
sudo ln -s /media/ephemeral0/apps/ /apps 
