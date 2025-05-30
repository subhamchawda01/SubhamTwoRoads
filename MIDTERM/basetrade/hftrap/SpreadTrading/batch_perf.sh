for file in /spare/local/MDSlogs/MT_SPRD_TRADES/*_$1; do tail -n3 $file|head -n1|awk -F',\t' -v "FILE=$file" '{split(FILE,a,"/");print a[6] "\t" $7}'; done
