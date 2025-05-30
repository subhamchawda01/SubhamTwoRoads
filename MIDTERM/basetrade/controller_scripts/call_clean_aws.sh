#!/bin/bash
#delete older than 10 days

for loc in `/home/dvctrader/controller_scripts/find_instances.sh | awk '{print $4}'` ;
    do ssh $loc '/mnt/sdf/scripts/clean_aws_dir.sh' ;
#    do ssh $loc '/mnt/sdf/scripts/clean_aws_exp.sh' ;
done ;
