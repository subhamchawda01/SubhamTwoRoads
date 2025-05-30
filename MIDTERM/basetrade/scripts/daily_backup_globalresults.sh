DEST_USER=dvcinfra;
DEST_SERVER=10.23.74.40;

rsync -avz --quiet /home/dvctrader/ec2_globalresults $DEST_USER@$DEST_SERVER:/apps/ 
ssh $DEST_USER@$DEST_SERVER /home/$DEST_USER/fix_globalresults_permissions.sh

# also to crt server
CRT_SERVER=10.23.142.51
rsync -avz --delete --quiet /home/dvctrader/ec2_globalresults dvctrader@$CRT_SERVER:/home/dvctrader/
