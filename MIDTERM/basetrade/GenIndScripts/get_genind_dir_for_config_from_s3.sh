#!/bin/bash

if [ $# -lt 2 ]; then echo "<script> <config> <dest_dir>"; exit; fi
config=$1;shift;
dest_dir=$1;shift;

for dir in `$HOME/basetrade/GenIndScripts/get_dirnames_for_config.pl $config` ; do
	echo syncing $dir; 
	mkdir -p $dest_dir/$dir ; 
	/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd get --no-progress --recursive s3://s3dvc/NAS1/indicatorwork/$dir $dest_dir/ ;
done
