for file in /spare/local/MDSlogs/MT_SPRD_TRADES/*_$1; do tail -n1 $file | awk -v "FILE=$file" '{split(FILE,a,"/");print a[6] "\t" $1}'; done
