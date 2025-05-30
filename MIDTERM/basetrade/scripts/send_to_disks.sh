FILE_NAME=$1; shift
EXCHANGE=$1; shift
CURRENT_LOCATION=$1; shift
dest_path=$1; shift
exch_timing=$1;

ARCH_NAME=`basename $FILE_NAME`
DIR_NAME=`dirname $FILE_NAME`

SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"
echo "`date` Start: $FILENAME $EXCHANGE $CURRENT_LOCATION $dest_path $exch_timing" >> /tmp/logfile
cd $DIR_NAME

NUM_DISKS=16

file_path="/NAS1/data/"$EXCHANGE"LoggedData/"$CURRENT_LOCATION/$dest_path

for i in $(seq 0 $NUM_DISKS); do
	mkdir -p /media/ephemeral$i/s3_cache$file_path #Change
done

#To prevent conflict between multiple datacopy instances
temp_dir=$EXCHANGE"_"$CURRENT_LOCATION
mkdir -p $temp_dir
mv $ARCH_NAME $temp_dir/
cd $temp_dir

for file in `tar -xvf $ARCH_NAME`
do
	FILENAME=$file_path;
	nas_file=$FILENAME/$file ;
	hs1_disk=`$HOME/get_hs1_path $nas_file`; #returns /media/ephemeral?
	path="$hs1_disk/s3_cache/$FILENAME" ; #change
	mv $file $path
	touch $path
	chmod a+r $path
done

rm $ARCH_NAME
#Create file to test data copy
if [ "$exch_timing" != "" ]; then
       dt=`echo $dest_path | sed 's/\///g'`
       mkdir -p /home/dvctrader/datacopy/$EXCHANGE/$CURRENT_LOCATION/$dt
       touch /home/dvctrader/datacopy/$EXCHANGE/$CURRENT_LOCATION/$dt/$exch_timing
       ssh $SSH_VARS dvcinfra@10.23.74.51 "mkdir -p /home/dvcinfra/trash/datacopy/$EXCHANGE/$CURRENT_LOCATION/$dt; touch /home/dvcinfra/trash/datacopy/$EXCHANGE/$CURRENT_LOCATION/$dt/$exch_timing" &>> /tmp/logfile
fi
echo "`date` End: $FILENAME $EXCHANGE $CURRENT_LOCATION $dest_path $exch_timing" >> /tmp/logfile
