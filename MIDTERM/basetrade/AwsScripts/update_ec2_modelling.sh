#!/bin/bash

for machine in `cat /mnt/sdf/JOBS/all_instances.txt | grep nil | awk '{print $4}'`; do ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=20 $machine '/home/dvctrader/basetrade/AwsScripts/push_to_modelling_worker.sh'; done
for machine in `cat /mnt/sdf/JOBS/all_instances.txt | grep nil | awk '{print $4}'`; do ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=20 $machine 'cd /home/dvctrader/modelling; git pull --no-edit'; done

#Update modelling on HS1, for autoscaling instances
ssh -o StrictHostKeyChecking=no  -o ConnectTimeout=20 52.87.81.158 'cd /home/dvctrader/modelling; git pull --no-edit'
