for file in /spare/local/MDSlogs/MT_SPRD_EXEC/*_$1; do python3 trade_file_analysis.py --file $file | grep open | awk -v "FILE=$file" '{split(FILE,a,"/");print a[6] "\t" $3}'; done
