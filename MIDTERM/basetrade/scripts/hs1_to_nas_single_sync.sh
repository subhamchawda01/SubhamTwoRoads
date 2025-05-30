USAGE="$0  FILENAME [/NAS1/data/NSELoggedData/NSE/2016/01/27/NSE_ACC_CE_1080.00_20160225_20160127.gz]";

if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit;
fi

FILENAME=$1
BASENAME=`basename $1`
DIRNAME=`dirname $1`

hs1_disk=`$HOME/get_hs1_path $FILENAME`;
path="$hs1_disk/s3_cache/$FILENAME" ;

rsync -avz $path dvcinfra@10.23.74.41:$DIRNAME/
