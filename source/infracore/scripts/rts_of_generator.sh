if [ -f /home/dvcinfra/trash/temp_out.gz ] ; then 
  rm /home/dvcinfra/trash/temp_out.gz ; 
fi ; 


for dir in `find /NAS1/data/RTS_OFLoggedData/MOS/2017 -type f  | awk -F"/" '{ print "/"$2"/"$3"/"$4"/"$5"/"$6"/"$7"/"$8 }' | uniq | sort | uniq ` ; do  
for file in `ls $dir/Si*` ; 
do 
filename=`echo $file | awk -F"/" '{ print $9 }'` ; 
outfile=`echo $dir | awk -F"RTS_OFLoggedData" '{ print $1"RTS_OFv2LoggedData"$2 }'`"/"$filename ; 
echo $file" "$filename" "$outfile ; 


/home/dvcinfra/trash/rts_of_converter $file /home/dvcinfra/trash/temp_out > '/home/dvcinfra/important/pjain/'$filename'cout_cerr' 2>&1 ;
gzip /home/dvcinfra/trash/temp_out;
scp /home/dvcinfra/trash/temp_out.gz dvcinfra@10.23.74.41:$outfile ;

if [ -f /home/dvcinfra/trash/temp_out.gz ] ; then rm /home/dvcinfra/trash/temp_out.gz ; fi ;  

done ;  done
