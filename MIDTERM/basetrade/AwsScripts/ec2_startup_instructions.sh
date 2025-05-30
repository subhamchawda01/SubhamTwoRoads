#!/bin/bash

#s3fs cache directory
mkdir /media/ephemeral0/s3_cache
chmod 755 /media/ephemeral0/s3_cache
chown dvctrader:infra /media/ephemeral0/s3_cache
ln -s /media/ephemeral0/s3_cache /s3_cache

#s3_404 directory
mkdir /media/ephemeral0/s3_404
chmod 755 /media/ephemeral0/s3_404
chown dvctrader:infra /media/ephemeral0/s3_404

ln -s /media/ephemeral0/s3_404 /s3_404 #do this manually if the link is not present in the EBS snapshot


#/spare directory
mkdir /media/ephemeral1/spare
chmod 755 /media/ephemeral1/spare

cp /apps/aws/local.tar.gz /media/ephemeral1/spare/local.tar.gz
cd /media/ephemeral1/spare/
tar -xzf local.tar.gz
rm local.tar.gz

chown -R dvctrader:infra /media/ephemeral1/spare
ln -s /media/ephemeral1/spare /spare

echo "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAyUQPJDVQ00zq8vR57BjN4JFZhPq2VXiRhoPpng7pXKpESoLrImJK+QUCfJki67lbjaqhJPkd69oQPajFbIvXBsufqb1N7sAgqsIUF4CoglERZ0pEiiQ0mpQ08UWr8i8K3x6uYmF+dEYME4MDmetMEBdWFJrIido51JolVgf+zTF6n47xuX4aoQABFZpvhvj2Y3F7XdJiJdhvLV5mA2KIy89LQPGuSprKWkq/fvf0j5MZNt40EVtS2P0mL/7LJDTx0+rOLNJ47rn/ZuSh5dxUllVJgqBfcz/Bt6eHRJMeQCW2FWv0HShh52wISHM7U865nespGppuCiKR4JssT1olpw== ec2_nat@dvc.com" >> /home/dvctrader/.ssh/authorized_keys

resize2fs /dev/xvda1 20G
