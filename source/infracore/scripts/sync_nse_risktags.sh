#! /bin/bash

while [ true ];
do
  hhmmss=$(date "+%H%M%S")
  if [ "$hhmmss" -gt "100000" ]; then
     break;
  fi

ssh dvcinfra@10.23.115.62 /bin/bash << \EOF  > /spare/local/logs/pnl_data/hft/saci_tags_qid_files/sdv-ind-srv12_tag_saci_qid 2>&1
   YYYYMMDD=`date +"%Y%m%d"` ;
   grep "SACI mapping:" /spare/local/logs/risk_logs/risk_client_log_$YYYYMMDD | awk '{print $3" "$4" "$5}'
EOF

ssh dvcinfra@10.23.115.61 /bin/bash << \EOF  >> /spare/local/logs/pnl_data/hft/saci_tags_qid_files/sdv-ind-srv12_tag_saci_qid 2>&1
   YYYYMMDD=`date +"%Y%m%d"` ;
      grep "SACI mapping:" /spare/local/logs/risk_logs/risk_client_log_$YYYYMMDD | awk '{print $3" "$4" "$5}'
EOF

sleep 60;
done

