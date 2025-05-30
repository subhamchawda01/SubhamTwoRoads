echo "unlinking"
unlink /spare/local/BarData
unlink /spare/local/BarData_SPOT
unlink /spare/local/INDEX_BARDATA_BSE
unlink /spare/local/Db_DATASET
unlink /spare/local/INDEX_BARDATA
unlink /spare/local/CASH_BarData
unlink /spare/local/INDEX_BARDATA_CBOE

echo "linking"
ln -s /spare/local/Bardata/BarData /spare/local/BarData
ln -s /spare/local/Bardata/BarData_SPOT /spare/local/BarData_SPOT
ln -s /spare/local/Bardata/INDEX_BARDATA_BSE /spare/local/INDEX_BARDATA_BSE
ln -s /spare/local/Bardata/Db_DATASET /spare/local/Db_DATASET
ln -s /spare/local/Bardata/INDEX_BARDATA /spare/local/INDEX_BARDATA
ln -s /spare/local/Bardata/CASH_BarData /spare/local/CASH_BarData
ln -s /spare/local/Bardata/INDEX_BARDATA_CBOE/ /spare/local/INDEX_BARDATA_CBOE
