#!/bin/bash
perl $HOME/basetrade/scripts/get_missing_config_dir.pl > /tmp/temp.dir ; 
scp /tmp/temp.dir dvcinfra@10.23.74.40:/tmp/ ; 
ssh dvcinfra@10.23.74.40 'sh /apps/indicatorwork/scripts/create_ind_stats_setup.sh /tmp/temp.dir';

for dir in `cat /tmp/temp.dir` ; do /apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd put /NAS1/indicatorwork/$dir --recursive s3://s3dvc/NAS1/indicatorwork/ ; done ;

