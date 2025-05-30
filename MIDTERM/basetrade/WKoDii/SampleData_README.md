## Objective: 
- Gather some important features for products for intraday samples of 15 minutes duration.
- Sample the intraday pnls for all the strats for products. 
- Using the sample data, build tools for:
  - Finding samples for products pertaining to specific kind of period ( e.g. high volume period )
  - Compute the performance of strats during those periods
  - Compute the similarity between two strats
  - Maintain diversity of the pool
- Use this data in the existing scripts such as pickstrats and generate_strategies for tasks such as:
  - Maintaining diversity / Picking diverse set of strats
  - Compute the performance of strats during specific periods


# Setup:
  
- the pnlsamples ( for duration of 15 mins ) for strats in the pool 
  - Location: `/NAS1/ec2_pnl_samples`
  - synced from: ny11: `/home/dvctrader/ec2_pnl_samples`

- Sample Features Data
  - Location: `/NAS1/SampleData`
  - synced from ny11: `/home/dvctrader/SampleData`
  - exec source: `Tools/collect_features_samples.cpp`
  - crontabbed to run daily for products in `modelling/samples_features_configs/prodlist`


## Generate the feature samples
- `Tools/collect_features_samples.cpp`
- Usage: `~/basetrade_install/bin/collect_features_samples <config_file> <date> [<lookback-days>]`
- Config files: `modelling/samples_features_configs/<shortcode>_config.txt`
- Default Config-file Example:
  - `cat ~/modelling/samples_features_configs/FVS_0_config.txt`
    - `SHC FVS_0 0000 2400
       `DURATIONS 300`
       `FEATURE SimpleTrend`
       `FEATURE StableScaledTrend`
       `FEATURE RollingAvgL1Size`
       `FEATURE RollingAvgOrdSize`
       `FEATURE RollingSumTrades`
       `FEATURE RollingSumVolume`
       `FEATURE RollingAvgTradeSize`
       `FEATURE TurnOverRate`
       `FEATURE RollingStdev`
- Config-format
  - `SHC <shortcode> <start_hhmm> <end_hhmm>`
  - `DURATIONS <duration1> <duration2> ..`
    - specifies the default durations to be used for each feature that don’t have duration specified 
- Three types of features are read in config
  - `FEATURE <feature_name> [<durations>]`
    - `<feature_name>` should be a self-indicator of the format:
      - `INDICATOR 1 <feature_name> <DEP_SHC> <Duration> <PriceType>`
    - `<durations>`: list of durations for which this feature is required
    - Example: `FEATURE RollingStdev`
  - `SOURCE_FEATURE <feature_name> <source_shc> [<durations>]`
    - `<feature_name>` should be a source-indicator of the format:
      - `INDICATOR 1 <feature_name> <DEP_SHC> <INDEP_SHC> <Duration> <PriceType>`
    - `<durations>`: list of durations for which this feature is required
    - Example: `SOURCE_FEATURE RollingCorrelation FESX_0`
  - `RAW_INDICATOR <complete indicator with its arguments>`
    - this feature would be mentioned in the ilist as
      - `INDICATOR 1 <complete indicator with its arguments>`
  - Note: for `FEATURE` or `SOURCE_FEATURE`, if `<durations>` is not mentioned then the default durations is taken for this feature


- Utility to read the SampleData and other basic functionality 
  - `GenPerlLib/sample_data_utils.pl`
  - Functions:
    - GetFeatureMap
      - Returns the map [ time_slot -> feature_value ]
        - As the feature_file argument, for simplicity, it has the following aliases :
        - VOL => RollingSumVolume300.txt
        - STDEV => RollingStdev300.txt
        - L1SZ => RollingAvgL1Size300.txt
        - TREND => SimpleTrend300.txt
        - CORR => RollingCorrelation300_<INDEP>.txt
      - For other features, it accepts the feature filename without “.txt” ( for e.g.: StableScaledTrend300 )
    - GetFeatureSum
      - Returns the sum of the feature_values for timeslots between start_time and end_time
    - GetFeatureAverage
      - Returns the average of the feature_values for timeslots between start_time and end_time
  - Note: All other end-user scripts using SampleData must use the GenPerlLib/sample_data_utils.pl for accessing the sample values.


- `scripts/get_avg_samples.pl`
  - Usage: `<script> SHORTCODE DATE NUM_DAYS START_TIME END_TIME (0:AvgOverall / 1:AvgPerDay / 2:AvgPerTimeslot) (VOL / STDEV / L1SZ / TREND / <CORR indep_shortcode> / <other_features> )`
  - Outputs the feature_values for a shortcode in an aggregrate manner.. 3 options
    - `0:AvgOverall`: Outputs the avg of feature_values across timeslot across all days
    - `1:AvgPerday`: Outputs the avg of feature_values across timeslot for each day
    - `2:AvgPerTimeslo`t: Outputs the avg of feature_values across days for each timeslot


- `scripts/get_pnl_strats_specific_samples.pl`
- Outputs Avg PNLs for strats for selective samples ( for e.g. : top 20% volume samples )
- Usage: `<script> SHORTCODE STRAT_DIR/STRAT_LIST DATE NUM_DAYS START_TIME END_TIME ( VOL / STDEV / L1SZ / <CORR indep_shortcode> / <other features> ) ( FRACTION ) HIGH / LOW`


