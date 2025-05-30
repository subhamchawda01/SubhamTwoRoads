#! /bin/bash

ssh dvctrader@10.23.115.62 /bin/bash << \EOF  >> /spare/local/logs/pnl_data/hft/saci_tags_qid_files/unknown_tag_saci_qid 2>&1
        for f in `ls /spare/local/logs/tradelogs/risklog.*`; 
	do
		yyyymmdd=`date +\%Y\%m\%d`;
		grep "Notifying of triplet" $f| grep $yyyymmdd | awk '{print $4" "$5" "$6}';
	done
EOF

ssh dvctrader@10.23.115.61 /bin/bash << \EOF  >> /spare/local/logs/pnl_data/hft/saci_tags_qid_files/unknown_tag_saci_qid 2>&1
	for f in `ls /spare/local/logs/tradelogs/risklog.*`; 
        do
                yyyymmdd=`date +\%Y\%m\%d`;
                grep "Notifying of triplet" $f| grep $yyyymmdd | awk '{print $4" "$5" "$6}';
        done
EOF
