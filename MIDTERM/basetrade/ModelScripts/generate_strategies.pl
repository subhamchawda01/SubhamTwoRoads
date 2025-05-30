#!/usr/bin/perl

# \fileaModelScripts/generate_strategies.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes an instructionfilename :
#
# indicatorlistfile ( this does not have weights ... so this is an indicatorlistfile and not a model_file ... the only difference between them is that the former has dummy weights = 1.0 )
# startdate 
# enddate
# predalgo
# predduration
# regress exec choice
# ( predduration and ) paramfiles 
#
# Broadly speaking, from the indicatorlistfile it generates normal timed_data
# then based on the arguments, it makes reg_data 
# and then different model_files by running the specified regressions
#
# For each of these days:
# it generates normal data 
# foreach duration
# foreach predalgo
# processes timed_data by 'timed_data_to_reg_data' on reg_data 
# foreach regress_exec_with_params makes a different model
# adds the model to the map duration_to_model_filevec_
# and based on the set of paramfiles for that duration makes the corresponding strategyfile
# finds results on the strategyfiles :

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use Scalar::Util qw(looks_like_number);
use sigtrap qw(handler signal_handler normal-signals error-signals);
no if ($] >= 5.018), 'warnings' => 'experimental';

sub LoadInstructionFile ; # used to read the instructions
sub SanityCheckInstructionFile ; 
sub RunRegressMakeModelFiles ; # Generates the regdata, run regression and makes the Model-files
sub AddModelFileToList ; # privately called in RunRegressMakeModelFiles
sub MakeStrategyFiles ; # takes model and paramfile and makes strategyfile
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub SummarizeLocalResultsAndChoose ; # from the files created in the local_results_base_dir choose the best ones to send to pool
sub ComputeStatsSingleRegFile ; # not used now but to comput info of a single reg file
sub AnalyseModelFile ; # test Modelfile
sub MakeRegimeModels ; 
sub ProcessDatagenTimeoutString ; # Process the timeout argument of the datagen
sub ChooseIlist ; # choose the ilists from the options
sub DatagenPeriodsProcess ; # Processes if any kinds of period (for e.g.: bad-period) selection is to be used after the datagen
sub RescaleModelL1Norm; # Rescales the model to a l1norm for a set of dates
sub RunRegressExec ; # Runs the regression and make the model files depending on the regress-algo
sub GenerateRegdataFiles ; # Generate the reg_data_ files from the ilist
sub FilterRegdata ; # Applies the filter/sampling to the regdata
sub ChooseModelFromCrossValidation ; # chooses the regressalgo for obtain_model_from_crossvalidation_
sub CreateParamListVecFromRegimeIndandParam ; # creates the regime paramlists with regime indicator
sub CreateParamListVecFromRegimeParamList ; # creates the regime paramlists
sub PredDurationToTickValues ; #Change predduration vector to tick threshold vector
sub RunSaveResults ; #Generate and save results for the last 200 days for the strats to be installed
sub GenerateStory ; #Generate and save stratstories AND place the simula-link for the strats in the mail 
sub AddIndicatorAfterIndicatorStart ; # in case of regime models, add the regime indicator to the ilist
sub SplitRegimeRegdata; # split the regdata in case of regime models (when regime_ilists not used)
sub GetOutSamplePPC; # out-of-sample pnl-per-contract
sub CleanFiles; # remove the intermediatory fiels

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $hostname_ = `hostname`;

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $PYSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/PyScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_BASE_DIR=$HOME_DIR."/".$REPO."/scripts";
my $shared_prefix_="/media/shared/ephemeral17/temp_strats/";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $MODELING_STAGED_CONFIGS_DIR=$MODELING_BASE_DIR."/wf_staged_strats"; # this directory is used to store the chosen strategy files
my $MODELING_MODELS_DIR=$MODELING_BASE_DIR."/models"; # this directory is used to store the chosen model files
my $MODELING_PARAMS_DIR=$MODELING_BASE_DIR."/params"; # this directory is used to store the chosen param files
my $GLOBALRESULTSDIR = $HOME_DIR."/ec2_globalresults";
my $GLOBALPNLSAMPLESDIR = $HOME_DIR."/ec2_pnl_samples";

require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir
my $SPARE_LOCAL="/spare/local/";
$SPARE_LOCAL = GetSpareLocalDir() if index ( $hostname_ , "ip-10-0" ) >= 0;
my $SPARE_HOME=$SPARE_LOCAL.$USER."/";

my $GENTIMEDDATA_DAILY_DIR=$SPARE_LOCAL."DailyTimedDataDir/"; # this directory is used to store data so that the calling script can also expect it there
my $GENREGDATA_DAILY_DIR=$SPARE_LOCAL."DailyRegDataDir/"; # this directory is used to store data so that the calling script can also expect it there
my $GENREGDATA_DIR=$SPARE_LOCAL."RegDataDir/"; # this directory is used to store data so that the calling script can also expect it there
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";
my $GENSTRATWORKDIR=$SPARE_HOME."GSW/";

my $DISTRIBUTED_STATUS_SCRIPT = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/view_job_status.py";

`mkdir -p $GENREGDATA_DAILY_DIR` if ( ! -d $GENREGDATA_DAILY_DIR );
`mkdir -p $GENTIMEDDATA_DAILY_DIR` if ( ! -d $GENTIMEDDATA_DAILY_DIR );
`mkdir -p $GENREGDATA_DIR` if ( ! -d $GENREGDATA_DIR );

require "$GENPERLLIB_DIR/is_model_corr_consistent.pl"; # IsModelCorrConsistent
require "$GENPERLLIB_DIR/get_bad_samples_pool_for_shortcode.pl"; # GetBadSamplesPoolForShortcode
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/name_strategy_from_model_and_param_index.pl"; # for NameStrategyFromModelAndParamIndex
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo
require "$GENPERLLIB_DIR/get_indicator_lines_from_ilist.pl"; # GetIndicatorLinesFromIList
require "$GENPERLLIB_DIR/get_high_sharpe_indep_text.pl"; # GetHighSharpeIndepText
require "$GENPERLLIB_DIR/create_sub_model_files.pl"; # CreateSubModelFiles
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec
require "$GENPERLLIB_DIR/find_item_from_vec_ref.pl"; #FindItemFromVecRef
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams cartesian_product
require "$GENPERLLIB_DIR/install_strategy_modelling.pl"; # InstallStrategyModelling
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; # MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_substr.pl"; # MakeStratVecFromDirSubstr
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/get_business_days_between.pl"; # GetBusinessDaysBetween
require "$GENPERLLIB_DIR/is_server_out_of_disk_space.pl"; # IsServerOutOfDiskSpace
require "$GENPERLLIB_DIR/get_sort_algo.pl"; # GetSortAlgo
require "$GENPERLLIB_DIR/get_index_of_high_sharpe_indep.pl"; #GetIndexOfHighSharpeIndep
require "$GENPERLLIB_DIR/is_strat_dir_in_timeperiod.pl"; # IsStratDirInTimePeriod
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/check_ilist_data.pl"; # CheckIndicatorData
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode
require "$GENPERLLIB_DIR/get_avg_trade_count_per_sec_for_shortcode.pl"; # GetAvgTradeCountPerSecForShortcode
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt_and_tag.pl"; # MakeStratVecFromDirAndTTAndTag
require "$GENPERLLIB_DIR/get_param_list_and_l1_norm.pl"; # GetParamListAndL1Norm
require "$GENPERLLIB_DIR/create_mid_base_model.pl"; # CreateMidBaseModel
require "$GENPERLLIB_DIR/get_num_regimes.pl"; # GetNumRegimesFromRegimeInd
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/strat_utils.pl"; # IsModelScalable , AddL1NormToModel , GetModelL1NormVec, GetTargetL1NormForShortcode
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; #BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/sample_data_utils.pl"; #GetFilteredDays
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl"; # GetPnlSamplesCorrelation
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; # InsertCorrelation
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/genstrat_utils.pl"; # GenerateDaysVec

my $reg_data_daily_dir = $GENREGDATA_DAILY_DIR;
my $timed_data_daily_dir = $GENTIMEDDATA_DAILY_DIR; # in case the calling script wants data that is not in the global timed data directory just yet
my $reg_data_dir = $GENREGDATA_DIR;

my $work_dir_ = "";
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my $use_distributed_ = 0; ## DISTRIBUTED Version by default

my $use_fake_faster_data_ = rand(1) > 0.5 ? 1 : 0;  #using delays randomly, to set usage use param USE_FAKE_FASTER_DATA 

my $USAGE="$0 instructionfilename [use_distributed_version(0/1) = 1] [ work_dir ]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 1 ); }
my $instructionfilename_ = $ARGV[0];

if ( $#ARGV >= 1 ) {
  $use_distributed_ = $ARGV[1];
  if ( $use_distributed_ ne "0" && $use_distributed_ ne "1" ) {
    print $USAGE."\nArgument use_distributed_version has to be 0 or 1\n"; 
    exit ( 1 );
  }
}

if ( $use_distributed_ ) {
  $GENSTRATWORKDIR = $shared_prefix_.$GENSTRATWORKDIR;
  `mkdir -p $GENSTRATWORKDIR` if ( ! -d $GENSTRATWORKDIR );
}

if ( $#ARGV >= 2 ) {
  if ( index($ARGV[2], "/media/shared/ephemeral") != 0 ) {
    print "With Distributed Version, please use shared folder as work_dir\n";
    exit ( 1 );
  }
  $work_dir_ = $ARGV[2]."/".$unique_gsm_id_;
}
else {
  my $instbase_ = basename ( $instructionfilename_ ); 
  chomp ( $instbase_ );
  $work_dir_ = $GENSTRATWORKDIR.$instbase_."/".$unique_gsm_id_; 

  for ( my $i = 0 ; $i < 30 ; $i ++ ) {
    if ( -d $work_dir_ ) {
      $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
      $work_dir_ = $GENSTRATWORKDIR.$instbase_."/".$unique_gsm_id_;
    }
    else {
      last;
    }
  }
}

# start 
my $stime_ =`date`;
my $mail_body_ = " process_start_time : ".$stime_."\n" ;
$mail_body_.= "SPARE_LOCAL_DIR : ".$SPARE_LOCAL."\n" ;
$mail_body_.= "USE_DISTRIBUTED_VERSION : ".$use_distributed_."\n";
$mail_body_.= "WORKING_DIR : ".$work_dir_."\n";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $failure_string_ = "";

my $SAVE_CORR_FILE = 0;
my $SAVE_VAR_CORR_FILE = 0;
my $SAVE_CORRMATRIX_FILE = 0;
my $SAVE_HCP_FILE = 0;

my $RUN_SINGLE_REGRESS_EXEC = 1;
my $RUN_SINGLE_PREDALGO = 1;
my $RUN_SINGLE_DEP_BASED_FILTER = 1;
my $RUN_SINGLE_INDICATOR_LIST_FILE = 1; # choose a file if 1, else, iterate over all files

# given it is one file
# option 0: pick randomly from all
# option 1: pick in round robin fashion using LAST_PICKED_INDICATOR
# option 2: more intelligent pick, the 'expected' best performing indicator file #TODO
my $INDICATOR_PICK_STRATEGY = -1;
my $LAST_PICKED_INDICATOR_INDEX = -1;

my $CREATE_SUBMODELS = 0; #not turning out to be much useful but time consuming, so disabling

my @regress_exec_choices_ = ();
my $use_cross_validation_ = 0;
my $obtain_model_from_cross_validation_ = 0;
my $use_simple_sim_ = 0;
my $granularity_ = 1;
my $number_of_models_ = 0;
my $bad_models_ = 0;
my $cross_validation_cutoff_ = 0.02;
my $max_error_on_test_ = 1;
my $num_folds_ = 5;
my $min_mse_ = 1;
my $shortcode_ = "";
my $name_ = "s";

my @indicator_list_file_vec_ = ( );
my @ilist_filename_vec_ = (); ##store ilist filenames
my %regime_ilist_to_ilist_vec_map_ = ();  ##for regime ilists store their ilists

my $datagen_start_yyyymmdd_ = "";
my $datagen_day_filter_start_yyyymmdd_ = "";
my $datagen_day_filter_max_days_ = 200;
my $datagen_end_yyyymmdd_ = "";
my $datagen_day_inclusion_prob_ = 1.00;
my $validation_day_inclusion_prob_ = 0.50;
my $datagen_start_hhmm_ = "";
my $datagen_end_hhmm_ = "";
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 15;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0; # non economic times periods
my @datagen_day_vec_ = ( ); # in case we want to give a set of days to regress on 
my @datagen_exclude_days_ = ( ); # Do not run datagen on a selected set of days.
my $to_print_on_economic_times_set_already_ = -1;
my $use_insample_days_for_trading_ = 0;

my @target_l1norm_model_vec_ = ();
my $use_norm_stdev_ = 0;
my $add_modelinfo_to_model_ = 0;

my $large_price_move_periods_ = 0; # Large directional price move periods
my $large_price_move_thresh_factor_ = 3; # The thresh factor to use. Default = 3
my $large_price_move_time_period_ = 120; # Time in secs to find large price moves. Default = 120 secs.
my $bad_periods_only_ = 0;
my $periodfilter_samples_only_ = 0;
my $periodfilter_samples_filename_ = "";

my $trading_start_yyyymmdd_ = "";
my $trading_day_filter_start_yyyymmdd_ = "";
my $trading_day_filter_max_days_ = 200;
my $trading_end_yyyymmdd_ = "";
my $trading_start_hhmm_ = "";
my $trading_start_hhmm_set_already_ = -1;
my $trading_end_hhmm_ = "";
my $trading_end_hhmmss_ = "";
my $trading_end_hhmm_set_already_ = -1;
my @trading_days_ = ( ); # Only run sims on a selected set of days.
my @trading_exclude_days_ = ( ); # Do not run sims on a selected set of days.
my $pool_tag_ = "";

my $outsample_trading_start_yyyymmdd_ = "";
my $outsample_trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-1" );

my $validation_start_yyyymmdd_ = "";
my $validation_end_yyyymmdd_ = "";
my $use_validation_median_check_ = 0;
my @validation_days_ = ( );

my $strategyname_ = "";

my $num_files_to_choose_ = "";
my $min_num_files_to_choose_ = 0; # now we have some files ... don't need to select any every time
# filtering strategies
my $min_pnl_per_contract_to_allow_ = -1.0 ;
my $min_volume_to_allow_ = 50 ;
my $volume_per_l1perc_ = 2 ;
my $min_days_traded_perc_ = 0;
my $uts_ = 1 ;
my $max_message_count_ = -1;
my $max_ttc_to_allow_ = 150 ;
my $historical_sort_algo_ = "kCNASqDDAdjPnlSqrtVolume" ;
my $delete_intermediate_files_ = 1;
my $delete_regdata_files_ = 1;
my $mail_address_ = "";
my $author_ = "";
my $use_median_cutoff_ = 1;
my $cut_off_rank_ = 30;
my $use_dynamic_cutoffs_ = 1;
my $traded_ezone_ = "";
my $traded_ezone_pref_ = "";
my $print_only_on_traded_ezone_ = 0;
my $event_days_file_= "";
my $trade_only_on_traded_ezone_ = 0;
my $use_config_sort_algo_ = 0;
my $config_sort_algo_ = $historical_sort_algo_;
my @sampling_shortcodes_ = ();
my @pc_cutoffs_ = ();
my $sampling_shortcodes_str_ = "" ;
my $install_ = 1;
my $ALLOW_PPC_RESET = 1;
my $use_param_list_and_scale_ = 0;
my $regime_model_type_ = "";              
my $use_trade_price_ = "";
my $continuous_regime_indicator_;
my @continuous_regime_indicator_vec_ = ();
my $continuous_regime_cutoff_;
my $use_continuous_regime_model_ = 0 ;
my @regime_config_list_ = () ;
my $use_regime_param_ = "" ;
my $make_regime_params_ = 0 ;
my $this_param_list_name_ = "";
my $use_mid_base_model_in_dat_ = 1 ;
my $regime_indicator_ = "";
my $use_mirrored_data_ = 0;
my @regime_indicator_vec_ = ( );
my $num_regimes_ = -1;
my $use_regime_model_ = 0;
my $build_all_regime_models_ = 0;
my $regime_to_trade_flag_ = 0;
my $regime_to_trade_number_ = -1;

my %model_to_regtype_ = ();
my %regressalgo2duration2param_filevec_ = ();
my $t_random_time_cap_ = 1000;
my $t_random_lower_threshold_ = 0.5;
my $t_random_predduration_to_scale_ = 32;  #preduration to use to calculate previous regdata in case we are generating models based on new regdata
my $use_new_regdata_ = 0;
my $filter_uncertain_values_ = 1;
my $norm_regdata_stdev_indicator_ ="";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
  $use_median_cutoff_ = 0;
}

my $min_external_datagen_day_vec_size_ = 3;

my @dep_based_filter_ = ();
# my $apply_all_dep_filters_ = 0;
my $apply_trade_vol_filter = 0;
my $fsudm_level_ = 0;
my @predduration_to_scale_ = ();
my @predduration_ = ();
my @predconfig_ = ();
my @predalgo_ = ();
my @upper_threshold_ = ();
my @upper_threshold_sd_ratio_ = ();
my @lower_threshold_ = ();
my @time_cap_ = ();
my @regress_exec_ = ();
my %duration_to_param_filevec_ ;
my %duration_to_model_filevec_ ;
my @strategy_filevec_ = ();
my @intermediate_files_ = ();

# training and testing dates using sample data
my $gd_string_ = "";

# ebt support variables
my $pick_stdev_from_model_file;

# temporary
my $local_params_dir_ = $work_dir_."/params_dir";
my $local_ilist_dir_ = $work_dir_."/ilist_dir";
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $avoid_high_sharpe_indep_check_ = 0;
my $avoid_high_sharpe_indep_check_index_filename_ = $work_dir_."/avoid_high_sharpe_check_index_file.txt";
my $main_log_file_handle_ = FileHandle->new;

my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose
my @final_trading_days_ = ();
my $local_results_base_dir = $work_dir_."/local_results_base_dir";

my @final_validation_days_ = ();
my @unique_validation_results_filevec_ = ();
my $local_validation_results_base_dir = $work_dir_."/local_validation_results_base_dir";

my $prod_install_ = 0 ;

my $use_regime_ilists_ = 0 ;

my $filter_zero_inds_ = 0;

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }
if ( ! ( -d $local_ilist_dir_ ) ) { `mkdir -p $local_ilist_dir_`; }
if ( ! ( -d $MODELING_STRATS_DIR ) ) { `mkdir -p $MODELING_STRATS_DIR`; }
if ( ! ( -d $MODELING_MODELS_DIR ) ) { `mkdir -p $MODELING_MODELS_DIR`; }
if ( ! ( -d $MODELING_PARAMS_DIR ) ) { `mkdir -p $MODELING_PARAMS_DIR`; }

print "$main_log_file_\n";
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

# load instruction file
LoadInstructionFile ( );

# Check if the arguments are complete
SanityCheckInstructionFile ( );

my $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $yyyymmdd_` ; chomp ( $min_price_increment_ );

# generate reg data ( and timed data, and remove mean if needed ) and then find weights
RunRegressMakeModelFiles ( );

if ( $author_ eq "dvctrader_a" )
{
  AnalyseModelFile ( ) ;
}

# with %duration_to_model_filevec_ and %duration_to_param_filevec_ make the strategyfiles
MakeStrategyFiles ( );

# find results of the strategies and put them in local_results_base_dir
RunSimulationOnCandidates ( );

# among the candidates choose the best
SummarizeLocalResultsAndChoose ( );

SendMail ( );
# end script
$main_log_file_handle_->close;

exit ( 0 );


sub LoadInstructionFile 
{
  print $main_log_file_handle_ "LoadInstructionFile $instructionfilename_\n";

  open INSTRUCTIONFILEHANDLE, "+< $instructionfilename_ " or PrintStacktraceAndDie ( "$0 Could not open $instructionfilename_\n" );
  my @instruction_lines_ = <INSTRUCTIONFILEHANDLE>; chomp ( @instruction_lines_ );
  close INSTRUCTIONFILEHANDLE;
  print $main_log_file_handle_ join("\n", @instruction_lines_)."\n";

  my $current_instruction_;
  my $datagen_start_hhmm_set_already_ = 0; #signal first time
  my $datagen_end_hhmm_set_already_ = 0; #signal first time
  my @predalgo_choices_ = (); 
  my @dep_based_filter_choices_ = ();
  my @datagen_day_filter_choices_ = ();
  my @datagen_period_filter_choices_ = ( );
  my @trading_day_filter_choices_ = ();
  my @timeout_string_choices_ = ();
  my @max_ttc_to_allow_choices_ = ();
  my @min_volume_to_allow_choices_ = ();

  foreach my $thisline_ ( @instruction_lines_ )
  {
    if ( $thisline_ =~ /^\s*$/ )
    { # empty line hence reset current_instruction_
      undef $current_instruction_;
      next;
    }

    my $comment_idx_ = index( $thisline_, '#' );
    $thisline_ = substr($thisline_, 0, $comment_idx_) if $comment_idx_ >= 0;

    my @instruction_line_words_ = grep { ! /^$/ } split ( /\s+/, $thisline_ );

    if ( $#instruction_line_words_ >= 0 )
    {
      if ( ! defined $current_instruction_ ) 
      { # no instruction set currently being processed
        $current_instruction_ = $instruction_line_words_[0];
      } 
      else 
      { 
        given ( $current_instruction_ ) 
        {
          when ("SHORTCODE") 
          {
            $shortcode_ = $instruction_line_words_[0];
          }
          when ("NAME") 
          {
            $name_ = $instruction_line_words_[0];
          }
          when ("INDICATORLISTFILENAME") 
          {
            if ( ExistsWithSize ( $instruction_line_words_ [ 0 ] ) )
            { # Eliminate non-existant ilists.
              push ( @indicator_list_file_vec_ , $instruction_line_words_ [ 0 ] );
            }
            print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
          }
          when ("AVOID_HIGH_SHARPE_INDEP_CHECK")
          {
            if ( $instruction_line_words_[0] ) {
              $avoid_high_sharpe_indep_check_ = 1;
            }
          }
          when ( "USE_REGIME_ILISTS" )
          {
            $use_regime_ilists_ = $instruction_line_words_[ 0 ];
            print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";

          }
          when ("REGIMEINDICATORNAME")
          {
            $regime_indicator_ = $thisline_;
            push ( @regime_indicator_vec_, $regime_indicator_ );
            print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
          }
          when("CONTINUOUSREGIMEINDICATORNAME")
          {
            $continuous_regime_indicator_ = $thisline_;
            push ( @continuous_regime_indicator_vec_, $continuous_regime_indicator_ );
            print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
          }
          when ("CUTOFF_RANK" )
          {
            $cut_off_rank_ = int ( $instruction_line_words_[0] );
          }
          when ("RUN_SINGLE_INDICATOR_LIST_FILE")
          {
            $RUN_SINGLE_INDICATOR_LIST_FILE = $instruction_line_words_[0];
          }
          when ( "TRADED_EZONE" )
          {
            $traded_ezone_ = $instruction_line_words_[0];
            $traded_ezone_pref_ = $traded_ezone_;
            $traded_ezone_pref_ =~ s/[_0-9]*$//g;
          }
          when ( "PRINT_ONLY_ON_TRADED_EZONE")
          {
            $print_only_on_traded_ezone_ = $instruction_line_words_[0];
            $to_print_on_economic_times_ = 3;
          }
          when ( "TRADE_ONLY_ON_TRADED_EZONE")
          {	
            $trade_only_on_traded_ezone_ = $instruction_line_words_[0];
          }
          when ( "USE_MID_AS_BASE")
          {
            $use_mid_base_model_in_dat_ = $instruction_line_words_[0];
          }
          when ( "USE_CROSS_VALIDATION")
          {
            $use_cross_validation_ = $instruction_line_words_[0];
          }
          when ( "OBTAIN_MODEL_FROM_CROSS_VALIDATION")
          {
            $obtain_model_from_cross_validation_ = $instruction_line_words_[0];
          }
          when ( "NORM_REGDATA_STDEV_INDICATOR")
          {
            $norm_regdata_stdev_indicator_ = $thisline_;
          }            
          when ( "GRANULARITY" )
          {
            $granularity_ = $instruction_line_words_[0];
          }
          when ( "USE_SIMPLE_SIM" )
          {
            $use_simple_sim_ = ( $instruction_line_words_[0] == 1 ) ? 1 : 0;
          }
          when ( "NUM_FOLDS")
          {
            $num_folds_ = $instruction_line_words_[0];
          }
          when ( "MIN_MSE")
          {
            $min_mse_ = $instruction_line_words_[0];
          }
          when ( "CROSS_VALIDATION_CUTOFF")
          {
            $cross_validation_cutoff_ = $instruction_line_words_[0];
          }
          when ("INDICATOR_PICK_STRATEGY")
          {
            if ($instruction_line_words_[0] == 1 || $instruction_line_words_[0] == 0)
            {
              $INDICATOR_PICK_STRATEGY = $instruction_line_words_[0];
            }
          }
          when ("RUN_SINGLE_REGRESS_EXEC")
          {
            $RUN_SINGLE_REGRESS_EXEC = $instruction_line_words_[0];
          }
          when("RUN_SINGLE_DEP_BASED_FILTER")
          {
            $RUN_SINGLE_DEP_BASED_FILTER = $instruction_line_words_[0];
          }
          when ("DATAGEN_TRADING_START_END_YYYYMMDD")
          {
            if ( ( $#instruction_line_words_ >= 3 ) &&
                ( ( ! ( $datagen_start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) ) )
            {
              $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
              $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[1] );
              $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[2] );
              $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[3] );
              $outsample_trading_start_yyyymmdd_ = CalcNextWorkingDateMult ( $trading_end_yyyymmdd_, 1 );
            }			    
          }
          when ("DATAGEN_START_END_YYYYMMDD") 
          {
            if ( ( $#instruction_line_words_ >= 1 ) && 
                ( ( ! ( $datagen_start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) ) )
            {
              $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
              $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[1] );
            }
          }
          when ("DATAGEN_START_YYYYMMDD") 
          {
            if ( ( ! ( $datagen_start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) )
            {
              if ( $instruction_line_words_[0] =~ /FROM_END/ )
              { # e.g. FROM_END 30
                $datagen_start_yyyymmdd_ = $instruction_line_words_[1] ;
              }
              else
              {
                $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
              }
            }
          }
          when ("DATAGEN_END_YYYYMMDD") 
          {
            if ( ( ! ( $datagen_end_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) )
            {
              $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
            }
          }
          when ("DATAGEN_DAYS")
          {
            push ( @datagen_day_vec_, @instruction_line_words_ );
          }
          when ("DATAGEN_DAYS_FILE")
          {
            if ( ExistsWithSize ( $instruction_line_words_ [0] ) )
            {
              my $days_file_ = $instruction_line_words_ [0];
              open DFHANDLE, "< $days_file_" or PrintStacktraceAndDie ( "$0 Could not open $days_file_\n" );
              my @day_vec_ = <DFHANDLE>; chomp ( @day_vec_ );
              push ( @datagen_day_vec_ , @day_vec_ );
              close DFHANDLE;
            }
          }
          when ("DATAGEN_EVENT")
          {
            my $event_name_ = $instruction_line_words_[0] ;
            my $ex_cmd_ = "grep $event_name_ $HOME_DIR/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201?_processed.txt  | awk '{print \$5}' | cut -d'_' -f1" ;
            my @day_vec_ = `$ex_cmd_`; chomp ( @day_vec_ );
            push ( @datagen_day_vec_, @day_vec_ );
          }
          when ("DATAGEN_BAD_DAYS")
          {
            @datagen_day_filter_choices_ = ( "bd" );
          }
          when ("DATAGEN_POOL_BAD_DAYS")
          {
            @datagen_day_filter_choices_ = ( "pbd ".join(" ", @instruction_line_words_) );
          }
          when ("DATAGEN_VERY_BAD_DAYS")
          {
            @datagen_day_filter_choices_ = ( "vbd" );
          }
          when ("DATAGEN_HIGH_VOLUME_DAYS")
          {
            @datagen_day_filter_choices_ = ( "hv" );
          }
          when ("DATAGEN_LOW_VOLUME_DAYS")
          {
            @datagen_day_filter_choices_ = ( "lv" );
          }
          when ("DATAGEN_HIGH_STDEV_DAYS")
          {
            @datagen_day_filter_choices_ = ( "hsd ".join(" ", @instruction_line_words_) );
          }
          when ("DATAGEN_DAY_FILTER")
          {
            push ( @datagen_day_filter_choices_, join(" ", @instruction_line_words_) );
          }
          when ("DATAGEN_DAY_FILTER_START_DATE")
          {
            $datagen_day_filter_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
          }
          when ("DATAGEN_DAY_FILTER_MAX_DAYS")
          {
            $datagen_day_filter_max_days_ = int($instruction_line_words_[0]);
          }
          when ("DATAGEN_DAY_INCLUSION_PROB")
          {
            $datagen_day_inclusion_prob_ = max ( 0.00, min ( 1.00, $instruction_line_words_[0] ) ) ;
          }
          when ("VALIDATION_DAY_INCLUSION_PROB")
          {
            $validation_day_inclusion_prob_ = max ( 0.00, min ( 1.00, $instruction_line_words_[0] ) ) ;
          }
          when ("DATAGEN_EXCLUDE_DAYS")
          {
            push ( @datagen_exclude_days_ , @instruction_line_words_ );
          }
          when ("DATAGEN_EXCLUDE_DAYS_FILE")
          {
            if ( ExistsWithSize ( $instruction_line_words_ [0] ) )
            {
              my $exclude_days_file_ = $instruction_line_words_ [0];
              open DFHANDLE, "< $exclude_days_file_" or PrintStacktraceAndDie ( "$0 Could not open $exclude_days_file_\n" );
              my @exclude_days_ = <DFHANDLE>; chomp ( @exclude_days_ );
              push ( @datagen_exclude_days_ , @exclude_days_ );
              close DFHANDLE;
            }
          }
	  when ("GET_DATA")
          {
            $gd_string_ = join(" ", @instruction_line_words_[0..3]);
          }
          when ("DATAGEN_START_END_HHMM") 
          {
            if ( ( $datagen_start_hhmm_set_already_ != 1 ) ||
                ( rand(1) > 0.50 ) )
            { # with 50 % probability take this one
              $datagen_start_hhmm_ = $instruction_line_words_[0];
              $datagen_start_hhmm_set_already_ = 1;

              $datagen_end_hhmm_ = $instruction_line_words_[1];
              $datagen_end_hhmm_set_already_ = 1;
            }
          }
          when ("DATAGEN_START_HHMM") 
          {
            if ( ( $datagen_start_hhmm_set_already_ != 1 ) ||
                ( rand(1) > 0.50 ) )
            { # with 50 % probability take this one
              $datagen_start_hhmm_ = $instruction_line_words_[0];
              $datagen_start_hhmm_set_already_ = 1;
            }
          }
          when ("DATAGEN_END_HHMM") 
          {
            if ( ( $datagen_end_hhmm_set_already_ != 1 ) ||
                ( rand(1) > 0.50 ) )
            { # with 50 % probability take this one
              $datagen_end_hhmm_ = $instruction_line_words_[0];
              $datagen_end_hhmm_set_already_ = 1;
            }
          }
          when ("DATAGEN_TIMEOUT")
          {
            if ( $#instruction_line_words_ >= 0 )
            { 
              push ( @timeout_string_choices_, $thisline_ );
            }
          }
          when ("USE_CONFIG_SORT_ALGO")
          {
            $use_config_sort_algo_ = $instruction_line_words_[0] ;
          }
          when ("NAME") 
          {
            $name_ = $instruction_line_words_[0];
          }
          when ("INSTALL") 
          {
            $mail_body_.= "-------INSTALL FLAG IS OFF---------\n" ;
            $install_ = $instruction_line_words_[0];
          }
          when ( "PROD_INSTALL" )
          {
            $prod_install_ = $instruction_line_words_[0];
            print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
          }
          when ("ALLOW_PPC_RESET") 
          {
            $ALLOW_PPC_RESET = $instruction_line_words_[0];
          }
          when ("USE_FAKE_FASTER_DATA")
          {
            $use_fake_faster_data_ = $instruction_line_words_[0]>0 ? 1 : 0;
            $name_ = $name_."_F".$use_fake_faster_data_."_" ;
            print $main_log_file_handle_ "USE_FAKE_FASTER_DATA set to $use_fake_faster_data_\n";
          }

          when ("TO_PRINT_ON_ECO")
          {
            if ( ( $to_print_on_economic_times_set_already_ != 1 ) ||
                ( rand(1) > 0.50 ) )
            { # with 50 % probability take this one
              $to_print_on_economic_times_ = $instruction_line_words_[0];
              $to_print_on_economic_times_set_already_ = 1;
              print $main_log_file_handle_ "TO_PRINT_ON_ECO_SET_TO $to_print_on_economic_times_\n"; 
            }
          }

          when ("TO_PRINT_ON_TRADED_ECO")
          {
            if ( $instruction_line_words_[0] == 1 )
            {
              $to_print_on_economic_times_ = 3; # 3 means print only traded economic events
                print $main_log_file_handle_ "TO_PRINT_ON_ECO_SET_TO $to_print_on_economic_times_\n";
            }
          }

          when ( "LARGE_PRICE_MOVE_PERIODS" )
          {
            push ( @datagen_period_filter_choices_, "lpm ".join(" ", @instruction_line_words_) );
          }

          when ( "THIN_BOOK_PERIODS" )
          {
            push ( @datagen_period_filter_choices_, "thinbookp ".join(" ", @instruction_line_words_) );  
          }
          when ( "BAD_PERIODS" )
          {
            push ( @datagen_period_filter_choices_, "badp " );  
          }
          when ( "POOL_BAD_SAMPLES" )
          {
            push ( @datagen_period_filter_choices_, "badsamples ".join(" ", @instruction_line_words_) );  
          }
          when ( "HIGH_LOW_CORR_PERIODS" )
          {
            push ( @datagen_period_filter_choices_, "hlcorr ".join(" ", @instruction_line_words_) );  
          }
          when ( "DATAGEN_PERIOD_FILTER" )
          {
            if ( $instruction_line_words_[0] =~ /^(lpm|hlcorr|thinbookp|badp|samples)$/ )
            {
              push ( @datagen_period_filter_choices_, join(" ", @instruction_line_words_) );
            }
          }

          when ("PREDDURATION") 
          { # expects one duration per line
            push ( @predduration_, $instruction_line_words_[0] );
          }
          when ("PREDCONFIGURATION")
          {
            push ( @predconfig_, $instruction_line_words_[0] );
          }
          when ("UPPER_THRESHOLD")
          {
            push ( @upper_threshold_, $instruction_line_words_[0] );
          }
          when ("UPPER_THRESHOLD_STDEV_RATIO")
          {
            push ( @upper_threshold_sd_ratio_, $instruction_line_words_[0] );
          }
          when ("LOWER_THRESHOLD")
          {
            push ( @lower_threshold_, $instruction_line_words_[0] );
          }
          when ("TIME_CAP")
          {
            push ( @time_cap_, $instruction_line_words_[0] );
          }
          when ("PREDDURATION_TO_SCALE") 
          { # expects one duration per line
            push ( @predduration_to_scale_, $instruction_line_words_[0] );
          }
          when ("PREDALGO") 
          {
            if ( looks_like_number ( $instruction_line_words_[0] ) &&
                ( $#instruction_line_words_ >= 1 ) &&
                ( ( int ( $instruction_line_words_[0] ) >= 1 ) &&
                  ( int ( $instruction_line_words_[0] ) <= 10 ) ) )
            {
              for ( my $rc_idx_ = 0 ; $rc_idx_ <= int ( $instruction_line_words_[0] ) ; $rc_idx_ ++ )
              {
                push ( @predalgo_choices_, $instruction_line_words_[1] ) ;
              }
            }
            else
            {
              push ( @predalgo_choices_, $instruction_line_words_[0] );
            }
          }
          when ("DEP_BASED_FILTERS") 
          {
            push ( @dep_based_filter_choices_, $instruction_line_words_[0] );	
          }
          when ("REGRESS_EXEC") 
          {
            if ( $#instruction_line_words_ >= 0 )
            {
              push ( @regress_exec_choices_, [ @instruction_line_words_ ] );
            }
          }
          when ("NORM_STDEV")
          {
            $use_norm_stdev_ = 1;
            @target_l1norm_model_vec_ = ();
            $add_modelinfo_to_model_ = 0;
            push ( @target_l1norm_model_vec_, -1 );
          }
          when ("TARGET_STDEV_MODEL")
          {
            if ( $use_norm_stdev_ ne 1 )
            {                             
              for ( my $i = 0 ; $i <= $#instruction_line_words_ ; $i ++ )
              {
                push ( @target_l1norm_model_vec_, $instruction_line_words_[$i] ); 
                if ( $instruction_line_words_[$i] > 0 ) { $add_modelinfo_to_model_ = 1; }
              }
            }			
          }
          when ( "ADD_MODELINFO_TO_MODEL" )
          {
            $add_modelinfo_to_model_ = $instruction_line_words_[0];
          }
          when ("DELETE_INTERMEDIATE_FILES") 
          {
            $delete_intermediate_files_ = $instruction_line_words_[0]; 
          }
          when ("DELETE_REGDATA_FILES") 
          {
            $delete_regdata_files_ = $instruction_line_words_[0]; 
          }
          when ("USE_PPC_CUTOFF")
          {
            $use_median_cutoff_ = 0; # will use the old pnl_per_contract cutoffs to select a strategy for installation.
          }
          when ("USE_NEW_REGDATA")
          {
            $use_new_regdata_ = $instruction_line_words_[0];
          }
          when ("FILTER_LOWER_THRESHOLD_VALUES")
          {
            $filter_uncertain_values_ = $instruction_line_words_[0];
          }
          when ( "USE_INSAMPLE_DAYS_FOR_TRADING" )
          {
            $use_insample_days_for_trading_ = 1;
          }
          when ("STRATEGYNAME") 
          {
            $strategyname_ = $instruction_line_words_[0];
# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";

            if ( $strategyname_ eq "EventDirectionalAggressiveTrading" || $strategyname_ eq "EventPriceBasedAggressiveTrading" ){
              $use_dynamic_cutoffs_ = 0 ;
            }
          }
          when ("USE_DYNAMIC_CUTOFFS")
          {
            $use_dynamic_cutoffs_ = $instruction_line_words_[0];
          }
          when ("TRADING_START_END_YYYYMMDD") 
          {
            if ( ( $#instruction_line_words_ >= 1 ) && 
                ( ( ! ( $trading_start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) ) )
            {
              $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
              $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[1] );
              $outsample_trading_start_yyyymmdd_ = CalcNextWorkingDateMult ( $trading_end_yyyymmdd_, 1 );			    
            }
# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
          }
          when ("VALIDATION_START_END_YYYYMMDD") 
          {
            if ( $#instruction_line_words_ >= 1 ) 
            {
              $validation_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
              $validation_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[1] );
              $use_validation_median_check_ = 1;
            }
# print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
          }
          when ("TRADING_START_YYYYMMDD") 
          {
            if ( ( ! $trading_start_yyyymmdd_ ) ||
                ( rand(1) > 0.50 ) )
            {
              $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
            }
          }
          when ("TRADING_END_YYYYMMDD") 
          {
            if ( ( ! $trading_end_yyyymmdd_ ) ||
                ( rand(1) > 0.50 ) )
            {
              $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( @instruction_line_words_ );
              $outsample_trading_start_yyyymmdd_ = CalcNextWorkingDateMult ( $trading_end_yyyymmdd_, 1 );
            }
          }
          when ("TRADING_DAYS")
          {
            push ( @trading_days_ , @instruction_line_words_ );
          }
          when ("TRADING_DAYS_FILE")
          {
            if ( ExistsWithSize ( $instruction_line_words_ [0] ) )
            {
              my $days_file_ = $instruction_line_words_ [0];
              open DFHANDLE, "< $days_file_" or PrintStacktraceAndDie ( "$0 Could not open $days_file_\n" );
              my @day_vec_ = <DFHANDLE>; chomp ( @day_vec_ );
              push ( @trading_days_ , @day_vec_ );
              close DFHANDLE;
            }
          }
          when ("VALIDATION_DAYS")
          {
            push ( @validation_days_ , @instruction_line_words_ );
          }
          when ("VALIDATION_DAYS_FILE")
          {
            if ( ExistsWithSize ( $instruction_line_words_ [0] ) )
            {
              my $days_file_ = $instruction_line_words_ [0];
              open DFHANDLE, "< $days_file_" or PrintStacktraceAndDie ( "$0 Could not open $days_file_\n" );
              my @day_vec_ = <DFHANDLE>; chomp ( @day_vec_ );
              push ( @validation_days_ , @day_vec_ );
              close DFHANDLE;
            }
          }
          when ("TRADING_POOL_BAD_DAYS")
          {
            @trading_day_filter_choices_ = ( "pbd ".join(" ", @instruction_line_words_) );
          }
          when ("TRADING_DAY_FILTER")
          {
            push ( @trading_day_filter_choices_, join(" ", @instruction_line_words_) );
          }
          when ("TRADING_DAY_FILTER_START_DATE")
          {
            $trading_day_filter_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
          }
          when ("TRADING_DAY_FILTER_MAX_DAYS")
          {
            $trading_day_filter_max_days_ = int($instruction_line_words_[0]);
          }
          when ("TRADING_EVENT")
          {
            my $event_name_ = $instruction_line_words_[0] ;
            my $ex_cmd_ = "grep $event_name_ $HOME_DIR/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201?_processed.txt  | awk '{print \$5}' | cut -d'_' -f1" ;
            my @day_vec_ = `$ex_cmd_`; chomp ( @day_vec_ );
            push ( @trading_days_, @day_vec_ );
          }
          when ("TRADING_EXCLUDE_DAYS")
          {
            push ( @trading_exclude_days_ , @instruction_line_words_ );
          }
          when ("TRADING_EXCLUDE_DAYS_FILE")
          {
            if ( ExistsWithSize ( $instruction_line_words_ [0] ) )
            {
              my $exclude_days_file_ = $instruction_line_words_ [0];
              open DFHANDLE, "< $exclude_days_file_" or PrintStacktraceAndDie ( "$0 Could not open $exclude_days_file_\n" );
              my @exclude_days_ = <DFHANDLE>; chomp ( @exclude_days_ );
              push ( @trading_exclude_days_ , @exclude_days_ );
              close DFHANDLE;
            }
          }
          when ( "TRADING_END_HHMMSS" )
          {
            $trading_end_hhmmss_ = $instruction_line_words_[0];
          }
          when ( "TRADING_START_END_HHMM" )
          {
            {
              if ( ( $trading_start_hhmm_set_already_ != 1 ) ||
                  ( rand(1) > 0.50 ) )
              { # with 50 % probability take this one
                $trading_start_hhmm_ = $instruction_line_words_[0];
                $trading_start_hhmm_set_already_ = 1;
                $trading_end_hhmm_ = $instruction_line_words_[1];
                $trading_end_hhmm_set_already_ = 1;			    
              }
            }
          }
          when ("TRADING_START_HHMM") 
          {
            if ( ( $trading_start_hhmm_set_already_ != 1 ) ||
                ( rand(1) > 0.50 ) )
            { # with 50 % probability take this one
              $trading_start_hhmm_ = $instruction_line_words_[0];
              $trading_start_hhmm_set_already_ = 1;
            }
          }
          when ("TRADING_END_HHMM") 
          {
            if ( ( $trading_end_hhmm_set_already_ != 1 ) ||
                ( rand(1) > 0.50 ) )
            { # with 50 % probability take this one
              $trading_end_hhmm_ = $instruction_line_words_[0];
              $trading_end_hhmm_set_already_ = 1;
            }
          }
          when ("POOL_TAG")
          {
            $pool_tag_ = $instruction_line_words_[0];
          }
          when ("USE_PARAM_LIST_AND_SCALE")
          {
            $use_param_list_and_scale_ =  $instruction_line_words_[0];
          }
          when ("PARAMFILENAME") 
          {
            if ( $#instruction_line_words_ >= 1 ) # has to have two words ... preddur paramfile
            {
              my $this_pred_duration_ = $instruction_line_words_[0];
              my $this_orig_param_filename_ = $instruction_line_words_[1];
              $this_pred_duration_ = 0 if ( $this_pred_duration_ eq "ALL" );

              if ( ExistsWithSize ( $this_orig_param_filename_ ) )
              { # file exists and isn't empty sized
                print $main_log_file_handle_ "PermuteParams ( $this_orig_param_filename_, $local_params_dir_ )\n";
                my @this_param_filename_vec_ = PermuteParams ( $this_orig_param_filename_, $local_params_dir_ );
                for ( my $tpfv_index_ = 0 ; $tpfv_index_ <= $#this_param_filename_vec_ ; $tpfv_index_ ++ )
                {
                  my $this_param_filename_ = $this_param_filename_vec_[$tpfv_index_];

                  if ( exists $duration_to_param_filevec_{$this_pred_duration_} ) { # seen duration for a different param_file already
                    my $scalar_ref_ = $duration_to_param_filevec_{$this_pred_duration_} ;
                    push ( @$scalar_ref_, $this_param_filename_ );
                  } else { # seeing this duration for the first time
                    my @param_file_args_ = ();
                    push ( @param_file_args_, $this_param_filename_ );
                    $duration_to_param_filevec_{$this_pred_duration_} = [ @param_file_args_ ] ;
                  }
                }
              }
              else
              {   # PARAMDIR dirname_
# PARAMLIST file_with_params
                if ( $#instruction_line_words_ >= 2 )
                {
                  given ( $instruction_line_words_[1] )
                  {
                    when ( "PARAMDIR" )
                    {
                      print $main_log_file_handle_ "PARAMDIR $instruction_line_words_[2]\n";
                      my @this_param_filename_vec_ = MakeStratVecFromDirSubstr ( $instruction_line_words_[2], "param" );
                      print $main_log_file_handle_ join ( "\n", @this_param_filename_vec_ )."\n";
                      for ( my $tpfv_index_ = 0 ; $tpfv_index_ <= $#this_param_filename_vec_ ; $tpfv_index_ ++ )
                      {
                        my $this_param_filename_ = $this_param_filename_vec_[$tpfv_index_];

                        if ( exists $duration_to_param_filevec_{$this_pred_duration_} ) { # seen duration for a different param_file already
                          my $scalar_ref_ = $duration_to_param_filevec_{$this_pred_duration_} ;
                          push ( @$scalar_ref_, $this_param_filename_ );
                        } else { # seeing this duration for the first time
                          my @param_file_args_ = ();
                          push ( @param_file_args_, $this_param_filename_ );
                          $duration_to_param_filevec_{$this_pred_duration_} = [ @param_file_args_ ] ;
                        }
                      }
                    }
                    when ( "PARAMLIST" )
                    {
                      $this_param_list_name_ = $instruction_line_words_[2] ;
                      print $main_log_file_handle_ "PARAMLIST $this_param_list_name_\n";
                      my @this_param_filename_vec_ = MakeFilenameVecFromList ( $this_param_list_name_ );
                      if ( $use_regime_param_ ) 
                      {
                        my @new_param_list_ = CreateParamListVecFromRegimeParamList ( $this_param_list_name_ , @regime_config_list_ );
                        print $main_log_file_handle_ $this_param_list_name_." ".join(' ', @regime_config_list_)." size: $#new_param_list_ \n";
                        print $main_log_file_handle_ join("\n", @new_param_list_)."\n";
                        @this_param_filename_vec_ = [ $this_param_list_name_ ] ;
                      }
                      print $main_log_file_handle_ join ( "\n", @this_param_filename_vec_ )."\n";
                      for ( my $tpfv_index_ = 0 ; $tpfv_index_ <= $#this_param_filename_vec_ ; $tpfv_index_ ++ )
                      {
                        my $this_param_filename_ = $this_param_filename_vec_[$tpfv_index_];

                        if ( ! -f $this_param_filename_ ) {
                          print $main_log_file_handle_ "No such paramfile: ".$this_param_filename_.".. Skipping it..\n";
                          next;
                        }
                        push ( @{ $duration_to_param_filevec_{$this_pred_duration_} }, $this_param_filename_ );
                      }
                    }
                    when ( "PARAMLIST_REGTYPE" )
                    {
                      if ( $#instruction_line_words_ >= 3 ) {
                        my $this_regress_algo_ = $instruction_line_words_[2] ;
                        $this_param_list_name_ = $instruction_line_words_[3] ;
                        print $main_log_file_handle_ "PARAMLIST_REGTYPE $this_regress_algo_ $this_param_list_name_\n";
                        my @this_param_filename_vec_ = MakeFilenameVecFromList ( $this_param_list_name_ );
                        if ( $use_regime_param_ ) 
                        {
                          my @new_param_list_ = CreateParamListVecFromRegimeParamList ( $this_param_list_name_ , @regime_config_list_ );
                          print $main_log_file_handle_ $this_param_list_name_." ".join(' ', @regime_config_list_)." size: $#new_param_list_ \n";
                          print $main_log_file_handle_ join("\n", @new_param_list_)."\n";
                          @this_param_filename_vec_ = [ $this_param_list_name_ ] ;
                        }
                        print $main_log_file_handle_ join ( "\n", @this_param_filename_vec_ )."\n";
                        for ( my $tpfv_index_ = 0 ; $tpfv_index_ <= $#this_param_filename_vec_ ; $tpfv_index_ ++ )
                        {
                          my $this_param_filename_ = $this_param_filename_vec_[$tpfv_index_];

                          if ( ! -f $this_param_filename_ ) {
                            print $main_log_file_handle_ "No such paramfile: ".$this_param_filename_.".. Skipping it..\n";
                            next;
                          }
                          push ( @{ $regressalgo2duration2param_filevec_{$this_regress_algo_}{$this_pred_duration_} }, $this_param_filename_ );
                          print $main_log_file_handle_ "$this_param_filename_ $this_regress_algo_ $this_pred_duration_\n";
                        }
                      }
                    }
                    default
                    {
                      print $main_log_file_handle_ "PARAMLINE $thisline_ ignored\n";
                    }
                  }
                }
              }
            }
          }
          when ("MIN_NUM_FILES_TO_CHOOSE") 
          {
            $min_num_files_to_choose_ = $instruction_line_words_[0];
          }
          when ("MIN_PNL_PER_CONTRACT") 
          {
            $min_pnl_per_contract_to_allow_ = $instruction_line_words_[0];
          }
          when ("MIN_VOLUME") 
          {
            push ( @min_volume_to_allow_choices_, $instruction_line_words_[0] );
            if ( $instruction_line_words_[0] =~ "PERC" && scalar ( @instruction_line_words_ ) == 2 )
            {
              $volume_per_l1perc_ = 0.1*$instruction_line_words_[1];
            }
          }
          when ("MAX_TTC") 
          {
            push ( @max_ttc_to_allow_choices_, $instruction_line_words_[0] );
          }
          when ("MIN_DAYS_TRADED_PERC")
          {
            $min_days_traded_perc_ = $instruction_line_words_[0];
          }
          when ("NUM_FILES_TO_CHOOSE") 
          {
            $num_files_to_choose_ = $instruction_line_words_[0];
          }
          when ("HISTORICAL_SORT_ALGO")
          {
            $config_sort_algo_ = $instruction_line_words_[0];
            if ( rand(1) > 0.50 )
            { # with 50 % probability take this one
              $historical_sort_algo_ = $instruction_line_words_[0];
              print $main_log_file_handle_ "HISTORICAL_SORT_ALGO set to $historical_sort_algo_\n"; 
            }
          }

          when ("MAIL_ADDRESS") 
          {
            $mail_address_ = $instruction_line_words_[0];
          }

          when ( "AUTHOR" )
          {
            $author_ = $instruction_line_words_ [ 0 ];
          }
          when ( "MAX_MESSAGE_COUNT" )
          {
            $max_message_count_ = $instruction_line_words_ [ 0 ];
          }
          when ("SAMPLING_SHORTCODES_AND_PC_CUTOFFS")
          {
            print $main_log_file_handle_ "SAMPLING SHORTCODES AND PC CUTOFFS \n"; 
            push ( @sampling_shortcodes_ , $instruction_line_words_[0] );
            push ( @pc_cutoffs_ , $instruction_line_words_[1] );
          }
          when ( "USE_TRADE_PRICE" )
          {
            print $main_log_file_handle_ "USE_TRADE_PRICE \n"  ;
            if ( $instruction_line_words_[0] != 0 ) 
            {
              $use_trade_price_ = "true";
            }
          }
          when ("REGIME_PARAM")
          {
            print $main_log_file_handle_ "REGIME_PARAM \n" ;
            print $main_log_file_handle_ join (' ',@instruction_line_words_ )."\n";
            push ( @regime_config_list_, join (' ',@instruction_line_words_ ) );
            $use_regime_param_ = "1";
          }
          when ("MAKE_REGIME_PARAMS")
          {
            $make_regime_params_ = $instruction_line_words_[0];
          }
          when ("FILTER_ZERO_INDS")
          {
            $filter_zero_inds_ = $instruction_line_words_[0];
          }
	  when("REGIMES_TO_TRADE"){
	     $regime_to_trade_flag_ = 1;
	     $regime_to_trade_number_ = $instruction_line_words_[0];
	  }
          when("PICK_STDEV_FROM_MODEL_FILE"){
            $pick_stdev_from_model_file = 1;
          } 
	  default
	  {
	  }
	}
      }
    }
  }
  close ( INSTRUCTIONFILEHANDLE );

  if ( $#min_volume_to_allow_choices_ >= 0 )
  {
# Randomly pick a single min_volume_to_allow_ instead
# of running them all.
    my $t_random_min_volume_to_allow_index_ = int ( rand ( $#min_volume_to_allow_choices_ + 1 ) );
    my $t_random_min_volume_to_allow_ = $min_volume_to_allow_choices_ [ $t_random_min_volume_to_allow_index_ ];

    $min_volume_to_allow_ = $t_random_min_volume_to_allow_ ;
    print $main_log_file_handle_ "volume per l1perc ".$volume_per_l1perc_,"\n"; 
  }
  else
  {
    $min_volume_to_allow_ = 50;
  }

  if ( $#max_ttc_to_allow_choices_ >= 0 )
  {
    my $t_random_max_ttc_to_allow_index_ = int ( rand ( $#max_ttc_to_allow_choices_ + 1 ) );
    my $t_random_max_ttc_to_allow_ = $max_ttc_to_allow_choices_ [ $t_random_max_ttc_to_allow_index_ ];

    $max_ttc_to_allow_ = $t_random_max_ttc_to_allow_ ;
  }
  else
  {
    $max_ttc_to_allow_ = 1500;
  }

  if ( $#timeout_string_choices_ >= 0 )
  {
    my $t_random_timeout_index_ = int ( rand ( $#timeout_string_choices_ + 1 ) );
    my $t_random_timeout_string_ = $timeout_string_choices_ [ $t_random_timeout_index_ ];

    ProcessDatagenTimeoutString ( $t_random_timeout_string_ );
  }

  if ( rand(1) > 0.5 )
  { # with probability 0.5 use input historical_sort_algo_
    if ( -e $HOME_DIR."/modelling/pick_strats_config/US/${shortcode_}.US.txt" ) 
    {
      my @t_sort_algo_options_ = `$SCRIPTS_DIR/get_config_field.pl ~/modelling/pick_strats_config/US/$shortcode_.US.txt SORT_ALGO`;
      chomp ( @t_sort_algo_options_ );
      my @sort_algo_options_ = grep { ! /^#/ } @t_sort_algo_options_ ;
      if ( $#sort_algo_options_ >= 0 )
      {
        my $t_random_sortalgo_index_ = int ( rand ( $#sort_algo_options_ + 1 ) );
        my $pick_strats_sort_algo_ = GetSortAlgo( $sort_algo_options_[ $t_random_sortalgo_index_ ] );
        if ( $pick_strats_sort_algo_ ne "kCNAMAX" )
        { 
          $historical_sort_algo_ = $pick_strats_sort_algo_;
        }
      }
      print $main_log_file_handle_ "using pick_strat_sort_algo_ : ".$historical_sort_algo_."\n";
    }
  }

  if($use_config_sort_algo_ == 1)
  {
    $historical_sort_algo_ = $config_sort_algo_;
    print $main_log_file_handle_ "using config_sort_algo_ : ".$historical_sort_algo_."\n";
  }
  if ( $#continuous_regime_indicator_vec_ >= 0 )
  {
    my $t_random_continuos_regime_indicator_index_ = int ( rand ( $#continuous_regime_indicator_vec_ + 1 ) );
    $continuous_regime_indicator_ = $continuous_regime_indicator_vec_[ $t_random_continuos_regime_indicator_index_ ];
    print $main_log_file_handle_ "Continuous Regime Indicator is ", $continuous_regime_indicator_, "\n";
    my @continuous_var_ = split ' CREGIMECUT ', $continuous_regime_indicator_;
    $continuous_regime_indicator_ = $continuous_var_[ 0 ];
    $continuous_regime_cutoff_ = $continuous_var_[ 1 ];
  } else{
    print $main_log_file_handle_ "No continuous regime indicators mentioned \n";
  }

  if ( $#regime_indicator_vec_ >= 0 )
  {
    $use_regime_model_ = 1; 
    $name_=$name_.".regm" if ( ! $use_regime_ilists_ );

    my $t_random_regime_indicator_index_ = int ( rand ( $#regime_indicator_vec_ + 1 ) );
    $regime_indicator_ = $regime_indicator_vec_ [ $t_random_regime_indicator_index_ ];
    print $main_log_file_handle_ "RegimeIndicator is ",$regime_indicator_,"\n";
    my @regime_words_ = split ' ',$regime_indicator_;
    $num_regimes_ = GetNumRegimesFromRegimeInd( $regime_words_[ 0 ], $#regime_words_+1, $regime_indicator_ );        
    print $main_log_file_handle_ "Num Regimes is ",$num_regimes_,"\n";
    if ( $regime_words_[ 0 ] eq "TrendSwitchRegime" )
    { 
      $use_mirrored_data_ = 1;
      print $main_log_file_handle_ "Mirrored data flag is set to 1\n";
    }
  } else {
    $use_regime_model_ = 0; 
    $use_regime_ilists_ = 0;
    $make_regime_params_ = 0;
    print $main_log_file_handle_ "No Regime Indicators mentioned..\n";
  }

  ChooseIlist ( );

  GenerateDaysVectors ( \@datagen_day_filter_choices_, \@trading_day_filter_choices_ );
  
  DatagenPeriodsProcess ( \@datagen_period_filter_choices_ );

  if ( $#predalgo_choices_ >= 0 ) {
    if ( $RUN_SINGLE_PREDALGO ) {
      my $t_random_predalgo_index_ = int ( rand ( $#predalgo_choices_ + 1 ) );
      my $t_random_predalgo_ = $predalgo_choices_ [ $t_random_predalgo_index_ ];

      @predalgo_ = ( $t_random_predalgo_ );
    }
    else
    {
      @predalgo_ = @predalgo_choices_;
    }
  }
  @predalgo_ = ( "na_e5" ) if $#predalgo_ < 0;
  print $main_log_file_handle_ "Predalgos: ".join(' ',@predalgo_)."\n";

  if ( $#lower_threshold_ >= 0 )
  {
    my $t_random_lower_threshold_index_ = int ( rand ( $#lower_threshold_ + 1 ) );
    $t_random_lower_threshold_ = $lower_threshold_ [ $t_random_lower_threshold_index_ ];
  }

  if ( $#time_cap_ >= 0 )
  {
    my $t_random_time_cap_index_ = int ( rand ( $#time_cap_ + 1 ) );
    $t_random_time_cap_ = $time_cap_ [ $t_random_time_cap_index_ ];
  }

  if ( $#predduration_to_scale_ >= 0 )
  {
    my $t_random_predduration_to_scale_index_ = int ( rand ( $#predduration_to_scale_ + 1 ) );
    $t_random_predduration_to_scale_ = $predduration_to_scale_ [ $t_random_predduration_to_scale_index_ ];
  }

  if ( $#dep_based_filter_choices_ >= 0 )
  {
    if ( $RUN_SINGLE_DEP_BASED_FILTER )
    {
      my $t_random_dep_based_filter_index_ = int ( rand ( $#dep_based_filter_choices_ + 1 ) );
      my $t_random_dep_based_filter_ = $dep_based_filter_choices_ [ $t_random_dep_based_filter_index_ ];

      @dep_based_filter_ = ( $t_random_dep_based_filter_ );
    }
    else
    {
      @dep_based_filter_ = @dep_based_filter_choices_ ;
    }
  }
  @dep_based_filter_ = ( "fsg1" ) if $#dep_based_filter_ < 0;

  if ( $#regress_exec_choices_ >= 0 )
  {
    if ( $use_regime_model_ ) 
    {
      $RUN_SINGLE_REGRESS_EXEC = 1;
      $obtain_model_from_cross_validation_ = 0;
      if ( $#target_l1norm_model_vec_ > 0 )
      {
        my $t_target_l1norm_model_ = $target_l1norm_model_vec_ [ int ( rand ( $#target_l1norm_model_vec_ + 1 ) ) ];
        @target_l1norm_model_vec_ = ( $t_target_l1norm_model_ );  #using only one target_stdev in case of regime
      }
      
      @regress_exec_choices_ = grep { ! ( $$_[0] =~ /MULTLR|LOGIT|RANDOMFOREST|TREEBOOSTING|CRR/ ) } @regress_exec_choices_;
    }
    if ( $#continuous_regime_indicator_vec_ < 0 )
    {
      @regress_exec_choices_ = grep { ! ( $$_[0] =~ /CRR/ ) } @regress_exec_choices_;
    }

    if ( $RUN_SINGLE_REGRESS_EXEC && ( not $obtain_model_from_cross_validation_ ) )
    {
# Randomly pick a single regress-exec instead
# of running them all.
      if ( $#regress_exec_choices_ >= 0 )
      {
        my $t_random_regress_exec_index_ = int ( rand ( $#regress_exec_choices_ + 1 ) );
        my $t_random_regress_exec_ = $regress_exec_choices_ [ $t_random_regress_exec_index_ ];
        print $main_log_file_handle_ " RegressExec = ".join(" ", @{ $t_random_regress_exec_ } )."\n";
        @regress_exec_ = ( $t_random_regress_exec_ );
      }
    }
    else
    {
      @regress_exec_ = @regress_exec_choices_ ;
    }
  }

  if ( $use_regime_model_ ) 
  {
    $regime_model_type_ =    ( $regress_exec_[ 0 ][ 0 ] =~ /KALMAN_SIGLR/ ) ? "ONLINESELECTIVESIGLR" : (
                             ( $regress_exec_[ 0 ][ 0 ] =~ /SIGLR/ ) ? "SELECTIVESIGLR" : (
                              ( $regress_exec_[ 0 ][ 0 ] =~ /KALMAN/ ) ? "ONLINESELECTIVENEW" 
                              : "SELECTIVENEW" ) ) ;
    print $main_log_file_handle_ "ModelType is $regime_model_type_ and RegressExec is $regress_exec_[ 0 ][ 0 ]\n";
  }


  if ( $#regress_exec_ < 0 )
  {
    my $regress_exec_text_ = "FSLR 0.01 0 0 0.7 18";
    my @regress_exec_words_ = split ( ' ', $regress_exec_text_ );
    @regress_exec_ = ( [ @regress_exec_words_ ] );
  }

  if ( ( grep { $$_[0] eq "MULTLR" } @regress_exec_ )
     || ( grep { $_ eq "na_s4" || $_ eq "na_m4" } @predalgo_ ) )
  {
    @regress_exec_ = grep { $$_[0] eq "MULTLR" } @regress_exec_;
    my $default_multlr_ = "MULTLR 4 0.01 0 0.85 18 N";
    @regress_exec_ = ( [ split ( ' ', $default_multlr_ ) ] ) if $#regress_exec_ < 0;
    @predalgo_ = grep { $_ eq "na_s4" || $_ eq "na_m4" } @predalgo_;
    @predalgo_ = ( "na_s4" ) if $#predalgo_ < 0;
  }
  if ( ( grep { $$_[0] eq "MULTLR2" } @regress_exec_ )
      || ( grep { $_ eq "na_mult" } @predalgo_ ) )
  {
    @regress_exec_ = grep { $$_[0] eq "MULTLR2" } @regress_exec_;
    my $default_multlr_ = "MULTLR2 4 0.01 0 0.85 18";
    @regress_exec_ = ( [ split ( ' ', $default_multlr_ ) ] ) if $#regress_exec_ < 0;
    @predalgo_ = grep { $_ eq "na_mult" } @predalgo_;
    @predalgo_ = ( "na_mult" ) if $#predalgo_ < 0;
  }

  if ( $use_regime_model_ )
  {
#deleting because these have different type of files
    $delete_intermediate_files_ = "1"; 
#this reg data will be diff from other
    $reg_data_daily_dir = $SPARE_LOCAL.$USER."/DailyRegDataDir/";
    $reg_data_dir = $SPARE_LOCAL.$USER."/RegDataDir/";
  }

  if( $use_new_regdata_ )
  {
    if ( $#upper_threshold_sd_ratio_ >= 0 ) {
      my $min_price_incr_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $yyyymmdd_ 2>/dev/null`; chomp ( $min_price_incr_ );
      my $sd_price_cmd_ = "$SCRIPTS_DIR/get_avg_samples.pl $shortcode_ $yyyymmdd_ 30 $datagen_start_hhmm_ $datagen_end_hhmm_ 0 STDEV 2>/dev/null | grep Avg | awk \'{print \$NF;}\'";
      my $sd_price_ = `$sd_price_cmd_`; chomp ( $sd_price_ );
      if ( $sd_price_ ne "" && $min_price_incr_ ne "" && $min_price_incr_ > 0 ) {
        $sd_price_ = $sd_price_ / $min_price_incr_;
        foreach my $upper_threshold_ratio_ ( @upper_threshold_sd_ratio_ ) {
          push ( @upper_threshold_, $upper_threshold_ratio_ * $sd_price_ );
        }
      }
    }
    #Change PredDuration values with upper threshold(tick) values
    PredDurationToTickValues( );
  }

  foreach my $this_pred_duration_ ( @predduration_ )
  { # for each pred_duration
    if ( ! ( exists $duration_to_param_filevec_{$this_pred_duration_} ) )
    {
      my $first_dur_param_key = "0";
      if ( ! ( exists $duration_to_param_filevec_{$first_dur_param_key} ) )
      {
        $first_dur_param_key = (keys %duration_to_param_filevec_)[0];
      }
      print $main_log_file_handle_ "PARAMFILES for $this_pred_duration_ does not exist.\n";
      print $main_log_file_handle_ "Copying from PARAMFILENAMES of $first_dur_param_key:\n";
      $duration_to_param_filevec_{$this_pred_duration_} = $duration_to_param_filevec_{$first_dur_param_key};
    }
  }

  foreach my $this_regress_algo_ ( keys %regressalgo2duration2param_filevec_ )
  {
    my $this_dur2param_filevec_ = \%{ $regressalgo2duration2param_filevec_{$this_regress_algo_} };
    foreach my $this_pred_duration_ ( @predduration_ )
    { # for each pred_duration
      if ( ! ( exists $$this_dur2param_filevec_{$this_pred_duration_} ) )
      {
        my $first_dur_param_key = "0";
        if ( ! ( exists $$this_dur2param_filevec_{$first_dur_param_key} ) )
        {
          $first_dur_param_key = (keys %$this_dur2param_filevec_)[0];
        }
        print $main_log_file_handle_ "PARAMFILES for $this_pred_duration_ for $this_regress_algo_ does not exist.\n";
        print $main_log_file_handle_ "Copying from PARAMFILENAMES of $first_dur_param_key\n";
        $$this_dur2param_filevec_{$this_pred_duration_} = $$this_dur2param_filevec_{$first_dur_param_key};
      }
    }
  }

  PrintRegModelParams ( );
}

sub GenerateDaysVectors
{
  my ( $datagen_day_filter_ref_, $trading_day_filter_ref_ ) = @_;

  my @t1_min_train_date_ ;
  my @t1_max_train_date_;
  
  if ( $traded_ezone_ ne "" ) {
    my $t_start_yyyymmdd_ = CalcPrevWorkingDateMult( $yyyymmdd_, 600 );
    my $ex_cmd_ = "$SCRIPTS_DIR/get_dates_for_traded_ezone.pl $traded_ezone_ $t_start_yyyymmdd_ $yyyymmdd_";
    my @event_day_vec_ = `$ex_cmd_ 2>/dev/null`; chomp ( @event_day_vec_ );

    if ( $#datagen_day_vec_ < 0 ) {
      @datagen_day_vec_ = grep { $_ >= $datagen_start_yyyymmdd_ && $_ <= $datagen_end_yyyymmdd_ } @event_day_vec_;
    }

    if ( $#trading_days_ < 0 ) {
      @trading_days_ = grep { $_ >= $trading_start_yyyymmdd_ && $_ <= $trading_end_yyyymmdd_ } @event_day_vec_;
    }

    if ( $use_validation_median_check_ ) {
      if ( $#validation_days_ < 0 ) {
        @validation_days_ = grep { $_ >= $validation_start_yyyymmdd_ && $_ <= $validation_end_yyyymmdd_ } @event_day_vec_;
      }
    }

    $event_days_file_ = GetCSTempFileName ( $work_dir_."/dateslist" );
    open CSTF, "> $event_days_file_" or PrintStacktraceAndDie("Could not open $event_days_file_ for writing \n");
    print CSTF $_."\n" foreach @event_day_vec_;
    close CSTF;
  }

# Datagen Days
  my $filter_name_ = GenerateDaysVec ( $shortcode_, $datagen_start_yyyymmdd_, $datagen_end_yyyymmdd_, $datagen_start_hhmm_, $datagen_end_hhmm_, \@datagen_day_vec_, $datagen_day_filter_ref_, $datagen_day_filter_start_yyyymmdd_, $datagen_day_filter_max_days_, \@datagen_exclude_days_, "datagen", $main_log_file_handle_, $gd_string_ );
  $name_ = $filter_name_."_".$name_;
  push(@t1_min_train_date_, min @datagen_day_vec_);
  push(@t1_max_train_date_, max @datagen_day_vec_); 

# Trading Days
  GenerateDaysVec ( $shortcode_, $trading_start_yyyymmdd_, $trading_end_yyyymmdd_, $trading_start_hhmm_, $trading_end_hhmm_, \@trading_days_, $trading_day_filter_ref_, $trading_day_filter_start_yyyymmdd_, $trading_day_filter_max_days_, \@trading_exclude_days_, "trading", $main_log_file_handle_, "" );

  if ( $use_insample_days_for_trading_ == 0 ) {
    my %trading_map_ = map { $_ => 1 } @trading_days_;
    my @common_days_for_datagen_ = grep { defined $trading_map_{ $_ } && rand(1) <= $datagen_day_inclusion_prob_ } @datagen_day_vec_;
    delete $trading_map_{ $_ } foreach @common_days_for_datagen_;

    @datagen_day_vec_ = grep { ! defined $trading_map_{ $_ } } @datagen_day_vec_;
    @trading_days_ = keys %trading_map_;
  }
  
  print $main_log_file_handle_ "datagen dates: ".join(" ", @datagen_day_vec_)."\n";
  if ( $#datagen_day_vec_ >= $min_external_datagen_day_vec_size_ ) {
    $datagen_start_yyyymmdd_ = $datagen_day_vec_[0];
    $datagen_end_yyyymmdd_ = $datagen_day_vec_[$#datagen_day_vec_];
  } else {
    print $main_log_file_handle_ "Low count of datagen_day_vec ( $#datagen_day_vec_ ), Exiting..\n";
    @datagen_day_vec_ = ();
    exit ( 1 ) ;
  }
  push(@t1_min_train_date_, min @trading_days_);
  push(@t1_max_train_date_, max @trading_days_);

  if ( $use_validation_median_check_ ) {
# Validation Days
    GenerateDaysVec ( $shortcode_, $validation_start_yyyymmdd_, $validation_end_yyyymmdd_, $trading_start_hhmm_, $trading_end_hhmm_, \@validation_days_, [], undef, undef, \@trading_exclude_days_, "validation", $main_log_file_handle_, "" );

    my %trading_map_ = map { $_ => 1 } @trading_days_;
    my @common_days_for_validation_ = grep { defined $trading_map_{ $_ } && rand(1) <= $validation_day_inclusion_prob_ } @validation_days_;
    delete $trading_map_{ $_ } foreach @common_days_for_validation_;

    @validation_days_ = grep { ! defined $trading_map_{ $_ } && ! FindItemFromVec ( $_, @datagen_day_vec_ ) } @validation_days_;
    @trading_days_ = keys %trading_map_;

    push(@t1_min_train_date_, min @validation_days_); 
    push(@t1_max_train_date_, max @validation_days_); 
  }

  if ( $#trading_days_ < 0 ) {
    print $main_log_file_handle_ "Error: Trading Days Empty.. Exiting\n";
    PrintStacktraceAndDie("Error: Trading Days Empty.. Exiting");
  }
  print $main_log_file_handle_ "trading dates: ".join(" ", @trading_days_)."\n";

  if ( $use_validation_median_check_ ) {
    if ( $#validation_days_ < 0 ) {
      print $main_log_file_handle_ "Error: Validation Days Empty.. Exiting\n";
      PrintStacktraceAndDie("Error: Validation Days Empty.. Exiting");
    }
    print $main_log_file_handle_ "validation dates: ".join(" ", @validation_days_)."\n";
  }

  #datagen_day_vec_, validation_days_, trading_days_
  #
  my $t_min_train_date_ = min @t1_min_train_date_;
  my $t_max_train_date_ = max @t1_max_train_date_;
  my $t_cmd_ = "python $PYSCRIPTS_DIR/generate_dates.py -m 2 -sd ".$t_min_train_date_." -ed ".$t_max_train_date_;
  my $t_gtraindates_ = `$t_cmd_`;
  print $main_log_file_handle_ "FilterDates: ".$t_cmd_." ".$t_gtraindates_."\n";
  my %gtdates_ = map{$_ => 1} (split(" ", $t_gtraindates_));
  print("\n");
  print(join(" ", @datagen_day_vec_)."\n");
  print(join(" ", @validation_days_)."\n");
  print(join(" ", @trading_days_)."\n");
  @trading_days_ = grep($gtdates_{$_}, @trading_days_);
  @datagen_day_vec_ = grep($gtdates_{$_}, @datagen_day_vec_);
  @validation_days_ = grep($gtdates_{$_}, @validation_days_);
  print(join(" ", @datagen_day_vec_)."\n");
  print(join(" ", @validation_days_)."\n");
  print(join(" ", @trading_days_)."\n");

}

sub PrintRegModelParams
{
  my $regmodel_args_logstr_ = "";

  $regmodel_args_logstr_ .= "Ilist: ".join(" ", @indicator_list_file_vec_)."\n";
  $regmodel_args_logstr_ .= "PredAlgo: ".join(" ", @predalgo_)."\n";
  $regmodel_args_logstr_ .= "PredDur: ".join(" ", @predduration_)."\n";
  $regmodel_args_logstr_ .= "Filters: ".join(" ", @dep_based_filter_)."\n";
  $regmodel_args_logstr_ .= "RegressExecs: ".join(", ", map { join(" ", @$_) } @regress_exec_)."\n";
  $regmodel_args_logstr_ .= "Use_Regime_Model: ".$use_regime_model_."\n";
  $regmodel_args_logstr_ .= "Use_New_Regdata: ".$use_new_regdata_."\n";

  print $main_log_file_handle_ "\n$regmodel_args_logstr_\n";

  $mail_body_ .= "\n$regmodel_args_logstr_\n";
}

sub ProcessDatagenTimeoutString 
{
  my $random_timeout_string_ = shift;
  my $timeout_set_already_ = 0;
  my @timeout_string_words_ = split ( ' ', $random_timeout_string_ );
  if ( $timeout_string_words_[0] eq "TIM1" )
  {
    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 1000, 0, 0 );
    $timeout_set_already_ = 1;
  }
  elsif ( index ( $timeout_string_words_[0] , "EVT" ) != -1 ) 
  {
    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 4000, 10, 0 );
    if ( $shortcode_ )
    {
      $datagen_l1events_timeout_ = max ( 1 ,int ( ( GetAvgEventCountPerSecForShortcode ( $shortcode_, $datagen_start_hhmm_, $datagen_end_hhmm_, $datagen_end_yyyymmdd_ ) / 2.0 ) + 0.5 ) ) ;
      if ( $datagen_l1events_timeout_ < 2 )
      { # very slow products
        $datagen_msecs_timeout_ = max ( $datagen_msecs_timeout_, int ( 1000 / $datagen_l1events_timeout_) ) ;
        $datagen_msecs_timeout_ = min ( 60000, $datagen_msecs_timeout_ );
      }
    }
    if ( $timeout_string_words_[0] eq "EVT2" )
    {
# msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
      $datagen_l1events_timeout_ = "c1" ;  
      if ( scalar ( @sampling_shortcodes_ ) > 0 )
      {
        print $main_log_file_handle_ "sampling codes specificed_\n";
        $sampling_shortcodes_str_ = "ADD_SAMPLING_CODES";
        for ( my $i = 0 ; $i < scalar ( @sampling_shortcodes_ ) ; $i ++ )
        {
          $sampling_shortcodes_str_ .= " ".$sampling_shortcodes_[$i];
        }
        $sampling_shortcodes_str_ .= " -1";
      }
    }
    if ( $timeout_string_words_[0] eq "EVT3" )
    {
# msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
      $datagen_l1events_timeout_ = "c2" ;
      if ( scalar ( @sampling_shortcodes_ ) > 0 )
      {
        print $main_log_file_handle_ "sampling codes specificed_\n";
        $sampling_shortcodes_str_ = "ADD_SAMPLING_CODES";
        for ( my $i = 0 ; $i < scalar ( @sampling_shortcodes_ ) ; $i ++ )
        {
          $sampling_shortcodes_str_ .= " ".$sampling_shortcodes_[$i];
        }
        $sampling_shortcodes_str_ .= " -1";
      }
    }
    if ( $timeout_string_words_[0] eq "EVT4" )
    {
# msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
      $datagen_l1events_timeout_ = "c3" ;
      if ( scalar ( @sampling_shortcodes_ ) > 0 )
      {
        print $main_log_file_handle_ "sampling codes specificed_\n";
        $sampling_shortcodes_str_ = "ADD_SAMPLING_CODES";
        for ( my $i = 0 ; $i < scalar ( @sampling_shortcodes_ ) ; $i ++ )
        {
          $sampling_shortcodes_str_ .= " ".$sampling_shortcodes_[$i]." ".$pc_cutoffs_[$i];
        }
        $sampling_shortcodes_str_ .= " -1";
      }
    }
    $timeout_set_already_ = 1;
  }
  elsif ( $timeout_string_words_[0] eq "TRD1" )
  {
    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 8000, 20, 1 );
    if ( $shortcode_ )
    {
      $datagen_l1events_timeout_ = max ( 1 ,int ( ( GetAvgEventCountPerSecForShortcode ( $shortcode_, $datagen_start_hhmm_, $datagen_end_hhmm_, $datagen_end_yyyymmdd_ ) * 2.0 ) + 0.5) ) ;
      $datagen_num_trades_timeout_ = max ( 1, int ( GetAvgTradeCountPerSecForShortcode ( $shortcode_ ) ) ) ;

      if ( $datagen_l1events_timeout_ < 2 )
      { # very slow products
        $datagen_msecs_timeout_ = max ( $datagen_msecs_timeout_, int(1000/$datagen_l1events_timeout_) );
        $datagen_msecs_timeout_ = min ( 60000, $datagen_msecs_timeout_ );
      }
    }
    $timeout_set_already_ = 1;
  }
  if ( $#timeout_string_words_ >= 2 )
  {
    if ( $timeout_set_already_ == 0 )
    {
      $datagen_msecs_timeout_ = $timeout_string_words_[0];
      $datagen_l1events_timeout_ = $timeout_string_words_[1];
      $datagen_num_trades_timeout_ = $timeout_string_words_[2];
    }
  }
}

sub ChooseIlist
{ # Pick a random ilist file.
  my $t_ilist_index_ = 0;
  my $t_ilist_ = "";

  if ( $RUN_SINGLE_INDICATOR_LIST_FILE )
  {
    if ( $INDICATOR_PICK_STRATEGY == 1 && $LAST_PICKED_INDICATOR_INDEX > 0 && $LAST_PICKED_INDICATOR_INDEX <= scalar(@indicator_list_file_vec_))
    {
      $t_ilist_index_ = int ( $LAST_PICKED_INDICATOR_INDEX - 1);
    }
    else
    {
      $t_ilist_index_ = int ( rand ( $#indicator_list_file_vec_ + 1 ) );
    }
    my $t_random_indicator_list_file_ = $indicator_list_file_vec_ [ $t_ilist_index_ ];
    @indicator_list_file_vec_ = ( );
    push ( @indicator_list_file_vec_ , $t_random_indicator_list_file_ );
#print @indicator_list_file_vec_;
  }

  for ( my $t_ilist_index_ = 0; $t_ilist_index_ <= $#indicator_list_file_vec_; $t_ilist_index_ ++ )
  {# for every indicator list file

    $t_ilist_ = $indicator_list_file_vec_ [ $t_ilist_index_ ]; chomp ( $t_ilist_ );

    if ( length ( $t_ilist_ ) < 5 || ! ExistsWithSize ( $t_ilist_ ) )
    {
      print $main_log_file_handle_ "Weird indicatorlistfilename = $t_ilist_ \n";
      PrintStacktraceAndDie ( "Weird indicatorlistfilename = $t_ilist_ \n" );
    }

    my $indicator_list_filename_ = FileNameInNewDir ( $t_ilist_, $local_ilist_dir_ );
    my $exec_cmd = "cp $t_ilist_ $indicator_list_filename_";
    print $main_log_file_handle_ "$exec_cmd\n";
    `$exec_cmd`;

    push ( @ilist_filename_vec_ , $indicator_list_filename_ );

    if ( $use_regime_ilists_ ) {
      open ILISTHANDLE, "< $indicator_list_filename_";
      my @ilist_vec_ = <ILISTHANDLE>; chomp ( @ilist_vec_ );
      close ILISTHANDLE;

      if ( scalar ( @ilist_vec_ ) != $num_regimes_ ) {
        PrintStacktraceAndDie ( "Exiting, no of ilists specified in the set $indicator_list_filename_ doesnt match with number of regimes : $num_regimes_\n" ) ;
      }

      print $main_log_file_handle_ "REGIME_ILIST: All Ilists in the list file: ".join(" ", @ilist_vec_)."\n";
      $regime_ilist_to_ilist_vec_map_{ $indicator_list_filename_ } = \@ilist_vec_;
    }
  }
}

sub DatagenPeriodsProcess 
{
  my $datagen_period_filter_choices_ref_ = shift;
  my @datagen_period_filter_choices_ = @$datagen_period_filter_choices_ref_;
  my $datagen_period_filter_ = "";

  if ( $#datagen_period_filter_choices_ >= 0 && 
      rand ( 1.0 ) > ( 1 / ( 2 + $#datagen_period_filter_choices_ ) ) ) #giving eq_prob to normal case with no filter
  {
    my @uniq_datagen_period_filter_choices_ = do { my %seen; grep { !$seen{$_}++ } @datagen_period_filter_choices_ };
    print $main_log_file_handle_ "Total datagen period filter choices: ",join( ' ', @uniq_datagen_period_filter_choices_ ),"\n";
    my $t_random_datagen_period_filter_index_ = int( rand ( $#uniq_datagen_period_filter_choices_ + 1) );
      
    $datagen_period_filter_ = $uniq_datagen_period_filter_choices_ [ $t_random_datagen_period_filter_index_ ];  
    print $main_log_file_handle_ "From the DATAGEN PERIOD CHOICES provided, chosen: $datagen_period_filter_ \n";
  }
  else
  {  
    print $main_log_file_handle_ "DATAGEN PERIOD CHOICES skipped due to random check or no available choices\n"; 
  }
  my @datagen_period_filter_words_ = split(/\s+/, $datagen_period_filter_);

  if ( $#datagen_period_filter_words_ >= 0 )
  {
    my $filter_tag_ = shift @datagen_period_filter_words_;

    given ( $filter_tag_ )
    {
      when ( "lpm" )
      {
        $large_price_move_periods_ = 1;

        $large_price_move_time_period_ = shift @datagen_period_filter_words_ || 120;
        $large_price_move_thresh_factor_ = shift @datagen_period_filter_words_ || 3;

        $name_ = "lpm.".$large_price_move_time_period_.".".$large_price_move_thresh_factor_.".".$name_; # change name to indicate lpm
      }
      when ( "badp" )
      {
        $bad_periods_only_ = 1;
        $name_ = "badp".$name_; # Change name to indicate bad period strat.
      }
      when ( /^(hlcorr|thinbookp|samples)$/ )
      {        $periodfilter_samples_only_ = 1;

        my $frac_samples_ = 0.3;
        my $highlow_ = "HIGH";
        my $sampletag_ = "";
        my @tag_aux_ = ();

        if ( $filter_tag_ =~ /^samples$/ ) {
          $sampletag_ = shift @datagen_period_filter_words_;
          if ( $sampletag_ eq "CORR" ) {
            my $corr_indep_ = shift @datagen_period_filter_words_;
            push ( @tag_aux_, $corr_indep_ );
          }
          $frac_samples_ = shift @datagen_period_filter_words_ || 0.3;
          $highlow_ = shift @datagen_period_filter_words_ || "HIGH";
        }
        elsif ( $filter_tag_ eq "hlcorr" ) {
          $highlow_ = shift @datagen_period_filter_words_ || "HIGH";
          my $corr_indep_ = shift @datagen_period_filter_words_;
          if ( ! defined $corr_indep_ ) {
            print $main_log_file_handle_ "ERROR: DATAGEN_PERIOD_FILTER hlcorr: no indep mentioned.. Ignoring DATAGEN_PERIOD_FILTER\n";
            $periodfilter_samples_only_ = 0;
            return;
          }
          push ( @tag_aux_, $corr_indep_ );
          $sampletag_ = "CORR";
        }
        elsif ( $filter_tag_ eq "thinbookp" ) {
          $sampletag_ = "L1SZ";
          $highlow_ = "LOW";
        }

        my @samples_vec_ = ( );
        print $main_log_file_handle_ "Datagen day vec: ".join(" ", @datagen_day_vec_)."\n";
        GetFilteredSamples ( $shortcode_, \@datagen_day_vec_, $frac_samples_, $highlow_, $sampletag_, \@tag_aux_, \@samples_vec_, $trading_start_hhmm_, $trading_end_hhmm_ );
        print $main_log_file_handle_ "Only model on $filter_tag_ period: ",join ( ' ', @samples_vec_ ),"\n";

        $periodfilter_samples_filename_ = $work_dir_."/periodfilter_samples_list.txt";
        open BADSHANDLE, "> $periodfilter_samples_filename_";
        print BADSHANDLE join ( "\n", @samples_vec_ )."\n";
        close BADSHANDLE;

        print $main_log_file_handle_ join ( "\n", @samples_vec_ )."\n";

        push ( @intermediate_files_, $periodfilter_samples_filename_ );

        $name_ = (($highlow_ eq "LOW") ? "l": "h").lc($sampletag_).lc(join("_",@tag_aux_)).$name_;
      }
      when ( "badsamples" )
      {
        $periodfilter_samples_only_ = 1;

        my $frac_days_ = shift @datagen_period_filter_words_ || 0.3;

        my $timeperiod_ = "$trading_start_hhmm_-$trading_end_hhmm_";

        my @bad_samples_vec_ = ( );

        GetBadSamplesPoolForShortcode ( $shortcode_, $timeperiod_, \@datagen_day_vec_, \@bad_samples_vec_, $frac_days_ );
        print $main_log_file_handle_ "Only model on pool bad samples",join ( ' ', @bad_samples_vec_ ),"\n";

        $periodfilter_samples_filename_ = $work_dir_."/bad_samples_list.txt";
        open BADSHANDLE, "> $periodfilter_samples_filename_";
        print BADSHANDLE join ( "\n", @bad_samples_vec_ )."\n";
        close BADSHANDLE;

        push ( @intermediate_files_, $periodfilter_samples_filename_ );

        $name_ = "badsamples".$name_;
      }
      default
      {
      }
    }
  }
}

sub SanityCheckInstructionFile 
{
  print $main_log_file_handle_ "SanityCheckInstructionFile\n";

  if ( ! ( $shortcode_ ) )
  {
    print $main_log_file_handle_ "SHORTCODE missing\n";
    exit ( 1 );
  }

  if ( $#ilist_filename_vec_ <0 )
  {
    print $main_log_file_handle_ "INDICATORLISTFILENAME vector empty\n";
    exit ( 1 );
  }

  if ( ! ( $datagen_start_yyyymmdd_ ) )
  {
    print $main_log_file_handle_ "DATAGEN_START_YYYYMMDD missing\n";
    exit ( 1 );
  }

  if ( ! ( ValidDate ( $datagen_start_yyyymmdd_ ) ) )
  {
    print $main_log_file_handle_ "DATAGEN_START_YYYYMMDD not Valid\n";
    exit ( 1 );
  }

  if ( ! ( $datagen_end_yyyymmdd_ ) )
  {
    print $main_log_file_handle_ "DATAGEN_END_YYYYMMDD missing\n";
    exit ( 1 );
  }

  if ( ! ( ValidDate ( $datagen_end_yyyymmdd_ ) ) )
  {
    print $main_log_file_handle_ "DATAGEN_END_YYYYMMDD not Valid\n";
    exit ( 1 );
  }

  if ( ! ( $datagen_start_hhmm_ ) )
  {
    print $main_log_file_handle_ "DATAGEN_START_HHMM missing\n";
    exit ( 1 );
  }

  if ( ! ( $datagen_end_hhmm_ ) )
  {
    print $main_log_file_handle_ "DATAGEN_END_HHMM missing\n";
    exit ( 1 );
  }

#my @valid_filter_list_ = ("f0", "fst.5", "fst1", "fst2", "fsl2", "fsl3", "fsg.5", "fsg1", "fsg2", "fsr1_5", "fsr.5_3" , "flogit", "fv", "fsudm", "fsudm1", "fsudm2", "fsudm3", "fnz.25", "fnz.5" );
  foreach my $this_filter_ ( @dep_based_filter_ )
  {
    my $is_valid = 0;

    if ( (grep { /^$this_filter_$/ } ("f0","flogit","fsudm", "fsudm1", "fsudm2", "fsudm3","fv"))
        || $this_filter_ =~ /^(fst|fsl|fsg|old_fsl|old_fsg|fnz)[.0-9]+$/ 
        || $this_filter_ =~ /^(fsr|old_fsr)[.0-9]+_[.0-9]+$/ ) { 
      $is_valid = 1;
    }
    
    if($is_valid == 0)
    {
      print $main_log_file_handle_ "ERROR : filtration $this_filter_ not implemented\n";
      print "ERROR : filtration $this_filter_ not implemented\n";
    }
  }

  if ( !( $strategyname_ ) )
  {
    print $main_log_file_handle_ "STRATEGYNAME missing\n";
    exit ( 1 );
  }

  if ( ! ( $trading_start_yyyymmdd_ ) )
  {
    print $main_log_file_handle_ "TRADING_START_YYYYMMDD missing\n";
    exit ( 1 );
  }

  if ( ! ( ValidDate ( $trading_start_yyyymmdd_ ) ) )
  {
    print $main_log_file_handle_ "TRADING_START_YYYYMMDD not Valid\n";
    exit ( 1 );
  }

  if ( ! ( $trading_end_yyyymmdd_ ) )
  {
    print $main_log_file_handle_ "TRADING_END_YYYYMMDD missing\n";
    exit ( 1 );
  }

  if ( ! ( ValidDate ( $trading_end_yyyymmdd_ ) ) )
  {
    print $main_log_file_handle_ "TRADING_END_YYYYMMDD not Valid\n";
    exit ( 1 );
  }

  if ( ! ( $trading_start_hhmm_ ) )
  {
    print $main_log_file_handle_ "TRADING_START_HHMM missing\n";
    exit ( 1 );
  }

  if ( ! ( $trading_end_hhmm_ ) )
  {
    print $main_log_file_handle_ "TRADING_END_HHMM missing\n";
    exit ( 1 );
  }

  if ( ! ( $num_files_to_choose_ ) )
  {
    print $main_log_file_handle_ "NUM_FILES_TO_CHOOSE missing\n";
    exit ( 1 );
  }

  if ( int ( $num_files_to_choose_ ) <= 0 )
  {
    $num_files_to_choose_ = 1;
  }

  if ( FindItemFromVec ( "na_mult", \@predalgo_ ) )
  {
    if ( $#predconfig_ < 0 )
    {
      print $main_log_file_handle_ "PREDCONFIG missing\n";
      exit ( 1 );
    }
    for ( my $i = 0; $i<=$#predconfig_; $i++ )
    {
      if ( !( -e $predconfig_[$i] ) )
      {
        print $main_log_file_handle_ "PREDCONFIG ".$predconfig_[$i]." does not exist\n";
        exit ( 1 );
      }
    }
  }

  if ( $#indicator_list_file_vec_ < 0 )
  { # None of the ilistfiles exist !
    PrintStacktraceAndSendMailDie ( "Bad Config $instructionfilename_ . No indicatorilistfiles available." );
  }

  foreach my $t_ilist_ ( @indicator_list_file_vec_ ) {
    if ( ! -f $t_ilist_ ) {
      PrintStacktraceAndSendMailDie ( "Bad Config $instructionfilename_. No such file: ".$t_ilist_."\n" );
    }
  }

  foreach my $regtype_ ( keys %regressalgo2duration2param_filevec_ ) {
    foreach my $pred_dur_ ( keys %{ $regressalgo2duration2param_filevec_{ $regtype_ } } ) {
      foreach my $pfile_ ( @{ $regressalgo2duration2param_filevec_{ $regtype_ }{ $pred_dur_ } } ) {
        if ( ! -f $pfile_ ) {
          PrintStacktraceAndSendMailDie ( "Bad Config $instructionfilename_. No such file: ".$pfile_."\n" );
        }
      }
    }
  }
  foreach my $pred_dur_ ( keys %duration_to_param_filevec_ ) {
    foreach my $pfile_ ( @{ $duration_to_param_filevec_{ $pred_dur_ } } ) {
      if ( ! -f $pfile_ ) {
        PrintStacktraceAndSendMailDie ( "Bad Config $instructionfilename_. No such file: ".$pfile_."\n" );
      }
    }
  }
}

sub RunRegressMakeModelFiles 
{
  print $main_log_file_handle_ "RunRegressMakeModelFiles\n";

## create mktdatalogs for use in simple_sim
  my $mktdataloggerdir_ = $work_dir_."/mktdataloggerdir";
  if ( $use_simple_sim_ ) {
      my $exec_cmd = "$SCRIPTS_DIR/generate_mktdatalogger_list.sh $shortcode_ $datagen_start_yyyymmdd_ $datagen_end_yyyymmdd_ $mktdataloggerdir_";
      `$exec_cmd`;
  }
 
## Create Models for each <ilist, pred_duration, pred-algo, regress-exec>
  my $t_iter = -1;
  foreach my $indicator_list_filename_ ( @ilist_filename_vec_ ) {
    $t_iter += 1;
    print $main_log_file_handle_ "ILIST_FILE = $indicator_list_filename_\n";
    my $indicator_list_filename_base_ = basename ( $indicator_list_filename_  );

    if ( $avoid_high_sharpe_indep_check_ ) {
      my $num_indicators_ = `grep "^INDICATOR " $indicator_list_filename_ | wc -l`; chomp ( $num_indicators_ );

      open FHANDLE, "> $avoid_high_sharpe_indep_check_index_filename_" or PrintStacktraceAndSendMailDie ("Could not open $avoid_high_sharpe_indep_check_index_filename_ for writing");
      print FHANDLE join(" ", 0..$num_indicators_)."\n";
      close FHANDLE;
    }

    foreach my $this_pred_duration_ ( @predduration_ ) {

      my $this_work_dir_ = $work_dir_."/".$this_pred_duration_;
      if ( ! ( -d $this_work_dir_ ) ) { `mkdir -p $this_work_dir_`; }

      foreach my $this_predalgo_ ( @predalgo_ ) {

        $indicator_list_filename_ = $ilist_filename_vec_[$t_iter];
        my $main_file_extension_ = $indicator_list_filename_base_."_".$this_pred_duration_."_".$this_predalgo_."_".$datagen_start_yyyymmdd_."_".$datagen_end_yyyymmdd_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_."_".$unique_gsm_id_ ; 
        my $datefile_ = $work_dir_."/datefile";
        my $datefile_old_ = $work_dir_."/datefile_old_";
        my $timestampfile_ = $work_dir_."/timestampfile";

        my @this_reg_data_filename_vec_ = ();
        my @this_reg_data_filename_vec_old_ = ();
        if ( $use_regime_model_ ) {
# create regdata files for each regime
          for ( my $i_ = 1; $i_ <= $num_regimes_; $i_++ ) {
            push ( @this_reg_data_filename_vec_, $reg_data_dir."/reg_data_".$main_file_extension_."_".$i_ );
            push ( @this_reg_data_filename_vec_old_, $reg_data_dir."/reg_data_".$main_file_extension_."_old_".$i_ );
          }
        }
        else {
          push ( @this_reg_data_filename_vec_, $reg_data_dir."/reg_data_".$main_file_extension_ );
          push ( @this_reg_data_filename_vec_old_, $reg_data_dir."/reg_data_".$main_file_extension_."_old_" );
        }
        push ( @intermediate_files_, @this_reg_data_filename_vec_ );
        push ( @intermediate_files_, @this_reg_data_filename_vec_old_ );


        if($use_new_regdata_ == 1) {
          $indicator_list_filename_ = GenerateRegdataFiles ( $indicator_list_filename_, $this_pred_duration_, $this_predalgo_, \@this_reg_data_filename_vec_,1,$t_random_lower_threshold_, $t_random_time_cap_, $filter_uncertain_values_ );
          $indicator_list_filename_ = GenerateRegdataFiles ( $indicator_list_filename_, $t_random_predduration_to_scale_, $this_predalgo_, \@this_reg_data_filename_vec_old_ ,0,0,0,0);
        }
        else {
          $indicator_list_filename_ = GenerateRegdataFiles ( $indicator_list_filename_, $this_pred_duration_, $this_predalgo_, \@this_reg_data_filename_vec_, 0, 0, 0, 0 );
        }

        if ( $#this_reg_data_filename_vec_ < 0 ) {
          print $main_log_file_handle_ "The reg data file name vector is empty.\n";
          print "The Reg data file name vector is empty\n";
          next;
        }
        else {
          my %individual_model_file_vec_ = ();
          my $indicator_list_filename_orig_ = $indicator_list_filename_;

          for ( my $reg_data_counter_ = 0; $reg_data_counter_ <= $#this_reg_data_filename_vec_ ; $reg_data_counter_++ )
          { # now $this_reg_data_filename_ exists
            $indicator_list_filename_ = $indicator_list_filename_orig_;

            if ( $use_regime_ilists_ > 0 ) {
              my $ilists_list_ref_ = $regime_ilist_to_ilist_vec_map_{ $indicator_list_filename_ };
              print $main_log_file_handle_ "REGIME_ILIST: All Ilists in the list file: ".join(" ", @$ilists_list_ref_)."\n";
              $indicator_list_filename_ = $$ilists_list_ref_[ $reg_data_counter_ ];
              print $main_log_file_handle_  "Run for the regime indx: ".$reg_data_counter_."\nIlist: ".$indicator_list_filename_."\n";
            }

            my $this_reg_data_filename_ = $this_reg_data_filename_vec_[ $reg_data_counter_];
            my $this_reg_data_filename_old_ = $this_reg_data_filename_vec_old_[ $reg_data_counter_];
            print $main_log_file_handle_ " This Reg Data file name: ". $this_reg_data_filename_." ".$reg_data_counter_;
            
            my $this_reg_data_file_extension_ = $main_file_extension_."_".$reg_data_counter_; 
            for ( my $findex_ = 0; $findex_ <= $#dep_based_filter_; $findex_ ++ ) {
# for every filtration of the main file
              my $this_filter_ = $dep_based_filter_[$findex_];
              my $this_filtered_main_file_extension_ = $this_reg_data_file_extension_."_".$this_filter_;
              my $this_filtered_reg_data_filename_ = $reg_data_dir."/reg_data_".$this_filtered_main_file_extension_;
              my $this_filtered_reg_data_filename_old_ = $reg_data_dir."/reg_data_".$this_filtered_main_file_extension_."_old";

              FilterRegdata ( $this_reg_data_filename_, $this_filter_, $this_filtered_reg_data_filename_, $norm_regdata_stdev_indicator_ );

              if($use_new_regdata_ == 1) {
                 FilterRegdata ( $this_reg_data_filename_old_, $this_filter_, $this_filtered_reg_data_filename_old_, $norm_regdata_stdev_indicator_); 
              }

              if ( ! ExistsWithSize ( $this_filtered_reg_data_filename_ ) ) {
# This is highly unexpected. This means there is something wrong with the ilist
                print $main_log_file_handle_ "ERROR: $this_filtered_reg_data_filename_ still missing or 0sized:.. skipping ModelGeneration for this\n";
              }
              else {
                if ( $obtain_model_from_cross_validation_) {
                  ChooseModelFromCrossValidation ( $this_filtered_reg_data_filename_, $indicator_list_filename_ );
                }

#remove the date information obtained for datagen prob distribution computation
                my $exec_cmd = "awk '{print \$NF}' $this_filtered_reg_data_filename_ | uniq -c > $datefile_";
                `$exec_cmd`;
                $exec_cmd = "awk '{for(i=1;i<NF-1;i++){printf \"%s \",\$i}printf \"%s\\n\",\$(NF-1)}' $this_filtered_reg_data_filename_ > $this_reg_data_filename_";
                `$exec_cmd`;
                `mv $this_reg_data_filename_ $this_filtered_reg_data_filename_`;

                 if($use_new_regdata_ == 1) {
                   my $exec_cmd = "awk '{print \$NF}' $this_filtered_reg_data_filename_old_ | uniq -c > $datefile_old_";
                   `$exec_cmd`;
                   $exec_cmd = "awk '{for(i=1;i<NF-1;i++){printf \"%s \",\$i}printf \"%s\\n\",\$(NF-1)}' $this_filtered_reg_data_filename_old_ > $this_reg_data_filename_old_";
                   `$exec_cmd`;
                   `mv $this_reg_data_filename_old_ $this_filtered_reg_data_filename_old_`;
                 }


                if ( $use_simple_sim_ == 1) {
                  $exec_cmd = "awk '{for(i=1;i<NF-1;i++){printf \"%s \",\$i}printf \"%s\\n\",\$(NF-1)}' $this_filtered_reg_data_filename_ > $this_reg_data_filename_";
                  `$exec_cmd`;
                  $exec_cmd = "awk '{print \$NF}' $this_filtered_reg_data_filename_ > $timestampfile_ ";
                  `$exec_cmd`;
                  `mv $this_reg_data_filename_ $this_filtered_reg_data_filename_`;
                }

# For each type of Regress Exec, generate models
                for ( my $i = 0 ; $i <= $#regress_exec_; $i ++ ) {
                  my $scalar_ref_ = $regress_exec_[$i];
                  next if ( $#$scalar_ref_ < 0 );

                  my $regtype_ = $$scalar_ref_[0];
                  my @model_fname_vec_old_ = ( );
                  my @model_fname_vec_ = ( );
## f_r_extension_: filename extension used for temp files in this run 
                  my $f_r_extension_ = "";
## regress_cmd_string_: the cmd used for building the regressalgo-output/model (unless cross-validate or obtain_model_from_cross_validation_ is used)
                  my $regress_cmd_string_ = "";
## cross_validate_arg_string_: the arguments (5th argument onwards) for cross_validate_model.py
                  my $cross_validate_arg_string_ = "";
## evaluate_optstring_: 0 (for: evaluate_model.py 0), 1 (for lasso_type: evaluate_model.py 1), rest (the entire cmd used directly for evaluating)
                  my $evaluate_optstring_ = 0;
## use_continuous_regime_model_ = 0 unless regress_exec is CRR
                  $use_continuous_regime_model_ = 0;
## place_coeff_script_: script for building the model from the regressalgo-exec output 
                  my $place_coeff_script_ = "";
                  if ( $regtype_ eq "LASSORIDGE" ) { $regtype_ = "LR"; }
                  my $call_runregressexec_ = 1;
                  print $main_log_file_handle_ "EXEC TYPE $regtype_\n";

                  given ( $regtype_ ) 
                  {
                    when ( "L1_REGRESSION" )
                    {
                      print $main_log_file_handle_ "L1_REGRESSION\n";
                      my $tvalue_cutoff_l1_reg_ = 5;
                      my $tvalue_change_ratio_ = .5;
                      if ( $#$scalar_ref_ >= 1) {
                        $tvalue_cutoff_l1_reg_ = $$scalar_ref_[0];
                        $tvalue_change_ratio_ = $$scalar_ref_[1];
                      }

                      my $regress_dotted_string_ = join ( '_', ( $regtype_, $tvalue_cutoff_l1_reg_, $tvalue_change_ratio_  ) );
                      print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";
                      $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                      $regress_cmd_string_ = "$MODELSCRIPTS_DIR/lad_regression.R -i REGDATAFNAME -o REGOUTFNAME -c $tvalue_cutoff_l1_reg_ -r $tvalue_change_ratio_ ";
                      $cross_validate_arg_string_ = "L1_REGRESSION $tvalue_cutoff_l1_reg_ $tvalue_change_ratio_";
                      $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl";
                    }
                    when ("PCA_REGRESSION")
                    {
                      print $main_log_file_handle_ "PCA_REGRESSION\n";
                      my $num_pca_components_ = $$scalar_ref_[1];
                      my $max_model_size_ = $$scalar_ref_[2];
                      my $pca_heuristic_ = $$scalar_ref_[3];
                      my $min_correlation_ = $$scalar_ref_[4];

                      my $regress_dotted_string_ = join ( '_', ( $regtype_, $num_pca_components_, $max_model_size_, $pca_heuristic_, $min_correlation_ ) );
                      print $main_log_file_handle_ "$regress_dotted_string_ on $this_pred_duration_ by $this_predalgo_\n";
                      $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                      if ( $#$scalar_ref_ >=1 ) {
                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/call_pcareg.pl REGDATAFNAME $num_pca_components_ $max_model_size_ $pca_heuristic_ $min_correlation_ REGOUTFNAME $$scalar_ref_[1]";
                        $f_r_extension_ = $f_r_extension_."_".$$scalar_ref_[1];
                      }
                      else {
                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/call_pcareg.pl REGDATAFNAME $num_pca_components_ $max_model_size_ $pca_heuristic_ $min_correlation_ REGOUTFNAME ";
                      }

                      $cross_validate_arg_string_ = "PCA_REGRESSION $num_pca_components_ $max_model_size_ $pca_heuristic_ $min_correlation_";
                      $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl";
                    }
                    when ("LASSO")
                    { # LASSO 12
                      $CREATE_SUBMODELS = 0;
                      if ( $#$scalar_ref_ >= 1 ) {
                        my $max_model_size_ = $$scalar_ref_[1];

                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $max_model_size_));
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/call_lasso.pl REGDATAFNAME $max_model_size_ REGOUTFNAME ";
                        $cross_validate_arg_string_ = "LASSO $max_model_size_";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_lasso_model.pl";
                        $evaluate_optstring_ = 1; 
                      }
                      else {
                        print $main_log_file_handle_ "LASSO line needs 2 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ("SLASSO")
                    { # SLASSO 12
                      $CREATE_SUBMODELS = 0;
                      if ( $#$scalar_ref_ >= 1 )
                      {
                        my $max_model_size_ = $$scalar_ref_[1];

                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $max_model_size_));
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/call_slasso.pl REGDATAFNAME $max_model_size_ REGOUTFNAME ";
                        $cross_validate_arg_string_ = "SLASSO $max_model_size_";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_slasso_model.pl"; 
                        $evaluate_optstring_ = 1; 
                      }
                      else {
                        print $main_log_file_handle_ "SLASSO line needs 2 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }					
                    } 
                    when ("LR")
                    { # LR 12
                      $CREATE_SUBMODELS = 0;
                      if ( $#$scalar_ref_ >= 1 ) {
                        my $lasso_or_ridge_ = $$scalar_ref_[1];

                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $lasso_or_ridge_));
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/build_unconstrained_lasso_model.R REGDATAFNAME $lasso_or_ridge_ REGOUTFNAME ";
                        $cross_validate_arg_string_ = "LR $lasso_or_ridge_";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_lasso_model.pl"; 
                        $evaluate_optstring_ = 1; 
                      }
                      else {
                        print $main_log_file_handle_ "LR line needs 2 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ("RIDGE")
                    { # RIDGE 7 0.95
                      $CREATE_SUBMODELS = 0;
                      if ( $#$scalar_ref_ >= 2 ) {
                        my $df_ = $$scalar_ref_[1];
                        my $min_variance_explained_ = $$scalar_ref_[2];

                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $df_, $min_variance_explained_ ));
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/build_unconstrained_ridge_model.R REGDATAFNAME $df_ $min_variance_explained_ REGOUTFNAME ";
                        $cross_validate_arg_string_ = "RIDGE $df_ $min_variance_explained_";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl";
                        $evaluate_optstring_ = 1;
                      }
                      else {
                        print $main_log_file_handle_ "RIDGE line needs 3 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ("LM")
                    {
                      $CREATE_SUBMODELS = 0;
                      if ( $#$scalar_ref_ >= 0 ) {
                        my $regress_dotted_string_ = $regtype_ ;
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/build_linear_model.R REGDATAFNAME REGOUTFNAME ";
                        $cross_validate_arg_string_ = "LM"; 
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_lasso_model.pl"; 
                        $evaluate_optstring_ = 1; 
                      }
                    }   
                    when ("NNLS")
                    ## Variance Inflation Factor and non negative least squares fit, takes target model size as input (actual model size is about half this)
                    {
                      $CREATE_SUBMODELS = 0;
                      if ( $#$scalar_ref_ >= 1 ) {
                        my $max_model_size_ = $$scalar_ref_[1];

                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $max_model_size_  ));
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        
                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/build_nnls.R REGDATAFNAME REGOUTFNAME $max_model_size_ ";
                        $cross_validate_arg_string_ = "NNLS $max_model_size_"; 
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_lasso_model.pl"; 
                        $evaluate_optstring_ = 1; 
                      }
                    } 
                    when ( "SOM" )
                    {
                      $CREATE_SUBMODELS = 0;
                      if ( $#$scalar_ref_ >= 0 ) {
                        my $regress_dotted_string_ = $regtype_ ;
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/build_unconstrained_lasso_model.R REGDATAFNAME REGOUTFNAME ";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_lasso_model.pl"; 
                      }
                    }
                    when ("SIGLR")
                    {
                      $CREATE_SUBMODELS = 0;
                      my $domination_penalty_ = "N";
                      my $penalty_ = "N";
                      print $main_log_file_handle_ "SIGLR\n";
 
                      if ( $#$scalar_ref_ >= 1 ) {
                        my $max_model_size_ = $$scalar_ref_[1];
                        #my $domination_penalty_ =  $$scalar_ref_[2];
                        #my $penalty_ = $$scalar_ref_[3];
                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $max_model_size_, $domination_penalty_, $penalty_  ) );
                        my $p_norm_ = 0.5 ;
                        
                        if ( $penalty_ eq "LP" ) {
                          $p_norm_ = $$scalar_ref_[4] if ( $#$scalar_ref_ > 3 );
                          $regress_dotted_string_ = join ( '_' , ( $regtype_, $max_model_size_, $domination_penalty_, $penalty_, $p_norm_ ) );
                        }

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/SIGLR_grad_descent_3.R REGDATAFNAME REGOUTFNAME $max_model_size_ $domination_penalty_ $penalty_ ";
                        $regress_cmd_string_ = $regress_cmd_string_."$p_norm_ " if $penalty_ eq "LP";

                        $evaluate_optstring_ = "$MODELSCRIPTS_DIR/SIGLR_grad_descent_3.R REGDATAFNAME REGOUTFNAME $max_model_size_ $domination_penalty_ $penalty_ $p_norm_ $work_dir_/tmp_test_reg_data $max_error_on_test_ STATUSFILE";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_siglr_model.pl";
                      }
                      else {
                        print $main_log_file_handle_ "SIGLR line needs 4 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }                                                  				
                    }
                    when ("SIGLRF")
                    {
                      $CREATE_SUBMODELS = 0;
                      print $main_log_file_handle_ "SIGLRF\n";
                      if ( $#$scalar_ref_ >= 1 ) {
                        my $penalty_ = $$scalar_ref_[1];
                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $penalty_  ) );
                        my $p_norm_ = 0.5 ;

                        if ( $penalty_ eq "LP" ) {
                          $p_norm_ = $$scalar_ref_[2] if ( $#$scalar_ref_ > 1 );
                          $regress_dotted_string_ = join ( '_' , ( $regtype_, $penalty_, $p_norm_ ) );
                        }

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/SIGLR_grad_descent.R REGDATAFNAME REGOUTFNAME $penalty_ ";
                        $regress_cmd_string_ = $regress_cmd_string_."$p_norm_ " if $penalty_ eq "LP";

                        $evaluate_optstring_ = "$MODELSCRIPTS_DIR/SIGLR_grad_descent.R REGDATAFNAME REGOUTFNAME $penalty_ $p_norm_ $work_dir_/tmp_test_reg_data $max_error_on_test_ STATUSFILE";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_siglr_model.pl";
                      }
                      else {
                        print $main_log_file_handle_ "SIGLRF line needs 2 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }                                                   				
                    }
                    when ("RANDOMFOREST")
                    { # RANDOMFOREST 15 20 20 100 9
                      $CREATE_SUBMODELS = 0;
                      print $main_log_file_handle_ "RANDOMFOREST\n";

                      if ( $#$scalar_ref_ >= 3 ) {
                        my $max_model_size_ = $$scalar_ref_[1];
                        my $num_iters_ = $$scalar_ref_[2];
                        my $num_trees_ =  $$scalar_ref_[3];
                        my $max_nodes_ = $$scalar_ref_[4];
                        
                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $max_model_size_, $num_iters_, $num_trees_, $max_nodes_  ) );
                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/randomForest_model.R REGDATAFNAME REGOUTFNAME $max_model_size_ $num_iters_ $num_trees_ $max_nodes_ ";
                        if ( $#$scalar_ref_ >= 5 ) {
                          my $num_quantiles_ = $$scalar_ref_[5];
                          $regress_cmd_string_= $regress_cmd_string_." $num_quantiles_";
                          $regress_dotted_string_ = $regress_dotted_string_." $num_quantiles_";
                        }

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        $evaluate_optstring_ = "$MODELSCRIPTS_DIR/randomForest_model.R $this_filtered_reg_data_filename_ REGOUTFNAME $max_model_size_ $num_iters_ $num_trees_ $work_dir_/tmp_test_reg_data $max_error_on_test_ STATUSFILE ";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_random_forest_model.pl";
                      }
                      else {
                        print $main_log_file_handle_ "RANDOMFOREST line needs 4 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }                                                   				
                    }
                    when ("PNLBOOSTING")
                    {
                      $CREATE_SUBMODELS = 0;
                      print $main_log_file_handle_ "PNLBOOSTING\n";

                      if ( $#$scalar_ref_ >= 3 ) {
                        my $reg_coeff_ = $$scalar_ref_[1];
                        my $loss_function_ = $$scalar_ref_[2];
                        my $max_indicator_size_ = $$scalar_ref_[3];
                        my $paramfile_to_train_ = "";
                        $paramfile_to_train_ = $$scalar_ref_[4] if ( $#$scalar_ref_ >=  4);

                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $reg_coeff_  ) );
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        
                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/pnl_adaboost.py REGDATAFNAME $datefile_ $reg_coeff_ $loss_function_ $max_indicator_size_ $indicator_list_filename_ $work_dir_/temp_modelfile_ $paramfile_to_train_ $work_dir_/temp_stratfile_ $trading_start_hhmm_ $trading_end_hhmm_ $shortcode_ $strategyname_ > REGOUTFNAME";

                        if ( $use_simple_sim_ ) {
                          $regress_cmd_string_ = "$MODELSCRIPTS_DIR/pnl_adaboost.py REGDATAFNAME $datefile_ $reg_coeff_ $loss_function_ $max_indicator_size_ $indicator_list_filename_ $work_dir_/temp_modelfile_ $paramfile_to_train_ $work_dir_/temp_stratfile_ $trading_start_hhmm_ $trading_end_hhmm_ $shortcode_ $strategyname_ $timestampfile_ $mktdataloggerdir_/all_datalog > REGOUTFNAME";
                        }
                      }
                      else {
                        print $main_log_file_handle_ "PNLBOOSTING line needs 4 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ( /^(CONSOLIDATED|COARSE|)(TREE|)BOOSTING$/ )
                    {
                      $CREATE_SUBMODELS = 0;
                      print $main_log_file_handle_ "$regtype_\n";
                      my $regress_exec_bname_ = ( $regtype_ =~ /^CONSOLIDATED/ ) ? "consolidated_adaboost.py" : ( ( $regtype_ =~ /^COARSE/ ) ? "coarse_adaboost.py" : "adaboost.py" );

                      if ( $#$scalar_ref_ >= 3 ) {
                        my $reg_coeff_ = $$scalar_ref_[1];
                        my $loss_function_ = $$scalar_ref_[2];
                        my $max_indicator_size_ = $$scalar_ref_[3];
                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $reg_coeff_  ) );
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        
                        my $t_granularity_ = ( $regtype_ =~ /^(CONSOLIDATED|COARSE)/ ) ? $granularity_ : "";
                        my $t_datefile_ = ( $regtype_ =~ /^(CONSOLIDATED|COARSE)/ ) ? $datefile_ : "";
                        my $t_crossvalidate_train_datefile_ = ( $regtype_ =~ /^(CONSOLIDATED|COARSE)/ ) ? $work_dir_."/tmp_train_datefile" : "";

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/$regress_exec_bname_ $regtype_ REGDATAFNAME $t_datefile_ $reg_coeff_ $loss_function_ $max_indicator_size_ $indicator_list_filename_ $t_granularity_ > REGOUTFNAME";
                        $evaluate_optstring_ = "$MODELSCRIPTS_DIR/$regress_exec_bname_ $regtype_ REGDATAFNAME $t_crossvalidate_train_datefile_ $reg_coeff_ $loss_function_ $max_indicator_size_ $indicator_list_filename_ $t_granularity_ $work_dir_/tmp_test_reg_data $max_error_on_test_ STATUSFILE > REGOUTFNAME";
                      }
                      else {
                        print $main_log_file_handle_ "$regtype_ line needs 4 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ("MLOGIT")
                    { # MLOGIT 0.1 10 # logistic regression
                      $CREATE_SUBMODELS = 0;
                      print $main_log_file_handle_ "MLOGIT\n";
                      if( $#$scalar_ref_ >= 2 ) {
                        my $zero_threshold_ = $$scalar_ref_[1];
                        my $max_model_size_ = $$scalar_ref_[2];
                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $zero_threshold_,$max_model_size_));

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/call_mlogit.pl REGDATAFNAME $zero_threshold_ $max_model_size_ REGOUTFNAME";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_logit_model.pl";
                      }
                    }
                    when ("NEWMLOGIT")
                    { #new logistic regression
                      print $main_log_file_handle_ "NEWMLOGIT\n";
                      if ( $#$scalar_ref_ >= 2 ) {
                        my $zero_threshold_ = $$scalar_ref_[1];
                        my $max_model_size_ = $$scalar_ref_[2];

                        my $l2_penalty = 0.01;
                        $l2_penalty = $$scalar_ref_[3] if ( $#$scalar_ref_ >= 3 );
                       
                        my $cost_concession = 0.0;
                        $cost_concession = $$scalar_ref_[4] if ( $#$scalar_ref_ >= 4 );
                         
                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $zero_threshold_,$max_model_size_,$l2_penalty,$cost_concession));
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        
                        my $this_sampled_reg_data_ = $this_work_dir_."/sampled_regdata_".$f_r_extension_;
                        my $this_sampled_mlogit_lables_ = $this_work_dir_."/sampled_mlogit_lables_".$f_r_extension_;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/create_mlogit_data.pl REGDATAFNAME $zero_threshold_ $this_sampled_reg_data_ $this_sampled_mlogit_lables_; ";
                        $regress_cmd_string_ = $regress_cmd_string_."$MODELSCRIPTS_DIR/logistic_regression.R $this_sampled_mlogit_lables_ $max_model_size_ REGOUTFNAME $this_sampled_reg_data_ $l2_penalty $cost_concession; ";
                        $regress_cmd_string_ = $regress_cmd_string_."rm $this_sampled_reg_data_ $this_sampled_mlogit_lables_";

                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_logit_model_new.pl";
                      }
                    }
                    when ( "EARTH" )
                    { # EARTH
                      $CREATE_SUBMODELS = 0; 
                      $f_r_extension_ = $this_filtered_main_file_extension_."_EARTH"; 
# dummy vars
                      my $regularization_coeff_ = 0.01;
                      my $min_correlation_ = 0.01;
                      my $first_indep_weight_ = 0.0;
                      my $must_include_first_k_independants_ = -1;
                      my $max_indep_correlation_ = 0.7;
                      my $max_model_size_ = 20;

                      $regress_cmd_string_ = "$LIVE_BIN_DIR/callMARS REGDATAFNAME $regularization_coeff_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ REGOUTFNAME $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ ";

                      $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_mars_models.py";
                    }
                    when ( /^(FSLR|FSHLR|FSHDVLR)$/ )
                    { # <regtype> 0.05 0 0 0.85 20 H/N
                      if ( $#$scalar_ref_ >= 4 ) {
                        my $min_correlation_ = $$scalar_ref_[1];
                        my $first_indep_weight_ = $$scalar_ref_[2];
                        my $must_include_first_k_independants_ = $$scalar_ref_[3];
                        my $max_indep_correlation_ = $$scalar_ref_[4];

                        my $max_model_size_ = ( $regtype_ =~ /FSHDVLR/ ) ? 14 : 20;
                        $max_model_size_ = $$scalar_ref_[5] if ( $#$scalar_ref_ >= 5 );
                        
                        my $match_icorrs_ = "N";
                        $match_icorrs_ = $$scalar_ref_[6] if ( $#$scalar_ref_ >= 6 );

                        my $regress_dotted_string_ = join ( '_', ( $regtype_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_, $max_model_size_, $match_icorrs_ ) );

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        
                        $regress_cmd_string_ = "$LIVE_BIN_DIR/call".$regtype_." REGDATAFNAME $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ REGOUTFNAME $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ ";
# match against historical hash correlations
                        if ( $match_icorrs_ eq "I" ) { 
                          $regress_cmd_string_ = $regress_cmd_string_." $indicator_list_filename_ ";
                          $f_r_extension_ = $f_r_extension_."_I"; 
                        }
                        if ( $#$scalar_ref_ >= 7 ) {
                          $regress_cmd_string_ = $regress_cmd_string_." $$scalar_ref_[7]";
                          $f_r_extension_ = $f_r_extension_."_".$$scalar_ref_[7];
                        }
                        if ( $#$scalar_ref_ >= 8 ) {
                          $regress_cmd_string_ = $regress_cmd_string_." $$scalar_ref_[8]";
                          $f_r_extension_ = $f_r_extension_."_".$$scalar_ref_[8];
                        }
                        
                        $cross_validate_arg_string_ = "$regtype_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $max_model_size_";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl"; 
                      }
                      else {
                        print $main_log_file_handle_ "$regtype_ line needs 4 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ( "FSVLR" )
                    { # FSVLR 2.0 0.04 0 0 0.7 18 H/N
                      if ( $#$scalar_ref_ >= 5 ) {
                        my $stdev_thresh_factor_ = $$scalar_ref_[1];
                        my $min_correlation_ = $$scalar_ref_[2];
                        my $first_indep_weight_ = $$scalar_ref_[3];
                        my $must_include_first_k_independants_ = $$scalar_ref_[4];
                        my $max_indep_correlation_ = $$scalar_ref_[5];

                        my $max_model_size_ = 14;
                        $max_model_size_ = $$scalar_ref_[6] if ( $#$scalar_ref_ >= 6 );

                        my $match_icorrs_ = "N";
                        $match_icorrs_ = $$scalar_ref_[7] if ( $#$scalar_ref_ >= 7 );

                        my $regress_dotted_string_ = join ( '_', ( $regtype_, $stdev_thresh_factor_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_, $max_model_size_, $match_icorrs_ ) );

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        
                        $regress_cmd_string_ = "$LIVE_BIN_DIR/callFSVLR REGDATAFNAME $stdev_thresh_factor_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ REGOUTFNAME $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ ";
# match against historical hash correlations
                        if ( $match_icorrs_ eq "I" ) { 
                          $regress_cmd_string_ = $regress_cmd_string_." $indicator_list_filename_ "; 
                        }
                        if ( $#$scalar_ref_ >= 8 ) {
                          $regress_cmd_string_ = $regress_cmd_string_." $$scalar_ref_[8]";
                        }
                        
                        $cross_validate_arg_string_ = "FSVLR $stdev_thresh_factor_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $max_model_size_";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl"; 
                      }
                      else {
                        print $main_log_file_handle_ "FSVLR line needs 5 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when( "CRR" )
                    {
                      $use_continuous_regime_model_ = 1;
                      $regress_cmd_string_ = "Rscript $MODELSCRIPTS_DIR/continuous_regime_model.R REGDATAFNAME REGOUTFNAME $continuous_regime_cutoff_";
                      $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeff_continuous_regime.pl";
                    } 
                    when ( "FSRR" )
                    { # FSRR 0.75 0.04 0 0 0.95 28 H/N
                      if ( $#$scalar_ref_ >= 5 ) {
                        my $regularization_coeff_ = $$scalar_ref_[1];
                        my $min_correlation_ = $$scalar_ref_[2];
                        my $first_indep_weight_ = $$scalar_ref_[3];
                        my $must_include_first_k_independants_ = $$scalar_ref_[4];
                        my $max_indep_correlation_ = $$scalar_ref_[5];

                        my $max_model_size_ = 12;
                        $max_model_size_ = $$scalar_ref_[6] if ( $#$scalar_ref_ >= 6 );

                        my $match_icorrs_ = "N";
                        $match_icorrs_ = $$scalar_ref_[7] if ( $#$scalar_ref_ >= 7 );

                        $regress_cmd_string_ = "";
                        if ( ! looks_like_number( $regularization_coeff_ ) && $regularization_coeff_ =~ /^DF_/ ) {
                            my @df_tokens_ = split("_", $regularization_coeff_);
                            my $df_ = $df_tokens_[1];
                            my $var_explained_ = $df_tokens_[2];

                            $regress_cmd_string_ = "reg_coeff=`Rscript $MODELSCRIPTS_DIR/find_regularization_coeff_from_df.R REGDATAFNAME $df_ $var_explained_ $max_model_size_ 2>/dev/null | grep \"lambda: \" | awk \'{print \$2;}\'`; ";
                        }
                        else {
                          $regress_cmd_string_ = "reg_coeff=`echo $regularization_coeff_`; ";
                        }


                        my $regress_dotted_string_ = join ( '_', ( $regtype_, $regularization_coeff_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_, $max_model_size_, $match_icorrs_ ) );

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;
                        
                        $regress_cmd_string_ = $regress_cmd_string_."$LIVE_BIN_DIR/callFSRR REGDATAFNAME \$reg_coeff $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ REGOUTFNAME $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_ ";
# match against historical hash correlations
                        if ( $match_icorrs_ eq "I" ) { 
                          $regress_cmd_string_ = $regress_cmd_string_." $indicator_list_filename_ ";
                          $f_r_extension_ = $f_r_extension_."_I"; 
                        }
                        if ( $#$scalar_ref_ >= 8 ) {
                          $regress_cmd_string_ = $regress_cmd_string_." $$scalar_ref_[8]";
                          $f_r_extension_ = $f_r_extension_."_".$$scalar_ref_[8];
                        }
                        if ( $#$scalar_ref_ >= 9 ) {
                          $regress_cmd_string_ = $regress_cmd_string_." $$scalar_ref_[9]";
                          $f_r_extension_ = $f_r_extension_."_".$$scalar_ref_[9];
                        }
                        
                        $cross_validate_arg_string_ = "FSRR $regularization_coeff_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $max_model_size_ ";

                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl"; 
                      }
                      else {
                        print $main_log_file_handle_ "FSVLR line needs 5 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ( /^(FSRLM|FSRMFSS|FSRMSH|FSRSHRSQ)$/ )
                    { # <regtype> 6 0.02 0 0 0.85 40 H/N
                      if ( $#$scalar_ref_ >= 7 ) { 
                        my $num_local_folds_ = $$scalar_ref_[1];
                        my $min_correlation_ = $$scalar_ref_[2];  
                        my $first_indep_weight_ = $$scalar_ref_[3];  
                        my $must_include_first_k_independants_ = $$scalar_ref_[4]; 
                        my $max_indep_correlation_ = $$scalar_ref_[5];
                        my $max_model_size_ = $$scalar_ref_[6];
                        my $match_icorrs_ = $$scalar_ref_[7];

                        my $regress_dotted_string_ = join ( '.', ( $regtype_, $num_local_folds_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_,$max_model_size_,$match_icorrs_ ) );

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        my $regress_exec_bname_ = ( $regtype_ eq "FSRLM" ) ? "callFSRLM" : (
                                            ( $regtype_ eq "FSRMFSS" ) ? "callfsr_mean_fss_corr" : (
                                            ( $regtype_ eq "FSRMSH" ) ? "callfsr_mean_sharpe_corr" : (
                                            ( $regtype_ eq "FSRSHRSQ" ) ? "callfsr_sharpe_rsq" : "" ) ) );  
                        
                        $regress_cmd_string_ = "$LIVE_BIN_DIR/$regress_exec_bname_ REGDATAFNAME $num_local_folds_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ REGOUTFNAME $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_";
# match against historical hash correlations
                        if ( $match_icorrs_ eq "I" ) { 
                          $regress_cmd_string_ = $regress_cmd_string_." $indicator_list_filename_ "; 
                        }
                        if ( $#$scalar_ref_ >= 8 ) {
                          $exec_cmd = $exec_cmd." $$scalar_ref_[8]";
                        }
                        
                        $cross_validate_arg_string_ = "$regtype_ $num_local_folds_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $max_model_size_ $match_icorrs_";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl"; 
                      }
                      else {
                        print $main_log_file_handle_ "$regtype_ line now needs 6 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ( "MULTLR" )
                    { # <regtype> 6 0.02 0 0 0.85 40 H/N
                      if ( $#$scalar_ref_ >= 7 ) 
                      {
                        my $num_deps_ = $$scalar_ref_[1];
                        my $min_correlation_ = $$scalar_ref_[2];  
                        my $first_indep_weight_ = $$scalar_ref_[3];  
                        my $must_include_first_k_independants_ = $$scalar_ref_[4]; 
                        my $max_indep_correlation_ = $$scalar_ref_[5];
                        my $max_model_size_ = $$scalar_ref_[6];
                        my $match_icorrs_ = $$scalar_ref_[7];

                        my $regress_dotted_string_ = join ( '.', ( $regtype_, $num_deps_, $min_correlation_, $first_indep_weight_, $must_include_first_k_independants_, $max_indep_correlation_,$max_model_size_,$match_icorrs_ ) );

                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        $regress_cmd_string_ = "$LIVE_BIN_DIR/call_multiple_fslr REGDATAFNAME $num_deps_ $min_correlation_ $must_include_first_k_independants_ $max_indep_correlation_ REGOUTFNAME $max_model_size_ $avoid_high_sharpe_indep_check_index_filename_";
# match against historical hash correlations
                        if ( $match_icorrs_ eq "H" ) { 
                          $regress_cmd_string_ = $regress_cmd_string_." $indicator_list_filename_ "; 
                        }
                        if ( $#$scalar_ref_ >= 8 ) {
                          $exec_cmd = $exec_cmd." $$scalar_ref_[8]";
                        }
                        
                        $cross_validate_arg_string_ = "$regtype_ $num_deps_ $min_correlation_ $first_indep_weight_ $must_include_first_k_independants_ $max_indep_correlation_ $max_model_size_ $match_icorrs_";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl"; 
                      }
                      else {
                        print $main_log_file_handle_ "$regtype_ line now needs 6 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }
                    }
                    when ( "MULTLR2" )
                    { #combining models
                      if ($#$scalar_ref_ >= 1) {
                        my $config_ = $$scalar_ref_[1];
                        my ($pred_dur_1_ , $pred_dur_2_ ) = `awk '{print \$1}' $predconfig_[0]`;
                        chomp($pred_dur_1_, $pred_dur_2_);

                        my $regress_dotted_string_ = join( '.' , ("MULTLR" , $pred_dur_1_, $pred_dur_2_, $this_pred_duration_) );
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_;
                        print $main_log_file_handle_ "MULTLR2 $pred_dur_1_ $pred_dur_2_ $this_pred_duration_ $config_\n";

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/multlr2.py REGDATAFNAME $this_pred_duration_ $config_ $indicator_list_filename_ $pred_dur_1_ $pred_dur_2_ REGOUTFNAME $avoid_high_sharpe_indep_check_index_filename_";
                        print $main_log_file_handle_ "$regress_cmd_string_ \n";
                        $place_coeff_script_ = "";
                      }
                      else {
                        print $main_log_file_handle_ "MULTLR2 expects a config file \n";
                      }
                    }
                    when ("SEQREP")
                    {
                      $CREATE_SUBMODELS = 0;
                      print $main_log_file_handle_ "SEQREP\n";

                      if( $#$scalar_ref_ >= 1 ) {
                        my $max_model_size_ = $$scalar_ref_[1];
                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $max_model_size_ ) );
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/seqrep.R REGDATAFNAME $max_model_size_ REGOUTFNAME";
                        $place_coeff_script_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl";
                      }
                      else {
                        print $main_log_file_handle_ "SEQREP line now needs 2 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }                                                   				
                    } 
                    when(m/^KALMAN/) 
                    {
                      $CREATE_SUBMODELS = 0;
                      print $main_log_file_handle_ "KALMAN\n";

                      if( $#$scalar_ref_ >= 1 ) {
                        my $max_model_size_ = $$scalar_ref_[1];
                        my $regress_dotted_string_ = join( '_' , ( $regtype_, $max_model_size_ ) );
                        $f_r_extension_ = $this_filtered_main_file_extension_."_".$regress_dotted_string_ ;

                        $regress_cmd_string_ = "$MODELSCRIPTS_DIR/kalman_filter.R $indicator_list_filename_ REGDATAFNAME REGOUTFNAME $max_model_size_ $regtype_";
                      }
                      else {
                        print $main_log_file_handle_ "KALMAN line now needs 2 words. Ignoring ".join ( ' ', $scalar_ref_ )."\n";
                      }                                                   				
                    }
                    default
                    {
                      $call_runregressexec_ = 0;
                      print $main_log_file_handle_ "Not handling regtype_ $regtype_ right now \n";
                    }
                  }
                  if ( $call_runregressexec_ ) {
                    if($use_new_regdata_ == 1) {
                      @target_l1norm_model_vec_ = (); #Resetting target l1 norm vector to ensure to only compute on basis of old regdata model
                      my $f_r_extension_old_ = $f_r_extension_."_old";
                      RunRegressExec ( $this_work_dir_, $regtype_, $this_filtered_reg_data_filename_old_, $f_r_extension_old_, $regress_cmd_string_, $cross_validate_arg_string_, $evaluate_optstring_, $place_coeff_script_, $max_error_on_test_, $indicator_list_filename_, \@model_fname_vec_old_,0, 0);
                      my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $model_fname_vec_old_[0] TODAY-80 TODAY-1 $datagen_start_hhmm_ $datagen_end_hhmm_ 2>/dev/null | head -n1";
                      $add_modelinfo_to_model_ = 1;
                      print $main_log_file_handle_ "$exec_cmd\n";
                      my $old_l1norm_model_line_ = `$exec_cmd`;
                      chomp ( $old_l1norm_model_line_);
                      my @old_l1norm_model_words_ = split ( ' ', $old_l1norm_model_line_ );
                      my $old_stdev_model_ = $old_l1norm_model_words_[0];
                      my $old_l1norm_model_ = $old_l1norm_model_words_[1];
                      print $main_log_file_handle_ "Old Stdev: $old_stdev_model_\n";
                      print $main_log_file_handle_ "Old L1Norm: $old_l1norm_model_ \n";
                      push ( @target_l1norm_model_vec_, $old_l1norm_model_ );                   
                      #In case of regimes we want only one target stdev
                      if ( $use_regime_model_ == 0 ) {
                        push ( @target_l1norm_model_vec_, 1.5*$old_l1norm_model_ );                   
                        push ( @target_l1norm_model_vec_, 0.5*$old_l1norm_model_ );                   
                      }
                    } 
                
                    RunRegressExec ( $this_work_dir_, $regtype_, $this_filtered_reg_data_filename_, $f_r_extension_, $regress_cmd_string_, $cross_validate_arg_string_, $evaluate_optstring_, $place_coeff_script_, $max_error_on_test_, $indicator_list_filename_, \@model_fname_vec_, $use_cross_validation_, $obtain_model_from_cross_validation_ );
                    
                    foreach my $this_model_filename_ ( @model_fname_vec_ ) {
                      if ( $this_model_filename_ ) {
                        if ( $use_regime_model_ ) {
                          $individual_model_file_vec_{ $reg_data_counter_ } = $this_model_filename_;
                        }
                        else {
                          AddModelFileToList ( $regtype_, $this_pred_duration_, $this_model_filename_ );
                        }
                      }
                      else {
                        print $main_log_file_handle_ "ERROR missing $this_model_filename_\n";
                      }
                    }
                  }
                }
              }

              if ( $delete_regdata_files_ ) {
                print $main_log_file_handle_ "Removing this filtration\n";
                my @files_to_remove_ = ( $this_filtered_reg_data_filename_ );
                push ( @files_to_remove_, $this_filtered_reg_data_filename_old_ ) if $use_new_regdata_ == 1;

                if ( $obtain_model_from_cross_validation_ ) {
                  push ( @files_to_remove_, $work_dir_."/tmp_train_reg_data" );
                  push ( @files_to_remove_, $work_dir_."/tmp_test_reg_data" );
                }
                CleanFiles ( \@files_to_remove_ );
              }
            }
            
            if ( $delete_regdata_files_ ) {
              my @files_to_remove_ = ( $this_reg_data_filename_ );
              push ( @files_to_remove_, $this_reg_data_filename_old_ ) if $use_new_regdata_ == 1;

              CleanFiles ( \@files_to_remove_ );
            }
          } #end of reg data loop
          
          if ( $use_regime_model_ ) {
            my $are_all_regime_models_present_ = 1;
            foreach my $j ( 0..($num_regimes_-1) ) {
              if ( !exists $individual_model_file_vec_{ $j } ) {
                $are_all_regime_models_present_ = 0;
                last;
              }
            }

            if ( $build_all_regime_models_ == 0 || $are_all_regime_models_present_ ) {
              my $model_file_regime_ = $individual_model_file_vec_{ 0 }."_s";
              MakeRegimeModels ( \%individual_model_file_vec_, $model_file_regime_ );
              if ( ExistsWithSize ( $model_file_regime_ ) ) {
                AddModelFileToList ( "", $this_pred_duration_ , $model_file_regime_);
              }
            }
            else {
              print $main_log_file_handle_ "*Error*: All the regime models could be built for main file extension: $main_file_extension_ \n";
            }
          }
        }
      }
    } #endof pred_duration loop
  } #endof ilist loop

  CleanFiles ( );
}

# this is to use the new way of rescaling models using stdev of a given model
# on a particular set of dates
sub RescaleModelL1Norm
{
  my ( $this_unscaled_model_filename_, $this_model_filename_ ) = @_;
  my $t_pool_tag_ = $trading_start_hhmm_."-".$trading_end_hhmm_ ;
  if ($pool_tag_ ne "")
  {
    $t_pool_tag_ = $t_pool_tag_."-".$pool_tag_;
  }

  my ($stdev_start_yyyymmdd_, $stdev_end_yyyymmdd_, $target_l1norm_model_ ) = GetTargetL1NormForShortcode ( $shortcode_, $datagen_end_yyyymmdd_, $t_pool_tag_ );    
  my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $this_unscaled_model_filename_ $stdev_start_yyyymmdd_ $stdev_end_yyyymmdd_ $trading_start_hhmm_ $trading_end_hhmm_ 2>/dev/null | head -n1";
  print $main_log_file_handle_ "$exec_cmd\n";
  my $current_l1norm_model_line_ = `$exec_cmd`;
  chomp ( $current_l1norm_model_line_);
  my @current_l1norm_model_words_ = split ( ' ', $current_l1norm_model_line_ );
  my $current_stdev_model_ = $current_l1norm_model_words_[0];
  my $current_l1norm_model_ = $current_l1norm_model_words_[1];
  print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
  print $main_log_file_handle_ "Current L1Norm: $current_l1norm_model_ \n";
  my $scale_const_ = $target_l1norm_model_ / $current_l1norm_model_;
  $exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_unscaled_model_filename_ $this_model_filename_ $scale_const_";
  print $main_log_file_handle_ "$exec_cmd\n";
  `$exec_cmd`; 
}

sub RunRegressExec 
{
  my ( $this_work_dir_, $regtype_, $regdata_fname_, $f_r_extension_, $regress_cmd_, $cross_validate_arg_string_, $evaluate_optstring_, $place_coeff_script_, $max_error_on_test_, $indicator_list_filename_, $model_fname_vec_ref_, $this_use_cross_validation_, $this_obtain_model_from_cross_validation_) = @_;
  
  my $this_unscaled_model_filename_ = $this_work_dir_."/unscaled_model_".$f_r_extension_;
  my $this_regression_output_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_;
  my $this_rejected_regout_filename_ = $this_work_dir_."/reg_out_".$f_r_extension_."_reject";

  $regress_cmd_ =~ s/REGOUTFNAME/$this_regression_output_filename_/g;
  $evaluate_optstring_ =~ s/REGOUTFNAME/$this_regression_output_filename_/g;

  if ( $this_use_cross_validation_ == 1 ) {
    $regress_cmd_ = "$MODELSCRIPTS_DIR/cross_validate_model.py $this_work_dir_ $regdata_fname_ $num_folds_ $min_mse_ $cross_validate_arg_string_ > $this_regression_output_filename_";
  }

  elsif ( $this_obtain_model_from_cross_validation_ ) {
    print $main_log_file_handle_ "Using cross validation for obtaining the model\n";
    my $t_train_reg_data_ = $work_dir_."/tmp_train_reg_data";
    my $status_; 
    my $test_error_;
    if ( looks_like_number ( $evaluate_optstring_ ) ) {
      $regress_cmd_ =~ s/REGDATAFNAME/$t_train_reg_data_/g;
      print $main_log_file_handle_ "$regress_cmd_\n";
      `$regress_cmd_`;
      my $exec_cmd="$MODELSCRIPTS_DIR/evaluate_model.py $evaluate_optstring_ $this_regression_output_filename_ $work_dir_/tmp_test_reg_data $max_error_on_test_";
      print $main_log_file_handle_ "$exec_cmd\n";
      my $dump_ = `$exec_cmd`; chomp($dump_);
      $status_ = `echo $dump_ | awk '{print \$1}'`;
      $test_error_ = `echo $dump_ | awk '{print \$2}'`;
    } 
    else {
      my $t_statusfile_ = $work_dir_."/statusfile_".$f_r_extension_;
      $evaluate_optstring_ =~ s/REGDATAFNAME/$t_train_reg_data_/g;
      $evaluate_optstring_ =~ s/STATUSFILE/$t_statusfile_/g;
      print $main_log_file_handle_ "$evaluate_optstring_\n";
      `$evaluate_optstring_`;
      $status_ = `cat $t_statusfile_ | head -n1`;
      $test_error_ = `cat $t_statusfile_ | tail -n1 | awk '{print \$NF}'`;
    }

    if ( $status_ == 1) {
      $number_of_models_++;
      print $main_log_file_handle_ "Model is robust.Proceeding with pnl analsysis\n";
    }
    else {
      print $main_log_file_handle_ "Model is not robust.Removing it from analysis\n";
      $failure_string_ = $failure_string_."Model $cross_validate_arg_string_ ( $this_rejected_regout_filename_ ) trained on $regdata_fname_ had to clear error cutoff of $max_error_on_test_ as it had a training error of ".$max_error_on_test_/(1+$cross_validation_cutoff_)." .It failed by producing a test error of $test_error_\n";
      print $main_log_file_handle_ $failure_string_;
      `mv $this_regression_output_filename_ $this_rejected_regout_filename_`;
      return "";
    }
  }

  else {
    $regress_cmd_ =~ s/REGDATAFNAME/$regdata_fname_/g;
    print $main_log_file_handle_ "$regress_cmd_\n";
    my @regoutlines_ = `$regress_cmd_`; chomp ( @regoutlines_ );
    print $main_log_file_handle_ "@regoutlines_";

## for FS* regtypes and for MULTLR, print the high sharpe indicators that have not been considered in the regress-algos
    if ( $regtype_ =~ /(^FS)|MULTLR/ ) {
      my $is_dep_high_sharpe_ = 0;
      my @high_sharpe_indep_indices_ = ();

      foreach my $regoutline_ ( @regoutlines_ ) {
        if ( $regoutline_ =~ /Sharpe of indep/ ) {
          my $high_sharpe_line_ = $regoutline_;
          my @high_sharpe_line_words_ = split ( ' ', $high_sharpe_line_ );
          if ( $#high_sharpe_line_words_ >= 4 ) {
            push ( @high_sharpe_indep_indices_, $regoutline_ );
          }
        }

        if ( $regoutline_ =~ /Sharpe of the dependant/ ) {
          $is_dep_high_sharpe_ = 1;
          print $main_log_file_handle_ "For $regdata_fname_ Dependant has high sharpe\n";
        }
      }
      @high_sharpe_indep_indices_ = GetUniqueList ( @high_sharpe_indep_indices_ );
      my @high_sharpe_indep_text_ = GetHighSharpeIndepText ( $indicator_list_filename_, @high_sharpe_indep_indices_ );
      
      for ( my $hsit_idx_ = 0 ; $hsit_idx_ <= $#high_sharpe_indep_text_ ; $hsit_idx_ ++ ) {
        print $main_log_file_handle_ "For $regdata_fname_ HIGH SHARPE INDEP: ".$high_sharpe_indep_text_[$hsit_idx_]."\n";
      }
    }
  }
  
  if ( ExistsWithSize ( $this_regression_output_filename_ ) ) {
    my $exec_cmd = "";
    if( $use_continuous_regime_model_ ) {
      my $ilist_file_ = $indicator_list_filename_."_CRR";
      AddIndicatorAfterIndicatorStart ( $indicator_list_filename_, $ilist_file_, $continuous_regime_indicator_ );
      $exec_cmd = "$place_coeff_script_ $this_unscaled_model_filename_ $ilist_file_ $this_regression_output_filename_ ";
    } else {
      $exec_cmd = "$place_coeff_script_ $this_unscaled_model_filename_ $indicator_list_filename_ $this_regression_output_filename_ ";
    }
    if ( $place_coeff_script_ eq "" ) {
      $exec_cmd="cp $this_regression_output_filename_ $this_unscaled_model_filename_";
    }
    print $main_log_file_handle_ "$exec_cmd\n";
    `$exec_cmd`;
  }

  if ( ExistsWithSize ( $this_unscaled_model_filename_ ) )  {
    print $main_log_file_handle_ "Target model l1norms: ".join(" ", @target_l1norm_model_vec_)."\n";

    if ((!$use_norm_stdev_) &&  ($use_regime_model_ || ( $#target_l1norm_model_vec_ < 0 ) || ( ( $#target_l1norm_model_vec_ == 0 ) && ( $target_l1norm_model_vec_[0] <= 0 ))))
    { # if either TARGET_STDEV_MODEL was not specified or 
      my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;

      print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
      `mv $this_unscaled_model_filename_ $this_model_filename_`;

      if ( ExistsWithSize ( $this_model_filename_ ) ) {
        push ( @$model_fname_vec_ref_, $this_model_filename_ );
      }
    }
    else
    { # Probably at least 1 non-trivial ( > 0 ) TARGET_STDEV_MODEL was specified OR Norm Stdev is mentioned 
      my $current_stdev_model_;
      my $current_l1norm_model_;

      if ($pick_stdev_from_model_file) {
        my $exec_cmd_std = "cat $this_unscaled_model_filename_ | grep StDev | tail";
        my $result = `$exec_cmd_std`;
        my @words = split ' ',$result;
        my ($sd_idx_) = grep { $words[$_] =~ /StDev/ } 0..($#words-1);

        if ( defined $sd_idx_ ) {
          $current_stdev_model_ = $words[$sd_idx_+1];
          $current_l1norm_model_ = $words[$sd_idx_+1];;
        }
      }

      my $exec_cmd = "$MODELSCRIPTS_DIR/get_stdev_model.pl $this_unscaled_model_filename_ TODAY-80 TODAY-1 $datagen_start_hhmm_ $datagen_end_hhmm_ 2>/dev/null | head -n1";
      if ( ! defined $current_stdev_model_ || ! defined $current_l1norm_model_ ) {
        print $main_log_file_handle_ "$exec_cmd\n";
        my $current_l1norm_model_line_ = `$exec_cmd`;
        chomp ( $current_l1norm_model_line_);

        my @current_l1norm_model_words_ = split ( ' ', $current_l1norm_model_line_ );
        $current_stdev_model_ = $current_l1norm_model_words_[0];
        $current_l1norm_model_ = $current_l1norm_model_words_[1];
      }
      print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
      print $main_log_file_handle_ "Current L1Norm: $current_l1norm_model_ \n";

      if ( $current_l1norm_model_ <= 0 && ! $use_norm_stdev_ )   # changed from stdev to l1norm
      { # error case ... no real reason why current stdev of model should be < 0 
        my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;

        print $main_log_file_handle_ "mv $this_unscaled_model_filename_ $this_model_filename_\n";
        `mv $this_unscaled_model_filename_ $this_model_filename_`;

        if ( ExistsWithSize ( $this_model_filename_ ) ) {
          push ( @$model_fname_vec_ref_, $this_model_filename_ );
        }
      }
      elsif ( $use_norm_stdev_ ) {
        my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_;

        RescaleModelL1Norm( $this_unscaled_model_filename_, $this_model_filename_ );
        push ( @intermediate_files_, $this_unscaled_model_filename_ );

        if ( ExistsWithSize ( $this_model_filename_ ) ) {
          push ( @$model_fname_vec_ref_, $this_model_filename_ );
        }
      }
      else {
        my $rs_i_ = 0;
        foreach my $target_l1norm_model_ ( @target_l1norm_model_vec_ ) {
          my $this_model_filename_ = $this_work_dir_."/w_model_".$f_r_extension_."_".$rs_i_;
          
          if ( $target_l1norm_model_ > 0 ) {
            my $scale_const_ = $target_l1norm_model_ / $current_l1norm_model_ ;
            if ( $scale_const_ < 0 ) {
              print $main_log_file_handle_ "cp $this_unscaled_model_filename_ $this_model_filename_\n";
              `cp $this_unscaled_model_filename_ $this_model_filename_`;
            }
            else {
              print $main_log_file_handle_ "For target_l1norm_model $target_l1norm_model_ and Current : $current_l1norm_model_ scale_const = $scale_const_\n";
              $exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_unscaled_model_filename_ $this_model_filename_ $scale_const_";
              print $main_log_file_handle_ "$exec_cmd\n";
              `$exec_cmd`;

              my @l1norm_vec_ = ($target_l1norm_model_);
              AddL1NormToModel ( $this_model_filename_, \@l1norm_vec_);
            }
          }
          else { # -1: disable
            print $main_log_file_handle_ "cp $this_unscaled_model_filename_ $this_model_filename_\n";
            `cp $this_unscaled_model_filename_ $this_model_filename_`;
          }

          if ( ExistsWithSize ( $this_model_filename_ ) ) {
            push ( @$model_fname_vec_ref_, $this_model_filename_ );
          }
          $rs_i_ ++;
        }
      }
    }
  }
}

sub GenerateRegdataFiles
{
  my ( $ilist_filename_, $this_pred_duration_, $this_predalgo_, $this_reg_data_filename_vec_ref_, $this_use_new_regdata_, $this_lower_threshold_, $this_time_cap_,$this_filter_uncertain_values_ ) = @_;

  SetFvFsudmFilterArguments ( );

  foreach my $sub_dir_ ( $reg_data_dir, $reg_data_daily_dir, $timed_data_daily_dir ) {
    `mkdir -p $sub_dir_` if ( ! -d $sub_dir_ );
  }

  foreach my $this_reg_data_filename_ ( @$this_reg_data_filename_vec_ref_ ) {
    if ( -e $this_reg_data_filename_ ) {
      print $main_log_file_handle_ "WARNING: file $this_reg_data_filename_ already present\n";
      `rm -f $this_reg_data_filename_ 2>/dev/null`;
    }
  }

  my @ilist_vec_ = ( );
  if ( $use_regime_ilists_ ) {
    @ilist_vec_ = @{ $regime_ilist_to_ilist_vec_map_{ $ilist_filename_ } };
  }
  else {
    push ( @ilist_vec_, $ilist_filename_ );
  }
  my %ilist_to_datagen_failure_days_ = ( );
  my %ilist_to_periodfilter_failure_days_ = ( );

## generate reg data of this type
  print $main_log_file_handle_ "generate reg data of this type\n";
  print $main_log_file_handle_ "NUMDAYS $#datagen_day_vec_\n";

  foreach my $tradingdate_ ( @datagen_day_vec_ ) {
    print $main_log_file_handle_ "\nDatagen date $tradingdate_\n";
# for this trading date generate the reg_data_file

    my @this_day_reg_data_filename_vec_ = ();

    foreach my $t_iter ( 0..$#ilist_vec_ ) {
      my $indicator_list_filename_ = $ilist_vec_[ $t_iter ];
      my $indicator_list_filename_base_ = basename( $indicator_list_filename_ );

      my $this_day_file_extension_ = $indicator_list_filename_base_."_".$this_pred_duration_."_".$this_predalgo_."_".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_."_".$unique_gsm_id_ ; 
      my $this_day_reg_data_filename_ = $reg_data_daily_dir."/reg_data_".$this_day_file_extension_;
      my $this_day_corr_filename_ = $reg_data_daily_dir."/corr_".$this_day_file_extension_;
      my $this_day_var_corr_filename_ = $reg_data_daily_dir."/var_corr_".$this_day_file_extension_;

      push ( @intermediate_files_, $this_day_corr_filename_ ) if ( $SAVE_CORR_FILE == 0 );
      push ( @intermediate_files_, $this_day_var_corr_filename_ ) if ( $SAVE_VAR_CORR_FILE == 0 );

      my $this_day_timed_data_filename_ = $timed_data_daily_dir."/timed_data_".$indicator_list_filename_base_."_".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_."_".$unique_gsm_id_ ;

      if ( !( -e $this_day_timed_data_filename_ ) ) { 
# generate this_day_timed_data_filename_
# check in indicator file if all sources are present for the given date
# if not, do not run datagen for this date
        if ( CheckIndicatorData($tradingdate_, $indicator_list_filename_) == 1) {
          print $main_log_file_handle_ "Skipping datagen for $indicator_list_filename_ for date $tradingdate_\n";
          next;
        }
        my $ilist_file_ = CreateTempIlist ( $indicator_list_filename_, $tradingdate_ );

        next if ( ! GenerateTimedData ($ilist_file_, $tradingdate_, $this_day_timed_data_filename_, $t_iter, \%ilist_to_datagen_failure_days_) );

        $this_day_timed_data_filename_ = ApplyPeriodFilter ($this_day_timed_data_filename_, $ilist_file_, $tradingdate_, \%ilist_to_periodfilter_failure_days_);

        next if ( ! $this_day_timed_data_filename_ );
      }

      if ( -e $this_day_timed_data_filename_ ) {
# if now timed_data_file exists and has non zero size
        my $this_day_reg_data_filename_ = $reg_data_daily_dir."/reg_data_".$this_day_file_extension_;

        TimedDataToRegData ( $indicator_list_filename_, $this_day_timed_data_filename_, $this_day_reg_data_filename_, \@this_day_reg_data_filename_vec_, $tradingdate_, $this_predalgo_, $this_pred_duration_, $this_use_new_regdata_, $this_lower_threshold_, $this_time_cap_, $this_filter_uncertain_values_ );

# see whether this data should be excluded if the stats of the dependant are abnormal
# see which columns should be excluded, a column may be exculded if it's peak sharpe is very high
        if ( $SAVE_VAR_CORR_FILE != 0 ) {
          if ( -e $this_day_reg_data_filename_ ) {
            my $exec_cmd="$LIVE_BIN_DIR/get_dep_corr $this_day_reg_data_filename_ > $this_day_corr_filename_";
            print $main_log_file_handle_ "$exec_cmd\n";
            `$exec_cmd`;

            $exec_cmd="$MODELSCRIPTS_DIR/sort_by_corr.pl $indicator_list_filename_ $this_day_corr_filename_ > $this_day_var_corr_filename_";
            print $main_log_file_handle_ "$exec_cmd\n";				    
            `$exec_cmd`;
          }
        }
      }
    }

    if ( $#this_day_reg_data_filename_vec_ < $#$this_reg_data_filename_vec_ref_ ) {
      print $main_log_file_handle_ "ERROR: Could not generate RegData for date: ".$tradingdate_."\n";
      next;
    }

    foreach my $count_ ( 0 .. $#this_day_reg_data_filename_vec_ ) {
      if ( -e $this_day_reg_data_filename_vec_[ $count_ ] )
      {
# append this day's file to the global reg data file
        my $exec_cmd_ = "awk '{print \$0,$tradingdate_}' ".$this_day_reg_data_filename_vec_[ $count_ ]." >> ".$$this_reg_data_filename_vec_ref_[ $count_ ];
        print $main_log_file_handle_ $exec_cmd_."\n";
        `$exec_cmd_`;
      }
      else {
        print $main_log_file_handle_ "WARN: $this_day_reg_data_filename_vec_[ $count_ ] does not exist\n";
      }
    }

    CleanFiles ( \@this_day_reg_data_filename_vec_ );
  }

  foreach my $this_reg_data_filename_ ( @$this_reg_data_filename_vec_ref_ ) {

    if ( ! -e $this_reg_data_filename_ ) {
      my $err_str_ = "RegData file does not exist: " . $this_reg_data_filename_."\n";
      
      foreach my $ilist_file_ ( @ilist_vec_ ) {
        if ( exists $ilist_to_datagen_failure_days_{ $ilist_file_ } ) {
         $err_str_ .= "Datagen was not successful for ilist $ilist_file_ for dates ".join(" ", @{$ilist_to_datagen_failure_days_{ $ilist_file_ }})."\n";
        }
        if ( exists $ilist_to_periodfilter_failure_days_{ $ilist_file_ } ) {
         $err_str_ .= "PeriodFilter was not successful for ilist $ilist_file_ for dates ".join(" ", @{$ilist_to_periodfilter_failure_days_{ $ilist_file_ }})."\n";
        }
      }
      print $main_log_file_handle_ "\n*Error*: $err_str_\n";

      PrintStacktraceAndSendMailDie ( $err_str_ );
    }
    elsif ( ! $use_regime_model_ ) {
      if ( $filter_zero_inds_ == 1 ) {
        my $this_reg_data_filename_temp_ = $this_reg_data_filename_."_zero_inds_removed";
        my $ilist_filename_temp_ = $ilist_filename_."_zero_inds_removed";
        my $exec_cmd_2_="$MODELSCRIPTS_DIR/filter_zero_inds.pl $ilist_filename_ $this_reg_data_filename_ $ilist_filename_temp_ $this_reg_data_filename_temp_";
        print $main_log_file_handle_ "$exec_cmd_2_\n";
        `$exec_cmd_2_`;
        $this_reg_data_filename_ = $this_reg_data_filename_temp_;
        $ilist_filename_ = $ilist_filename_temp_;
      }
    }
  }

  return $ilist_filename_;
}

sub SetFvFsudmFilterArguments
{
  $apply_trade_vol_filter = 0;
  $fsudm_level_ = 0;

  foreach my $this_filter_ ( @dep_based_filter_ ) {
# for every filtration of the main file
    $apply_trade_vol_filter = 1 if $this_filter_ eq "fv";

    if ( $this_filter_ eq "fsudm1" || $this_filter_ eq "fsudm" ) {
      $fsudm_level_ = 1;
    }
    elsif ( $this_filter_ eq "fsudm2" ) {
      $fsudm_level_ = 2 ;
    }
    elsif ( $this_filter_ eq "fsudm3" ) {
      $fsudm_level_ = 3 ;
    }
  }
}

sub CreateTempIlist
{
  my ($indicator_list_filename_, $tradingdate_) = @_;

  my $ilist_file_ = $indicator_list_filename_."_copy";

  if( $use_continuous_regime_model_ ) {
    $ilist_file_ = $indicator_list_filename_."_CRR";
    AddIndicatorAfterIndicatorStart ( $indicator_list_filename_, $ilist_file_, $continuous_regime_indicator_ );
  }
  elsif ( $use_regime_model_ ) {
# in case of regime_model, make regime ilists
    $ilist_file_ = $indicator_list_filename_."_regime";
    AddIndicatorAfterIndicatorStart ( $indicator_list_filename_, $ilist_file_, $regime_indicator_ );
  }
  else {
    `cp $indicator_list_filename_ $ilist_file_`;
  }

  if ( $norm_regdata_stdev_indicator_ ) {
    my $t_ilist_file_ = $ilist_file_."_norm";
    AddIndicatorBeforeIndicatorEnd ( $ilist_file_, $t_ilist_file_, $norm_regdata_stdev_indicator_ );
    `mv $t_ilist_file_ $ilist_file_`;
  }

  if ( $tradingdate_ < 20130901 && $shortcode_ =~ /DI1/ )
  {
    my $t_ilist_file_ = $ilist_file_."_dichange";
    my $exec_cmd_ = "cat $ilist_file_ |  sed 's/DI_7/DI_8/g' | sed 's/DI_6/DI_7/g' | sed 's/DI_5/DI_6/g' | sed 's/DI_4/DI_5/g' | sed 's/DI_3/DI_4/g' | sed 's/DI_2/DI_3/g' | sed 's/DI_1/DI_2/g' | sed 's/DI_0/DI_1/g' > $t_ilist_file_ ";
    `$exec_cmd_`;
    `mv $t_ilist_file_ $ilist_file_`;
  }

  if ( $use_trade_price_ && rand(1) > 0.50  )
  {
    my $t_ilist_file_ = $ilist_file_."_tradepx";
    my $exec_cmd_ = "cat $ilist_file_ | sed 's/OfflineMixMMS/TradeOmixPrice/g' | sed 's/Midprice/TradeBasePrice/g' | sed 's/MidPrice/TradeBasePrice/g' | sed 's/MktSizeWPrice/TradeMktSizeWPrice/g' | sed 's/TradeWPrice/TradeTradeWPrice/g'| sed 's/OrderWPrice/TradeOrderWPrice/g' | sed 's/MktSinusoidal/TradeMktSinPrice/g' > $t_ilist_file_ " ;
    `$exec_cmd_`;
    `mv $t_ilist_file_ $ilist_file_`;
  }

  return $ilist_file_;
}

sub ApplyPeriodFilter
{
  my ($this_day_timed_data_filename_, $ilist_file_, $tradingdate_, $ilist_to_periodfilter_failure_days_ref_) = @_;

# applying Period Filter on the timed_data
  my $this_day_period_filter_timed_data_filename_ = "";
  my $period_filter_cmd_ = "";
  
  if ( $large_price_move_periods_ ) {
# Filter for only periods with large directional price moves.
    $this_day_period_filter_timed_data_filename_ = $this_day_timed_data_filename_."_large_price";
    $period_filter_cmd_ = "$MODELSCRIPTS_DIR/select_timed_data_rows_with_price_moves.pl $shortcode_ $this_day_timed_data_filename_ $this_day_period_filter_timed_data_filename_ $large_price_move_time_period_ $large_price_move_thresh_factor_";
  }

  if ( $bad_periods_only_ ) {
    $this_day_period_filter_timed_data_filename_ = $this_day_timed_data_filename_."_bad_periods";
    $period_filter_cmd_ = "$MODELSCRIPTS_DIR/select_timed_data_rows_for_bad_periods.pl $this_day_timed_data_filename_ $this_day_period_filter_timed_data_filename_ $shortcode_ $tradingdate_";
  }

  if ( $periodfilter_samples_only_ ) {
    $this_day_period_filter_timed_data_filename_ = $this_day_timed_data_filename_."_periodfilter_samples";
    $period_filter_cmd_ = "$MODELSCRIPTS_DIR/select_timed_data_rows_for_bad_samples.pl $this_day_timed_data_filename_ $this_day_period_filter_timed_data_filename_ $shortcode_ $tradingdate_ $periodfilter_samples_filename_";
  }

  if ( $this_day_period_filter_timed_data_filename_ ) {
    print $main_log_file_handle_ "$period_filter_cmd_\n";
    `$period_filter_cmd_`;

    if ( ExistsWithSize ( $this_day_period_filter_timed_data_filename_ ) ) {
      print $main_log_file_handle_ "Adding $this_day_period_filter_timed_data_filename_ to be deleted later\n" ;
      push ( @intermediate_files_, $this_day_period_filter_timed_data_filename_ ); # if this is a new file we created then add it to set of files to delete at the end
      return $this_day_period_filter_timed_data_filename_;
    }
    else {
      push ( @{$$ilist_to_periodfilter_failure_days_ref_{ $ilist_file_ }}, $tradingdate_ );
      print $main_log_file_handle_ "Could not create non-zero-sized $this_day_period_filter_timed_data_filename_\n" ;
      CleanFiles ( [ $this_day_period_filter_timed_data_filename_ ] );
      return 0;
    }
  }

  return $this_day_timed_data_filename_;
}

sub GenerateTimedData
{
  my ($ilist_file_, $tradingdate_, $this_day_timed_data_filename_, $t_iter, $ilist_to_datagen_failure_days_ref_) = @_;
  my $exec_cmd_ = "";
  my $regime_str_ = "";
  $regime_str_ = "REGIME ".($t_iter + 1 )." -1" if ( $use_regime_ilists_ > 0 );

  if ( $print_only_on_traded_ezone_ == 1 ) {				    	
    $exec_cmd_="$LIVE_BIN_DIR/datagen $ilist_file_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ 3 USE_FAKE_FASTER_DATA $use_fake_faster_data_ $sampling_shortcodes_str_ $regime_str_ $traded_ezone_ ADD_DBG_CODE -1 2>&1";
  }
  else {
    $exec_cmd_="$LIVE_BIN_DIR/datagen $ilist_file_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ USE_FAKE_FASTER_DATA $use_fake_faster_data_ $sampling_shortcodes_str_ $regime_str_ ADD_DBG_CODE -1 2>&1";
  }

  print $main_log_file_handle_ "$exec_cmd_\n";
  my @datagen_output_lines_ = `$exec_cmd_`;

# check if this datagen aborted because of disk space reasons , if so stop and terminate right now.
  foreach my $datagen_line_ ( @datagen_output_lines_ ) {
    if ( index ( $datagen_line_ , "Less Space for Datagen" ) >= 0 ) {
# this datagen call exited due to a lack of diskspace , proceed no further.
      PrintStacktraceAndSendMailDie ( "Exiting because datagen exited with output : Less Space for Datagen" );
    }
  }

  my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
  CleanFiles ( [ $datagen_logfile_ ] );

  if ( ExistsWithSize ( $this_day_timed_data_filename_ ) ) {
    print $main_log_file_handle_ "Adding $this_day_timed_data_filename_ to be deleted later\n" ;
    push ( @intermediate_files_, $this_day_timed_data_filename_ ); # if this is a new file we created then add it to set of files to delete at the end
    return 1;
  }
  else {
    push ( @{$$ilist_to_datagen_failure_days_ref_{ $ilist_file_ }}, $tradingdate_ );
    print $main_log_file_handle_ "Could not create non-zero-sized $this_day_timed_data_filename_\n" ;
    CleanFiles ( [ $this_day_timed_data_filename_ ] );
    return 0;
  }
}

sub TimedDataToRegData
{
  my ( $indicator_list_filename_, $this_day_timed_data_filename_, $this_day_reg_data_filename_, $this_day_reg_data_filename_vec_ref_, $tradingdate_, $this_predalgo_, $this_pred_duration_, $this_use_new_regdata_, $this_lower_threshold_, $this_time_cap_, $this_filter_uncertain_values_ ) = @_;
  my $exec_cmd="";

  if ( $this_predalgo_ ne "na_t3_bd" ) {
    my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( $shortcode_ , $this_pred_duration_, $this_predalgo_, $this_day_timed_data_filename_ );

    my $trade_per_sec_file = "INVALIDFILE";

    if ( $apply_trade_vol_filter == 1 ) {
#do the trade volume file generation part
      $trade_per_sec_file = $DATAGEN_LOGDIR.int( rand(10000) )."_".$shortcode_."_trd_per_sec";
      my $exec_cmd="$LIVE_BIN_DIR/daily_trade_aggregator $shortcode_ $tradingdate_ $trade_per_sec_file";
      print $main_log_file_handle_ "$exec_cmd\n";
      my @daily_trade_agg_output_lines_ = `$exec_cmd`;
      print $main_log_file_handle_ @daily_trade_agg_output_lines_."\n";
    }
    
    if($this_use_new_regdata_ == 1) {
      $exec_cmd="$SCRIPTS_BASE_DIR/timed_data_to_reg_data.R $shortcode_ $indicator_list_filename_ $tradingdate_ $this_day_timed_data_filename_ $this_pred_duration_ $this_lower_threshold_ $this_time_cap_ $this_day_reg_data_filename_ $this_filter_uncertain_values_ $trade_per_sec_file 0 $fsudm_level_"; 
      print $main_log_file_handle_ "$exec_cmd\n";
    }
    elsif ( $this_predalgo_ eq "na_mult") {
      $exec_cmd="$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_filename_ $this_day_timed_data_filename_ $predconfig_[0] $this_predalgo_ $this_day_reg_data_filename_ $trade_per_sec_file $use_simple_sim_ $fsudm_level_";
      print $main_log_file_handle_ "$exec_cmd\n";
    }
    else {     
      $exec_cmd="$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_filename_ $this_day_timed_data_filename_ $this_pred_counters_ $this_predalgo_ $this_day_reg_data_filename_ $trade_per_sec_file $use_simple_sim_ $fsudm_level_";
      print $main_log_file_handle_ "$exec_cmd\n";
    }
    `$exec_cmd`;

    if ( $apply_trade_vol_filter == 1 ) {
      CleanFiles ( [ $trade_per_sec_file ] );
    }
  }
  else {
    my $this_pred_counters_ = $this_pred_duration_ * 1000;
    $exec_cmd="$LIVE_BIN_DIR/timed_data_to_reg_data_bd $this_day_timed_data_filename_ $this_day_reg_data_filename_ $this_pred_counters_ 1000 $min_price_increment_";
    `$exec_cmd`;
  }

## if use_regime_model_ replace the regime indicator column of regdata with that in timeddata
  if ( $use_regime_model_ && $use_regime_ilists_ < 1 )
  {
    open ( REGDATAIN, "<", $this_day_reg_data_filename_ ) or PrintStacktraceAndDie ( "Could not open the input  $this_day_reg_data_filename_ \n" );
    my @reg_data_lines_ = <REGDATAIN>; chomp ( @reg_data_lines_ ); close ( REGDATAIN );
    open ( TIMEDDATA, "<", $this_day_timed_data_filename_ ) or PrintStacktraceAndDie ( "Could not open the input  $this_day_timed_data_filename_ \n" );
    my @timed_data_lines_ = <TIMEDDATA>; chomp ( @timed_data_lines_ ); close (TIMEDDATA);
    open ( REGDATAOUT, ">", $this_day_reg_data_filename_ ) or PrintStacktraceAndDie ( "Could not open the file $this_day_reg_data_filename_ for writing \n" );
    
    for ( my $i=0; $i<=$#reg_data_lines_ && $i<=$#timed_data_lines_; $i++ )
    {
      my @reg_line_words_ = split (' ', $reg_data_lines_[ $i ] );
      my @timed_line_words_ = split (' ', $timed_data_lines_[ $i ] );
      $reg_line_words_[ 1 ] = $timed_line_words_[ 4 ];
      my $line_ = join(' ', @reg_line_words_ );
      print REGDATAOUT "$line_\n";
    }
    close (REGDATAOUT);

    my $this_day_regimed_reg_data_out_file_name_ = $reg_data_daily_dir."/daily_timed_split_".basename($this_day_reg_data_filename_);
    @$this_day_reg_data_filename_vec_ref_ = map { $this_day_regimed_reg_data_out_file_name_."_".$_ } 1..$num_regimes_;

    SplitRegimeRegdata ( $this_day_reg_data_filename_, $this_day_reg_data_filename_vec_ref_ );
  }
  else {
    push ( @$this_day_reg_data_filename_vec_ref_, $this_day_reg_data_filename_);
  }
}

sub AddIndicatorAfterIndicatorStart
{
  my ( $src_ilist_, $dest_ilist_, $indicator_ ) = @_;
  return if $indicator_ eq "";
  
  open ILIST, "< $src_ilist_";
  open (ILIST_REG, ">", $dest_ilist_);
  my @ilist_lines_ = <ILIST>; chomp ( @ilist_lines_ );
  
  foreach my $t_line_ ( @ilist_lines_ ) {
    print ILIST_REG $t_line_."\n";
    if ( $t_line_ =~ /^INDICATORSTART/ ) {
      print ILIST_REG "INDICATOR 1.0 ".$indicator_."\n";
    }
  }
  close ILIST_REG;
  close ILIST;
}

sub AddIndicatorBeforeIndicatorEnd
{
  my ( $src_ilist_, $dest_ilist_, $indicator_ ) = @_;
  return if $indicator_ eq "";

  open ILIST, "< $src_ilist_";
  open (ILIST_REG, ">", $dest_ilist_);
  my @ilist_lines_ = <ILIST>; chomp ( @ilist_lines_ );

  foreach my $t_line_ ( @ilist_lines_ ) {
    if ( $t_line_ =~ /^INDICATOREND/ ) {
      print ILIST_REG "INDICATOR 1.0 ".$indicator_."\n";
    }
    print ILIST_REG $t_line_."\n";
  }
  close ILIST_REG;
  close ILIST;
}

sub SplitRegimeRegdata
{
  my ( $this_day_reg_data_filename_, $this_day_regimed_reg_data_filename_vec_ref_ ) = @_;

  print $main_log_file_handle_ "Splitting the reg data \n";
  print $main_log_file_handle_ "The regime regdata files being ".join( " ", @$this_day_regimed_reg_data_filename_vec_ref_ );
  print $main_log_file_handle_ "$this_day_reg_data_filename_ \n";
  
  open REGIMEDREG, "< $this_day_reg_data_filename_";
  
  my @regime_reg_data_file_handles_ = ();
  foreach my $file_ ( @$this_day_regimed_reg_data_filename_vec_ref_ ) {
    open my $file_handle_, "> $file_";
    push ( @regime_reg_data_file_handles_, $file_handle_ );
  }

  my $this_day_reg_data_filename_copy_ = $this_day_reg_data_filename_."_copy";
  open REGDCOPY, "> $this_day_reg_data_filename_copy_";

  while ( my $line_ = <REGIMEDREG> ) {
    chomp ( $line_ );
    my @words_ = split ' ',$line_;
    my $regime_ = $words_[1];
    next if ( $regime_ < 1 || $regime_ > $num_regimes_ );

    my $fh_ = $regime_reg_data_file_handles_[ $regime_-1 ];

    print $fh_ $words_[0]." ".join( " ", @words_[2..$#words_] )."\n";

    if ( $use_mirrored_data_ ) {
#to get a zero-mean data, taking image of every data point wrt origin
      my @mirrored_words_ = map { -1 * $_ } @words_;
      print $fh_ $mirrored_words_[0]." ".join( " ", @mirrored_words_[2..$#words_] )."\n";
    }

    print REGDCOPY $words_[0]." ".join( " ", @words_[2..$#words_] )."\n";
  }

  foreach my $fh_ ( @regime_reg_data_file_handles_ ) { close $fh_; }
  close REGDCOPY;

  my $cpy_cmd_ = "cp $this_day_reg_data_filename_ $this_day_reg_data_filename_copy_";
  `$cpy_cmd_`;

  push ( @intermediate_files_, @$this_day_regimed_reg_data_filename_vec_ref_ );
  push ( @intermediate_files_, $this_day_reg_data_filename_ );
}

sub FilterRegdata
{
  my ( $this_reg_data_filename_, $this_filter_, $this_filtered_reg_data_filename_, $this_norm_regdata_stdev_indicator_ ) = @_;

  if ( ! ExistsWithSize ( $this_filtered_reg_data_filename_ ) )
  {
    
    my $exec_cmd="$MODELSCRIPTS_DIR/apply_dep_filter.pl $shortcode_ $this_reg_data_filename_ $this_filter_ $this_filtered_reg_data_filename_ $yyyymmdd_ " ;
    print $main_log_file_handle_ "$exec_cmd\n";
    `$exec_cmd`;
    

    if($#datagen_day_vec_ > 100) {
      my $temp_file_ = $this_filtered_reg_data_filename_."_tmp";
      `awk '{if(NR%4==0){print \$_;}}' $this_filtered_reg_data_filename_ > $temp_file_ `;
      `mv $temp_file_ $this_filtered_reg_data_filename_`;
    }
  }

  if( $this_norm_regdata_stdev_indicator_ ) {
    my $normalize_regdata_ = "$MODELSCRIPTS_DIR/normalize_regdata.R $this_filtered_reg_data_filename_";
    print $main_log_file_handle_ $normalize_regdata_."\n";
    `$normalize_regdata_ 2>/dev/null`;
  }
}

sub ChooseModelFromCrossValidation
{
  my ( $this_filtered_reg_data_filename_, $indicator_list_filename_ ) = @_;
## In obtain_model_from_cross_validation, some regress algos are not supported, remove them
## Then choose the best performing regress_algo choice from obtain_model_from_cross_validation.py
## From the non-supported regress_algos choose one randomly to run as well.
  my $arg_to_pass_ = "";
  my @regress_exec_choices_indices_without_crossval_supp_ = ();

  for ( my $i = 0 ; $i <= $#regress_exec_choices_ ; $i++) {
    my @scalar_ref_ = @{$regress_exec_choices_[$i]};

    if ( $scalar_ref_[0] =~ /^(CONSOLIDATED|COARSE|)(TREE|)BOOSTING$/ ) {
      $arg_to_pass_ = $arg_to_pass_." ".join(',',($scalar_ref_[0],$scalar_ref_[1],$scalar_ref_[2],$scalar_ref_[3],$indicator_list_filename_));
    }
    elsif ( $scalar_ref_[0] ne "SOM" && 
        $scalar_ref_[0] ne "MLOGIT" &&
        $scalar_ref_ [0] ne "NEWMLOGIT" ) {
      $arg_to_pass_ = $arg_to_pass_." ".join(',',@scalar_ref_);
    }
    else {
      push ( @regress_exec_choices_indices_without_crossval_supp_, $i);
    }
  }

  @regress_exec_ = ();
  if ($arg_to_pass_ ne "")
  {
    print $main_log_file_handle_ "copying $this_filtered_reg_data_filename_ to rbit\n";
    my $exec_cmd_ = "$MODELSCRIPTS_DIR/obtain_model_from_cross_validation.py $work_dir_ $this_filtered_reg_data_filename_ $cross_validation_cutoff_ $num_folds_ $arg_to_pass_";
    print $main_log_file_handle_ "Argument = ".$exec_cmd_."\n ";
    my $regress_exec_text_ = `$exec_cmd_`;
    print $main_log_file_handle_ "Algorithm selected : ".$regress_exec_text_."\n";
    my @regress_exec_words_ = split ( ' ', $regress_exec_text_ );
    $max_error_on_test_ = $regress_exec_words_[$#regress_exec_words_];		#maximum tolerance to be allowed on the test data
      pop @regress_exec_words_;
    push ( @regress_exec_ , [ @regress_exec_words_ ] );
  }

  if ( $#regress_exec_choices_indices_without_crossval_supp_ >= 0  ) {
    if ( $RUN_SINGLE_REGRESS_EXEC ) {
      my $t_random_index_ = int ( rand ( $#regress_exec_choices_indices_without_crossval_supp_ + 1 ) );
      my $t_random_regress_exec_ = $regress_exec_choices_ [ $regress_exec_choices_indices_without_crossval_supp_[$t_random_index_] ];

      if ( $#regress_exec_ < 0 || rand(1.0) > 0.5 ) {
        @regress_exec_ = ( $t_random_regress_exec_ );
      }  
    }  
    else {
#add all regress execs which dont have support for cross_val along with the one which was selected from crossval
      push ( @regress_exec_ , @regress_exec_choices_[ @regress_exec_choices_indices_without_crossval_supp_ ] );
    }
  }
}

sub AddModelFileToList
{
  my ( $this_regtype_, $this_pred_duration_, $this_model_filename_ ) = @_;
  
  if ( ( -e $this_model_filename_ ) &&
      ( ( -f $this_model_filename_ ) > 0 ) ) {
# if the full model_file was successfully created
    if ( exists $duration_to_model_filevec_{$this_pred_duration_} ) {
      my $scalar_ref_ = $duration_to_model_filevec_{$this_pred_duration_} ;

      my $found_ = 0;
      for ( my $list_idx_ = 0 ; $list_idx_ <= $#$scalar_ref_ ; $list_idx_ ++ ) {
        if ( $$scalar_ref_[$list_idx_] eq $this_model_filename_ ) {
          $found_ = 1;
          last;
        }
      }
      push ( @$scalar_ref_, $this_model_filename_ ) if ( $found_ == 0 );
    }
    else { # initialization
      my @just_args_ = ();
      push ( @just_args_, $this_model_filename_ );
      $duration_to_model_filevec_{$this_pred_duration_} = [ @just_args_ ] ;
    }
  }

  if ( $this_regtype_ ne "" ) {
    $model_to_regtype_{$this_model_filename_} = $this_regtype_;
  } else {
    $model_to_regtype_{$this_model_filename_} = "UNDEF";
  }
}

sub AnalyseModelFile
{
  foreach my $this_pred_duration_ ( keys %duration_to_model_filevec_ )
  {
    my $scalar_ref_model_filevec_ = $duration_to_model_filevec_{$this_pred_duration_};
    for ( my $model_filevec_index_ = 0 ; $model_filevec_index_ <= $#$scalar_ref_model_filevec_ ; $model_filevec_index_ ++ )
    {
      my $a_cmd_ = "$MODELSCRIPTS_DIR/analyse_model_file.pl ".$$scalar_ref_model_filevec_[$model_filevec_index_]." ".$datagen_start_yyyymmdd_." ".$datagen_end_yyyymmdd_." ".$datagen_start_hhmm_." ".$datagen_end_hhmm_." ".$datagen_msecs_timeout_." ".$datagen_l1events_timeout_." ".$datagen_num_trades_timeout_." ".$to_print_on_economic_times_ ;
      print $main_log_file_handle_ "$a_cmd_\n";
      my @a_result_lines_ = `$a_cmd_` ;

      foreach my $line ( @a_result_lines_ )
      {
        $mail_body_.= $line ;
        print $main_log_file_handle_ "$line\n";
      }
    }
  }
}

sub CreateParamListVecFromRegimeParamList
{
  my ( $param_list_file_name_ , @regime_config_list_ ) = @_ ;
  my @high_vol_param_vec_ = ();
  my @low_vol_param_vec_ = ();
  my @paramlist_vec_ = ();
  my $dir_name_ = "";

  open PARAMLISTNAME, "< $param_list_file_name_" or PrintStacktraceAndDie ( "Could not open the input  $param_list_file_name_ \n" );

  my @param_list_file_vec_ = <PARAMLISTNAME>;

  foreach my $paramlist_line_  ( @param_list_file_vec_)  
  {
    my @paramlist_line_words_ = split (' ', $paramlist_line_ );  chomp ( @paramlist_line_words_ );
    if ( $paramlist_line_words_ [0] eq "HIGH" )
    {
      $dir_name_ = `dirname $paramlist_line_words_[1] `; chomp ( $dir_name_ );
      push ( @high_vol_param_vec_ , $paramlist_line_words_[1] );
    }
    elsif ( $paramlist_line_words_[0] eq "LOW" )
    {
      $dir_name_ = `dirname $paramlist_line_words_[1] `; chomp ( $dir_name_ );
      push ( @low_vol_param_vec_, $paramlist_line_words_[1] );
    }
  }

  foreach my $high_vol_param_ (@high_vol_param_vec_ )
  {
    foreach my $low_vol_param_ ( @low_vol_param_vec_ )
    {
      foreach my $config_ ( @regime_config_list_ )
      {
        my $high_vol_param_base_ = `basename $high_vol_param_`; chomp ( $high_vol_param_base_);
        my $low_vol_param_base_ = `basename $low_vol_param_`; chomp ( $low_vol_param_base_ ) ;

        my @config_words_ = split (" ", $config_ );
        my $name_words_ = join ("_", $config_words_[2], $config_words_[3], $config_words_[4] );
        my $param_name_ = $dir_name_."/reg_$name_words_"."_".$high_vol_param_base_.".".$low_vol_param_base_;
        print  $main_log_file_handle_ " This paramName: " . $param_name_."\n";
        my $param_str_ = "PARAMFILELIST ".$high_vol_param_."\nPARAMFILELIST ".$low_vol_param_."\n"."$config_words_[0] $config_words_[1] $shortcode_ $config_words_[2] $config_words_[3] $config_words_[4]\n";
        open OUTPARAMFILE , "> $param_name_" or PrintStacktraceAndDie ( "Could not open the output_strategyfilename $param_name_\n" ); 
        print OUTPARAMFILE $param_str_ ;
        close OUTPARAMFILE ;
        push ( @paramlist_vec_, $param_name_ );
      }
    }
  }
  @paramlist_vec_ ;
}

sub CreateParamListVecFromRegimeIndandParam
{
  my ( $param_list_file_name_ , $regime_indicator_ ) = @_ ;
  print $main_log_file_handle_ "Start CreateParamListVecFromRegimeIndandParam\n";
  my @regime_words_ = split (" ", $regime_indicator_ );
  my @param_substr_words_ = ();
  for ( my $i=0; $i<=$#regime_words_; $i++ )
  {
    push ( @param_substr_words_, $regime_words_[ $i ] );
  }
  my $param_substr_ = join ("_", @param_substr_words_ );

  my @param_list_file_vec_ = MakeFilenameVecFromList ( $param_list_file_name_ );
  chomp ( @param_list_file_vec_ );

  my @paramlist_vec_ = @param_list_file_vec_; #including individual params also
      if ( $#param_list_file_vec_ >= 0 )
      {
        my $reg_param_prefix_ = $local_params_dir_."/regm_$param_substr_";
        
        my $permutations_ = GetSubsetsofSize ( \@param_list_file_vec_, $num_regimes_ );
        for my $param_ ( @$permutations_ )
        {
          if ( CheckSubsetElementUniqueness( $param_, ($num_regimes_ - 1) ) == 1 )  #avoid regime_params with all regimes having same params, will include them as normal param
          {
            my $reg_param_ = $reg_param_prefix_;
            for my $x ( @$param_ )
            {
              $reg_param_ = $reg_param_."_".basename( $x );
            }
            if ( !(-e $reg_param_) )
            {
              open OUTPARAM, "> $reg_param_" or PrintStacktraceAndDie ( "could not open the param_file to write $reg_param_\n" );
              for my $x ( @$param_ )
              {
                print OUTPARAM "PARAMFILELIST $x\n";
              }
              print OUTPARAM "INDICATOR 1.00 $regime_indicator_\n";
              close (OUTPARAM);
            }
            push (@paramlist_vec_, $reg_param_ );
          }
        }
      }
  print $main_log_file_handle_ "Finish CreateParamListVecFromRegimeIndandParam\n";
  @paramlist_vec_;

}

sub MakeRegimeModels
{
  my ( $scalar_ref_model_map_ref_, $regime_model_filename_ ) = @_;

  print $main_log_file_handle_ "MakeRegimeModels\n";
  print $main_log_file_handle_ "Regime Model : $regime_model_filename_" ;

  my $exec_cmd_ = "head -n1 ".$$scalar_ref_model_map_ref_{ 0 }." >> ".$regime_model_filename_;
  `$exec_cmd_`;
  $exec_cmd_ = "echo MODELMATH ".$regime_model_type_." CHANGE >> ".$regime_model_filename_;
  `$exec_cmd_`;
  $exec_cmd_ = "echo REGIMEINDICATOR 1.00 ".$regime_indicator_." >> ".$regime_model_filename_;
  `$exec_cmd_`;

  foreach my $j ( 0..($num_regimes_-1) )
  {
    if ( $j == 0 )
    {
      $exec_cmd_ = "echo INDICATORSTART >> ".$regime_model_filename_;
      `$exec_cmd_`;
    }
    if ( exists $$scalar_ref_model_map_ref_{ $j } )
    {
      print $main_log_file_handle_ "Individual Model File:".$$scalar_ref_model_map_ref_{ $j }."\n";
      $exec_cmd_ = "grep \"INDICATOR \" ".$$scalar_ref_model_map_ref_{ $j }." >> ".$regime_model_filename_;
      `$exec_cmd_`;
    } else {
      print $main_log_file_handle_  "Individual RegFile for ".(1+$j)." does not exist.\n";
    }

    if ( $j < ($num_regimes_-1) )
    {
      $exec_cmd_ = "echo INDICATORINTERMEDIATE >> ".$regime_model_filename_;
      `$exec_cmd_`;
    }
    else
    {
      $exec_cmd_ = "echo INDICATOREND >> ".$regime_model_filename_;
      `$exec_cmd_`;
    }
  }
}

sub MakeStrategyFiles 
{
  print $main_log_file_handle_ "MakeStrategyFiles\n";
  my $strategy_progid_ = 1001;
  foreach my $this_pred_duration_ ( keys %duration_to_model_filevec_ )
  { # foreach duration for which at least one model file was made
    my $scalar_ref_model_filevec_ = $duration_to_model_filevec_{$this_pred_duration_};
    for ( my $model_filevec_index_ = 0 ; $model_filevec_index_ <= $#$scalar_ref_model_filevec_ ; $model_filevec_index_ ++ )
    { # foreach model_file generated for this duration
# my $this_model_filename_ = $$scalar_ref_model_filevec_[$model_filevec_index_];
      if ( $CREATE_SUBMODELS == 1 )
      {
        print $main_log_file_handle_ "creating submodels\n";
        my $regtype_ = $model_to_regtype_{ $$scalar_ref_model_filevec_[$model_filevec_index_] };
        foreach my $this_model_filename_ ( CreateSubModelFiles ( $$scalar_ref_model_filevec_[$model_filevec_index_], "TODAY-10", "TODAY-5", $datagen_start_hhmm_, $datagen_end_hhmm_ ) )
        {
          my @dummy_empty_vec_ = (); 
          my $scalar_ref_param_filevec_ = \@dummy_empty_vec_;
          my @param_filevec_from_list_ = ();
          my @param_l1_norm_vec_ = ();
          my $current_l1norm_model_ = -1; 
#-ve sign will also tell us whether to scale or not

          if ( exists $regressalgo2duration2param_filevec_{$regtype_}{$this_pred_duration_} ) { 
            $scalar_ref_param_filevec_ = $regressalgo2duration2param_filevec_{$regtype_}{$this_pred_duration_}; 
          }
          elsif(exists $duration_to_param_filevec_{$this_pred_duration_} ) {
            $scalar_ref_param_filevec_ = $duration_to_param_filevec_{$this_pred_duration_};
          }
          
          if($use_param_list_and_scale_ && IsModelScalable($this_model_filename_))
          {
            GetParamListAndL1Norm($shortcode_, $trading_start_hhmm_, $trading_end_hhmm_, $this_model_filename_, $strategyname_, \@param_filevec_from_list_, \@param_l1_norm_vec_);

	    print $main_log_file_handle_ "GetParamListAndL1Norm : ".join(' ', @param_filevec_from_list_);
            print $main_log_file_handle_ "GetParamListAndL1Norm : ".join(' ', @param_l1_norm_vec_);
            if($#param_filevec_from_list_ >=0 ) 
            {
              $scalar_ref_param_filevec_ = \@param_filevec_from_list_;
              my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $this_model_filename_ TODAY-5 TODAY-1 $trading_start_hhmm_ $trading_end_hhmm_ 2>/dev/null | head -n1";
              print $main_log_file_handle_ "$exec_cmd\n";
              my $current_l1norm_model_line_ = `$exec_cmd`;
              chomp ( $current_l1norm_model_line_);
              my @current_l1norm_model_words_ = split ( ' ', $current_l1norm_model_line_ );
              my $current_stdev_model_ = $current_l1norm_model_words_[0];
              $current_l1norm_model_ = $current_l1norm_model_words_[1];
              print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
              print $main_log_file_handle_ "Current L1Norm: $current_l1norm_model_ \n";
            }
          }
	  
          my @new_param_list_ = ();
          if ( $use_regime_param_ )
          {
            @new_param_list_ = CreateParamListVecFromRegimeParamList ( $this_param_list_name_ , @regime_config_list_ );
            $scalar_ref_param_filevec_ = \@new_param_list_;
          } 

          if ( $make_regime_params_ )
          {
            @new_param_list_ = CreateParamListVecFromRegimeIndandParam ( $this_param_list_name_ , $regime_indicator_ );
            $scalar_ref_param_filevec_ = \@new_param_list_;
          }

          for ( my $param_filevec_index_ = 0 ; $param_filevec_index_ <= $#$scalar_ref_param_filevec_ ; $param_filevec_index_ ++ )
          { # foreach param_file input for this duration
            my $this_param_filename_ = $$scalar_ref_param_filevec_[$param_filevec_index_];
            my $this_scaled_model_filename_ = $this_model_filename_;
            if($current_l1norm_model_ > 0)
            {
              my  $scale_const_ = $param_l1_norm_vec_[$param_filevec_index_]/$current_l1norm_model_;
              if($scale_const_ > 0)
              {
                $this_scaled_model_filename_ = $this_model_filename_."_sc_".$param_filevec_index_;                        
                my $exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_model_filename_ $this_scaled_model_filename_ $scale_const_";
                print $main_log_file_handle_ "$exec_cmd\n";
                `$exec_cmd`;

                my @l1norm_vec_ = ($param_l1_norm_vec_[$param_filevec_index_]);                                                                                        
                AddL1NormToModel ( $this_scaled_model_filename_, \@l1norm_vec_);

              }
            }

#doing this here to prevent any extra computations for regime models
            my $is_modelinfo_added_ = `awk '{if(\$1==\"MODELINFO\"){print \$1;}}' $this_scaled_model_filename_ | head -n1`; chomp($is_modelinfo_added_);
            if ( (!$is_modelinfo_added_) && IsModelScalable($this_scaled_model_filename_) && $add_modelinfo_to_model_ )
            {
              my @t_l1norm_vec_ = ();
              GetModelL1NormVec($this_scaled_model_filename_, "TODAY-80", "TODAY-1", $trading_start_hhmm_, $trading_end_hhmm_, \@t_l1norm_vec_);
              AddL1NormToModel($this_scaled_model_filename_, \@t_l1norm_vec_);
            }

            my $this_strategy_filename_ = NameStrategyFromModelAndParamIndex ( $name_, $this_scaled_model_filename_, $param_filevec_index_, $trading_start_hhmm_, $trading_end_hhmm_ ) ;
            if ( $author_ && $author_ eq "dvctrader_p" ) 
            {
              my $temp_str_ = $param_filevec_index_."_".substr ( basename ( $this_param_filename_ ), 0, 40 );
              $this_strategy_filename_ = NameStrategyFromModelAndParamIndex ( $name_, $this_scaled_model_filename_, $temp_str_, $trading_start_hhmm_, $trading_end_hhmm_ ) ;
            }
            {
              my $exec_cmd_ = "";
              if ( $use_mid_base_model_in_dat_ )
              {
                CreateMidBaseModel ( $this_scaled_model_filename_, $strategyname_ );
              }

              my $t_trading_end_time_to_use_ = $trading_end_hhmm_;
              if ( $trading_end_hhmmss_ ne "" )
              {
#this is a hack for eq_open/close kind of strats, where we want trade just 1 sec after market close
                $t_trading_end_time_to_use_ = $trading_end_hhmmss_;
              }

              if ( $trade_only_on_traded_ezone_ == 1 )
              {
                $exec_cmd_ = "$MODELSCRIPTS_DIR/create_strategy_file.pl $this_strategy_filename_ $shortcode_ $strategyname_ $this_scaled_model_filename_ $this_param_filename_ $trading_start_hhmm_ $t_trading_end_time_to_use_ $strategy_progid_ $traded_ezone_";
              }
              else
              {
                $exec_cmd_ = "$MODELSCRIPTS_DIR/create_strategy_file.pl $this_strategy_filename_ $shortcode_ $strategyname_ $this_scaled_model_filename_ $this_param_filename_ $trading_start_hhmm_ $t_trading_end_time_to_use_ $strategy_progid_";
              }
              print $main_log_file_handle_ "$exec_cmd_\n";
              $uts_ = `grep UNIT_TRADE_SIZE $this_param_filename_ | awk '{ print \$3 }'` ; 
              chomp ( $uts_ ) ;
              `$exec_cmd_`;
              $strategy_progid_++;
            }

            if ( -e $this_strategy_filename_ )
            {
              push ( @strategy_filevec_, $this_strategy_filename_ ); 
            }
          }
        }
      }
      else
      {
	my $this_model_filename_ = $$scalar_ref_model_filevec_[$model_filevec_index_];
        my $regtype_ = $model_to_regtype_{ $this_model_filename_ };
        
        my @dummy_vec_ = ();
        my $scalar_ref_param_filevec_ = \@dummy_vec_;
        my @param_filevec_from_list_ = ();
        my @param_l1_norm_vec_ = ();
#-ve sign will also tell us whether to scale or not
        my $current_l1norm_model_ = -1; 

        if ( exists $regressalgo2duration2param_filevec_{$regtype_}{$this_pred_duration_} ) { 
          $scalar_ref_param_filevec_ = $regressalgo2duration2param_filevec_{$regtype_}{$this_pred_duration_}; 
        }
        elsif ( exists $duration_to_param_filevec_{$this_pred_duration_} ) {
          $scalar_ref_param_filevec_ = $duration_to_param_filevec_{$this_pred_duration_};
        }
        
        if ( $use_param_list_and_scale_ && IsModelScalable($this_model_filename_) )
        {
	  GetParamListAndL1Norm($shortcode_, $trading_start_hhmm_, $trading_end_hhmm_, $this_model_filename_, $strategyname_, \@param_filevec_from_list_, \@param_l1_norm_vec_);
        
          if($#param_filevec_from_list_ >=0 )
          {
	    $scalar_ref_param_filevec_ = \@param_filevec_from_list_;
            my $exec_cmd="$MODELSCRIPTS_DIR/get_stdev_model.pl $this_model_filename_ TODAY-5 TODAY-1 $trading_start_hhmm_ $trading_end_hhmm_ 2>/dev/null | head -n1";
            print $main_log_file_handle_ "$exec_cmd\n";
            my $current_l1norm_model_line_ = `$exec_cmd`;
            chomp ( $current_l1norm_model_line_);
            my @current_l1norm_model_words_ = split ( ' ', $current_l1norm_model_line_ );
            my $current_stdev_model_ = $current_l1norm_model_words_[0];
            $current_l1norm_model_ = $current_l1norm_model_words_[1];
            print $main_log_file_handle_ "Current Stdev: $current_stdev_model_\n";
            print $main_log_file_handle_ "Current L1Norm: $current_l1norm_model_ \n";
          }
        }

        my @new_param_list_ = ();
        if ( $use_regime_param_ )
        {
	  @new_param_list_ = CreateParamListVecFromRegimeParamList ( $this_param_list_name_ , @regime_config_list_ );
          print $use_regime_param_." number of params : ".$#new_param_list_."\n";
          $scalar_ref_param_filevec_ = \@new_param_list_;
        }
        if ( $make_regime_params_ )
        {
	  @new_param_list_ = CreateParamListVecFromRegimeIndandParam ( $this_param_list_name_ , $regime_indicator_ );
          $scalar_ref_param_filevec_ = \@new_param_list_;
        }


        for ( my $param_filevec_index_ = 0 ; $param_filevec_index_ <= $#$scalar_ref_param_filevec_ ; $param_filevec_index_ ++ )
        { # foreach param_file input for this duration
	  my $this_param_filename_ = $$scalar_ref_param_filevec_[$param_filevec_index_];
	  
          if ( $regime_indicator_ && $regime_to_trade_flag_ )
          {
            open PARAMFILE, "< $this_param_filename_" or PrintStacktraceAndDie ( "Could not open $this_param_filename_ for reading" );
            my @param_file_ = <PARAMFILE>; chomp ( @param_file_ );
            close PARAMFILE;

            push(@param_file_, "PARAMVALUE REGIMEINDICATOR $regime_indicator_");
            push(@param_file_, "PARAMVALUE REGIMES_TO_TRADE $regime_to_trade_number_");

            print $main_log_file_handle_ "PARAM_FILENAME: $this_param_filename_\n";
            print $main_log_file_handle_ "REGIME_INDICATOR_NAME: $regime_indicator_\n";
            my $paramfile_basename_ = basename($this_param_filename_) ;
            my $regime_indc_name_ = (split / /, $regime_indicator_)[0] ;

            $this_param_filename_ = $local_params_dir_."/".$paramfile_basename_."_regime_".$regime_indc_name_."_".$regime_to_trade_number_;
            open NEWPARAMFILE, "> $this_param_filename_" or PrintStacktraceAndDie ( "Could not open $this_param_filename_ for writing" );
            print NEWPARAMFILE $_."\n" foreach @param_file_;
            close NEWPARAMFILE;
          }

	  my $this_scaled_model_filename_ = $this_model_filename_;
          if($current_l1norm_model_ > 0)
          {

	    my  $scale_const_ = $param_l1_norm_vec_[$param_filevec_index_]/$current_l1norm_model_;
            if($scale_const_ > 0)
            {
              $this_scaled_model_filename_ = $this_model_filename_."_sc_".$param_filevec_index_;                        
              my $exec_cmd="$MODELSCRIPTS_DIR/rescale_model.pl $this_model_filename_ $this_scaled_model_filename_ $scale_const_";
              print $main_log_file_handle_ "$exec_cmd\n";
              `$exec_cmd`;

              my @l1norm_vec_ = ($param_l1_norm_vec_[$param_filevec_index_]);                                                                                            
              AddL1NormToModel ( $this_scaled_model_filename_, \@l1norm_vec_);
            }
          }

#doing this here to prevent any extra computations for regime models
          my $is_modelinfo_added_ = `awk '{if(\$1==\"MODELINFO\"){print \$1;}}' $this_scaled_model_filename_ | head -n1`; chomp($is_modelinfo_added_);
          if ( (!$is_modelinfo_added_) && IsModelScalable($this_scaled_model_filename_) && $add_modelinfo_to_model_ )
          {

#will only come here if not already scaled or no modelinfo added
            my @t_l1norm_vec_ = ();
            GetModelL1NormVec($this_scaled_model_filename_, "TODAY-80", "TODAY-1", $trading_start_hhmm_, $trading_end_hhmm_, \@t_l1norm_vec_);
            if ( $#t_l1norm_vec_ >=0 && $#target_l1norm_model_vec_ >=0 && $t_l1norm_vec_[-1]>0 && $target_l1norm_model_vec_[0]>0 )
            {
#if scaling is required
              my $t_scale_fact_ = $target_l1norm_model_vec_[0]/$t_l1norm_vec_[-1];
              my $t_unscaled_model_file_ = $this_scaled_model_filename_."_temp"; 
              `mv $this_scaled_model_filename_ $t_unscaled_model_file_`;
              my $t_exec_cmd_ = "$MODELSCRIPTS_DIR/rescale_model.pl $t_unscaled_model_file_ $this_scaled_model_filename_ $t_scale_fact_"; #scaling the modelfile
              print $main_log_file_handle_ "$t_exec_cmd_";
              `$t_exec_cmd_`;
              `rm -f $t_unscaled_model_file_`;
              @t_l1norm_vec_ = map { $_*$t_scale_fact_ } @t_l1norm_vec_; #scaling the modelinfo to be added to the modelfile
            }
            AddL1NormToModel($this_scaled_model_filename_, \@t_l1norm_vec_);
          }    

          my $this_strategy_filename_ = NameStrategyFromModelAndParamIndex ( $name_, $this_scaled_model_filename_, $param_filevec_index_, $trading_start_hhmm_, $trading_end_hhmm_ ) ;
          if ( $author_ && $author_ eq "dvctrader_p" ) 
          {
            my $temp_str_ = $param_filevec_index_."_".substr ( basename ( $this_param_filename_ ), 0, 40 );
            $this_strategy_filename_ = NameStrategyFromModelAndParamIndex ( $name_, $this_scaled_model_filename_, $temp_str_ , $trading_start_hhmm_, $trading_end_hhmm_ ) ;
          }
          {
            my $exec_cmd_ = "";

            if ( $use_mid_base_model_in_dat_ )
            {
              CreateMidBaseModel ( $this_scaled_model_filename_, $strategyname_ );
            }

            my $t_trading_end_time_to_use_ = $trading_end_hhmm_;
            if ( $trading_end_hhmmss_ ne "" )
            {
              #this is a hack for eq_open/close kind of strats, where we want trade just 1 sec after market close
              $t_trading_end_time_to_use_ = $trading_end_hhmmss_;
            }

            if ( $trade_only_on_traded_ezone_ == 1 )
            {
              $exec_cmd_ = "$MODELSCRIPTS_DIR/create_strategy_file.pl $this_strategy_filename_ $shortcode_ $strategyname_ $this_scaled_model_filename_ $this_param_filename_ $trading_start_hhmm_ $t_trading_end_time_to_use_ $strategy_progid_ $traded_ezone_";
            }
            else
            {
              $exec_cmd_ = "$MODELSCRIPTS_DIR/create_strategy_file.pl $this_strategy_filename_ $shortcode_ $strategyname_ $this_scaled_model_filename_ $this_param_filename_ $trading_start_hhmm_ $t_trading_end_time_to_use_ $strategy_progid_";
            }

            print $main_log_file_handle_ "$exec_cmd_\n";
	    $uts_ = `grep UNIT_TRADE_SIZE $this_param_filename_ | awk '{ print \$3 }'` ; 
	    chomp ( $uts_ ) ;
            `$exec_cmd_`;
            $strategy_progid_++;
          }

          if ( -e $this_strategy_filename_ )
          {
            push ( @strategy_filevec_, $this_strategy_filename_ ); 
          }
        }
      }
      if ( $use_regime_ilists_ > 0 )
      {
        last ; # hack, we know all the models files created for this duration are submodels, one each corresponding to different regime.
      }
    }
  }
}

sub RunSimulationOnCandidates
{
  print $main_log_file_handle_ "RunSimulationOnCandidates\n";

  if ( $obtain_model_from_cross_validation_ == 1 && $number_of_models_ <= $bad_models_) {
    print $main_log_file_handle_ "No robust model was found.Ending analysis here\n";
    return;
  } 
  print $main_log_file_handle_ "Number of unique strat file ".scalar ( @strategy_filevec_ )."\n";

  my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file.txt";
  open TSLF, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_ for writing\n" );
  print TSLF join("\n", @strategy_filevec_);
  close TSLF;

  my $trading_days_file_ = $work_dir_."/trading_days.txt";
  open TDLF, "> $trading_days_file_" or PrintStacktraceAndDie ( "Could not open $trading_days_file_ for writing\n" );
  print TDLF join("\n", @trading_days_);
  close TDLF;

  my $runsim_cmd_ = "$MODELSCRIPTS_DIR/run_simulations.pl $shortcode_ $temp_strategy_list_file_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ $local_results_base_dir -dtlist $trading_days_file_";
  
  if ( $use_distributed_ ) {
    print $main_log_file_handle_ "$runsim_cmd_\n";
    my @runsim_lines_  = `$runsim_cmd_ 2>/dev/null`; chomp ( @runsim_lines_ );
    my ($groupid_line_) = grep { $_ =~ /Group ID:/ } @runsim_lines_;
    my $groupid_ = (split /\s+/, $groupid_line_)[2];

    while (1) {
      sleep 120;
      my ($total_tasks_, $completed_tasks_) = `$DISTRIBUTED_STATUS_SCRIPT -g $groupid_ | grep 'TOTAL\\\|COMPLETED' | awk '{print \$2}'`;
      chomp ( $total_tasks_, $completed_tasks_ );
      last if $completed_tasks_ >= $total_tasks_;
    }
  }
  else {
    $runsim_cmd_ .= " -d 0";
    print $main_log_file_handle_ "$runsim_cmd_\n";
    `$runsim_cmd_ 2>/dev/null`;
  }

  foreach my $tradingdate_ ( @trading_days_ ) {
    my ( $yyyy_ , $mm_ , $dd_ ) = BreakDateYYYYMMDD ( $tradingdate_ );
    my $this_local_results_database_file_ = $local_results_base_dir."/".$shortcode_."/".$yyyy_."/".$mm_."/".$dd_."/results_database.txt";

    if ( ExistsWithSize( $this_local_results_database_file_ ) ) {
      push ( @unique_results_filevec_, $this_local_results_database_file_ );
      push ( @final_trading_days_, $tradingdate_ );
    }
  }

  if ( $#unique_results_filevec_ < 0 ) {
    $mail_body_ .= "\n*Error:* Simulation Runs for the Trading Days were not successful.. Please check.\n\n";
    print $main_log_file_handle_ "\n*Error:* Simulation Runs for the Trading Days were not successful.. Please check.\n\n";
  }

  `rm -f $temp_strategy_list_file_ 2>/dev/null`;
}

sub RunSimulationForValidation
{
  my $strats_vec_ref_ = shift;

  print $main_log_file_handle_ "RunSimulationForValidation\n";
  print $main_log_file_handle_ "Number of selected strat files to be validated ".scalar ( @$strats_vec_ref_ )."\n";

  my @stratpaths_vec_ = map { FindItemFromVecWithBase( $_, @strategy_filevec_ ) } @$strats_vec_ref_;

  my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file.txt";
  open TSLF, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_ for writing\n" );
  print TSLF join("\n", @stratpaths_vec_);
  close TSLF;

  my $trading_days_file_ = $work_dir_."/validation_days.txt";
  open TDLF, "> $trading_days_file_" or PrintStacktraceAndDie ( "Could not open $trading_days_file_ for writing\n" );
  print TDLF join("\n", @validation_days_);
  close TDLF;

  my $runsim_cmd_ = "$MODELSCRIPTS_DIR/run_simulations.pl $shortcode_ $temp_strategy_list_file_ $validation_start_yyyymmdd_ $validation_end_yyyymmdd_ $local_validation_results_base_dir ";

  if ( $use_distributed_ ) {
    print $main_log_file_handle_ "$runsim_cmd_\n";
    my @runsim_lines_  = `$runsim_cmd_ 2>/dev/null`; chomp ( @runsim_lines_ );
    my ($groupid_line_) = grep { $_ =~ /Group ID:/ } @runsim_lines_;
    my $groupid_ = (split /\s+/, $groupid_line_)[3];

    while (1) {
      sleep 120;
      my ($total_tasks_, $completed_tasks_) = `$DISTRIBUTED_STATUS_SCRIPT -g $groupid_ | grep 'TOTAL\|COMPLETED' | awk '{print \$2}'`;
      chomp ( $total_tasks_, $completed_tasks_ );
      last if $completed_tasks_ >= $total_tasks_;
    }
  }
  else {
    $runsim_cmd_ .= " -d 0";
    print $main_log_file_handle_ "$runsim_cmd_\n";
    `$runsim_cmd_ 2>/dev/null`;
  }

  foreach my $tradingdate_ ( @validation_days_ )
  {
    my ( $yyyy_ , $mm_ , $dd_ ) = BreakDateYYYYMMDD ( $tradingdate_ );
    my $this_local_results_database_file_ = $local_validation_results_base_dir."/".$shortcode_."/".$yyyy_."/".$mm_."/".$dd_."/results_database.txt";

    if ( ExistsWithSize( $this_local_results_database_file_ ) ) {
      push ( @unique_validation_results_filevec_, $this_local_results_database_file_ );
      push ( @final_validation_days_, $tradingdate_ );
    }
  }

  `rm -f $temp_strategy_list_file_ 2>/dev/null`;
}

sub RunSaveResults
{
  my $strategy_list_ref_ = shift;

  print $main_log_file_handle_ "RunSaveResults\n";

  if ( $obtain_model_from_cross_validation_ == 1 && $number_of_models_ <= $bad_models_) {
    print $main_log_file_handle_ "No robust model was found.Ending analysis here\n";
    return;
  } 
  print $main_log_file_handle_ " number of unique strat file ".scalar ( @$strategy_list_ref_ )."\n" ;

  my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file.txt";

  open TSLF, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_ for writing\n" );
  print TSLF join("\n", @$strategy_list_ref_);
  close TSLF;

# Generate the sample-data for the last 100 days
  my $lookback_days_ = 250;
  my $t_end_trading_day_ = $yyyymmdd_;
  my $dates_str_ = `$SCRIPTS_DIR/get_list_of_dates_for_shortcode.pl $shortcode_ $t_end_trading_day_ $lookback_days_ 2>/dev/null`; chomp ( $dates_str_ );
  my @dates_vec_ = split( ' ', $dates_str_ );
  my $t_start_trading_day_ = min @dates_vec_;

  my $datelist_arg_ = "";
  $datelist_arg_ = "-dtlist $event_days_file_" if $event_days_file_ ne "";

#Run simulations and store results to DB (file system not longer supported)
  my $runsim_cmd_ = "$MODELSCRIPTS_DIR/run_simulations.pl $shortcode_ $temp_strategy_list_file_ $t_start_trading_day_ $t_end_trading_day_ DB $datelist_arg_";
  $runsim_cmd_ .= " -d 0" if $use_distributed_ == 0;

  print $main_log_file_handle_ "$runsim_cmd_\n";
  my @runsim_log_ = `$runsim_cmd_ 2>&1`; chomp ( @runsim_log_ );

  print "Log of the run_simulations.pl: \n".join("\n", @runsim_log_)."\n";
  
  `rm -f $temp_strategy_list_file_`;
}

sub GetOutSamplePPC
{
  my $strat_basename_ = shift;
  my $pnl_per_contract_word_index_ = 9; # PNL_PER_CPONTRACT is 9th word in STATISTICS line in summarize_single_strategy_results
  my @t_outsample_text_ = ();

  my $outsample_sim_day_count_ = GetBusinessDaysBetween ( $outsample_trading_start_yyyymmdd_, $outsample_trading_end_yyyymmdd_ );
  print $main_log_file_handle_ "Outsample Days between $outsample_trading_start_yyyymmdd_ $outsample_trading_end_yyyymmdd_ = $outsample_sim_day_count_\n";
  if ( $outsample_sim_day_count_ > 5 )
  {
    $mail_body_ = $mail_body_."Outsample\n";

# outsample results
    my $exec_cmd="$LIVE_BIN_DIR/summarize_single_strategy_results $shortcode_ $strat_basename_ $work_dir_/local_results_base_dir $outsample_trading_start_yyyymmdd_ $outsample_trading_end_yyyymmdd_";
    print $main_log_file_handle_ "$exec_cmd\n";
    @t_outsample_text_ = `$exec_cmd`; chomp ( @t_outsample_text_ );
    $mail_body_ = $mail_body_.join ( "", @t_outsample_text_ ) ;
    print $main_log_file_handle_ @t_outsample_text_;
    print $main_log_file_handle_ "\n";
  }

  my $t_outsample_pnl_per_contract_ = -1;
  if ( $#t_outsample_text_ >= 0 )
  {
    for ( my $t_o_idx_ = 0 ; $t_o_idx_ <= $#t_outsample_text_ ; $t_o_idx_ ++ )
    {
      if ( $t_outsample_text_[$t_o_idx_] =~ /STATISTICS/ )
      {
        my @stat_line_words_ = split ( ' ', $t_outsample_text_[$t_o_idx_] );
        if ( $#stat_line_words_ >= $pnl_per_contract_word_index_ )
        {
          $t_outsample_pnl_per_contract_ = $stat_line_words_[$pnl_per_contract_word_index_];
          last;
        }
      }
    }
  }
  $t_outsample_pnl_per_contract_;
}

sub PrintSummarizeStrategyCutoffsError
{
    my $exec_cmd="$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo $historical_sort_algo_ 10000 1000 $min_pnl_per_contract_to_allow_ $min_volume_to_allow_ $max_ttc_to_allow_ $max_message_count_ $local_results_base_dir | grep STAT";
    print $main_log_file_handle_ "$exec_cmd\n";
    my @all_strats_summary_lines_=`$exec_cmd`;

    if ( $#all_strats_summary_lines_ < 0 ) {
      $mail_body_ .= "\n*ERROR:* Could Not Summarize for the generated strategies\n";
      print $main_log_file_handle_ "\n*ERROR:* Could Not Summarize for the generated strategies\n";
      return;
    }

    my $err_str_ .= "\n*ERROR:* None of the strategies could pass the Summarize Strategy cutoffs.\n";
    $err_str_ .= "*ERROR:* Either of MIN_PPC MIN_VOL MAX_TTC MAX_MSG is violated for all the strategies.\n";
    $err_str_ .= "Mentioned below are the stats for the generated strategies:\n\n";

    my @all_strats_summary_lines_t_ = ( );
    my $strat_idx_ = 1;
    foreach my $line_ ( @all_strats_summary_lines_ ) {
      my @t_words_ = split(' ', $line_);
      my ($pnl_, $vol_, $ttc_, $ppc_) = @t_words_[ (1,3,8,9) ];
      my $t_summ_str_ = "Strat_".$strat_idx_.": PNL:$pnl_ VOL:$vol_ TTC:$ttc_ PPC:$ppc_";
      push ( @all_strats_summary_lines_t_, $t_summ_str_ );
      $strat_idx_++;
    }

   $err_str_ .= join("\n", @all_strats_summary_lines_t_)."\n";
   $mail_body_ .= $err_str_;

   print $main_log_file_handle_ $err_str_."\n";
}

sub SummarizeLocalResultsAndChoose
{
  print $main_log_file_handle_ "SummarizeLocalResultsAndChoose\n";

  if ( $min_volume_to_allow_ =~ "PERC" )
  {
    my @min_vol_cutoff_ = ( );
    foreach my $tradingdate_ ( @trading_days_ )
    {
      my $start_utc_time_ = `$HOME_DIR/basetrade_install/bin/get_utc_hhmm_str $trading_start_hhmm_ $tradingdate_`; chomp ( $start_utc_time_ );
      my $end_utc_time_ = `$HOME_DIR/basetrade_install/bin/get_utc_hhmm_str $trading_end_hhmm_ $tradingdate_`; chomp ( $end_utc_time_ );

      my $min_vol_ = `$HOME_DIR/basetrade_install/bin/get_all_mds_stats_for_day $shortcode_ $tradingdate_ $start_utc_time_ $end_utc_time_ | awk '{ if ( \$1 == \"Volume\" ) { vol=\$2; } if ( \$1 == \"AvgL1Size\" ) { l1sz=\$2; } } END { print vol/($volume_per_l1perc_*l1sz) ; }'`;

      push ( @min_vol_cutoff_, $min_vol_ );
    }
    $min_volume_to_allow_ = $uts_ * GetAverage ( \@min_vol_cutoff_ ) ;
  }
      
  $mail_body_ .= "\n
    **SummarizeStrategyParams:** \n
    Sort_Algo_: $historical_sort_algo_
    Min_Num_Files_to_Choose: $min_num_files_to_choose_
    Num_Files_To_Choose: $num_files_to_choose_
    Min_Pnl_Per_Contract: $min_pnl_per_contract_to_allow_
    Min_Volum: $min_volume_to_allow_
    Max_Messages: $max_message_count_ 
    Max_TTC: $max_ttc_to_allow_\n===============================\n\n";

  if ( $obtain_model_from_cross_validation_ == 1 && $number_of_models_ <= $bad_models_)
  {
    print $main_log_file_handle_ "Sending mail about no robust models\n";
    $mail_body_ = $mail_body_."\n".$failure_string_;
    $mail_body_ = $mail_body_."\nERROR: No model file was robust enough for pnl analysis.Log details available at $main_log_file_\n";
    SendMail ( );
  }

  if ( $#unique_results_filevec_ >= 0 )
  {
## support for dynamic cutoffs for VOL, TTC, PPC based on current pool
    if ( $use_dynamic_cutoffs_ )
    {
      FindDynamicCutoffsVolTTCPPC ( );
    }

    print $main_log_file_handle_ " Using min_num_files_to_choose_=".$min_num_files_to_choose_." min_volume_to_allow_=".$min_volume_to_allow_."\n";

# take the list of local_results_database files @unique_results_filevec_
    my $exec_cmd="$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo $historical_sort_algo_ $min_num_files_to_choose_ $num_files_to_choose_ $min_pnl_per_contract_to_allow_ $min_volume_to_allow_ $max_ttc_to_allow_ $max_message_count_ $local_results_base_dir";
    print $main_log_file_handle_ "$exec_cmd\n";
    my @slrac_output_lines_=`$exec_cmd`;
    print $main_log_file_handle_ @slrac_output_lines_."\n";

    if ( $#slrac_output_lines_ < 0 ) {
      PrintSummarizeStrategyCutoffsError ( );
      return;
    }

    if ( $min_days_traded_perc_ > 0 ) {
      FilterOnMinDaysTraded ( \@slrac_output_lines_ );
    }

    my @strat_files_selected_ = ();
    my %strat_indices_ = ();

    if ($use_median_cutoff_)
    {
      MedianCutoff ( \@slrac_output_lines_, \@strat_files_selected_, \%strat_indices_ );
    }
    else 
    {
      PPCCutoff ( \@slrac_output_lines_, \@strat_files_selected_ );
    }

    my @best_validation_result_lines_ = ( );
    if ( $#strat_files_selected_ >= 0 && $use_median_cutoff_ && $use_validation_median_check_ )
    {
      RunSimulationForValidation ( \@strat_files_selected_ );
      ValidationMedianCutoff ( \@strat_files_selected_, \@best_validation_result_lines_ );
    }

    if ( $#strat_files_selected_ >= 0 )
    {
      InstallStrategiesAndLog ( \@strat_files_selected_, \%strat_indices_ );
    }
    elsif ($#slrac_output_lines_>=0)
    {
      if($use_median_cutoff_){
        $mail_body_ .= "*ERROR*: None of the strategies are better than the median of the existing strategies.\n\n";
      }
      else{
        $mail_body_ .= "*ERROR*: None of the strategies could cross the minPNL cutoff.\n\n";
      }

      if ( $RUN_SINGLE_INDICATOR_LIST_FILE )
      {
        $mail_body_ .= "Ilist File: ".$ilist_filename_vec_[0]."\n";
      }

      my @best_strat_statistics = @slrac_output_lines_;
      my @strats_fl_idx_ = grep { $slrac_output_lines_[ $_ ] =~ /^STRATEGYFILEBASE/ } 0..$#slrac_output_lines_;
      if ( $#strats_fl_idx_ >= 1 ) {
        @best_strat_statistics = @slrac_output_lines_[ $strats_fl_idx_[0]..($strats_fl_idx_[1]-1) ];
      }
      chomp (@best_strat_statistics); 

      my ($strat_fl_) = grep("STRATEGYFILEBASE", @best_strat_statistics ); my @st_fl_ = split(' ', $strat_fl_); 
      $strat_fl_ = FindItemFromVecWithBase ( $st_fl_[1], @strategy_filevec_  );
      {
        $exec_cmd = "$SCRIPTS_DIR/check_indicator_performance.py $strat_fl_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ 2>/dev/null | sort -nr -t':' -k3 | sed 's/\\[.*\\].*: //g'";
        print $main_log_file_handle_ "Checking Indicator Performance of best strats:\n$exec_cmd\n";
        my @ind_perf_ = `$exec_cmd`;	    
        $mail_body_ .= "Indicator_performance: \n @ind_perf_\n";
      }

      $mail_body_ .= "=========Best Strategy(s):=========\n".join("\n",@best_strat_statistics)."\n================END=========================\n\n";
      $mail_body_ .= "=========Best Validation Strategy(s):=========\n".join("\n",@best_validation_result_lines_)."\n================END=========================\n\n";

      $mail_body_ = $mail_body_."STATISTICS PNL_avg PNL_stdev VOL_avg PNL_sharpe PNL_<avg-0.33*stdev> avg_min_adj_PNL median_TTC PPC NBL_order BL_order AGG_order avg_MAX_DD\n";
      $mail_body_ .= "StratPath: ".$strat_fl_."\n";
      my $model_ = `cat $strat_fl_ | awk '{print \$4}'`;
      $mail_body_ .= "ModelPath: ".$model_."\n";
      $mail_body_ .= "Model: \n".`cat $model_`."\n";
    }
  }
}

sub FindDynamicCutoffsVolTTCPPC
{
  my $num_strats_in_global_results_=0;
  print $main_log_file_handle_ "Using Dynamic Cutoff\n";
  my $global_results_dir_path = "/NAS1/ec2_globalresults/";
  $global_results_dir_path = $HOME_DIR."/ec2_globalresults/" if $hostname_ =~ /crt/;

  my $final_trading_days_file_ = $work_dir_."/final_trading_days_file_";
  open FTDF, "> $final_trading_days_file_" or PrintStacktraceAndDie ( "Could not open $final_trading_days_file_ for writing\n" );
  print FTDF join('\n', @final_trading_days_);
  close FTDF;


  my $timeperiod_ = "$trading_start_hhmm_-$trading_end_hhmm_";
  my @all_strats_in_dir_ = ( );

  if( $traded_ezone_ ne "") {
    my $dir_base_ = $traded_ezone_pref_;
    $dir_base_ .= "-".$pool_tag_ if $pool_tag_ ne "";
    @all_strats_in_dir_ = MakeStratVecFromDir ( $MODELING_STRATS_DIR."/".$shortcode_."/EBT/".$dir_base_ );
  }
  elsif ( $pool_tag_ ne "" ) {
    @all_strats_in_dir_ = MakeStratVecFromDirAndTTAndTag($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_, $pool_tag_);
  }
  else {
    @all_strats_in_dir_ = MakeStratVecFromDirAndTT($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_);
  }

  print $main_log_file_handle_ "TimePeriod: $timeperiod_\n";
  my $cstempfile_ = GetCSTempFileName ( $work_dir_."/cstemp" );
  open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
  print $main_log_file_handle_ "All global strats: ".join("\n", @all_strats_in_dir_)."\n";
  for(my $i=0; $i <= $#all_strats_in_dir_; $i++){
    my $t_strat_file_ = basename($all_strats_in_dir_[$i]);
    print CSTF "$t_strat_file_\n";
  }
  $num_strats_in_global_results_ = $#all_strats_in_dir_ + 1;

  close CSTF;

  if( "$num_strats_in_global_results_" eq ""){ 
    print STDERR "No previous strats in the pool";
    $num_strats_in_global_results_ = 0; 
  }

  print $main_log_file_handle_ "Num of strats in global result: $num_strats_in_global_results_\n";

  my $percentile_best_ = 0.75;
  my $dynamic_cutoff_rank_ = min(int($percentile_best_ * $num_strats_in_global_results_ ), 150);
  my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $cstempfile_ $global_results_dir_path $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ INVALIDFILE $historical_sort_algo_ 0 $final_trading_days_file_";
  `rm -f $final_trading_days_file_`;
  print $main_log_file_handle_ $exec_cmd."\n";

  my @ttc_vec = ();
  my @vol_vec = ();
  my @ppc_vec = ();

  {
    my @global_res_out_ = `$exec_cmd`; 
    print $main_log_file_handle_ "@global_res_out_\n";chomp(@global_res_out_);

    foreach my $res_line ( @global_res_out_ )
    {
      my @res_line_words = split ( ' ', $res_line );
# bad code ... this prevented the changing of order in summarize_strategy_results.cpp
      push ( @ttc_vec, $res_line_words[9] ) ;
      push ( @vol_vec, $res_line_words[4] ) ;
      push ( @ppc_vec, $res_line_words[11] ) ;
    }
  }

  @ttc_vec = sort {$a <=> $b} @ttc_vec;
  @ppc_vec = sort {$b <=> $a} @ppc_vec;
  @vol_vec = sort {$b <=> $a} @vol_vec;
  if ( $#ttc_vec < $dynamic_cutoff_rank_ ) { 
    $mail_body_ .= "Hey not sure how, but your cut-off($dynamic_cutoff_rank_) is greater than arraysize($#ttc_vec), So skipping dynamic setting.\n" ;
  }
  if ( $#ttc_vec >= $dynamic_cutoff_rank_ && $ttc_vec[$dynamic_cutoff_rank_] ne "" && $ttc_vec[$dynamic_cutoff_rank_] > $max_ttc_to_allow_ )
  {
    $mail_body_ .= "Max ttc reset from $max_ttc_to_allow_ ";
    $max_ttc_to_allow_ =  $ttc_vec[$dynamic_cutoff_rank_] ;
    $mail_body_ .= "to $max_ttc_to_allow_\n";
  }
  if ( $#vol_vec >= $dynamic_cutoff_rank_ && $vol_vec[$dynamic_cutoff_rank_] ne "" && $vol_vec[$dynamic_cutoff_rank_] < $min_volume_to_allow_ )
  {
    $mail_body_ .= "Min vol reset from $min_volume_to_allow_ ";
    $min_volume_to_allow_ = $vol_vec[$dynamic_cutoff_rank_] ;
    $mail_body_ .= "to $min_volume_to_allow_\n";
  }
  if ( $#ppc_vec >= $dynamic_cutoff_rank_ && $ppc_vec[$dynamic_cutoff_rank_] ne "" && $ppc_vec[$dynamic_cutoff_rank_] < $min_pnl_per_contract_to_allow_ && $ALLOW_PPC_RESET == 1 )
  {
    $mail_body_ .= "Min ppc reset from $min_pnl_per_contract_to_allow_ ";
    $min_pnl_per_contract_to_allow_ = $ppc_vec[$dynamic_cutoff_rank_] ;
    $mail_body_ .= "to $min_pnl_per_contract_to_allow_\n";
  }
}

sub FilterOnMinDaysTraded
{
  my $slrac_lines_ref_ = shift;
  print $main_log_file_handle_ "Using MinDaysTradedCutoff\n";

  my @slrac_lines_dup_ = @$slrac_lines_ref_;
  @$slrac_lines_ref_ = ( );

  my $traded_days_cutoff_ = int( ($#trading_days_+1) * $min_days_traded_perc_ + 0.5 );

  my $curr_days_count_ = 0;
  my $curr_strat_start_;

  foreach my $sline_idx_ ( 0..$#slrac_lines_dup_ ) {
    my $sline_ = $slrac_lines_dup_[ $sline_idx_ ];

    if ( $sline_ =~ /^STRATEGYFILEBASE/ ) {
      $curr_days_count_ = 0;
      $curr_strat_start_ = $sline_idx_;
    }
    elsif ( $sline_ =~ /^STATISTICS/ ) {
      if ( defined $curr_strat_start_ ) {
        if ( $curr_days_count_ >= $traded_days_cutoff_ ) {
          push ( @$slrac_lines_ref_, @slrac_lines_dup_[ $curr_strat_start_..$sline_idx_ ] );
          push ( @$slrac_lines_ref_, "\n" );
        }
        else {
          my $curr_strat_ = (split(/\s+/, $slrac_lines_dup_[ $curr_strat_start_ ]))[1];
          print $main_log_file_handle_ "Strategy $curr_strat_ has non-0 Volume for just $curr_days_count_ days; Minimum required trading_days: $traded_days_cutoff_\n";
        }
      }
    }
    elsif ( $sline_ =~ /^201/ ) {
      $curr_days_count_++;
    }
  }
}

sub MedianCutoff
{
  my ( $slrac_lines_ref_, $strats_selected_ref_, $strat_indices_map_ref_ ) = @_;
  print $main_log_file_handle_ "Using Median Cutoff\n";

  my $global_results_dir_path = "/NAS1/ec2_globalresults/$shortcode_/";
  my $srv_name_=`hostname | cut -d'-' -f2`; chomp($srv_name_);
  if($srv_name_ =~ "crt")
  {
    $global_results_dir_path = $HOME_DIR."/ec2_globalresults/$shortcode_/";
  }
  
  (my $localresults_dir_path = $unique_results_filevec_[0]) =~ s/201[0-9]\/.*//;
  my @global_results_files = @unique_results_filevec_;
  foreach my $fl_(@global_results_files)
  {
    my $global_fl_ = $fl_; $global_fl_ =~ s/$localresults_dir_path/$global_results_dir_path/g;
    if ( ! -e $global_fl_ ) { next;  }
    print $main_log_file_handle_ "cat $global_fl_ >> $fl_\n";
    `cat $global_fl_ >> $fl_`;
  }

  my @all_strats_in_dir_ = ( );
  my $timeperiod_ = "$trading_start_hhmm_-$trading_end_hhmm_";

  if( $traded_ezone_ ne ""  ) {
    my $dir_base_ = $traded_ezone_pref_;
    $dir_base_ .= "-".$pool_tag_ if $pool_tag_ ne "";
    @all_strats_in_dir_ = MakeStratVecFromDir ( $MODELING_STRATS_DIR."/".$shortcode_."/EBT/".$dir_base_ );
  }
  elsif ( $pool_tag_ ne "" ) {
    @all_strats_in_dir_ = MakeStratVecFromDirAndTTAndTag($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_, $pool_tag_);
  }
  else {
    @all_strats_in_dir_ = MakeStratVecFromDirAndTT($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_);
  }

  my $cstempfile_ = GetCSTempFileName ( $work_dir_."/cstemp" );
  open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
  print $main_log_file_handle_ "All global strats: ".join("\n", @all_strats_in_dir_)."\n";
  for(my $i=0; $i <= $#all_strats_in_dir_; $i++){
    my $t_strat_file_ = basename($all_strats_in_dir_[$i]);
    print CSTF "$t_strat_file_\n";
  }
  my $num_strats_in_global_results_ = $#all_strats_in_dir_ + 1;
  for ( @{$slrac_lines_ref_} ){
    if ( $_ =~ /STRATEGYFILEBASE/ ){
      my $strat_line = $_;
      my $strat_ = (split(' ', $strat_line))[1];
      print CSTF "$strat_\n";
    }
  }
  close CSTF;

  print $main_log_file_handle_ "Num of strats in global result: $num_strats_in_global_results_\n.cut_off_rank_ for installation is $cut_off_rank_\n";

  my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $cstempfile_ $local_results_base_dir $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ INVALIDFILE $historical_sort_algo_";
  print $main_log_file_handle_ $exec_cmd."\n";
  my @global_res_out_ = `$exec_cmd`; 
  print $main_log_file_handle_ "@global_res_out_\n";chomp(@global_res_out_);

  if($num_strats_in_global_results_ <=0 || $cut_off_rank_ <=0) 
  {
    $mail_body_ .= "No previous strat in the pool to compare\n";
  }
  else
  {
    $mail_body_ .= "=============================  STRAT-POOL STATISTICS (Size: $num_strats_in_global_results_) =======================\n";
    $mail_body_ .= "     pnl  pnl_stdev  volume  pnl_shrp  pnl_cons  pnl_median  ttc  pnl_zs  avg_min_max_pnl  PPT  S B A  MAXDD\n";
    for (my $i=0; $i<$num_strats_in_global_results_ * 0.75 + 5; $i += int($cut_off_rank_/2.0)){
      if ( $i > $#global_res_out_ ) { next; }
      my @t_line_ = split(/ /, $global_res_out_[$i]);
      $mail_body_ .= "[$i]: @t_line_[2 .. $#t_line_]\n";
    }
    $mail_body_ .= "==========================\n\n";
  }

  for ( @{$slrac_lines_ref_} )
  {
    if ( $_ =~ /STRATEGYFILEBASE/ )
    {
      my $strat_line = $_;
      my $strat_ = (split(' ', $strat_line))[1];
      my @index_list = grep {$global_res_out_[$_] =~ $strat_} 0 .. $#global_res_out_;
      if ($#index_list >= 0 && $index_list[0] <= $cut_off_rank_){
        push(@$strats_selected_ref_, $strat_);
      }
      if($#index_list >= 0 ){
        $$strat_indices_map_ref_{$strat_} = int($index_list[0]);
      }
    }
  }

  print $main_log_file_handle_ "\n====================================\nAccepted Strats via global_result_analysis:\n@$strats_selected_ref_\n";
  while (my ($key, $value) = each(%$strat_indices_map_ref_) ){
    print $main_log_file_handle_ "$key: $value\n";
  }
  print $main_log_file_handle_ "=======================================\n";
}

sub ValidationMedianCutoff
{
  my ( $strats_selected_ref_, $best_validation_result_ ) = @_;
  print $main_log_file_handle_ "Using Median Cutoff on Validation Set\n";

  my $exec_cmd="$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo $historical_sort_algo_ 1 1 $min_pnl_per_contract_to_allow_ $min_volume_to_allow_ $max_ttc_to_allow_ $max_message_count_ $local_validation_results_base_dir | grep ST";
  print $main_log_file_handle_ "$exec_cmd\n";
  @$best_validation_result_ = `$exec_cmd`; chomp ( @$best_validation_result_ );

  my $global_results_dir_path = "/NAS1/ec2_globalresults/$shortcode_/";
  my $srv_name_=`hostname | cut -d'-' -f2`; chomp($srv_name_);
  if($srv_name_ =~ "crt")
  {
    $global_results_dir_path = $HOME_DIR."/ec2_globalresults/$shortcode_/";
  }
  
  (my $localresults_dir_path = $unique_validation_results_filevec_[0]) =~ s/201[0-9]\/.*//;
  my @global_results_files = @unique_validation_results_filevec_;
  foreach my $fl_(@global_results_files)
  {
    my $global_fl_ = $fl_; $global_fl_ =~ s/$localresults_dir_path/$global_results_dir_path/g;
    if ( ! -e $global_fl_ ) { next;  }
    print $main_log_file_handle_ "cat $global_fl_ >> $fl_\n";
    `cat $global_fl_ >> $fl_`;
  }

  my @all_strats_in_dir_ = ( );
  my $timeperiod_ = "$trading_start_hhmm_-$trading_end_hhmm_";

  if ( $traded_ezone_ ne ""  ) {
    my $dir_base_ = $traded_ezone_pref_;
    $dir_base_ .= "-".$pool_tag_ if $pool_tag_ ne "";
    @all_strats_in_dir_ = MakeStratVecFromDir ( $MODELING_STRATS_DIR."/".$shortcode_."/EBT/".$dir_base_ );
  }
  elsif ( $pool_tag_ ne "" ) {
    @all_strats_in_dir_ = MakeStratVecFromDirAndTTAndTag($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_, $pool_tag_);
  }
  else {
    @all_strats_in_dir_ = MakeStratVecFromDirAndTT($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_);
  }

  my $cstempfile_ = GetCSTempFileName ( $work_dir_."/cstemp" );
  open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
  print $main_log_file_handle_ "All global strats: ".join("\n", @all_strats_in_dir_)."\n";
  for(my $i=0; $i <= $#all_strats_in_dir_; $i++){
    my $t_strat_file_ = basename($all_strats_in_dir_[$i]);
    print CSTF "$t_strat_file_\n";
  }
  my $num_strats_in_global_results_ = $#all_strats_in_dir_ + 1;
  print CSTF join("\n", @$strats_selected_ref_ )."\n";
  close CSTF;

  $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $cstempfile_ $local_validation_results_base_dir $validation_start_yyyymmdd_ $validation_end_yyyymmdd_ INVALIDFILE $historical_sort_algo_";
  print $main_log_file_handle_ $exec_cmd."\n";
  my @global_res_out_ = `$exec_cmd`; 
  print $main_log_file_handle_ "@global_res_out_\n";chomp(@global_res_out_);

  if($num_strats_in_global_results_ <=0 || $cut_off_rank_ <=0) 
  {
    $mail_body_ .= "No previous strat in the pool to compare\n";
  }
  else
  {
    $mail_body_ .= "=============================  STRAT-POOL STATISTICS FOR VALIDATION DAYS (Size: $num_strats_in_global_results_) =======================\n";
    $mail_body_ .= "     pnl  pnl_stdev  volume  pnl_shrp  pnl_cons  pnl_median  ttc  pnl_zs  avg_min_max_pnl  PPT  S B A  MAXDD\n";
    for (my $i=0; $i<$num_strats_in_global_results_ * 0.75 + 5; $i += int($cut_off_rank_/2.0)){
      if ( $i > $#global_res_out_ ) { next; }
      my @t_line_ = split(/ /, $global_res_out_[$i]);
      $mail_body_ .= "[$i]: @t_line_[2 .. $#t_line_]\n";
    }
    $mail_body_ .= "==========================\n\n";
  }

  my @filtered_strats_ = ( );

  foreach my $strat_ ( @$strats_selected_ref_ ) {
    my @index_list = grep {$global_res_out_[$_] =~ $strat_} 0 .. $#global_res_out_;
    if ($#index_list >= 0 && $index_list[0] <= $cut_off_rank_){
      push(@filtered_strats_, $strat_);
    }
  }
  @$strats_selected_ref_ = @filtered_strats_;
}

sub PPCCutoff
{
  my ( $slrac_lines_ref_, $strats_selected_ref_ ) = @_;
  print $main_log_file_handle_ "Using PPC cutoff\n";
  my $pnl_per_contract_word_index_ = 9; # PNL_PER_CPONTRACT is 9th word in STATISTICS line in summarize_single_strategy_results
  my $last_strat_file_selected_ = "";
  
  for ( my $t_slrac_output_lines_index_ = 0 ; $t_slrac_output_lines_index_ <= $#$slrac_lines_ref_ ; $t_slrac_output_lines_index_++ )
  {
    my $this_line_ = $$slrac_lines_ref_ [$t_slrac_output_lines_index_];
    if ( $this_line_ =~ /STRATEGYFILEBASE/ )
    { # STRATEGYFILEBASE basename of strategyfile
      my @strat_line_words_ = split ( ' ', $this_line_ );
      if ( $#strat_line_words_ >= 1 )
      { # STRATEGYFILEBASE w_strategy_ilist_ZB_EU_PBT_30_na_e3_20110629_20110729_CET_805_EST_800_fsg.5_FSRR.0.5.0.01.0.0.0.85.tt_CET_805_EST_800.pfi_0
        $last_strat_file_selected_ = $strat_line_words_[1];
      }
    }
    if ( $this_line_ =~ /STATISTICS/ )
    { # STATISTICS ....
      if ( $last_strat_file_selected_ )
      { # not ""
        my @stat_line_words_ = split ( ' ', $this_line_ );
        if ( $#stat_line_words_ >= $pnl_per_contract_word_index_ )
        {
          my $t_insample_pnl_per_contract_ = $stat_line_words_[$pnl_per_contract_word_index_];
          if ( $t_insample_pnl_per_contract_ > $min_pnl_per_contract_to_allow_ )
          {
            push ( @$strats_selected_ref_, $last_strat_file_selected_ );
            $last_strat_file_selected_ = "";
          }
        }
      }
    }
  }
}

sub InstallStrategiesAndLog
{ 
  my ( $strats_selected_ref_, $strat_indices_map_ref_ ) = @_;
  my @strat_results_to_save_ = ( );
  my @config_results_to_save_ = ( );

  my $MAX_STAGED_POOLSIZE = 50;
    
  my $datagen_start_end_str_ = $datagen_start_hhmm_."-".$datagen_end_hhmm_ ;
  my $trading_start_end_str_ = $trading_start_hhmm_."-".$trading_end_hhmm_ ;
  $trading_start_end_str_= "EBT/$traded_ezone_pref_" if $traded_ezone_ ne "";
  $trading_start_end_str_ .= "-".$pool_tag_ if $pool_tag_ ne "";
  
  foreach my $strat_ ( @$strats_selected_ref_ ) {
    $mail_body_ = $mail_body_."For strat: ".$strat_."\n";
    print $main_log_file_handle_ "For strat: ".$strat_."\n";
# in sample results
    my $exec_cmd = "$LIVE_BIN_DIR/summarize_single_strategy_results $shortcode_ $strat_ $work_dir_/local_results_base_dir $trading_start_yyyymmdd_ $trading_end_yyyymmdd_";
    print $main_log_file_handle_ "$exec_cmd\n";
    my @t_insample_text_ = `$exec_cmd`;
    if ( $RUN_SINGLE_INDICATOR_LIST_FILE ) { $mail_body_ .= "Ilist File: ".$ilist_filename_vec_[0]."\n"; }
    $mail_body_ = $mail_body_."\nTRADING DAYS\n".join ( "", @t_insample_text_ );

    if ( $use_median_cutoff_ && $use_validation_median_check_ ) {
      $exec_cmd = "$LIVE_BIN_DIR/summarize_single_strategy_results $shortcode_ $strat_ $work_dir_/local_validation_results_base_dir $validation_start_yyyymmdd_ $validation_end_yyyymmdd_ | grep ST";
      print $main_log_file_handle_ "$exec_cmd\n";
      my @t_validation_insample_text_ = `$exec_cmd`;
      $mail_body_ = $mail_body_."VALIDATION DAYS\n".join ( "", @t_validation_insample_text_ );
    }

    $mail_body_ = $mail_body_."\nSTATISTICS PNL_avg PNL_stdev VOL_avg PNL_sharpe PNL_<avg-0.33*stdev> avg_min_adj_PNL median_TTC PPC NBL_order BL_order AGG_order avg_MAX_DD\n";

    if($use_median_cutoff_){
      $mail_body_ .= "\nRank (in Trading Days) in the existing Pool: $$strat_indices_map_ref_{$strat_}\n";
    }
    print $main_log_file_handle_ @t_insample_text_."\n";

    my $t_outsample_pnl_per_contract_ = GetOutSamplePPC ( $strat_ );
    if ( $install_ ) {
      my $staged_poolsize_ = GetStagedPoolSize ( $shortcode_, $trading_start_end_str_ );

      if ( $staged_poolsize_ < $MAX_STAGED_POOLSIZE ) {
        if ( $t_outsample_pnl_per_contract_ == -1 || $t_outsample_pnl_per_contract_ > $min_pnl_per_contract_to_allow_ )
        {
# Install if positive in outsample
          my $t_temp_strategy_filename_ = FindItemFromVecWithBase ( $strat_, @strategy_filevec_  ) ;
          printf $main_log_file_handle_ "InstallStrategyModelling ( $t_temp_strategy_filename_, $shortcode_, $datagen_start_end_str_, $trading_start_end_str_ )\n";
          my $final_stratname_ = InstallStrategyModelling ( $t_temp_strategy_filename_, $shortcode_, $datagen_start_end_str_, $trading_start_end_str_, $prod_install_ );
          
          my $final_configname_ = basename ( $final_stratname_ ).'.config';

          ### create walkforward config and upload to DBs START
          ### pass additional arguments to tell this script if the legacy strat is from a staged_pool or now, further if the strat is simula_approved?
          ### typically if a strat is created from genstrat, it is not simula approved yet. Hence A is 0 and P is set to S

          printf $main_log_file_handle_ "$HOME_DIR/basetrade/walkforward/wf_db_utils/scan_strats_and_upload_to_configsdb.py -S $final_stratname_ -A 0 -P 'S' -f 1 2>/dev/null\n";
          `$HOME_DIR/basetrade/walkforward/wf_db_utils/scan_strats_and_upload_to_configsdb.py -S $final_stratname_ -A 0 -P 'S' -f 1 2>/dev/null\n`;
          ### create walkforward config in correct directories and upload to DBs END

          ## push strats and configs for run simulations
          push ( @strat_results_to_save_, $final_stratname_ );
          push ( @config_results_to_save_, $final_configname_ );

          if ( $t_outsample_pnl_per_contract_ == -1 ) {
            printf $main_log_file_handle_ "Installing %s without outsample pnlcheck\n", $strat_;
            $mail_body_ = $mail_body_."Installing ".$strat_." without outsample pnlcheck\n";
          }
        }
        else
        {
          printf $main_log_file_handle_ "Failed to install %s since outsample pnl-avg = %d\n", $strat_, $t_outsample_pnl_per_contract_;
          $mail_body_ = $mail_body_."\nERROR: Failed to install ".$strat_." since outsample pnl-avg = ".$t_outsample_pnl_per_contract_."\n";
        }
      }
      else {
        printf $main_log_file_handle_ "Failed to install %s since Staged_Poolsize: %d >= %d\n", $strat_, $staged_poolsize_, $MAX_STAGED_POOLSIZE;
        $mail_body_ = $mail_body_."\nERROR: Failed to install $strat_ since Staged_Poolsize: $staged_poolsize_ >= $MAX_STAGED_POOLSIZE\n";
      } 
    }
  }
  if ( $install_ ) {
    if ( $#config_results_to_save_ >= 0 ) {
      RunSaveResults ( \@config_results_to_save_ );

      GenerateStory ( \@config_results_to_save_ );
      GenerateCorrelations ( \@config_results_to_save_, $trading_start_end_str_ );
    }
  }
  else
  {
    $mail_body_ .= " INSTALL flag is set to 0 \n\n";
  }
}

sub GenerateCorrelations
{
  my $configs_vec_ref_ = shift;
  my $trading_start_end_str_ = shift;
  my $numdays_ = 200;

  print $main_log_file_handle_ "\nGenerateCorrelations\n";

  my @pool_strats_ = `$HOME_DIR/basetrade/walkforward/get_pool_configs.py -m CONFIG -c $$configs_vec_ref_[0] 2>/dev/null`;
  chomp ( @pool_strats_ );

  my @dates_vec_ = GetDatesFromNumDays ( $shortcode_, $yyyymmdd_, $numdays_ );

  my %sample_pnls_strats_vec_;
  FetchPnlSamplesStrats ( $shortcode_, $configs_vec_ref_, \@dates_vec_, \%sample_pnls_strats_vec_ );
  FetchPnlSamplesStrats ( $shortcode_, \@pool_strats_, \@dates_vec_, \%sample_pnls_strats_vec_ );

  foreach my $new_strat_ ( @$configs_vec_ref_ ) {
    foreach my $pool_strat_ ( @pool_strats_ ) {
      my $similarity_ = GetPnlSamplesCorrelation ( $new_strat_, $pool_strat_, \%sample_pnls_strats_vec_ );
      InsertCorrelation($shortcode_, $new_strat_, $pool_strat_, $similarity_, 1);
    }
  }
} 

sub GenerateStory
{
  my $configs_vec_ref_ = shift;
  my $story_config_file_ = "INVALIDFILE";

  print $main_log_file_handle_ "\nGenerateStory:\n";
  $mail_body_ = $mail_body_."\nInstalled Stratlinks:\n";

  foreach my $strat_ ( @$configs_vec_ref_ ) {
    my $storygen_cmd_ = $HOME_DIR."/".$REPO."_install/WKoDii/feature_to_DB.pl $shortcode_ $story_config_file_ $strat_";
    print $main_log_file_handle_ "$storygen_cmd_\n";
    `$storygen_cmd_ 1>/dev/null 2>/dev/null`;
    $mail_body_ = $mail_body_."http://52.0.55.252:2080/strategy_page?sname=".basename($strat_)."\n";
  }
}

sub SendMail
{
  if ( ( $mail_address_ ) &&
      ( $mail_body_ ) )
  {
    open(MAIL, "|/usr/sbin/sendmail -t");

## Mail Header
    print MAIL "To: $mail_address_\n";
    print MAIL "From: $mail_address_\n";
    print MAIL "Subject: genstrat ( $instructionfilename_ ) $yyyymmdd_ $hhmmss_ $hostname_\n\n";
## Mail Body
    print MAIL $mail_body_ ;

    close(MAIL);

    print $main_log_file_handle_ "Mail Sent to $mail_address_\n$mail_body_\n";
  }
}

sub ComputeStatsSingleRegFile
{ # compute correlations of main file
  my ( $main_file_extension_, $indicator_list_filename_ ) = @_;
  
  print $main_log_file_handle_ "ComputeStatsSingleRegFile\n";

  my $this_reg_data_filename_ = $reg_data_dir."/reg_data_".$main_file_extension_;

  my $this_corr_filename_ = $reg_data_dir."/corr_".$main_file_extension_;
  my $this_corrmatrix_filename_ = $reg_data_dir."/corrmatrix_".$main_file_extension_;
  my $this_var_corr_filename_ = $reg_data_dir."/var_corr_".$main_file_extension_;
  my $this_high_corr_ind_filename_ = $reg_data_dir."/hcp_".$main_file_extension_;

  if ( $SAVE_CORR_FILE == 0 )
  { push ( @intermediate_files_, $this_corr_filename_ ); }
  if ( $SAVE_CORRMATRIX_FILE == 0 )
  { push ( @intermediate_files_, $this_corrmatrix_filename_ ); }
  if ( $SAVE_VAR_CORR_FILE == 0 )
  { push ( @intermediate_files_, $this_var_corr_filename_ ); }
  if ( $SAVE_HCP_FILE == 0 )
  { push ( @intermediate_files_, $this_high_corr_ind_filename_ ); }

  if ( $SAVE_VAR_CORR_FILE != 0 )
  { # the following code is only required if we are saving var_corr file
    if ( ! ( -e $this_corr_filename_ ) )
    {
      my $exec_cmd="$LIVE_BIN_DIR/get_dep_corr $this_reg_data_filename_ > $this_corr_filename_";
      print $main_log_file_handle_ "$exec_cmd\n";
      `$exec_cmd`;
    }
    if ( ! ( -e $this_var_corr_filename_ ) )
    {
      my $exec_cmd="$MODELSCRIPTS_DIR/sort_by_corr.pl $indicator_list_filename_ $this_corr_filename_ > $this_var_corr_filename_";
      print $main_log_file_handle_ "$exec_cmd\n";
      `$exec_cmd`;
    }
  }

  if ( $SAVE_HCP_FILE != 0 )
  { # the following code is only required if HCP file is being saved
    if ( ! ( -e $this_corrmatrix_filename_ ) )
    {
      my $exec_cmd="$LIVE_BIN_DIR/get_correlation_matrix $this_reg_data_filename_ > $this_corrmatrix_filename_";
      print $main_log_file_handle_ "$exec_cmd\n";
      `$exec_cmd`;
    }
    if ( ! ( -e $this_high_corr_ind_filename_ ) )
    {
      my $exec_cmd="$MODELSCRIPTS_DIR/print_highly_correlated_indicator_pairs.pl $indicator_list_filename_ $this_corrmatrix_filename_ > $this_high_corr_ind_filename_";
      print $main_log_file_handle_ "$exec_cmd\n";
      `$exec_cmd`;
    }
  }
}

sub PrintStacktraceAndSendMailDie
{
  my $err_str_ = shift;

  $mail_body_ .= "\n*Error*: $err_str_\n";
  SendMail ( );
  PrintStacktraceAndDie ( $err_str_ );
}

sub PredDurationToTickValues 
{
  @predduration_ =@upper_threshold_;
  @predalgo_ = ('na_e3'); #Putting a random value here to help with file naming and also to remove iterating through different pred algos
}

sub CleanFiles 
{
  my $filevec_ref_ = shift || \@intermediate_files_;

  if ( $delete_intermediate_files_ ) {
    print $main_log_file_handle_ "Delete_Intermediate_Files\n";

    foreach my $fname_ ( @$filevec_ref_ ) {
      if ( -e $fname_ ) {
        my $rm_cmd_ = "rm -f $fname_";
        print $main_log_file_handle_ $rm_cmd_."\n"; `$rm_cmd_`;
      }
    }
  }
}

sub signal_handler
{
  PrintStacktraceAndDie("Caught a signal in gen_strats $!\n");
}

sub END
{
  CleanFiles();
}
