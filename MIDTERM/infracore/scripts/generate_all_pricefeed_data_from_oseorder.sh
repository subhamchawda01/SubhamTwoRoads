
CONVERTER_EXEC=$HOME/infracore_install/bin/offline_ose_orderlvl_to_pricefeed_converter 

for files in `find /NAS1/data/OSELoggedData/TOK -name '*NK*' -type f` 
do 

   orderlevel_file_=$files ; 
   pricefeed_file_=`echo $orderlevel_file_ | awk -F"/" '{print $NF}'  | awk -F"." '{print $1}'` ;
   trddate=`echo $pricefeed_file_ | awk -F"_" '{print $2}'` ;

   yyyy=${trddate:0:4};
   mm=${trddate:4:2};
   dd=${trddate:6:2};

   output_dir=/apps/data/OSEPriceFeedLoggedData/TOK/$yyyy"/"$mm"/"$dd ;

   echo "ssh dvcinfra@10.23.74.40 'mkdir -p $output_dir'" ; 
   echo "$CONVERTER_EXEC $orderlevel_file_ /home/ravi/PRICEFEED/$pricefeed_file_ $trddate" ;

   echo "scp /home/ravi/PRICEFEED/$pricefeed_file_ dvcinfra@10.23.74.40:$output_dir/" ;
   echo "rm -rf /home/ravi/PRICEFEED/$pricefeed_file_" ;


done 
