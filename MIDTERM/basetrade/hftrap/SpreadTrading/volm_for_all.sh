for sym in `cat $1 | awk '{print $1"\n"$2}' | sort -u`; do 
	python3 get_volume.py --sym $sym --start $2 --end $3; 
done;
