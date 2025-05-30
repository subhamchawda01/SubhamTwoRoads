#!/bin/bash

clear_affinity_file () {
  AFFINED_PID_PROCS_LIST_FILENAME="/spare/local/files/affinity/affinity_pid_process.txt";
  >$AFFINED_PID_PROCS_LIST_FILENAME;	#clear the affinity file to contain affined process for this day
}

reset_saci_gen () {
  SACI_GEN_FILE=/spare/local/files/tmp/saci_gen.txt
  echo "1" > $SACI_GEN_FILE
}

compress_generic_files () {
	YYYYMMDD=`date +"%Y%m%d"`;
	for f in `find /spare/local/MDSlogs/GENERIC/ -not -name "*_$YYYYMMDD" -type f | grep -v gz`; 
	do
		gzip -f $f >/dev/null 2>&1;
	done
}

clear_affinity_file
reset_saci_gen
compress_generic_files
