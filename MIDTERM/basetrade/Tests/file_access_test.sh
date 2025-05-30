#!/bin/bash
function get_abs_filename {
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
}

CMD=$1;

EXEC=`echo $CMD | cut -d' ' -f1`
ARGS=`echo $CMD | cut -d' ' -f2-`

if [[ ${EXEC:0:1} == "~" ]] ; then
    EXEC=${EXEC:2} ;
    CMD=$EXEC" "$ARGS ;
fi

#echo $CMD

strace $CMD > /tmp/strace_out 2>&1 ;

cat /tmp/strace_out  | grep "open(" | grep -v "\.so" | grep -v NAS1  | cut -d'(' -f2 | cut -d',' -f1 | sed 's/"//g' | sort | uniq > /tmp/accessed_files ;

red='\033[0;31m' ;
blue='\033[1;34m' ;
NC='\033[0m' # No Color



COUNT=`grep -v -i -f /spare/local/files/files_accessed_by_sim.txt /tmp/accessed_files | wc -l`;

if [[ COUNT -gt 0 ]]; then
	echo -e "${red} Check if following files are being accessed as an error , else add these files to ignore list => /spare/local/files/files_accessed_by_sim.txt ${blue}" ;
	grep -vi -f /spare/local/files/files_accessed_by_sim.txt /tmp/accessed_files ;
	echo -e "${NC}" ; 
else
	echo "FINE" ;
fi 


rm /tmp/strace_out ; rm /tmp/accessed_files ;
