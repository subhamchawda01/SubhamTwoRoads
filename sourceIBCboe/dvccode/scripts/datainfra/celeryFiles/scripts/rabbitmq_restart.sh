SAFE=1
if [ $# -gt 1 ]; then
	SAFE=$1
fi

echo " == STOPIPING SERVER =="
sudo service rabbitmq-server stop

echo " == REMOVEING RABBITMQ FILES =="
if [ $SAFE -eq 0 ]; then
	sudo rm -r /var/lib/rabbitmq/mnesia/rabbit@localhost
fi

echo "== STARTING SERVER =="
sudo service rabbitmq-server start

echo "== SETTING USERSAND PERMISSIONS =="
bash /home/ec2-user/celeryFiles/restart.sh

echo "== RESTARTING FLOWER =="
if [ $SAFE -eq 0 ]; then
	$HOME/celeryFiles/scripts/flower_restart.sh
fi
