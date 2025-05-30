

export NEW_GCC_LIB=/usr/local/lib
export NEW_GCC_LIB64=/usr/local/lib64
export LD_LIBRARY_PATH=$NEW_GCC_LIB64:$NEW_GCC_LIB:$LD_LIBRARY_PATH

for name in ~/modelling/strats/BAX*/EST_*/* ; 
do 
    sstime=`echo $name | awk -F/ '{print $7}' | awk -F\- '{print $1}'`; 
    setime=`echo $name | awk -F/ '{print $7}' | awk -F\- '{print $2}'`; 
    gstime=`awk '{print $6}' $name`; 
    getime=`awk '{print $7}' $name`; 
    if [ $gstime != $sstime ]; then 
	echo $name ; echo $gstime $sstime; 
	replace " $gstime " " $sstime " -- $name ;
    fi ; 
    if [ $getime != $setime ]; then
	echo $name ; echo $getime $setime;
	replace " $getime " " $setime " -- $name ;
    fi;
done

# for name in ~/modelling/strats/*/CET*/* ; 
# do 
#     sstime=`echo $name | awk -F/ '{print $7}' | awk -F\- '{print $1}'`; 
#     setime=`echo $name | awk -F/ '{print $7}' | awk -F\- '{print $2}'`; 
#     gstime=`awk '{print $6}' $name`; 
#     getime=`awk '{print $7}' $name`; 
#     if [ $gstime != $sstime ]; then 
# 	echo $name ; echo $gstime $sstime; 
# 	replace " $gstime " " $sstime " -- $name ;
#     fi ; 
#     if [ $getime != $setime ]; then
# 	echo $name ; echo $getime $setime;
# 	replace " $getime " " $setime " -- $name ;
#     fi;
# done

