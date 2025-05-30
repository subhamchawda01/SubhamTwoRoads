LOPR_STATUS_FILE=/spare/local/files/TMX/LOPRUploadStatus.dat ;

LINECOUNT=`ssh 10.23.182.52 "wc -l /spare/local/files/TMX/LOPRUploadStatus.dat 2>/dev/null" 2>/dev/null | awk '{print $1}'`

if [ $LINECOUNT -lt 1 ] 
then

   echo "REMOTE CHECK FROM NY11 - LOPR UPLOAD FAILED" | /bin/mail -s "URGENT - LOPR UPLOAD FAILED" -r "ravi.parikh@tworoads.co.in" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in"

fi 
