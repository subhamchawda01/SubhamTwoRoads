# Mock Trading UI and Automation Scripts

## UI Access for Mock Machine Details

The UI for monitoring mock machine statuses is hosted at:

🔗 **[http://10.23.5.67/mock.html](http://10.23.5.67/mock.html)**

This UI displays data pulled from the JSON files located at:

```bash
/var/www/html/mockData/20250412/IND12_mock_trading_20250412.json
```

These files are synchronized from all relevant servers using the script and this to be run every 5 mins to get the updated files:

```bash
bash /home/pengine/prod/live_scripts/sync_mock_files.sh
```

---

## Example JSON Data Format

An example of the `IND12_mock_trading_20250412.json` structure:

```json
{
    "ors_up": true,
    "control_shm_up": false,
    "console_trader": false,
    "trade_engine_up": true,
    "trade_engine_data_listening": {
        "121191": true,
        "121192": true,
        "121393": true
    },
    "userwise_trades": {}
}
```

---

## Running Mock Trading

To manage mock trading sessions from the host server (`10.23.5.67`), use the following command:

```bash
bash /home/pengine/prod/live_scripts/mock_trading_on_all_servers.sh YYYYMMDD IST_930 IST_1530 ACTION
```

### ACTION values
- `START` – Starts all mock trading processes
- `STOP` – Stops all mock trading processes
- `MOVE` – Moves mock-related files to the `important` directory

### Example

```bash
bash /home/pengine/prod/live_scripts/mock_trading_on_all_servers.sh 20250412 IST_930 IST_1530 START
```

This command triggers the following script on all mock servers:

```bash
bash /home/pengine/prod/live_scripts/mock_trading_json.sh ACTION DATE START_TIME END_TIME /home/pengine/prod/live_configs/mock_trading_config.txt
```

---

## Configuration: `mock_trading_config.txt`

This config file defines the operations for each mock component:

```bash
exchange NSE
server IND24
start_ors /home/pengine/prod/live_scripts/SmartDaemonController.sh ORS MSEQ10 START KEEP
stop_ors /home/pengine/prod/live_scripts/SmartDaemonController.sh ORS MSEQ10 STOP KEEP
start_control_shm_writer /home/pengine/prod/live_scripts/SmartDaemonController.sh CONTROL_SHM_WRITER START
stop_control_shm_writer /home/pengine/prod/live_scripts/SmartDaemonController.sh CONTROL_SHM_WRITER STOP
addts_for_multiprod_console_trader /home/pengine/prod/live_scripts/orsRejectHandling.sh addts ~/important/product_file_eq
trading_by_multiprod_console_trader LD_PRELOAD=/home/dvcinfra/important/libcrypto.so.1.1 /home/dvcinfra/important/multi_prod_console_trader_bkp_expiry_changes ~/important/product_file_eq
start_trade_engine sudo -u dvctrader /home/dvctrader/ATHENA/run.sh
stop_trade_engine pkill -f trade_eng
kill_mock_background_process pkill -f mock
move_strat_files mv /spare/local/logs/tradelogs/*DATE* ~/important/
move_ors_files mv /spare/local/ORSlogs/NSE_EQ/MSEQ10/*DATE* ~/important/
```

---

## Notes

- The JSON file is updated every minute via background checks.
- Ensure proper permissions and runtime environments are maintained across all servers.

---

> This `README.md` provides comprehensive information for UI access, JSON structure, and scripts related to mock trading operations. Update paths, commands, and configs as needed.
