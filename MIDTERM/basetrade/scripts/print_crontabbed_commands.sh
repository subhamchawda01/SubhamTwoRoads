#!/bin/bash
crontab -l | grep start_real | awk '{$1=""; $2=""; $3=""; $4=""; $5=""; print }'
