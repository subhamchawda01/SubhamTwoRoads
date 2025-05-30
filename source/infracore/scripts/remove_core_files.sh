

# removes core files in home which are older than 5 days

for i in core\.* ; do

   delta=$((( $(date +%s) - $(date -r $i  +%s) )/(60*60*24))); 

   if [ $delta -gt 5 ]; then  
      rm $i ; 
   fi ; 
done
