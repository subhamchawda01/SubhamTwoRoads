#!/bin/bash


servers="IND14 IND15 IND16 IND17 IND18 IND19 IND20" ;
for server in $servers;
do
  rsync root@10.23.5.26:/run/media/root/Elements1/SERVERDATA/$server/tradelogs/log.* /home/hardik/PNLProject/copy/$server/
done

