#!/bin/bash


if [ $# -gt 0 ] ;
then
    SHC=$1; shift;

    for name in `grep -l /home/gchak ~/modelling/strats/$SHC/*/*`; do cat $name | sed -e 's#/home/gchak/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done
    for name in `grep -l /home/sghosh ~/modelling/strats/$SHC/*/*`; do cat $name | sed -e 's#/home/sghosh/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/piyush ~/modelling/strats/$SHC/*/*`; do cat $name | sed -e 's#/home/piyush/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/ravi ~/modelling/strats/$SHC/*/*`; do cat $name | sed -e 's#/home/ravi/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 

    for name in `grep -l /home/gchak stratwork/$SHC/*`; do cat $name | sed -e 's#/home/gchak/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done
    for name in `grep -l /home/sghosh stratwork/$SHC/*`; do cat $name | sed -e 's#/home/sghosh/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/piyush stratwork/$SHC/*`; do cat $name | sed -e 's#/home/piyush/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/ravi stratwork/$SHC/*`; do cat $name | sed -e 's#/home/ravi/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 

    for name in `grep -l /home/gchak sghosh/stratwork/$SHC/*`; do cat $name | sed -e 's#/home/gchak/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done
    for name in `grep -l /home/sghosh sghosh/stratwork/$SHC/*`; do cat $name | sed -e 's#/home/sghosh/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/piyush sghosh/stratwork/$SHC/*`; do cat $name | sed -e 's#/home/piyush/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/ravi sghosh/stratwork/$SHC/*`; do cat $name | sed -e 's#/home/ravi/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 

else
    for name in `grep -l /home/gchak ~/modelling/strats/*/*/*`; do cat $name | sed -e 's#/home/gchak/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done
    for name in `grep -l /home/sghosh ~/modelling/strats/*/*/*`; do cat $name | sed -e 's#/home/sghosh/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/piyush ~/modelling/strats/*/*/*`; do cat $name | sed -e 's#/home/piyush/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/ravi ~/modelling/strats/*/*/*`; do cat $name | sed -e 's#/home/ravi/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 

    for name in `grep -l /home/gchak stratwork/*/*`; do cat $name | sed -e 's#/home/gchak/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done
    for name in `grep -l /home/sghosh stratwork/*/*`; do cat $name | sed -e 's#/home/sghosh/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/piyush stratwork/*/*`; do cat $name | sed -e 's#/home/piyush/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/ravi stratwork/*/*`; do cat $name | sed -e 's#/home/ravi/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 

    for name in `grep -l /home/gchak sghosh/stratwork/*/*`; do cat $name | sed -e 's#/home/gchak/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done
    for name in `grep -l /home/sghosh sghosh/stratwork/*/*`; do cat $name | sed -e 's#/home/sghosh/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/piyush sghosh/stratwork/*/*`; do cat $name | sed -e 's#/home/piyush/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 
    for name in `grep -l /home/ravi sghosh/stratwork/*/*`; do cat $name | sed -e 's#/home/ravi/#'/home/dvctrader'/#g' > $name"_d"; mv $name"_d" $name ; done 

fi