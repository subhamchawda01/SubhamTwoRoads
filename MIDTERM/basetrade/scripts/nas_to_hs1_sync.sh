USAGE="$0  PATHNAME [/NAS1/data/NSELoggedData/NSE/2016/01/27/]";

if [ $# -ne 1 ] ;
then
    echo $USAGE
    exit;
fi

TMP_FOLDER="$HOME/trash"
HS1_TMP_FOLDER="/tmp"
SCRIPTS_FOLDER="$HOME/basetrade/scripts"
PATHNAME="$1"

SSH_VARS="-o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -o ConnectTimeout=15"
ssh $SSH_VARS dvctrader@52.0.55.252 "find /media/ephemeral*/s3_cache$PATHNAME/ -type f > $HS1_TMP_FOLDER/tmp_list_hs1.txt"
scp dvctrader@52.0.55.252:$HS1_TMP_FOLDER/tmp_list_hs1.txt $TMP_FOLDER/tmp_list_hs1.txt

awk -F'/' '{print $NF}' $TMP_FOLDER/tmp_list_hs1.txt > $TMP_FOLDER/file_list_hs1.txt 

ls $PATHNAME > $TMP_FOLDER/file_list_nas.txt

fgrep -x -f $TMP_FOLDER/file_list_hs1.txt -v $TMP_FOLDER/file_list_nas.txt > $TMP_FOLDER/not_in_hs1.txt

for file in `cat  $TMP_FOLDER/not_in_hs1.txt `; do
	full_path=$PATHNAME/$file
	$SCRIPTS_FOLDER/nas_to_hs1_single_sync.sh $full_path
done
