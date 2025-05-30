sudo rabbitmqctl stop_app
sudo rabbitmqctl reset
sudo rabbitmqctl start_app


sudo rabbitmqctl add_user mainClient mainClient

sudo rabbitmqctl add_vhost vhostClient
sudo rabbitmqctl set_permissions -p vhostClient mainClient ".*" ".*" ".*"

sudo rabbitmqctl add_user test test
sudo rabbitmqctl set_user_tags test administrator
sudo rabbitmqctl set_permissions -p / test ".*" ".*" ".*"
sudo rabbitmqctl set_permissions -p vhostClient test ".*" ".*" ".*"

