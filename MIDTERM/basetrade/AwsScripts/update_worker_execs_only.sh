#!/bin/bash

echo "Updating local exec directories on workers";
#Updating local exec directories on workers
for loc in `grep -v "10.0.0." /mnt/sdf/JOBS/all_instances.txt | grep running | awk '{print $NF}'`;
do
    echo $loc;
    rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --delete-after --exclude=*.a /mnt/sdf/basetrade_install $loc:/media/ephemeral1/;
    rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --delete-after --exclude=*.a /mnt/sdf/infracore_install $loc:/media/ephemeral1/;
    rsync -ravz --timeout=60 -e "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null" --delete-after /mnt/sdf/LiveExec $loc:/media/ephemeral1/;
done
