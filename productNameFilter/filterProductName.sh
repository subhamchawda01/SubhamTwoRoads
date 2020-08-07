#!/bin/bash

dt=`date +"%Y%m%d"`
filepath="/home/subham/subham/productNameFilter"
productlistfilepath="$filepath/productListFiles/"

filter_symbol() {
	mkdir -p "$productlistfilepath"
   for file in "$filepath/NSE_files"/*
   do 
		if [ "$file" == "$filepath/NSE_files/gsm_$dt.csv" ]; then
			awk 'NR>6' "$file" | awk -F "\"*,\"*" '{print $2}' >> "$productlistfilepath/productlist$dt.txt"

		else
			awk 'NR>1' "$file" | awk -F "\"*,\"*" '{ if(substr($1,2) != "NIFTY NEXT 50" && substr($1,2) != "NIFTY 50") print substr($1,2)}' | head -n -1 >> "$productlistfilepath/productlist$dt.txt"
		fi
   done
	cat "$productlistfilepath/productlist$dt.txt" | sort | uniq > "$productlistfilepath/productList$dt.txt"
	rm -f "$productlistfilepath/productlist$dt.txt"
}

filter_symbol;

