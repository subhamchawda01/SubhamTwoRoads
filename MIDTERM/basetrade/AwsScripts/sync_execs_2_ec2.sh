#!/bin/bash
rsync -ravz --timeout=60 --exclude=gcc-4.9.1 --exclude=gcc-4.8.3 --delete-after  /home/dvctrader/basetrade/.  54.208.92.178:/mnt/sdf/basetrade/
rsync -ravz --timeout=60 --delete-after  /home/dvctrader/basetrade_install/.  54.208.92.178:/mnt/sdf/basetrade_install/
rsync -ravz --timeout=60 --exclude=gcc-4.9.1 --exclude=gcc-4.8.3 --delete-after  /home/dvctrader/infracore/.  54.208.92.178:/mnt/sdf/infracore/
rsync -ravz --timeout=60 --delete-after  /home/dvctrader/infracore_install/.  54.208.92.178:/mnt/sdf/infracore_install/
rsync -ravz --timeout=60 --delete /home/dvctrader/LiveExec/.  54.208.92.178:/mnt/sdf/LiveExec/

ssh -o ConnectTimeout=60 54.208.92.178 'sh /home/dvctrader/basetrade/AwsScripts/update_worker_execs_only.sh'
