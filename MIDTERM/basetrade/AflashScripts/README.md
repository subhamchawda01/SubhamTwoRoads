# Alphaflash scripts

## Testing Setup: How to setup the strats and scale_values for an event for multiple shortcodes

### Steps:
1. Create a dat file with event numbers from bbg file (or fxstreet file)
  1. Script: `basetrade/AflashScripts/generate_event_dat.R` 
  2. Usage: `<script> <BBG/FXS> <dat_filename> <ev1_bbg_name> <ev2_bbg_name> .. `
  3. Arguments:
    1. `<BBG/FXS>` : Choose datasource as either BBG File or FXSTREET File
      - BBG File: `/home/dvctrader/modelling/alphaflash/bbg_us_eco.csv`
      - FXSTREET File: `/home/dvctrader/modelling/alphaflash/fxstreet_us_eventdates.csv`
    2. `<dat_filename>` : the Output Filename 
    3. `<ev1_bbg_name> <ev2_bbg_name> ..` : List of Event Names for which data is to be extracted
      - event names should match with that in the specific source data ( BBG File / FXSTREET File )
  4. Output File Format:
    - Each row: <data> <time> <Survey,Actual,Prior,Revised for each event>
  5. Data Sources:
    - Note: FXS File has time-corrected values.. which is NOT What we want here (hence NOT accurate)
    - BBG File, on that other hand, has the values as release on that date 

2. Create a folder in which all analysis for this event are to be done
  - Here, create a file (lets say, `shclist`) with each row as: `<SHC> <UTS for this SHC> <MUR for this SHC`
  - Create s sample paramfile to be replicated for all shortcodes:
    - Default sample paramfile: `basetrade/AflashScripts/paramfile_sample`

3. Generate Price_Changes for minutes(2,5,10,20) post-event for all shortcodes
  1. Script: `basetrade/AflashScripts/generate_pxch.sh
  2. Usage: `<script> <fld_path> <event_time> <event_datfile> <shclist>`
  3. Arguments:
    1. `fld_path`: the folder created in step.2
    2. `event_time`: event_time
    3. `event_datfile`: dat_file created in step.1
    4. `shclist` : the `shclist` file created in step.2
  4. Output:
    - It generates a file `<shc>/pxchange.dat` with each line as 1 instance of that event
      - this file has the price changes for minutes(2,5,10,20) post-event as compared to the prior-event price

4. (Can be SKIPPED) Test LM with X as the event_values (released - expected) and Y as price-changes 
  1. Script: `basetrade/AflashScripts/findScale.R`
  2. Usage: `<script> <event_datfile> <pxchange.dat file>`
    1. `<event_datfile>` : dat_file created in step.1
    2. `<pxchange.dat>` : created in step.3
  3. Output: beta with 0-mean LM model as `( Y ~ 0 + beta * X` )`
    - for each post-event duration (2,5,10), it outputs
      - beta values
      - adjusted r.squared
  4. This script can be further personlized/exteneded to facilitate other tests and ideas

5. Generate Beta Values and Sync them across all servers
  1. Beta Values stored here: `/home/spare/local/tradeinfo/Alphaflash/af_events_scale_ids2.txt`
  2. Script: `basetrade/AflashScripts/genscale.sh`
  3. Usage: `<script> <shortcode> <event_datfile> <event_id> <beta2/beta5/beta10> <cat1_id> <cat2_id> ..`
    1. shortcode
    2. `<event_datfile>` : dat_file created in step.1
    3. `<event_id>`: the alphaflash id for event ( for e.g. : 45 for NFP )
    4. `<beta2/beta5/beta10>`: depending on which post_mins duration of (2,5,10) is used, select the corresponding option
    5. `<datum1_id> <datum2_id> ..` : the datum ids in order which the dat_file is created
  4. It Outputs the lines that can be directly add to `/home/spare/local/tradeinfo/Alphaflash/af_events_scale_ids2.txt`
  5. Add it to `/home/spare/local/tradeinfo/Alphaflash/af_events_scale_ids2.txt` is `dvctrader@ny11`
    - then, sync to it all servers

6. Generate the Strat files for the shortcodes
  1. Script: `basetrade/AflashScripts/prepStrats.sh`
  2. Usage: `<script> <fld_path> <event_id> <event_time> <query_runtime_minutes> <event_datfile> <shclist>`
    1. `fld_path`: the folder created in step.2
    2. `<event_id>`: the alphaflash id for event ( for e.g. : 45 for NFP )
    3. `event_time`: event_time
    4. `<query_runtime_minutes>`: to specify when the strat will get flat
    5. `<event_datfile>` : dat_file created in step.1
    6. `<shclist>` : the `shclist` file created in step.2
  3. It will create the strategies
  4. It will also output the commands to run the strategies on all past-days


### Example:

1. `~/basetrade/AflashScripts/generate_event_dat.R BBG ~/nfp.dat Change_in_Nonfarm_Payrolls Unemployment_Rate`
2. `mkdir ~/af_backtest_offline/nfp2`
3. `echo -e "ZN_0 10 4\nCGB_0 20 4\nBR_DOL_0 50 4\n" > ~/af_backtest_offline/nfp2/utslist`
4. `~/basetrade/AflashScripts/generate_pxch.sh ~/af_backtest_offline/nfp2 EST_830 ~/nfp.dat utslist`
5. By analysis: say, I find that beta on price-changes of 10mins are doing the best
6. `for shc in `awk '{print $1}' utslist` ; do ~/basetrade/AflashScripts/genscale.sh $shc ~/nfp.dat 45 beta10 1 3 ; done`
7. Add its output to `/home/spare/local/tradeinfo/Alphaflash/af_events_scale_ids2.txt` in ny11
8. in ny11, `~/basetrade/scripts/sync_file_to_all_machines.pl /home/spare/local/tradeinfo/Alphaflash/af_events_scale_ids2.txt`
9. `~/basetrade/AflashScripts/prepStrats.sh /home/hagarwal/af_backtest_offline/nfp2/ 45 EST_830 10 ~/nfp.dat utslist`
10. Use its output as commands to run the strategies

### See the Categories description of the Alphaflash
1. `~/basetrade_install/bin/af_print_xmlspecs ALL | less`
2. Each Event has a category id, many datums (each with its datum id) and two messages (Release and Estimate)

## Queries Installation and Running Setup

1. Production Queries Standards:
  1. Production Strat Location:
    1. Strat File: `/home/dvctrader/af_strats/general/<shc>/w_af_exec`
    2. Param File: `/home/dvctrader/af_strats/general/<shc>/param_<shc>_af_agg`
    3. Model File: `/home/dvctrader/af_strats/general/<shc>/model_<shc>_empty`
  2. Tradeinit Location: `/home/dvctrader/af_strats/af_tradexec`
  3. For each shortcode, query_id is specified here:
    1. `basetrade/AflashScripts/queryids_map.txt`
  4. Estimate files:
    1. `/spare/local/tradeinfo/Alphaflash/Estimates/estimates_<YYYYMMDD>`


2. Installs Queries at Prod-Servers
  1. Script: `basetrade/AflashScripts/setprodqueries.sh`
  2. Usage: `<script> <shortcode_details_list_file> <event_id> <query_start_hhmm> <query_stop_hhmm> [<param_file>]`
    1. `<shortcode_details_list_file>`: list of shortcode details
      - each line: `<shc> <max_uts> <order_scale> <MUR>`
      - `<max_uts>`: the param "UNIT_TRADE_SIZE"
      - `<order_scale> (optional)` : the param "AF_EVENT_MAX_UTS_PXCH"
    2. `<event_id>` : event category id 
    3. `<query_start_hhmm>` : Usually 15 mins prior to the event
    4. `<query_stop_hhmm>` : Usually, 2/5/10/15 mins post the event
    5. `<param_file> (optional)`: If given, installs this param_file in the prod-server
  3. For each shortcode:
    1. NOTE: QueryIds should be mentioned here: `basetrade/AflashScripts/queryids_map.txt`
    2. If the Strat files are not already present, it installs them at proper locations
      - NOTE: in this case, `<param_file>` must be provided as the argument
    3. Updates the UTS, Start and End times, Query_ids and other details

3. For Adding and syncing the estimate values of the Event (DO IT IN dvctrader@ny11):
  1. Open `/spare/local/tradeinfo/Alphaflash/event_fxstreet_map` and find the event
  2. It has the format, `<event_id>, <field_id1:field_name1>, <field_id2:field_name2> .. and so on
  3. Look the the estimated values from fxstreet economic calendar
  4. Add a line in `/spare/local/tradeinfo/Alphaflash/Estimates/estimates_<DATE>` as:
    1. `<event_id> <msecs_from_midnight_for_event> <field_id1:field_estimate1> <field_id2:field_estimate2> ..`
    2. For reference, you can look at the past estimate lines for the event
      1. `grep "^<event_id>" /spare/local/tradeinfo/Alphaflash/Estimates/estimates_*`
  5. `~/basetrade/scripts/sync_file_to_all_machines.pl /spare/local/tradeinfo/Alphaflash/Estimates/estimates_<DATE>`


3. Load, Stop/KILL, send any user-msg to queries
  1. Script: `basetrade/AflashScripts/sendprodmsg.sh`
  2. Usage: `<script> <shortcode_list> <LOAD/PKILL/user_msg_in quotes>`
    1. `<shortcode_list>` : List of shortcodes in a file
    2. `<LOAD/PKILL/user_msg_in quotes>` : 
      - `LOAD` : loads the query with exec `/home/dvctrader/af_strats/af_tradexec`
      - `PKILL` : kills all processes in the prod-server with name `af_tradexec`
      - `<user_msg_in quotes>` : sends this user-msg to the query for this shortcode
        - It assumes that the process for this shortcode is running
  3. NOTE: QueryIds should be mentioned here: `basetrade/AflashScripts/queryids_map.txt`
  4. The Strats should be updated in the prod-server

### Example:

1. `echo "ZN_0 5\nFGBL_0 2\n" > prod_uts1` : ZN with uts of 5, FGBL with uts of 2
2. `~/basetrade/AflashScripts/setprodqueries.sh prod_uts1 98 EST_945 EST_1005`: Set ZN and FGBL for ISM_Manufacturing [event_id: 98] 
3. `~/basetrade/AflashScripts/sendprodmsg.sh prod_uts1 LOAD` : loads the queries
4. `~/basetrade/AflashScripts/sendprodmsg.sh prod_uts1 "--start"` : starts the queries
5. `~/basetrade/AflashScripts/sendprodmsg.sh prod_uts1 SEETRADES` : see the tail of tradelogfiles for the queries
6. `~/basetrade/AflashScripts/sendprodmsg.sh prod_uts1 PKILL` : kills the queries

