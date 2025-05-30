# Get the needed files from the NewEdge ftp.

cp /spare/local/files/LIFFE/liffe-ref.txt /spare/local/files/LIFFE/liffe-ref.txt_bkp

rm -rf /spare/local/files/LIFFE/NYSE_LIFFE_REFDATA/

mkdir -p /spare/local/files/LIFFE/NYSE_LIFFE_REFDATA/
cd /spare/local/files/LIFFE/NYSE_LIFFE_REFDATA/

DATE=`date +"%Y%m%d"`
YYYY=`date +"%Y"`
MM=`date +"%m"`

YYYYMMDD="_"`date +"%d%m%y"`

echo $DATE, $YYYY, $MM, $YYYYMMDD


if [ $DATE -gt "20160517" ]; then
	HOST='ftp.data.euronext.com';
	USER='dvcapitalDER';
	PASSWD='4b5j2edg21osk6l7ukqe4bcq7i';
        echo "Using Euronext FTP"
else
	HOST='156.48.55.1';
	USER='xdplive';
	PASSWD='xdplive';
        echo "Using xdplive"
fi

ftp -n $HOST <<SCRIPT
user $USER $PASSWD
binary

cd DER_EU_ENXT_REF_MASTER_BOD
cd DER_EU_ENXT_REF_MASTER_BOD_"$YYYY"
cd DER_EU_ENXT_REF_MASTER_BOD_"$YYYY$MM"

get "nyseliffe_stdata_eqt_A"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_B"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_C"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_D"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_F"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_G"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_H"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_J"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_K"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_O"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_P"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_Y"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_eqt_Z"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_C"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_H"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_J"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_L"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_M"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_O"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_S"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_X"$YYYYMMDD".xml.gz"
get "nyseliffe_stdata_fin_Y"$YYYYMMDD".xml.gz"

quit
SCRIPT

>$HOME/liffe-ref.txt ;

cd /spare/local/files/LIFFE/NYSE_LIFFE_REFDATA/

gunzip -df *.gz

for i in `ls /spare/local/files/LIFFE/NYSE_LIFFE_REFDATA/ | grep "$YYYYMMDD"`; do /home/pengine/prod/live_execs/generate_liffe_refdata /spare/local/files/LIFFE/NYSE_LIFFE_REFDATA/$i >> $HOME/liffe-ref.txt ; done

#cat $HOME/liffe-ref.txt | awk -F"~" '{print $5 "~" $6}' | grep "F~" > /spare/local/files/LIFFE/liffe-ref.txt
cat /home/dvcinfra/liffe-ref.txt | awk -F"~" '{print $5 "~" $6}' | grep "F~" > /spare/local/files/LIFFE/liffe-ref.txt_12
cat /home/dvcinfra/liffe-ref.txt | awk -F"~" '{print $5 "~" $6}' | grep "S~" | egrep "LFI|LFL" >> /spare/local/files/LIFFE/liffe-ref.txt_12

# If file of size zero then dont sync
if [ `cat /spare/local/files/LIFFE/liffe-ref.txt_12 | wc -l` -le 0 ]; then
 exit;
fi


scp /spare/local/files/LIFFE/liffe-ref.txt_12 10.23.52.51:/spare/local/files/LIFFE/liffe-ref.txt
scp /spare/local/files/LIFFE/liffe-ref.txt_12 10.23.52.52:/spare/local/files/LIFFE/liffe-ref.txt
scp /spare/local/files/LIFFE/liffe-ref.txt_12 10.23.52.53:/spare/local/files/LIFFE/liffe-ref.txt
scp /spare/local/files/LIFFE/liffe-ref.txt_12 10.23.74.51:/spare/local/files/LIFFE/liffe-ref.txt
scp /spare/local/files/LIFFE/liffe-ref.txt_12 10.23.74.52:/spare/local/files/LIFFE/liffe-ref.txt
scp /spare/local/files/LIFFE/liffe-ref.txt_12 10.23.74.53:/spare/local/files/LIFFE/liffe-ref.txt
scp /spare/local/files/LIFFE/liffe-ref.txt_12 10.23.74.54:/spare/local/files/LIFFE/liffe-ref.txt
scp /spare/local/files/LIFFE/liffe-ref.txt_12 10.23.74.55:/spare/local/files/LIFFE/liffe-ref.txt


ssh dvcinfra@10.23.74.51 "/home/dvcinfra/infracore/scripts/sync_file_to_all_machines.pl /spare/local/files/LIFFE/liffe-ref.txt"

