#!/bin/bash

declare -i time_old_threshold
declare -i time_recent_threshold


old_or_recent="" ;
current_dir="" ;
time_old_threshold=10 ;		#days
time_recent_threshold=5 ;	#days


list_empty_dir()
{
	#listing all empty subdirectories 
	printf "Empty Dir list: \n" ;	
	find $1 -type d -empty ;	
}

list_largest_old_files()
{
	#listing all files in directory which were accessed time_old_threshold days back
	#and taking top k such files based on their sizes

	find $1 -type f -atime +$time_old_threshold -printf '%s %p\n' | sort -nr | head -$2 ;	#for days
	#find $1 -type f -amin +$time_old_threshold -printf '%s %p\n' | sort -nr | head -$2 ;	#for minutes
}

list_largest_recent_files()
{
	#listing all files in directory which were accessed in last time_recent_threshold days
	#and taking top k such files based on their sizes
	
	find $1 -type f -atime -$time_recent_threshold -printf '%s %p\n' | sort -nr | head -$2 ;	#for days intervals
	#find $1 -type f -amin -$time_recent_threshold -printf '%s %p\n' | sort -nr | head -$2 ;	#for minutes intervals
}

track_disk_usage()
{
	[ $# -lt 3 ] && { echo "Usage: $0 directory_ num_files_ o/r(old or recent)"; exit 1; } #adding usage
	
	if [ $3 == "o" ] ; then
		list_largest_old_files $1 $2 ;
	elif [ $3 == "r" ] ; then
		list_largest_recent_files $1 $2 ;
	else 
		echo "Please specify the type of files you wish to list: old/recent" ; 
		exit 1 ;
	fi
		
	list_empty_dir $1 ; 
}

track_disk_usage $* ;


