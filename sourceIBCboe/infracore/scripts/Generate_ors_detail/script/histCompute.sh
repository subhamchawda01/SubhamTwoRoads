int(){
	exepath="/home/raghu/ThrottleProject/mds_log_reader"
	#dirpath="/run/media/dvcinfra/BACKUP/ORSBCAST_MULTISHM_Q19"
	dirpath="/run/media/dvcinfra/BACKUP/ORSBCAST_MULTISHM_BKP";
	declare -A datemap
	for file in $dirpath/*
	do
		filename=$(basename $file);
		if [[ $filename == *.gz ]];
		then
			date=`echo $filename | rev | cut -d'_' -f 1 | rev`
			date=${date::-3}
			if [[ -v "datemap[$date]" ]] ; then
				continue;
		
			else
				#$exepath "GEN_THROTTLE" $date
				datemap[$date]=1
				$exepath "GEN_THROTTLE" $date
				echo $exepath "GEN_THROTTLE" $date
			fi
		fi
	done
}
int $*
