yyyymmdd=$1
yyyy=${yyyymmdd:0:4}
mm=${yyyymmdd:4:2}
dd=${yyyymmdd:6:2}

ls /NAS1/data/*/*/$yyyy/$mm/$dd/* 
