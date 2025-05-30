ls -lrt /data1/apps/data/PData/Px/2500/log_dir_tranche/tranche*/ | grep 'log.'| awk '{split($9,a,".");print a[2]}' | awk '{split($0,a,"_");if(a[1]&&a[2])print a[1] "\t" a[2]}' | sort -u > all_pairs
