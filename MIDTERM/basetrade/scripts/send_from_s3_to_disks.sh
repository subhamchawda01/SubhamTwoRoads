FILE_NAME=$1; shift
EXCHANGE=$1; shift
CURRENT_LOCATION=$1; shift
dest_path=$1;

ARCH_NAME=`basename $FILE_NAME`

cd /tmp/
/home/s3cmd/s3cmd-1.5.0-alpha1/s3cmd get $FILE_NAME 

NUM_DISKS=16
file_path="/NAS1/data/"$EXCHANGE"LoggedData/"$CURRENT_LOCATION/$dest_path
for i in $(seq 0 $NUM_DISKS); do
	mkdir -p /media/ephemeral$i/s3_cache$file_path #Change
done

for file in `tar -xvf $ARCH_NAME`
do
	FILENAME="/NAS1/data/"$EXCHANGE"LoggedData/"$CURRENT_LOCATION/$dest_path;
	nas_file=$FILENAME/$file ;
	hs1_disk=`$HOME/get_hs1_path $nas_file`; #returns /media/ephemeral?
	path="$hs1_disk/s3_cache/$FILENAME" ; #change
	mv $file $path
done

rm $ARCH_NAME
