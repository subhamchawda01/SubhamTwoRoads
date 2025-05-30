echo "========================================`hostname`========================================" ;

ps -ef |grep cme_ilink_ors | grep -v grep
egrep -ai "Logged|now|INVALID|Append|CLOSED" `ps -ef |grep cme_ilink_ors | grep -v grep | awk '{print $NF}'`/log.`date +"%Y%m%d"`
ps -ef |grep CombinedShmWriter | grep -v grep

echo;
echo;
echo;
