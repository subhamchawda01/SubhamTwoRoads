
SHC=FGBS;
if [ $# -gt 0 ] ; then SHC=$1; shift; fi

for name in ~/modelling/strats/$SHC*/*/* ; 
do 
    if [ -e $name ] ; then
#	echo $name;
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
	fi ;
    fi ;
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

