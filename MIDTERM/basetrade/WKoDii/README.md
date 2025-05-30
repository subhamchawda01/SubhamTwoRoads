# What is today going to be like

[Initial Demo #Video](https://youtu.be/2c3M1FHclXM)

[Example of Use in Pickstrats #Video](https://youtu.be/h2otLdHkjhI)

## Motivation

On most days it is not easy to answer this question, but on a few days it’s relatively straightforward. Also translating qualitative answers to numbers, one can work with, is a non-trivial problem.

Following is one attempt to answer the above question on any given day.

For each day, we record certain statistics which we feel representative of how was that day. This could be things like what is VIX value, how much did ZN / ES trade, how much did ZN / ES move, etc. Then we try to come up with simple model to predict this statistic for the next day. It might seem that it is a difficult job to predict these stats but then my guess is typically there is a decent autocorrelation in time series of such stats. Also we should compare it to what we have currently which is no opinion on any of these statistics. Currently I think we do indirectly try to capture the above by giving higher weights to recent days in pick-strats. But that can often lead to picking overfitted and less robust models.

So lets assume, we have these predictions for the next day. Then for any day in history I can figure out how similar that day was to my predicted-today and use that similarity metric as a weight for that day’s results in picking strategies ( and risk ) for today. This helps in having large number of days for picking while still making sure that we’re taking current market conditions into consideration.


## Enumeration of Features for characterization of day

For each day, we want to keep a measure of high level features:
- general Volatility in the market
- market volume
- the market behavior in Equities in comparison to that in Bonds
- the trend in Equities
- the Currencies shift (i.e. exchange rate for currency of a stable market vs an unstable one) 

To realize these statistics, we are using the following features currently:
1. stdev(ES_0), stdev(FESX_0)
2. volume(ES_0), volume(FESX_0)
3. cor(ES_0,ZN_0)
4. cor(VX_0,ES_0)
5. cor(BR_DOL_0,ES_0)

Thus, currently, we are using certain statistics for US and EU markets only.. As, the system becomes more robust and useful, we can add the features for some other markets as well.

# Version 1

## Modules

### Return the feature vector for any given day

- script: `basetrade/WKoDii/get_day_features.pl`
- Usage: `SHC DATE [Lookback] [Features-Config-File]`
- Input: 
  - Shortcode
  - Date
  - (optional) No. of Lookback Days
  - (optional) Filepath enumerating the the features (Default: `/spare/local/tradeinfo/day_features/product_configs/<SHC>_config.txt`)
  - (optional) `start_hhmm end_hhmm`
- Output: 
  - Feature Data for all the days provided
- Constraints:
  - Features can be of :
    - `<SHC> VOL`
    - `<SHC> STDEV`
    - `<SHC> L1SZ`
    - `<SHC> ORDSZ`
    - `<SHC> TREND`
    - `<SHC> SSTREND`
    - `<SHC1> CORR <SHC2>`
    - Any other `<fname>` present as `/NAS1/SampleData/<SHC>/<DATE>/<fname>.txt`
  - Data for each feature should be present in the corresponding file in  /NAS1/SampleData
- Usage Example: `~/basetrade/WKoDii/get_day_features.pl 20150720`
  - `-0.0538 935188.0000 8.6600 563056.0000 0.3977 -0.4779 -0.0947` 


### Shell Script for taking the feature_file and computing the similarity metrics for all dates

- script: `basetrade/WKoDii/obtain_weights_on_days.sh`
- Usage: `<script> <current_date> <feature_file> [<arima_paramfile/ARIMA_DEF>] [<distance_metric>]`
- This file is used as user-front-end ( It uses `WKoDii/obtain_weights_on_days.py` )
- Inputs:
  1. the date for which we need to forecast the feature vector
  2. the features file 
  3. arima_param_file: 
    - If the model is arima, then the file specifying the arima order (p,d,q) for each feature
    - File format:
      - each line: `<feature_column_no.>,<P>,<D>,<Q>`
    - Default: ARIMA_DEF
  4. distance metric. Mahalanobis distance is used by default
- Output:
  1. `<date> <similarity_metric>` for all dates in the feature file


### Internal Script: Python module for obtaining the weights of the past days relative to the date whose feature vector we are predicting

- script: `basetrade/WKoDii/obtain_weights_on_days.py`
- Usage: `<script> <current_date> [<dates>(-1 for all)] [pretty_print(1)] [<feature_file>] [<arima_param_file>/ARIMA_DEF] [<distance_metric>] [USE_LAST_DAY]`
- This file can either be used as a script or a module. It has been designed heavily for usage from either a foreign python code or a front end script which requires the weights for different dates
- Inputs:
  1. the date for which we need to forecast the feature vector
  2. (-1 to ignore) a JSON string containing the dates for which we require the relative weights
  3. pretty_print ( 1:readable_format output, 2:JSON_output )
  4. feature_file: the training file for the model
  5. arima_param_file ( Default: ARIMA_DEF )
  6. distance metric. Mahalanobis distance is used by default
  7. USE_LAST_DAY: if Not NULL, USE_LAST_DAY is used for forecasting instead of arima
- Output:
  1. if pretty_print is 1: `<date> <similarity_metric>` for all dates in the feature file
  2. else: JSON string populated with the weights for the different dates which were fed to the input
- Example: `~/basetrade/WKoDii/obtain_weights_on_days.py 20150721 '{"20150629": 0.0, "20150630": 0.0}' 0 
  - `{"20150629": 0.24896063543947267, "20150630": 0.18016074812007823}`


### Internal Script: Generate the feature vector for the next day

- script: `basetrade/WKoDii/forecast_dayfeatures.R`
- Usage: `<script> <date> [num_of_pred=1] [model=ARIMA/USE_LAST_DAY] [<datafile>] [arima_param_file=/spare/local/tradeinfo/day_features/dayfeatures_arima_param, ARIMA_DEF=(p,d,q=1,1,1 for ALL)]`
- Input:
  1. Date (to be forecasted for)
  2. number of future predictions (default: 1)
  3. model_type: ( ARIMA(default) / USE_LAST_DAY)
  4. Datafile: Feature vectors for all past days (Default: `/spare/local/tradeinfo/day_features/dayfeatures_config.txt`)
    - each line: `<Date> <Feature Vector>`
  5. arima_param_file: 
    - If the model is arima, then the file specifying the arima order (p,d,q) for each feature
    - File format:
      - each line: `<feature_column_no.>,<P>,<D>,<Q>`
    - Default: /spare/local/tradeinfo/day_features/dayfeatures_arima_param
- For Arima analysis: the following script can be useful:
    - `basetrade/WKoDii/fit_arima_dayfeatures.R`
- Usage Example: `~/basetrade/WKoDii/forecast_dayfeatures.R 20150721`
    - `-0.03579097 1190211 11.22349 851117.4 0.4468696 -0.4910311 -0.1399669`


### Summarize script on the most similar days

- script: `ModelScripts/summarize_strats_for_similar_days.pl`
- Usage: `<script> shc strats_dir/strats_list_file result_dir trading_start_yyyymmdd [trading_target_yyyymmdd=TODAY] [similar_days_percentile=0.1] [Features_Config=DEFAULT] [Arima_Config=ARIMA_DEF] [sortalgo=kCNAPnlAdjAverage] [skip_file=INVALIFILE]`
- This script summarizes the results for K% most similar days to the target_trading_date(default: TODAY)
- Inputs:
  1. `shc strats_dir/strats_list_file result_dir trading_start_yyyymmdd` : similar to arguments for `summarize_strategy_results`
  2. `trading_target_yyyymmdd`: the target date (with which similarity is to be checked)
    - the end date for `summarize_strategy_results` is one date prior to trading_target_yyyymmdd
  3. `similar_days_percentile`: K, where top K% most similar days are o be picked
  4. `sortalgo skip_file` : similar to arguments for `summarize_strategy_results`
  5. Features_Config
  6. Arima_Config ( `ARIMA_DEF` for (p,d,q) as (1,1,1) for all features 



## USE_CASES

### Get a crude idea of which features are useful for a product:
- For this, we randomly select a few strats from the pool and and for each strat, see the correlations of the features with its pnl (over samples or days).. The features that consistently have low correlation for all the strats are deemed as unimportant feature and should be removed from the final set.
- Steps:
  1. See/Modify the product feature-set here: `/spare/local/tradeinfo/day_features/product_configs/<SHC>_config.txt`
  1. Select a few strats fom a pool.. For each strat (say, stt1), repeat the following steps
  2. Generate the features pnl table for stt1:
    - `~/basetrade/WKoDii/get_slot_features.pl <SHC> <stt1> <end_date> <lookback_days>`
    - Example: `~/basetrade/WKoDii/get_slot_features.pl LFR_0 w_dt_strat_ilist_LFR_0_US_OMix_OMix_mnl_c_128_na_e3_20150114_20150223_EST_800_BST_1700_4000_c1_0_0_fv_FSRMFSS.5.0.02.0.0.0.65.20.N.tt_EST_800_BST_1700.pfi_3 20150910 20 > ./feature_pnl.dat`
  3. See the correlations of the features from the output data
    - Example: `~/basetrade_install/bin/get_dep_corr ./feature_pnl.dat`
  4. Remove the features that have poor correlation on all/most strats..
  5. Repeat the procedure
   
### Compute Similarity based on New Features Config-File

- Steps
  1. basetrade/WKoDii/get_day_features.pl <date> <lookback_days> <features_config_file> > <features_file>
  2. basetrade/WKoDii/obtain_weights_on_days.sh <forecast_date> <features_file>
- Example:
  1. `~/basetrade/WKoDii/get_day_features.pl 20150728 200 ~/hrishav/wkodii/config.txt > ~/hrishav/wkodii/features.txt`
  2. `~/basetrade/WKoDii/obtain_weights_on_day.sh 20150728 ~/hrishav/wkodii/features.txt`

### Use in Pickstrats script

- In the pick_strats config, in `INTERVALS_TO_PICK_FROM` field:
  - add `<num_days> DAYFEATURES_CLUSTER <similar_days_percentile> <interval_weight>`
  - Example: `60 DAYFEATURES_CLUSTER 0.2 1.0`
  - This creates an interval including the most similar days.. If we want to use only this interval for pick_strats, remove all other intervals from `INTERVALS_TO_PICK_FROM` field

### Backtest of Pickstrats script
- Create 2 or more versions of the pickstrats config to compare.
- List the full path of the those configs in a file
- run basetrade/scripts/backtest_pickstrats_simple.pl 
  - `basetrade/scripts/backtest_pickstrats_simple.pl DATE SHORTCODE TIMEOFDAY [ LIST OF CONFIG-FILES ]`
- Example: 
- File: `yfebm0_configlist`
  - `cat yfebm0_configlist
     /home/dvctrader/modelling/automated_pick_strats_config/EU/YFEBM_0.EU.sim.txt
     /home/dvctrader/modelling/automated_pick_strats_config/EU/YFEBM_0.EU.nosim.txt
     /home/dvctrader/modelling/automated_pick_strats_config/EU/YFEBM_0.EU.txt1`
- `~/basetrade/scripts/backtest_pickstrats_simple.pl 20150831 YFEBM_0 EU_MORN_DAY yfebm0_configlist > yfebm0log 2>&1 &`
- At the end of the logfile, you will find the comparison stats for all the config-versions for 10,30,90,150 Days..
  - Note: Rely on the Stats of 150 Days, as it has the most data for the stats and thus is less prone to random noise

