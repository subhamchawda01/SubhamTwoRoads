for sfile in ~/modelling/strats/*/*/* ; do mfile=`awk '{print $4}' $sfile`; if [ ! -e $mfile ] ; then echo $sfile ; fi ; done
for sfile in ~/modelling/strats/*/*/* ; do pfile=`awk '{print $5}' $sfile`; if [ ! -e $pfile ] ; then echo $sfile ; fi ; done
