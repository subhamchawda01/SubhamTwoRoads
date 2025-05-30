
>/spare/local/files/tmp/seq_gen.txt

/home/pengine/prod/live_scripts/stop_infra.sh

#/home/pengine/prod/live_scripts/SmartDaemonController.sh ORS MSSIM STOP CLEAR >/dev/null 2>&1

#/home/pengine/prod/live_scripts/SmartDaemonController.sh EXCH_SIM STOP >/dev/null 2>/dev/null

#/home/pengine/prod/live_scripts/SmartDaemonController.sh CombinedSource STOP >/dev/null 2>&1
#/home/dvcinfra/trash/animesh/SmartDaemonController.sh CombinedSource STOP >/dev/null 2>&1

#/home/pengine/prod/live_scripts/SmartDaemonController.sh OEBU STOP >/dev/null 2>&1

#/home/pengine/prod/live_scripts/ors_broadcast_logger.sh CFE STOP OFF NF >/dev/null 2>&1


echo "STARTING INFRA"

/home/pengine/prod/live_scripts/SmartDaemonController.sh EXCH_SIM START >/dev/null 2>/dev/null

/home/pengine/prod/live_scripts/SmartDaemonController.sh ORS MSSIM START CLEAR >/dev/null 2>&1

/home/pengine/prod/live_scripts/SmartDaemonController.sh OEBU START >/dev/null 2>&1

/home/pengine/prod/live_scripts/SmartDaemonController.sh CombinedSource START >/dev/null 2>&1
#/home/dvcinfra/trash/animesh/SmartDaemonController.sh CombinedSource START >/dev/null 2>&1

/home/pengine/prod/live_scripts/ors_broadcast_logger.sh CFE START OFF NF >/dev/null 2>&1

