#Moves TMP_PATH to required disk
TMP_PATH=$1; shift
FULL_PATH=$1; shift
BASE_PATH=`dirname $FULL_PATH`

nas_file=$FULL_PATH ;
hs1_disk=`$HOME/get_hs1_path $nas_file`; #returns /media/ephemeral?
mkdir -p $hs1_disk/s3_cache$BASE_PATH
path="$hs1_disk/s3_cache/$nas_file" ; #change
mv $TMP_PATH $path

