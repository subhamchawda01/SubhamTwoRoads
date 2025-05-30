#!/bin/bash
    FILE="$HOME/positions/ALL_POSITIONS";

    if [ ! -d $HOME/positions ] ;
    then
	mkdir $HOME/positions;
    fi

	> $FILE;

	YYYYMMDD=$(date "+%Y%m%d");
	
	if [ $# -eq 1 ] ;
	then
	    YYYYMMDD=$1;
	fi
	
	function CopyPos {
         grep _Id $1 | head -`head -1 $1 | cut -d':' -f2 | sed 's/ //g'` >> $FILE
    }  

    #CME servers
	rsync -avz  --quiet dvcinfra@10.23.82.51:/spare/local/ORSlogs/CME/*/position.$YYYYMMDD   $HOME/positions/cme11.position.$YYYYMMDD    
	CopyPos $HOME/positions/cme11.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.82.52:/spare/local/ORSlogs/CME/*/position.$YYYYMMDD   $HOME/positions/cme12.position.$YYYYMMDD    
	CopyPos $HOME/positions/cme12.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.82.53:/spare/local/ORSlogs/CME/*/position.$YYYYMMDD   $HOME/positions/cme13.position.$YYYYMMDD    
	CopyPos $HOME/positions/cme13.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.82.54:/spare/local/ORSlogs/CME/*/position.$YYYYMMDD   $HOME/positions/cme14.position.$YYYYMMDD    
	CopyPos $HOME/positions/cme14.position.$YYYYMMDD ;

    #EUREX server
    #This way we will have a backup to revert to the last known file if rsync fails on the network
    #scp seems to be costly
    rsync -avz  --quiet dvcinfra@10.23.200.51:/spare/local/ORSlogs/EUREX/*/position.$YYYYMMDD   $HOME/positions/fr11.position.$YYYYMMDD
    CopyPos $HOME/positions/fr11.position.$YYYYMMDD ; 

	rsync -avz  --quiet dvcinfra@10.23.200.52:/spare/local/ORSlogs/EUREX/*/position.$YYYYMMDD   $HOME/positions/fr12.position.$YYYYMMDD
	CopyPos $HOME/positions/fr12.position.$YYYYMMDD ; 
	
	rsync -avz  --quiet dvcinfra@10.23.200.53:/spare/local/ORSlogs/EUREX/*/position.$YYYYMMDD   $HOME/positions/fr13.position.$YYYYMMDD    
	CopyPos $HOME/positions/fr13.position.$YYYYMMDD ;

    rsync -avz  --quiet dvcinfra@10.23.200.54:/spare/local/ORSlogs/EUREX/*/position.$YYYYMMDD   $HOME/positions/fr14.position.$YYYYMMDD
    CopyPos $HOME/positions/fr14.position.$YYYYMMDD ; 

    #TMX server
    #This way we will have a backup to revert to the last known file if rsync fails on the network
    #scp seems to be costly
	rsync -avz  --quiet dvcinfra@10.23.182.51:/spare/local/ORSlogs/TMX/*/position.$YYYYMMDD   $HOME/positions/tmx11.position.$YYYYMMDD    
	CopyPos $HOME/positions/tmx11.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.182.52:/spare/local/ORSlogs/TMX/*/position.$YYYYMMDD   $HOME/positions/tmx12.position.$YYYYMMDD    
	CopyPos $HOME/positions/tmx12.position.$YYYYMMDD ;
    # Brazil
    # Add retail trades first

	rsync -avz  --quiet dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/MS001/position.$YYYYMMDD   $HOME/positions/bmf11.position.$YYYYMMDD
	CopyPos $HOME/positions/bmf11.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.23.13:/spare/local/ORSlogs/BMFEP/MS003/position.$YYYYMMDD   $HOME/positions/bmf13.position.$YYYYMMDD    
	CopyPos $HOME/positions/bmf13.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.23.12:/spare/local/ORSlogs/BMFEP/MS002/position.$YYYYMMDD   $HOME/positions/bmf12.position.$YYYYMMDD
	CopyPos $HOME/positions/bmf12.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.23.14:/spare/local/ORSlogs/BMFEP/*/position.$YYYYMMDD   $HOME/positions/bmf14.position.$YYYYMMDD    
	CopyPos $HOME/positions/bmf14.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.23.15:/spare/local/ORSlogs/BMFEP/*/position.$YYYYMMDD   $HOME/positions/bmf15.position.$YYYYMMDD    
	CopyPos $HOME/positions/bmf15.position.$YYYYMMDD ;

	rsync -avz  --quiet dvcinfra@10.23.23.11:/spare/local/ORSlogs/BMFEP/BMFEQ1/position.$YYYYMMDD   $HOME/positions/bmfeq11.position.$YYYYMMDD
	CopyPos $HOME/positions/bmfeq11.position.$YYYYMMDD ;

    #LIFFE

    rsync -avz  --quiet dvcinfra@10.23.52.52:/spare/local/ORSlogs/LIFFE/MSBSL2/position.$YYYYMMDD   $HOME/positions/bsl12.position.$YYYYMMDD
    CopyPos $HOME/positions/bsl12.position.$YYYYMMDD ;

#ICE
	rsync -avz  --quiet dvcinfra@10.23.52.51:/spare/local/ORSlogs/ICE/MSICE1/position.$YYYYMMDD   $HOME/positions/bsl11.ice.position.$YYYYMMDD
	CopyPos $HOME/positions/bsl11.ice.position.$YYYYMMDD ;

    rsync -avz  --quiet dvcinfra@10.23.52.53:/spare/local/ORSlogs/ICE/MSICE2/position.$YYYYMMDD   $HOME/positions/bsl13.ice.position.$YYYYMMDD
    CopyPos $HOME/positions/bsl13.ice.position.$YYYYMMDD ;

#HKEX
     rsync -avz  --quiet dvcinfra@10.152.224.145:/var/TradingSystemLog/spare/local/ORSlogs/HKEX/FITGEN/position.$YYYYMMDD   $HOME/positions/allhk11.position.$YYYYMMDD
     CopyPos $HOME/positions/allhk11.position.$YYYYMMDD ;

#OSE

    rsync -avz  --quiet dvcinfra@10.134.210.182:/spare/local/ORSlogs/OSE/T2DVC22563/position.$YYYYMMDD   $HOME/positions/alltok12.position.$YYYYMMDD
    CopyPos $HOME/positions/alltok12.position.$YYYYMMDD ;

#MICEX
	rsync -avz  --quiet dvcinfra@172.18.244.107:/spare/local/ORSlogs/MICEX/MICEXPROD01/position.$YYYYMMDD $HOME/positions/allmicex11.position.$YYYYMMDD
	CopyPos $HOME/positions/allmicex11.position.$YYYYMMDD ;

#RTS
	rsync -avz  --quiet dvcinfra@172.18.244.107:/spare/local/ORSlogs/RTS/FORTSPROD01/position.$YYYYMMDD $HOME/positions/allrts11.position.$YYYYMMDD
	CopyPos $HOME/positions/allrts11.position.$YYYYMMDD ;


	rsync -avz  --quiet dvcinfra@10.23.74.61:/spare/local/ORSlogs/CFE/MSCFE/position.$YYYYMMDD   $HOME/positions/cfe11.position.$YYYYMMDD
	CopyPos $HOME/positions/cfe11.position.$YYYYMMDD ;
	
	cat $FILE | mail -s "Positions Loaded by ORS" nseall@tworoads.co.in
	



