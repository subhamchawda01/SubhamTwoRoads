echo "Num Queues:"
sudo rabbitmqctl list_queues -p vhostClient | wc -l
echo ""
echo "Queue Sizes:"
sudo rabbitmqctl list_queues -p vhostClient | grep autoscale
echo ""
echo "Total and Ready Msgs:"
sudo /usr/sbin/rabbitmqctl list_queues -p vhostClient name messages messages_ready | grep autoscale
