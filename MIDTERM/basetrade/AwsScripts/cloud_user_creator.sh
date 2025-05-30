#!/bin/bash
#This script creates a new user on HS1 and across all workers
#Supposed to be run from ec2-user@HS1

if [ $# -lt 2 ]; then
	echo "USAGE: $0 <user> <user_id>"
	exit 0
fi

user=$1
user_id=$2
password=password_changeme

#Setup user account first
sudo adduser -g infra $user
#sudo usermod -u $user_id $user
echo -e "$password\n$password\n" | sudo passwd $user

#sudo mkdir -p $user_dir/.ssh
#sudo chown $user:infra -R $user_dir

echo "local user created"

for server in 10.0.1.15 10.0.1.77;
do
	#Setup user account on alll workers
	ssh -i /home/ec2-user/ec2-keypair.pem $server << CREATE_USER

		#Setup user account first
		sudo adduser -g infra $user
		sudo usermod -u $user_id $user
		sudo usermod -aG dvctrader $user
		sudo usermod -aG execgrp $user
		sudo usermod -aG ec2_common $user
		echo -e "$password\n$password\n" | sudo passwd $user

		#Ownership of home dir
		#sudo chown ec2-user:ec2-user -R \$user_home_dir
    sudo umount /home/$user
    sudo mkdir /media/user-accounts/$user
    sudo mount --bind /media/user-accounts/$user /home/$user
    sudo mkdir -p /home/$user/.ssh
    sudo cp /home/dvctrader/.ssh/authorized_keys /home/$user/.ssh/
    sudo cp /home/dvctrader/.bash_profile /home/$user/
    sudo cp /home/dvctrader/.bashrc /home/$user/
    sudo cp /home/dvctrader/.bash_aliases /home/$user/
    sudo cp /home/dvctrader/.git-completion.sh /home/$user/
    sudo chown $user:infra /home/$user -R
    sudo chown $user:infra /media/user-accounts/$user -R

		#sudo mount -t nfs 52.87.81.158:$user_dir /home/$user
		

CREATE_USER

	echo "remote user created for $server"
	#sudo scp -i /home/ec2-user/ec2-keypair.pem /home/dvctrader/.ssh/authorized_keys $server@/home/$user/.ssh/
	#sudo scp -i /home/ec2-user/ec2-keypair.pem /home/dvctrader/.bashrc $server@/home/$user/

done
#Transfer ownership to the user
#ssh -i /home/ec2-user/ec2-keypair.pem $server sudo chown $user:infra -R /home/$user
#sudo chmod 700 -R $user_dir
#sudo chown $user:infra -R $user_dir
#echo "Ownership of home dirs transferred to new user"
