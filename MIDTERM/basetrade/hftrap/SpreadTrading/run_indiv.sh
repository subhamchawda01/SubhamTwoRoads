#Cleanup
for f in /spare/local/MDSlogs/MT_SPRD_EXEC/*_$1; do echo "deleting "$f; rm $f; done

for f in /spare/local/MDSlogs/MT_SPRD_TRADES/*_$1; do echo "deleting "$f; rm $f; done

for f in /spare/local/MDSlogs/MT_SPRD_NAV_SERIES/*_$1; do echo "deleting "$f; rm $f; done

sort -u $HOME/basetrade/hftrap/SpreadTrading/masterparam_$1 |awk '{system("bash ./hftrap/Scripts/run_hftrap_simulations.sh 0 1 "$0" 20151001 20160630 garbage")}'
