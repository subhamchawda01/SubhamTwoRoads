LOPR_STATUS_FILE=/spare/local/files/TMX/LOPRUploadStatus.dat ;

if [ ! -f $LOPR_STATUS_FILE ]
then

   echo "LOPR UPLOAD FAILED" | /bin/mail -s "URGENT - LOPR UPLOAD FAILED" -r "ravi.parikh@tworoads.co.in" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in"

else 

   cat $LOPR_STATUS_FILE | mail -s "LOPR UPLOAD STATUS" -r "ravi.parikh@tworoads.co.in" "ravi.parikh@tworoads.co.in nseall@tworoads.co.in"

fi 
