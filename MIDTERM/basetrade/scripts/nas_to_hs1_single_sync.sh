USAGE="$0  FILENAME [/NAS1/data/NSELoggedData/NSE/2016/01/27/NSE_ACC_CE_1080.00_20160225_20160127.gz]";

if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit;
fi

FILENAME=$1
BASENAME=`basename $1`
HS1_TMP_PATH="/media/ephemeral16"
scp "$FILENAME" dvctrader@52.0.55.252:$HS1_TMP_PATH/
SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=60"
ssh -n -f  $SSH_VARS dvctrader@52.0.55.252 "~/basetrade/scripts/send_from_nas_to_disks.sh $HS1_TMP_PATH/\"$BASENAME\" \"$FILENAME\" &>/dev/null &"
