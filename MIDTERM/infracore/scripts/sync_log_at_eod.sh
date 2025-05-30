#!/bin/bash
YYYYMMDD=$(date "+%Y%m%d")
  #rsync -avz --progress /spare/local/logs/alllogs/MediumTerm/weeklyshortgamma60_execlogic_trades_$YYYYMMDD.dat dvctrader@10.23.115.62:/spare/local/files/NSE/MidTermWeeklyShortgammaLogs/nse_complex_execlogic_trades_60_$YYYYMMDD.dat >/dev/null 2>&1
  #rsync -avz --progress /spare/local/logs/alllogs/MediumTerm/rv_execlogic_trades_$YYYYMMDD.dat dvctrader@10.23.115.62:/spare/local/files/NSE/FileExecLogs/nse_complex_execlogic_trades_60_$YYYYMMDD.dat >/dev/null 2>&1
  #rsync -avz --progress /spare/local/logs/alllogs/MediumTerm/midterm_execlogic_trades_$YYYYMMDD.dat dvctrader@10.23.115.62:/spare/local/files/NSE/ExecutionLogs/nse_complex_execlogic_trades_$YYYYMMDD.dat >/dev/null 2>&1
  #rsync -avz --progress /spare/local/logs/alllogs/MediumTerm/midterm_execlogic_snapshot_$YYYYMMDD dvctrader@10.23.115.62:/spare/local/files/NSE/ExecutionLogs/execlogic_snapshot_$YYYYMMDD >/dev/null 2>&1
gzip /spare/local/files/NSE/ExecutionLogs/execlogic_dbglog.$YYYYMMDD
gzip /spare/local/files/NSE/FileExecLogs/execlogic_dbglog.$YYYYMMDD
gzip /spare/local/files/NSE/DispersionExecLogs/execlogic_dbglog.$YYYYMMDD
  rsync -avz --progress /spare/local/logs/alllogs/MediumTerm/midterm_execlogic_dbglog.${YYYYMMDD}.gz dvctrader@10.23.115.62:/spare/local/files/NSE/ExecutionLogs/execlogic_dbglog.${YYYYMMDD}.gz
  rsync -avz --progress /spare/local/logs/alllogs/MediumTerm/rv_execlogic_dbglog.${YYYYMMDD}.gz dvctrader@10.23.115.62:/spare/local/files/NSE/FileExecLogs/execlogic_dbglog.${YYYYMMDD} .gz
  rsync -avz --progress /spare/local/logs/alllogs/MediumTerm/disp_execlogic_dbglog.${YYYYMMDD}.gz dvctrader@10.23.115.62:/spare/local/files/NSE/DispersionExecLogs/execlogic_dbglog.${YYYYMMDD}.gz
