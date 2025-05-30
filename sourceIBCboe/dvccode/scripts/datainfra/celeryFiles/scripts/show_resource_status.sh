echo "== DISK SPACE =="
df -h

echo "== RABBITMQ STATUS =="
sudo /usr/sbin/rabbitmqctl status

echo "== PS MEMORY =="
ps aux --sort -rss

echo  "== RABBITMQ DISK USAGE =="
sudo du -sch /var/lib/rabbitmq/mnesia/rabbit@localhost/*

