#!/bin/bash

YYYYMMDD=`date +%Y%m%d` ;
DD=${YYYYMMDD:6:2}
MM=${YYYYMMDD:4:2}
YY=${YYYYMMDD:2:2}
YYYY=${YYYYMMDD:0:4}


echo "rsync -avz -e "ssh -p 22763"  dvcinfra@103.69.169.140:/spare/local/MDSlogs/${YYYY}/${MM}/${DD}/ /NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}"
rsync -avz --progress --exclude=*_CE_* --exclude=*_PE_* -e "ssh -p 22763"  dvcinfra@103.69.169.140:/spare/local/MDSlogs/${YYYY}/${MM}/${DD}/NSE_[P-Z]* /NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/


echo "to 67"
rsync -avz --progress /NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/ dvcinfra@10.23.5.67:/NAS1/data/NSELoggedData/NSE/${YYYY}/${MM}/${DD}/


