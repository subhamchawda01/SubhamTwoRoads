#/bin/bash
IP=$1
echo "unmounting nfs"
ssh  -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -t  -i ~/.ssh/dvckeypair_virginia.pem  ec2-user@$IP ' sudo umount /mnt/sdf '
sleep 1;

ssh  -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -t  -i ~/.ssh/dvckeypair_virginia.pem  ec2-user@$IP ' sudo usermod -u 503 dvctrader '
sleep 1;

echo "mounting nfs"
ssh  -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -t  -i ~/.ssh/dvckeypair_virginia.pem  ec2-user@$IP ' sudo mount -t nfs 10.0.0.11:/mnt/sdf /mnt/sdf '
sleep 5;

echo "execute startup scripts"
ssh  -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no  -t  -i ~/.ssh/dvckeypair_virginia.pem  ec2-user@$IP ' sh /mnt/sdf/ec2_startup_instructions.sh '

