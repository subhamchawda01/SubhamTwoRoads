
for name in `cat ~/list_of_machines.txt`; 
do 
    echo $mac_name_ ; 
    scp -rp tradeinfo/LRDBBaseDir $USER@$mac_name_:/spare/local/tradeinfo/ ; 
    scp -rp tradeinfo/StdevInfo $USER@$mac_name_:/spare/local/tradeinfo/ ; 
    scp -rp tradeinfo/PortfolioInfo $USER@$mac_name_:/spare/local/tradeinfo/ ; 
    scp -rp tradeinfo/PortfolioRiskInfo $USER@$mac_name_:/spare/local/tradeinfo/ ; 
done
