if [ $# -gt 0 ] ;
then
    sfile=$1;

    mfile=`awk '{print $4}' $sfile`; 
    if [ ! -e $mfile ] ; 
    then 
	echo "Invalid "$sfile" missing model $mfile" ; 
    fi ; 

    pfile=`awk '{print $5}' $sfile`; 
    if [ ! -e $pfile ] ; 
    then 
	echo "Invalid "$sfile" missing param "$pfile ; 
    fi ; 
fi