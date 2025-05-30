FILE=$1
NUM_LINES=$2
CHANNEL="test"

if [ $# -gt 2 ]; then
	CHANNEL=$3
fi

id=`date +%N`
split -l $NUM_LINES $FILE /spare/local/logs/slack/$id
for file in `ls /spare/local/logs/slack/$id*`; do 
	echo $file
	/home/dvctrader/infracore_install/bin/send_slack_notification $CHANNEL FILE $file &>/dev/null
	rm $file
done
