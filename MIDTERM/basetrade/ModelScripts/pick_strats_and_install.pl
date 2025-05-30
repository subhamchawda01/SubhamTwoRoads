#!/usr/bin/perl

# \file ModelScripts/pick_strats_and_install.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#	   Suite No 162, Evoma, #14, Bhattarhalli,
#	   Old Madras Road, Near Garden City College,
#	   KR Puram, Bangalore 560049, India
#	   +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Mojolicious::Lite;
use Mojo::JSON qw(decode_json encode_json);
use Math::Complex; # sqrt
use FileHandle;
use Scalar::Util qw(looks_like_number);
use POSIX;
use Term::ANSIColor;
use sigtrap qw(handler signal_handler normal-signals error-signals);

use Term::ANSIColor 1.04 qw(uncolor);
my $names = uncolor('01;31');

sub LoadConfigFile;
sub SanityCheckConfigParams;
sub GetRealResults;
sub GetGlobalResults;
sub ComputeSimRealBias;
sub CompensateSimRealBias;
sub ScoreStrats;
sub PickStrats;
sub AssignTotalSizes;
sub SetStratUTS;
sub ComputeOptimalMaxLossCombined;
sub ComputeGlobalMaxloss;
sub DiversifyAndScore;
sub ComputeOptimalMaxLoss;
sub ComputeOptimalMaxLossAllDays;
sub ComputeOptimalStdevMaxLoss;
sub SetOMLSettings;
sub CheckResetMaxLosses;
sub GetLocalStratFromConfig;
sub InstallStrats;
sub ClearProductionStratList;
sub AddToProductionStratList;
sub BackupExistingProductionInstall;
sub FixProductionCrontab;
sub InstallProductionCrontab;
sub ScaleInstalledProductionParams;
sub SortPickedStratsByCombingKey;
sub CombineProductionStrats;
sub DisplayMsgCountInfo;
sub FillRatioCheck;
sub InstallProductionStratList;
sub RemoveIntermediateFiles;
sub ScaleParamFile;
sub SendReportMail;
sub IsTooInsample;
sub GetScoreFromResultLineAndSortAlgo;
sub NormalizeIntervalScores;
sub PrintErrorAndDie;
sub SendErrorMail;
sub SendErrorMailAndDie;
sub SendErrorMailifAutomatedAndDie;
sub CreateRunningLocks;
sub RemoveRunningLocks;
sub CreateDoneLocks;
sub AskUserPrompt;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $PYTHONEXEC = "python3.5";
my $REPO = "basetrade";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $WF_SCRIPTS_DIR=$HOME_DIR."/".$REPO."/walkforward";

my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list";
my $local_production_strat_list_file_ = "";
my $REMOTE_PRODUCTION_STRAT_LIST_DIR = "/home/dvctrader/production_strat_list";
my $remote_production_strat_list_file_ = "";
my $local_single_strat_file_ = "";
my $GLOBALRESULTSDBDIR = "DB"; # DB is the only option now (ec2_globalresults no longer maintained)

my $PICKSTRATS_DIR = "/spare/local/pickstrats_logs";
my $PICKSTRAT_TEMP_DIR = $PICKSTRATS_DIR."/temp_dir";

my $lock_manager_script = $HOME_DIR."/".$REPO."/scripts/pick_strats_lock.sh";

my @intermediate_files_ = ( );

require "$GENPERLLIB_DIR/sqrt_sign.pl"; # SqrtSign
require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/array_ops.pl"; # GetSum, GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/filter_combinable_strategies_for_date.pl";
require "$GENPERLLIB_DIR/strat_utils.pl"; # GetRegimeInd
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetStratsWithGlobalResultsForShortcodeDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; 
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats, PnlSamplesGetStats
require "$GENPERLLIB_DIR/read_shc_machine_mapping.pl"; # GetMachineForProduct
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # GetPickstratConfigId InsertPickstratConfigId InsertPickstratRecord
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; # FetchCorrelationForPair
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl"; # GetPnlSamplesCorrelation
require "$GENPERLLIB_DIR/install_strategy_production.pl";
require "$GENPERLLIB_DIR/copy_strategy.pl";
require "$GENPERLLIB_DIR/lock_utils.pl";
require "$GENPERLLIB_DIR/config_utils.pl"; # IsValidConfig

# Exec dependencies
my @ADDITIONAL_EXEC_PATHS=();
# Execs to be used here
my $get_contract_specs_exec = SearchExec ( "get_contract_specs", @ADDITIONAL_EXEC_PATHS ) ;
my $SUMMARIZE_SINGLE_STRATEGY_RESULTS = SearchExec ( "summarize_single_strategy_results", @ADDITIONAL_EXEC_PATHS ) ;
my $SUMMARIZE_EXEC = SearchExec ( "summarize_strategy_results", @ADDITIONAL_EXEC_PATHS ) ;
my $NOTIONAL_TO_UTS_NSE = SearchExec ( "notional_to_uts_nse", @ADDITIONAL_EXEC_PATHS ) ;

if ( ! $get_contract_specs_exec && ! $SUMMARIZE_EXEC ) {
  exit(0);
}

# start
my $verbose_ = 0 ;
my $ignore_new_strats_ = 0;
my $add_to_crontab_ = 1;    # if zero this argument would not change the crontab file on the server and put all the strategies in a single local file. Only works whem install is set to 1.

my $exit_on_simultaneous_run_ = 0; # if 0: ask for user prompt, if 1: directly exit

my $min_days_frac_ = 0.75; # Each Strat considered for automated picking must have atleast these number no. of days

# The Common days of the considered strats must be atleast $common_days_frac_ * ( Total Days )
# Set to 0 to avoid exiting in unnecessay conditions
my $common_days_frac_ = 0; 

my $print_OML_ = 0;

my $USAGE="$0 SHORTCODE TIMEPERIOD CONFIGFILE [ VERBOSE = 0/1 ] [ print_OML = 0/1 ] [add_to_crontab = 1/0, default:1] [exit_on_simultaneous_run = 1/0, default:0]";

my $sum_num_strats_to_install_ = 0;
my $prod_query_start_id_ = 0;
my $prod_query_stop_id_ = 0;
my $fpga_flag_ = 1;  # Only for CME
my $risk_tags_;  # Tags for risk monitor computations

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];

my $config_file_ = $ARGV [ 2 ];

if ( $#ARGV > 2 ) {
  $verbose_ = $ARGV [ 3 ];
}

if ( $#ARGV > 3 ) {
  $print_OML_ = $ARGV [ 4 ];
}

if ( $#ARGV > 4 ) {
  $add_to_crontab_ = $ARGV [ 5 ];
}

if ( $#ARGV > 5 ) {
  $exit_on_simultaneous_run_ = $ARGV [ 6 ];
}

my $exchange_ = "";

my %strat_name_to_subset_index_ = ( );

my @num_strats_to_install_ = ( );
my $num_strat_files_to_install_ = 1;
my @total_size_to_run_ = ( );
my @total_size_to_run_notional_ = ( );
my $use_total_size_from_max_loss_ = 0;
my %strat_copy_path_map_ = ( );
my %strat_localpath_to_base_ = ( );
my %queryid_to_combined_stratpath_ = ( );
my %queryid_to_remote_stratpath_ = ( );
my %queryid_to_stratbase_ = ( );

my %strat_to_uts_ = ( );
my %strat_to_uts_or_notional_ = ( );
my %strat_to_ceiled_uts_or_notional_ = ( );
my %strat_to_maxlosses_ = ( );
my %strat_to_maxposition_ = ( );

my $pickstrats_date_ = `date +%Y%m%d`; chomp ( $pickstrats_date_ );
my $curr_hh_ = `date +%H%M`; chomp ( $curr_hh_ );
if ( $curr_hh_ >= 2200 )
{
  $pickstrats_date_ = CalcNextDate ( $pickstrats_date_ );
}
my $curr_date_ = $pickstrats_date_;

# max_loss_comments :
# default is set to use optimal loss 
# the only checks we are interested to apply here are  ( max > opentrade  and shortterm loss ), max_loss < max_max_loss specified in the config file and max_loss > min_max_loss
# the installation will not proceed if any of the checks that are laid above are failed, to ensure the correctness
# we are interested in choosing optimal loss, however the period to calculate must be well thought of, for the value returned from the script will not accomadate the following concern
# allowed to disagree : if the correlations are not consistent, using the statistcs from longer period ( which include multiple regimes ) is not the best option ( do u want to optmize for all regimes / not is a choice )
# for example :
# assuming the signs are atleast consistent, if correlation of ( dep , indep1 ) is higher than ( dep , indep2 ) in half the period 
#	 and ( dep , indep1 ) is lower than ( dep, indep2 ) in the other half, the best number to use is the one calculated using the data from that current expectations period ( in cor space, i.e regime ).

my $uts_string_ = "UNIT_TRADE_SIZE";
my @max_loss_per_unit_size_ = ( );
my @use_optimal_max_loss_per_unit_size_ = ( );
my $min_max_loss_per_unit_size_ = -1;
my $max_max_loss_per_unit_size_ = -1;
my $max_loss_sim_real_bias_per_unit_size_ = 0;

my $min_max_loss_per_strat_;
my $max_max_loss_per_strat_;

my $read_OML_settings_ = 0 ;
my $OML_lookback_days_ = 0 ;
my $OML_hit_ratio_ = 0 ;
my $OML_number_top_loss_ = 0 ;

my $read_OSML_settings_ = 0;
my $OSML_lookback_days_ = 0 ;
my $OSML_hit_ratio_ = 0 ;
my $OSML_high_volatile_percentile_ = 0 ;
my $OSML_is_volatile_ = 0;

# opentrade_loss_comments :
# this has to from param the file.( read period ). for sim results are based on it, so setting the default flag to 1
# however if specified not to use specifically from param_file a warning is thrown showing the discrepancies in param_file and config_file but the installation will proceed.
# since opentrade loss param is learnt using the data sticking to paramfile number is the best option

my $use_paramfile_opentrade_loss_per_unit_size_ = 1;
my $opentrade_loss_per_unit_size_ = 0;

my $short_term_loss_per_unit_size_ = 0;

my $max_global_position_ = -1;
my $global_max_loss_ = -1;
my $ignore_global_max_loss_ = 0;
my $computed_global_maxloss_;
my $use_computed_global_max_loss_ = 0;
my $disable_sanity_check_global_maxloss_ = 0;

my @min_volume_per_strat_ = ( );
my @max_ttc_per_strat_ = ( );
my @max_mur_ = ( );
my @strats_to_keep_ = ( );
my @strats_to_exclude_ = ( );
my $max_message_count_cutoff_ = 0 ;
my $normalize_score_by_sqrt_message_count_ = 0 ;
my $normalize_score_by_message_count_ = 0 ;
my $release_core_premature_; # undefined indicates no user input

my @fld_time_periods_ = ( );
my @strats_in_pool_ = ( );

my $dayfeatures_config_file_ = -1; # -1 indicates the script to use the DEFALLT config file

my $dayfeatures_special_events_file_ = "IF"; # lists the events for which we would pick the dayfeatures from the last occurance of those event

my %intervals_to_pick_from_ = ( );
my %intervals_to_days_ = ( );
#my %intervals_to_days_map_ = ( );

my %intervals_to_pick_from_weighted_ = ( );
my $use_intervals_days_weight_vec_ = 0;

my %intervals_start_day_ = ( );
my %intervals_to_file_ = ( );
my $combined_days_file_ = "";

my %diversity_type_to_weight_ = ( );
my $diversity_threshold_ = 1.00; ## default: no diversity constraint
my $optimal_max_loss_check_ = 0;
my $max_loss_pnl_cutoff_ = 0;

my @sort_algo_ = ( );
my $email_address_ = "";
my @exchanges_to_remove_ = ( );
my @shortcodes_to_remove_ = ( );

my $use_combined_metric_ = 0;
my $combined_sort_algo_ = "";

my $install_picked_strats_ = 0;
my $install_location_ = "";
my $exec_start_hhmm_ = 0;
my $exec_end_hhmm_ = 0;
my $onload_trade_exec_ = 0;
my $vma_trade_exec_ = 0;
my $affinity_trade_exec_ = 0;
my $use_sharpe_pnl_check_ = 0;
my $pnl_prediction_algo_ = "None";
my $pnl_prediction_num_days_ = 0;

my $feature_modelfile_ = "";
my $dest_feature_modelfile_ = "";

my $compensate_sim_real_bias_ = 0;
my $sim_real_bias_compensation_ = 0.5;
my $use_min_pnl_cap_ = 1;
my $fill_ratio_ = 0.01;
#my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my %skip_dates_map_ = ( );
my $last_trading_date_ = "";

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
`mkdir -p $PICKSTRAT_TEMP_DIR`;
my $skip_dates_file_ = $PICKSTRAT_TEMP_DIR."/skip_dates_file_".$shortcode_."_".$timeperiod_."_".$unique_gsm_id_ ;
my $strats_list_file_ = $PICKSTRAT_TEMP_DIR."/strat_list_".$shortcode_."_".$timeperiod_."_".$unique_gsm_id_ ;
my $passed_strats_list_file_ = $PICKSTRAT_TEMP_DIR."/passed_strat_list_".$shortcode_."_".$timeperiod_."_".$unique_gsm_id_ ;

my $remote_lockfile_ = "/home/dvctrader/pick_strat_lock"; 
my $is_remote_lock_created_ = 0;
my @lock_keys_ = ( );

my $should_get_combined_flat_ = 0;
my $combined_get_flat_model_ = "";

my %strat_to_exec_logic_ = ( );
my %strat_to_param_file_ = ( );
my %strat_to_start_date_ = ( );
my %strat_to_end_date_ = ( );

my $install_from_config_pool_ = 0;

my $set_diff_end_times_ = 0;

my $normalize_interval_scores_ = "NA";

LoadConfigFile ( $config_file_ );

SanityCheckConfigParams ( );

#create running locks (to avoid multiple simultaneous runs across servers)
CreateRunningLocks ( );

my $automated_picking_ = 0;
if ( (1+$#strats_to_keep_) < $sum_num_strats_to_install_) {
  $automated_picking_ = 1;
}
else {
  print "Strats to Keep equals Total no. of Strats to install.. So No Automated Picking..\n";
  if ($risk_tags_ ne "EBT"){
    my $error_string_ = "Automated picking not used.Please use automated picking and reinstall.";
    SendErrorMail ( $error_string_ );
    $mail_body_ = $mail_body_."\n$error_string_\n\n";
  }
}

my %strat_name_to_considered_ = ( );
GetStratsToConsider ( );

my @picked_strats_ = ( );
my %passed_strats_ = ( );
my %interval_to_global_results_ = ( );
my %strat_name_to_global_result_score_ = ( );
my @algo_sorted_strat_names_ = ( );
my @algo_sorted_strat_results_ = ( );
my %strat_name_to_optimal_max_loss_ = ( );

for ( my $t_subset_index_ = 0; $t_subset_index_ <= $#num_strats_to_install_; $t_subset_index_++ )
{
  if ( $automated_picking_ ) {
    %passed_strats_ = ( );
    %interval_to_global_results_ = ( );
    GetGlobalResults ( $t_subset_index_ );
    %strat_name_to_global_result_score_ = ( );
    @algo_sorted_strat_names_ = ( );
    @algo_sorted_strat_results_ = ( );
    ScoreStrats ( $t_subset_index_ );
  }
  PickStrats ( $t_subset_index_ );
}

# if $use_total_size_from_max_loss_ is TRUE, then assign Total_size_to_run and UTSs to the strats 
# in case, 0 UTS is assigned to any strat, remove the strat from the @picked_strats
AssignTotalSizes ( );

SetStratUTS ( );

CheckResetMaxLosses ( );

if ( $use_computed_global_max_loss_ ) {
  $global_max_loss_ = ComputeGlobalMaxloss ( );
  $computed_global_maxloss_ = $global_max_loss_;
  print "GlobalComputedLoss: ".$global_max_loss_."\n";
}
else {
# Check if the global_max_loss is in sane range (for non-NSE products)
  if ( !($disable_sanity_check_global_maxloss_) && !( $shortcode_ =~ /^(NSE_|BSE_)/ ) ) {
    SanityCheckGlobalMaxloss ( );
  }
}

# Re-order strats so that only models with same base-px are grouped together ( if at all )
SortPickedStratsByCombingKey ( );

# For EUREX
DisplayMsgCountInfo ( );

#FillRatioCheck ( );


if ( $curr_date_ eq $pickstrats_date_ ) {
  InstallStrats ( );
}


SendReportMail ( );

#Installed successfully => Mark this query_start_id as done for today (to avoid multiple installs for same id)
CreateDoneLocks ( );

exit ( 0 );

sub LoadConfigFile
{
  print "Loading Config File ...\n";

  my ( $t_config_file_ ) = @_;

  open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." > CONFIG_FILE=".$t_config_file_."\n";

  my $current_param_ = "";
  foreach my $config_file_lines_ ( @config_file_lines_ )
  {
    if ( index ( $config_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
    {
      next;
    }

    my @t_words_ = split ( ' ' , $config_file_lines_ );

    if ( $#t_words_ < 0 )
    {
      $current_param_ = "";
      next;
    }

    if ( ! $current_param_ )
    {
      $current_param_ = $t_words_ [ 0 ];
      next;
    }
    else
    {
      given ( $current_param_ )
      {
        when ( "SHORTCODE" )
        {
          my $t_shortcode_ = $t_words_ [ 0 ];
          if ( $t_shortcode_ ne $shortcode_ )
          {
            PrintErrorAndDie ( "$t_shortcode_ in config file != $shortcode_" );
          }
          $mail_body_ = $mail_body_." \t > SHORTCODE=".$t_shortcode_."\n";
          if ($t_shortcode_ eq "6J_0" || $t_shortcode_ eq "6E_0" || $t_shortcode_ eq "FDAX_0") {
            $ignore_global_max_loss_ = 1;
          }            
        }

        when ( "EXCHANGE" )
        {
          $exchange_ = $t_words_ [ 0 ];
	  my $t_exchange_ = `$get_contract_specs_exec $shortcode_ $curr_date_ EXCHANGE | awk '{ print \$2}' `;
	  chomp($t_exchange_);
	  if(length $t_exchange_){
	      $exchange_=$t_exchange_;
	  }
          $mail_body_ = $mail_body_." \t > EXCHANGE=".$exchange_."\n";
        }

        when ( "NUM_STRATS_TO_INSTALL" )
        {
          my $t_num_strats_to_install_ = $t_words_ [ 0 ];
          push ( @num_strats_to_install_ , $t_num_strats_to_install_ );
          $mail_body_ = $mail_body_." \t > NUM_STRATS_TO_INSTALL=".$t_num_strats_to_install_."\n";
        }

        when ( "NUM_STRAT_FILES_TO_INSTALL" )
        {
# limiting the max strats in a file to 4 as the same is limited in tradeinit
          $num_strat_files_to_install_ = min ( 4, $t_words_ [ 0 ] );
          $mail_body_ = $mail_body_." \t > NUM_STRAT_FILES_TO_INSTALL=".$num_strat_files_to_install_."\n";
        }

        when ( "TOTAL_SIZE_TO_RUN" )
        {
          my $t_total_size_to_run_ = $t_words_ [ 0 ];
          push ( @total_size_to_run_ , $t_total_size_to_run_ );
          $mail_body_ = $mail_body_." \t > TOTAL_SIZE_TO_RUN=".$t_total_size_to_run_."\n";
        }

        when ( "TOTAL_NOTIONAL_SIZE_TO_RUN" )
        {
          my $t_total_size_to_run_notional_ = $t_words_ [ 0 ];
          push ( @total_size_to_run_notional_ , $t_total_size_to_run_notional_ );
          $mail_body_ = $mail_body_." \t > TOTAL_NOTIONAL_SIZE_TO_RUN=".$t_total_size_to_run_notional_."\n";
        }

        when ( "USE_TOTAL_SIZE_FROM_MAX_LOSS" )
        {
          $use_total_size_from_max_loss_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > USE_TOTAL_SIZE_FROM_MAX_LOSS=".$use_total_size_from_max_loss_."\n";
        }

        when ( "DATE" )
        {
          my $t_pickstrats_date_ = $t_words_ [ 0 ];
          if ( $t_pickstrats_date_ ne "TODAY" ) {
            $pickstrats_date_ = GetIsoDateFromStrMin1 ( $t_pickstrats_date_ );
          }
        }

        when ( "EXCHANGES_TO_REMOVE" )
        {
          my $t_exchange_to_remove_ = $t_words_ [ 0 ];
          push ( @exchanges_to_remove_ , $t_exchange_to_remove_ );
          $mail_body_ = $mail_body_." \t > EXCHANGES_TO_REMOVE=".$t_exchange_to_remove_."\n";
        }

        when ( "SHORTCODES_TO_REMOVE" )
        {
          my $t_shortcode_to_remove_ = $t_words_ [ 0 ];
          push ( @shortcodes_to_remove_ , $t_shortcode_to_remove_ );
          $mail_body_ = $mail_body_." \t > EXCHANGES_TO_REMOVE=".$t_shortcode_to_remove_."\n";
        }

        when ( "USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE" )
        {
          my $t_use_optimal_max_loss_per_unit_size_ = $t_words_ [ 0 ];
          push ( @use_optimal_max_loss_per_unit_size_ , $t_use_optimal_max_loss_per_unit_size_ );
          $mail_body_ = $mail_body_." \t > USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE=".$t_use_optimal_max_loss_per_unit_size_."\n";
        }

        when ( "MIN_MAX_LOSS_PER_UNIT_SIZE" )
        {
          $min_max_loss_per_unit_size_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MIN_MAX_LOSS_PER_UNIT_SIZE=".$min_max_loss_per_unit_size_."\n";
        }

        when ( "MAX_MAX_LOSS_PER_UNIT_SIZE" )
        {
          $max_max_loss_per_unit_size_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_MAX_LOSS_PER_UNIT_SIZE=".$max_max_loss_per_unit_size_."\n";
        }

## capping the total max_loss for the strat (currently enabled only for NSE/BSE)  
        when ( "MIN_MAX_LOSS_PER_STRAT" )
        {
          $min_max_loss_per_strat_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MIN_MAX_LOSS_PER_STRAT=".$min_max_loss_per_strat_."\n";
        }

        when ( "MAX_MAX_LOSS_PER_STRAT" )
        {
          $max_max_loss_per_strat_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_MAX_LOSS_PER_STRAT=".$max_max_loss_per_strat_."\n";
        }

        when ( "MAX_LOSS_PER_UNIT_SIZE" )
        { 	
# This overrides the two above.
          my $t_max_loss_per_unit_size_ = $t_words_ [ 0 ];
          push ( @max_loss_per_unit_size_, $t_max_loss_per_unit_size_ );
          $mail_body_ = $mail_body_." \t > MAX_LOSS_PER_UNIT_SIZE=".$t_max_loss_per_unit_size_."\n";
        }

        when ( "MAX_LOSS_SIM_REAL_BIAS_PER_UNIT_SIZE" )
        {
          $max_loss_sim_real_bias_per_unit_size_ = $t_words_[0];
          $mail_body_ = $mail_body_." \t > MAX_LOSS_SIM_REAL_BIAS_PER_UNIT_SIZE=".$max_loss_sim_real_bias_per_unit_size_."\n";
        }

        when ( "OPTIMAL_MAX_LOSS_SETTINGS" )
        { 
# This if present would override the default settings for optimal max_loss calculations.
          $read_OML_settings_ = 1 ;
          $OML_lookback_days_ = max ( 10, $t_words_ [ 0 ] ) ;
          $OML_hit_ratio_ = min ( 0.2, $t_words_ [ 1 ] ) ; #greater than 20% is not sane
          $OML_number_top_loss_ = 10; #Since it odes not matter here
        }

        when ( "OPTIMAL_STDEV_MAX_LOSS_SETTINGS" )
        { 
# This if present would override the default settings for optimal max_loss calculations.
          $read_OSML_settings_ = 1 ;
          $OSML_lookback_days_ = max ( 50, $t_words_ [ 0 ] ) ; #lookback days should atleat be 50 for stdev OML
          $OSML_hit_ratio_ = min ( 0.1, $t_words_ [ 1 ] ) ; #greater than 10% is not sane
          $OSML_high_volatile_percentile_ = 0.2;
          if ( $#t_words_ > 1 ) {
            $OSML_high_volatile_percentile_ = max ( 0.1, min ( 0.4, $t_words_ [ 2 ] ) );; # Volatile days to range from 0.1 to 0.4
          }
        }

        when ( "OPTIMAL_STDEV_MAX_LOSS_VOLATILE_FLAG" )
        {
          $OSML_is_volatile_ = $t_words_ [ 0 ];
        }

        when ( "USE_MIN_PNL_CAP" )
        {
          $use_min_pnl_cap_ = $t_words_ [ 0 ] ;
        }

        when ( "OPENTRADE_LOSS_PER_UNIT_SIZE" )
        {
          $opentrade_loss_per_unit_size_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > OPENTRADE_LOSS_PER_UNIT_SIZE=".$opentrade_loss_per_unit_size_."\n";
        }

        when ( "USE_PARAMFILE_OPENTRADE_LOSS_PER_UNIT_SIZE" )
        {
          $use_paramfile_opentrade_loss_per_unit_size_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > USE_PARAMFILE_OPENTRADE_LOSS_PER_UNIT_SIZE=".$use_paramfile_opentrade_loss_per_unit_size_."\n";
        }

        when ( "SHORT_TERM_LOSS_PER_UNIT_SIZE" )
        {
          $short_term_loss_per_unit_size_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > SHORT_TERM_LOSS_PER_UNIT_SIZE=".$short_term_loss_per_unit_size_."\n";
        }

        when ( "MAX_GLOBAL_POSITION" )
        {
          $max_global_position_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_GLOBAL_POSITION=".$max_global_position_."\n";
        }

        when ( "GLOBAL_MAX_LOSS" )
        {
          $global_max_loss_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > GLOBAL_MAX_LOSS=".$global_max_loss_."\n";
        }

        when ( "USE_COMPUTED_GLOBAL_MAX_LOSS" )
        {
          $use_computed_global_max_loss_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > USE_COMPUTED_GLOBAL_MAX_LOSS=".$use_computed_global_max_loss_."\n";
        }

        when ( "DISABLE_SANITY_GLOBAL_MAXLOSS" )
        {
          $disable_sanity_check_global_maxloss_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > DISABLE_SANITY_GLOBAL_MAXLOSS=".$disable_sanity_check_global_maxloss_."\n";
        }

        when ( "MAX_MUR" )
        {
          my $t_max_mur_ = $t_words_ [ 0 ];
          push ( @max_mur_ , $t_max_mur_ );
          $mail_body_ = $mail_body_." \t > MAX_MUR=".$t_max_mur_."\n";
        }

        when ( "MIN_VOLUME_PER_STRAT" )
        {
          my $t_min_volume_per_strat_ = $t_words_ [ 0 ];
          push ( @min_volume_per_strat_ , $t_min_volume_per_strat_ );
          $mail_body_ = $mail_body_." \t > MIN_VOLUME_PER_STRAT=".$t_min_volume_per_strat_."\n";
        }

        when ( "MAX_TTC_PER_STRAT" )
        {
          my $t_max_ttc_per_strat_ = $t_words_[ 0 ];
          push ( @max_ttc_per_strat_ , $t_max_ttc_per_strat_ );
          $mail_body_ = $mail_body_." \t > MAX_TTC_PER_STRAT=".$t_max_ttc_per_strat_."\n";
        }

        when ( "STRATS_TO_KEEP" )
        {
          if ( ! FindItemFromVec ( $t_words_ [ 0 ] , @strats_to_keep_ ) )
          {
          	my $sname = $t_words_ [ 0 ];
            push ( @strats_to_keep_ , $t_words_ [ 0 ] );
            
            my $basename = `basename $sname`; chomp($basename);
            if ( !IsValidConfig($basename)){
            	SendErrorMailAndDie("Not a valid config $basename\n");
            }
            $mail_body_ = $mail_body_." \t > STRATS_TO_KEEP=".$t_words_ [ 0 ]."\n";
          }
          else 
          {
            print "aborting...duplicate strats in STRATS_TO_KEEP\n";
            exit ( 0 );
          }
        }

        when ( "STRATS_TO_EXCLUDE" )
        {
          if ( ! FindItemFromVec ( $t_words_ [ 0 ] , @strats_to_exclude_ ) )
          {
            push ( @strats_to_exclude_ , $t_words_ [ 0 ] );
            $mail_body_ = $mail_body_." \t > STRATS_TO_EXCLUDE=".$t_words_ [ 0 ]."\n";
          }
        }

        when ( "DAYFEATURES_CONFIG_FILE" )
        {
          $dayfeatures_config_file_ = $t_words_ [ 0 ];
        }

        when ( "DAYFEATURES_SPECIAL_EVENTS_FILE" )
        {
          $dayfeatures_special_events_file_ = $t_words_[ 0 ];
        }

        when ( "INTERVALS_TO_PICK_FROM" )
        {
          if ( ! exists ( $intervals_to_pick_from_ { $t_words_ [ 0 ] } ) )
          {
            my @intervals_args_ = splice @t_words_, 0, -1 ;
            my $interval_ = join (' ', @intervals_args_) ;
            $intervals_to_pick_from_ { $interval_ } = $t_words_ [ $#t_words_ ];
            $mail_body_ = $mail_body_." \t > INTERVALS_TO_PICK_FROM=[".$interval_."]=".$t_words_ [ $#t_words_ ]."\n";
#print "INTERVALS_TO_PICK_FROM=[".$interval_."]=".$t_words_ [ $#t_words_ ]."\n";
          }
        }

        when ( "INTERVALS_TO_PICK_FROM_WEIGHTED" )
        {
          if ( ! exists ( $intervals_to_pick_from_weighted_ { $t_words_ [ 0 ] } ) )
          {
            my @intervals_args_ = splice @t_words_, 0, -1 ;
            my $interval_ = join (' ', @intervals_args_) ;
            $intervals_to_pick_from_weighted_ { $interval_ } = $t_words_ [ $#t_words_ ];
            $mail_body_ = $mail_body_." \t > INTERVALS_TO_PICK_FROM_NEW=[".$interval_."]=".$t_words_ [ $#t_words_ ]."\n";
#print "INTERVALS_TO_PICK_FROM=[".$interval_."]=".$t_words_ [ $#t_words_ ]."\n";
          }
        }
        
        when ( "SKIP_RESULTS_FOR_DAYS" )
        {
          $skip_dates_map_ { $t_words_[0] } = 1;
          $mail_body_ = $mail_body_." \t > SKIP_RESULTS_FOR_DAYS=".$t_words_ [ 0 ]."\n";
        }

        when ( "SKIP_RESULTS_FOR_DAYS_FILE" )
        {
          if ( ExistsWithSize ( $t_words_[0] ) )
          {
            my $skip_days_file_ = $t_words_[0];
            open DFHANDLE, "< $skip_days_file_" or PrintStacktraceAndDie ( "$0 Could not open $skip_days_file_\n" );
            my @skip_days_ = <DFHANDLE>; chomp ( @skip_days_ );
            $skip_dates_map_ { $_ } = 1 foreach @skip_days_;
            $mail_body_ = $mail_body_." \t > SKIP_RESULTS_FOR_DAYS=".join(',', @skip_days_)."\n";
            close DFHANDLE;
          }
        }
        
        when ( "DIVERSITY_SCORES" )
        {
          if ( ! exists ( $diversity_type_to_weight_ { $t_words_ [ 0 ] } ) )
          {
            $diversity_type_to_weight_ { $t_words_ [ 0 ] } = $t_words_ [ 1 ];
            $mail_body_ = $mail_body_." \t > DIVERSITY_SCORES=[".$t_words_ [ 0 ]."]=".$t_words_ [ 1 ]."\n";
          }
        }

        when ( "DIVERSITY_THRESHOLD" )
        {
          $diversity_threshold_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > DIVERSITY_THRESHOLD=".$t_words_ [ 0 ]."\n";
        }

        when ( "OPTIMAL_MAX_LOSS_CHECK" )
        {
          $optimal_max_loss_check_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > OPTIMAL_MAX_LOSS_CHECK=".$t_words_ [ 0 ]."\n";
        }

        when ( "MAX_LOSS_PNL_CHECK" )
        {
          $max_loss_pnl_cutoff_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_LOSS_PNL_CHECK=".$t_words_ [ 0 ]."\n";
        }

        when ( "USE_SHARPE_PNL_CHECK" )
        {
          $use_sharpe_pnl_check_ = $t_words_ [0];
          print "Using USE_SHARPE_PNL_CHECK_: $use_sharpe_pnl_check_\n";
        }

        when ( "SORT_ALGO" )
        {
          my $t_sort_algo_ = $t_words_ [ 0 ];
          push ( @sort_algo_ , $t_sort_algo_ );
          $mail_body_ = $mail_body_." \t > SORT_ALGO=".$t_sort_algo_."\n";
        }

        when ( "PNL_PREDICTION_ALGO" )
        {
          $pnl_prediction_algo_ = $t_words_[0];
          if ( scalar(@t_words_) > 1 ) {
            $pnl_prediction_num_days_ = int($t_words_[1]);
          }
          else{
            $pnl_prediction_num_days_ = 300;
          }
          if ( $pnl_prediction_num_days_ < 300 )
          {
            print "Given num_days for pnl_prediction less than 300. Taking 300 as default.\n";
            $pnl_prediction_num_days_ = 300;
          }
          $mail_body_ = $mail_body_." \t > PNL_PREDICITION_ALGO=".$pnl_prediction_algo_." ".$pnl_prediction_num_days_."\n";
        }

        when ( "NORMALIZE_INTERVAL_SCORES" )
        {
          $normalize_interval_scores_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > NORMALIZE_INTERVAL_SCORES=".$normalize_interval_scores_."\n";
        }
  

        when ( "COMBINED_SORT_ALGO" )
        {
          $use_combined_metric_ = 1;
          $combined_sort_algo_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > COMBINED_SORT_ALGO=".$combined_sort_algo_."\n";
        }

        when ( "EMAIL_ADDRESS" )
        {
          $email_address_ = $t_words_ [ 0 ];
        }

        when ( "INSTALL_PICKED_STRATS" )
        {
          $install_picked_strats_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_picked_strats_."\n";
        }

        when ( "INSTALL_LOCATION" )
        {
          $install_location_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_location_."\n";			
        }

        when ( "PROD_QUERY_START_ID" )
        {
          $prod_query_start_id_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > PROD_QUERY_START_ID=".$prod_query_start_id_."\n";
        }

        when ( "PROD_QUERY_STOP_ID" )
        {
          $prod_query_stop_id_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > PROD_QUERY_STOP_ID=".$prod_query_stop_id_."\n";
        }

        when ( "FPGA" )
        {
          $fpga_flag_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > FPGA_FLAG=".$fpga_flag_."\n";
        }

        when ( "RISK_TAGS" )
        {
          $risk_tags_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > RISK_TAGS=".$risk_tags_."\n";
        }

        when ( "FOLDER_TIME_PERIODS" )
        {
          my $tp_ = $t_words_[0];
          push ( @fld_time_periods_, $tp_ );
          $mail_body_ = $mail_body_." \t > FOLDER_TIME_PERIOD=".$tp_."\n";
        }

        when ( "EXEC_START_HHMM" )
        {
          $exec_start_hhmm_ = GetUTCHHMMStr ( $t_words_ [ 0 ], $pickstrats_date_ ); 
          if ( $exec_start_hhmm_ < 10 )
          { 
# 0007 -> 7 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "000".$exec_start_hhmm_;
          }
          elsif ( $exec_start_hhmm_ < 100 )
          { 
# 0010 -> 10 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "00".$exec_start_hhmm_;
          }
          elsif ( $exec_start_hhmm_ < 1000 )
          { 
# 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_start_hhmm_ = "0".$exec_start_hhmm_;
          }
          $mail_body_ = $mail_body_." \t > EXEC_START_HHMM=".$exec_start_hhmm_."\n";	
        }

        when ( "EXEC_END_HHMM" )
        {
          $exec_end_hhmm_ = GetUTCHHMMStr ( $t_words_ [ 0 ], $pickstrats_date_ );
          if ( $exec_end_hhmm_ < 10 )
          { 
# 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "000".$exec_end_hhmm_;
          }
          elsif ( $exec_end_hhmm_ < 100 )
          { 
# 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "00".$exec_end_hhmm_;
          }
          elsif ( $exec_end_hhmm_ < 1000 )
          { 
# 0710 -> 710 , causing substrs to fail when installing cron.
            $exec_end_hhmm_ = "0".$exec_end_hhmm_;
          }
          $mail_body_ = $mail_body_." \t > EXEC_END_HHMM=".$exec_end_hhmm_."\n";	
        }

        when ( "ONLOAD_TRADE_EXEC" )
        {
          $onload_trade_exec_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > ONLOAD_TRADE_EXEC=".$onload_trade_exec_."\n";
        }

        when ( "VMA_TRADE_EXEC" )
        {
          $vma_trade_exec_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > VMA_TRADE_EXEC=".$vma_trade_exec_."\n";
        }

        when ( "AFFINITY_TRADE_EXEC" )
        {
          $affinity_trade_exec_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > AFFINITY_TRADE_EXEC=".$affinity_trade_exec_."\n";
        }

        when ( "COMPENSATE_SIM_REAL_BIAS" )
        {
          $compensate_sim_real_bias_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > COMPENSATE_SIM_REAL_BIAS=".$compensate_sim_real_bias_."\n";
        }

        when ( "SIM_REAL_BIAS_COMPENSATION" )
        {
          $sim_real_bias_compensation_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > SIM_REAL_BIAS_COMPENSATION=".$sim_real_bias_compensation_."\n";
        }

        when ( "IGNORE_NEW_STRATS" )
        {
          $ignore_new_strats_ = ( $t_words_ [ 0 ] > 0 ) + 0;
          $mail_body_ = $mail_body_." \t > IGNORE_NEW_STRATS=".$ignore_new_strats_."\n";
        }


        when ( "MAX_MSG_COUNT_CUTOFF" )
        {
          $max_message_count_cutoff_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_MSG_COUNT_CUTOFF=".$max_message_count_cutoff_."\n";
        }

        when ( "NORMALIZE_SCORE_BY_MSG_COUNT" )
        {
          $normalize_score_by_message_count_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > NORMALIZE_SCORE_BY_MSG_COUNT=".$normalize_score_by_message_count_."\n";
        }

        when ( "NORMALIZE_SCORE_BY_SQRT_MSG_COUNT" )
        {
          $normalize_score_by_sqrt_message_count_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > NORMALIZE_SCORE_BY_SQRT_MSG_COUNT=".$normalize_score_by_sqrt_message_count_."\n";
        }

        when ( "RELEASE_CORE_PREMATURE" )
        {
          $release_core_premature_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > RELEASE_CORE_PREMATURE=".$release_core_premature_."\n";
        }

        when ( "SHOULD_GET_COMBINE_GET_FLAT" )
        {
          $should_get_combined_flat_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > SHOULD_GET_COMBINE_GET_FLAT=".$should_get_combined_flat_."\n";
        }

        when ( "COMBINED_GET_FLAT_MODEL" )
        {
          $combined_get_flat_model_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > COMBINED_GET_FLAT_MODEL=".$combined_get_flat_model_."\n";
        }

        when ( "MIN_DAYS_FRAC" )
        {
          $min_days_frac_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > MIN_DAYS_FRAC=".$min_days_frac_."\n";
        }
        
        when ( "COMMON_DAYS_FRAC" )
        {
          $common_days_frac_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > COMMON_DAYS_FRAC=".$common_days_frac_."\n";
        }

        when ( "SET_BUFFERED_END_TIMES" )
        {
          $set_diff_end_times_ = $t_words_[ 0 ];
          $mail_body_ = $mail_body_." \t > SET_BUFFERED_END_TIMES=".$set_diff_end_times_."\n";
        }
        when ( "USE_WF_CONFIGS")
        {
        	$install_from_config_pool_ = $t_words_[0];
        	$mail_body_ = $mail_body_." \t > USE_WF_CONFIGS=".$install_from_config_pool_."\n";
        }
        when("FILL_RATIO")
        {
		$fill_ratio_ = $t_words_[0];
		$mail_body_ = $mail_body_."\t > FILL_RATIO=".$fill_ratio_."\n";
	}
      }
    }
  }
  if ( $install_location_ eq "" ) {
    $install_location_ = GetMachineForProduct ( $shortcode_ );
    if ( ! defined $install_location_ ) {
      PrintErrorAndDie ( "INSTALL_LOCATION could NOT be fetched for shortcode $shortcode_" );
    }
    $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_location_."\n";
  }

  $mail_body_ = $mail_body_." \t > DATE=".$pickstrats_date_."\n";

  # FOLDER_TIME_PERIODS is made mandatory and therefore commented it.
  #if ( $#fld_time_periods_ < 0 ) {
  #  my $fetch_pool_cmd_ = "$WF_SCRIPTS_DIR/get_pools_for_shortcode.py -shc $shortcode_ -tp $timeperiod_";
  #  @fld_time_periods_ = `$fetch_pool_cmd_ 2>/dev/null`; chomp ( @fld_time_periods_ );
  #}

  @strats_in_pool_ = ();
  foreach my $fld_tp_ ( @fld_time_periods_ ) {
    my $fetch_configs_cmd_ = "$WF_SCRIPTS_DIR/get_pool_configs.py -m POOL -shc $shortcode_ -tp $fld_tp_ -type N";
    my @strats_in_dir_ = `$fetch_configs_cmd_ 2>/dev/null`; chomp ( @strats_in_dir_ );
    push ( @strats_in_pool_, @strats_in_dir_ );
  }
  @strats_in_pool_ = GetUniqueList ( @strats_in_pool_ ); 

  open STRAT_LIST_HANDLE, "> $strats_list_file_" or SendErrorMailAndDie ( "cannot open $strats_list_file_ for writing" );
  push ( @intermediate_files_, $strats_list_file_ );
  print STRAT_LIST_HANDLE join("\n", @strats_in_pool_)."\n";
  close (STRAT_LIST_HANDLE);

# if %intervals_to_pick_from_ is empty, then use the default interval and weights
  if ( ! %intervals_to_pick_from_ ) {
#    print "No interval is mentioned. Using the DayFeatures Clustering Method\n";
#    $intervals_to_pick_from_ { "30 DAYFEATURES_CLUSTER 0.3" } = 1.0;
#    print "Warning: No intervals are mentioned. Using DAYFEATURES_CLUSTER Method.\n";
    print "Warning: No intervals are mentioned. Using the default values ( 3: 0.2, 10: 0.5, 30: 0.2 ) ...\n";
    $intervals_to_pick_from_ { 3 } = 0.2;
    $intervals_to_pick_from_ { 10 } = 0.5;
    $intervals_to_pick_from_ { 30 } = 0.3;
  }

# if INTERVALS_TO_PICK_FROM_WEIGHTED is present in the config, then ignore INTERVALS_TO_PICK_FROM
  if ( %intervals_to_pick_from_weighted_ ) {
    print "Note: INTERVALS_TO_PICK_FROM_WEIGHTED is provided, so ignoring the INTERVALS_TO_PICK_FROM..\n";
    %intervals_to_pick_from_ = ( );
    $intervals_to_pick_from_{ "DAYS_WEIGHTS_VECTOR" } = 1.0;
    $use_intervals_days_weight_vec_ = 1;
  } 

  if ( $shortcode_ =~ /^(NSE_|BSE_)/ ) {
    print "Using TOTAL_NOTIONAL_SIZE_TO_RUN for NSE/BSE products..\n";
    $uts_string_ = "NOTIONAL_UTS";
    @total_size_to_run_ = @total_size_to_run_notional_;
  }

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}

sub CheckPickStratsDateisEvent
{
  my $interval_ = shift;
  my @interval_args_ = split(' ', $interval_);
  if ($#interval_args_ < 4) {
    PrintErrorAndDie ( "USAGE: <LOOKBACK_DAYS ECO_EVENT_CONDITIONAL CURRENCY [EVENT_NAME/ALL] [EVENT_DEGREE/ALL]" );
  }
  my $eco_event_ = $interval_args_[2];

  ### In case event name is given as ALL don't filter based on event type, similarly for event_degree ###

  if ($interval_args_[3] ne "ALL")
  {
    $eco_event_ = $eco_event_." ".$interval_args_[3];
  }
  else
  {
    $eco_event_ = $eco_event_." .*";
  }
  if ($interval_args_[4] ne "ALL")
  {
    $eco_event_ = $eco_event_." ".$interval_args_[4]." ";
  }
  else
  {
    $eco_event_ = $eco_event_." .* ";
  }

  my $exec_cmd_ = "grep -h \"$eco_event_\" /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_*_processed.txt";

  if ($verbose_) { print $exec_cmd_."\n"; }
  ### this is the event dates in string form seperated by \n
  my @exec_cmd_output_ = `$exec_cmd_`;
  chomp ( @exec_cmd_output_ );
  my $is_pickstrats_date_event_date_ = 0;
  foreach my $eco_line_ (@exec_cmd_output_)
  {
    my @eco_line_words_ = split ( ' ', $eco_line_ );
    chomp ( @eco_line_words_ );

    if ($#eco_line_words_ >= 4)
    {
      my $date_time_word_ = $eco_line_words_[ $#eco_line_words_ ];

      my @date_time_words_ = split ( '_', $date_time_word_ );
      chomp ( @date_time_words_ );
      if ($#date_time_words_ >= 0)
      {
        my $tradingdate_ = $date_time_words_[ 0 ];

        if ($tradingdate_ eq $pickstrats_date_)
        {
          $is_pickstrats_date_event_date_ = 1;
          last;
        }
      }
    }
  }
  return $is_pickstrats_date_event_date_;
}

sub SanityCheckConfigParams
{
  print "Sanity Checking ...\n";

  if ( $#fld_time_periods_ < 0 )
  {
    SendErrorMailAndDie ( "FOLDER_TIME_PERIODS is not set." );
  }

  if ( $#num_strats_to_install_ != $#use_optimal_max_loss_per_unit_size_ )
  {
    SendErrorMailAndDie ( "#NUM_STRATS_TO_INSTALL != #USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE" );
  }

  if ( $#num_strats_to_install_ != $#max_loss_per_unit_size_ )
  {
    push ( @max_loss_per_unit_size_, 0 ) if $#max_loss_per_unit_size_ < 0;
    @max_loss_per_unit_size_ = map { $max_loss_per_unit_size_[0] } 0 .. $#num_strats_to_install_;
  }

  if ( $#num_strats_to_install_ != $#min_volume_per_strat_ )
  {
    print "#NUM_STRATS_TO_INSTALL != #MIN_VOLUME_PER_STRAT.. MIN_VOLUME Cutoff would not be used on the absent sets\n";
  }

  if ( $#num_strats_to_install_ != $#max_ttc_per_strat_ )
  {
    print "#NUM_STRATS_TO_INSTALL != #MAX_TTC_PER_STRAT.. MAX_TTC Cutoff would not be used on the absent sets\n";
  }

  if ( $#num_strats_to_install_ != $#sort_algo_ )
  {
    SendErrorMailAndDie ( "#NUM_STRATS_TO_INSTALL != #SORT_ALGO" );
  }

# TODO: Check why this is done is such a peculiar way!
  $sum_num_strats_to_install_ = GetSum ( \@num_strats_to_install_ );

  if ( $sum_num_strats_to_install_ <= 0 )
  {
    SendErrorMailAndDie ( "SUM_NUM_STRATS_TO_INSTALL=".$sum_num_strats_to_install_ );
  }

  if ( ! $use_total_size_from_max_loss_ ) {
    if ( $#num_strats_to_install_ != $#total_size_to_run_ )
    {
      SendErrorMailAndDie ( "#NUM_STRATS_TO_INSTALL != #TOTAL_SIZE_TO_RUN" );
    }
    for ( my $i = 0 ; $i <= $#num_strats_to_install_ ; $i ++ )
    {
      if ( $total_size_to_run_ [ $i ] < $num_strats_to_install_ [ $i ] )
      {
        SendErrorMailAndDie ( "TOTAL_SIZE_TO_RUN=".$total_size_to_run_ [ $i ]." < NUM_STRATS_TO_INSTALL=".$num_strats_to_install_ [ $i ] );
      }
    }
  }

  if ( $global_max_loss_ <= 0 )
  {
    SendErrorMailAndDie ( "GLOBAL_MAX_LOSS not set ".$global_max_loss_."\n" );
  }

  if ( defined $release_core_premature_ && ( ! looks_like_number($release_core_premature_) || 
                                            ($release_core_premature_ != 0 && $release_core_premature_ != 1) ) ) {
    SendErrorMailAndDie ( "RELEASE_CORE_PREMATURE has incorrect value ".$release_core_premature_."\n" );
  }

  $local_production_strat_list_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ );

  push ( @intermediate_files_ , $local_production_strat_list_file_ );
 
  $local_single_strat_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ ).".single_strat_file";

  $remote_production_strat_list_file_ = $REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ );

  # Check if ECO_EVENT_CONDITIONAL is present in Intervals. If it is and if today is an
  # event then give it weight, else change its weight to zero

  foreach my $interval_ ( keys %intervals_to_pick_from_weighted_ )
  {
    if ( index($interval_, "ECO_EVENT_CONDITIONAL") != -1 && !CheckPickStratsDateisEvent($interval_))
    {
      $intervals_to_pick_from_weighted_{$interval_} = 0;
    }
  }

  # Normalize interval weights.
  my $sum_interval_weights_ = 0;
  foreach my $interval_ ( keys %intervals_to_pick_from_ )
  {
    $sum_interval_weights_ += $intervals_to_pick_from_ { $interval_ };
  }
  foreach my $interval_ ( keys %intervals_to_pick_from_ )
  {
    $intervals_to_pick_from_ { $interval_ } /= $sum_interval_weights_;
  }

  $sum_interval_weights_ = 0;
  foreach my $interval_ ( keys %intervals_to_pick_from_weighted_ )
  {
    $sum_interval_weights_ += $intervals_to_pick_from_weighted_ { $interval_ };
  }
  foreach my $interval_ ( keys %intervals_to_pick_from_weighted_ )
  {
    $intervals_to_pick_from_weighted_ { $interval_ } /= $sum_interval_weights_;
  }

# Normalize diversity scores.
  if ( $use_combined_metric_ == 0 ) {
    my @diversity_types_ = qw ( EXEC_LOGIC PARAM_FILE TRAINING_DATES PNL_SERIES );

    foreach my $diversity_type_ ( keys %diversity_type_to_weight_ )
    {
      if ( ! FindItemFromVec ( $diversity_type_, @diversity_types_ ) )
      {
        SendErrorMailAndDie ( "Diversity Type ".$diversity_type_." not supported\n" );
      }
    }

    if ( scalar keys %diversity_type_to_weight_ == 0 ) {
      SendErrorMailAndDie ( "No Diversity Values Provided" );
    }

    foreach my $diversity_type_ ( @diversity_types_ ) {
      if ( ! exists ( $diversity_type_to_weight_ { $diversity_type_ } ) ) {
        print "WARNING: Diversity Weight for ".$diversity_type_." is NOT provided. Assuming it to be 0\n";
        $diversity_type_to_weight_ { $diversity_type_ } = 0;
      }
    }

    my $sum_diversity_weights_ = 0;
    foreach my $diversity_type_ ( keys %diversity_type_to_weight_ )
    {
      $sum_diversity_weights_ += $diversity_type_to_weight_ { $diversity_type_ };
    }

    foreach my $diversity_type_ ( keys %diversity_type_to_weight_ )
    {
      $diversity_type_to_weight_ { $diversity_type_ } /= $sum_diversity_weights_;
    }
  }

# If install_picked_strats_ is set, check that all the required params are defined 
  if ( $install_picked_strats_ ) {
   if ( ! $install_location_ ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but INSTALL_LOCATION is not provided" );
   }
   elsif ( ! $prod_query_start_id_ ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but PROD_QUERY_START_ID is not provided" );
   }
   elsif ( ! $exchange_ ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but EXCHANGE is not provided" );
   }
   elsif ( ! ( $exec_start_hhmm_ && $exec_end_hhmm_ ) ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but EXEC_START_HHMM or EXEC_END_HHMM is not provided" );
   } 
   elsif ( ! $prod_query_stop_id_ ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but PROD_QUERY_STOP_ID is not provided" );
   }
   elsif ( $prod_query_stop_id_ < $prod_query_start_id_ ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but PROD_QUERY_START_ID and PROD_QUERY_STOP_ID are not correctly set" );
   }
   elsif ( $prod_query_stop_id_ > $prod_query_start_id_ + 30 ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but PROD_QUERY_START_ID and PROD_QUERY_STOP_ID are too far apart" );
   }
   elsif ( ! defined $risk_tags_ || $risk_tags_ eq "" ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but RISK_TAGS is not provided" );
   }
  }

  return;
}

=comment
Function: Computes the following properties of a strat.
(1) Exec logic
(2) Pred duration
(3) Pred logic
(4) Start date
(5) End date
(6) Filter
(7) Param
(8) Indicators
=cut
sub GetStratProperties 
{
  my $strat_name_ = shift;
  my @strat_path_list_ = `$SCRIPTS_DIR/print_strat_from_base.sh $strat_name_ 2>/dev/null`;
  if ( $#strat_path_list_ < 0 ) {
    print "Warning! No such strat: ".$strat_name_."\n";
    return 0;
  }
  if ( $#strat_path_list_ > 0 ) {
    print "Warning! Multiple strats with same basename exists: ".$strat_name_."\n";
  }
  my $strat_path_ = $strat_path_list_[ 0 ];
  my $exec_cmd_ = "cat $strat_path_"; 
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
  if ( $#exec_output_ < 0 ) {
# For some reason, this strat is empty
    print "PLEASE LOOK INTO IT: ".$strat_name_."\n";
    return 0;
  }
  my @exec_output_words_ = split ( ' ' , $exec_output_ [ 0 ] );
  my $exec_logic_ = $exec_output_words_ [ 2 ];
  my $param_file_ = $exec_output_words_ [ 4 ];
  my ( $start_date_, $end_date_ ) = ( $strat_name_ =~ m/.*_(20[\d]{6})_(20[\d]{6})_.*/ );
  if ( ! ValidDate ( $start_date_ ) || ! ValidDate ( $end_date_ ) ) {
    $start_date_ = 20090101; 
    $end_date_ = 20090201;
  }
  $strat_to_exec_logic_ { $strat_name_ } = $exec_logic_;
  $strat_to_param_file_ { $strat_name_ } = $param_file_;
  $strat_to_start_date_ { $strat_name_ } = $start_date_;
  $strat_to_end_date_ { $strat_name_ } = $end_date_; 
# Cannot think of a way to extract out (2), (3), (5) as not all strats have that.
  return 1;
}

sub GetDays 
{
  my $date_ = shift;
  ( my $yyyy, my $mm, my $dd ) = BreakDateYYYYMMDD ( $date_ );
# Approximate!
  return $yyyy * 365 + $mm * 30 + $dd;
}

sub GetTrainingScore 
{
  my $s1 = shift;
  my $e1 = shift;
  my $s2 = shift;
  my $e2 = shift;
  if ( $s1 > $s2 ) { my $tmp = $s1; $s1 = $s2; $s2 = $tmp; $tmp = $e1; $e1 = $e2; $e2 = $tmp; }
  if ( $s2 > $e1 ) { return 0; }
  return ( $e1 - $s2 + 1 ) / ( max ( $e1 , $e2 ) - min ( $s1 , $s2 ) + 1 );
}

# Add your similarty logic here.
sub GetSimilarityScore 
{
  my $strat1_ = shift;
  my $strat2_ = shift;
  my $sample_pnls_strats_vec_ref_ = shift;

  if ( ! exists $strat_to_exec_logic_ { $strat1_ } ) {
    print "WARN: Strat Properties for $strat1_ could not be loaded\n";
  }
  if ( ! exists $strat_to_exec_logic_ { $strat2_ } ) {
    print "WARN: Strat Properties for $strat2_ could not be loaded\n";
  }

  my $exec_logic_score_ = 0;
  my $param_file_score_ = 0;
  my $training_dates_score_ = 0;
  
  if ( exists $strat_to_exec_logic_ { $strat1_ } && exists $strat_to_exec_logic_ { $strat2_ } ) {
    my $exec_logic0_ = $strat_to_exec_logic_ { $strat1_ };
    my $exec_logic1_ = $strat_to_exec_logic_ { $strat2_ };
    my $param_file0_ = $strat_to_param_file_ { $strat1_ };
    my $param_file1_ = $strat_to_param_file_ { $strat2_ };

    if ( $exec_logic0_ eq $exec_logic1_ ) {
      $exec_logic_score_ = 1;
    }

    if ( $param_file0_ eq $param_file1_ ) {
      $param_file_score_ = 1;
    }

    my $start_date0_ = GetDays ( $strat_to_start_date_ { $strat1_ } );
    my $start_date1_ = GetDays ( $strat_to_start_date_ { $strat2_ } );
    my $end_date0_ = GetDays ( $strat_to_end_date_ { $strat1_ } );
    my $end_date1_ = GetDays ( $strat_to_end_date_ { $strat2_ } );
    $training_dates_score_ = GetTrainingScore ( $start_date0_ , $end_date0_ , $start_date1_ , $end_date1_ );
  }

  my $pnl_series_score_ = FetchCorrelationForPair ( $strat1_ , $strat2_ );
  if ( ! defined $pnl_series_score_ ) {
    if ( $verbose_ ) { print "Offline similarity Absent for strats: ".$strat1_." ".$strat2_."\n"; }
    $pnl_series_score_ = GetPnlSamplesCorrelation ( $strat1_, $strat2_, $sample_pnls_strats_vec_ref_ );
  }

  my $similarity_ = $diversity_type_to_weight_ { "EXEC_LOGIC" } * $exec_logic_score_ + 
    $diversity_type_to_weight_ { "PARAM_FILE" } * $param_file_score_ + 
    $diversity_type_to_weight_ { "TRAINING_DATES" } * $training_dates_score_ + 
    $diversity_type_to_weight_ { "PNL_SERIES" } * $pnl_series_score_;

  return $similarity_;
}



=comment
Function: Gets all strats from pool to be considered for picking.

(1) Excludes strats for which we don't have results. We need results for [$pickstrats_date_ - $longest_interval_ , $pickstrats_date_ - 1].
(2) Excludes strats whose training end date is on or after $pickstrats_date_.
(3) Pre-computes "properties" and "pnl-series" of each strat.
(4) TODO: Currently, only supports folder name, need to add TAG support.
=cut
sub GetStratsToConsider 
{
  print "\nGetting strats to be considered ...\n";


  BuildListOfDates ( );
  SetOMLSettings ( );

  if ( $verbose_ ) { print "\n"; }

# Read results from ec2_globalresults_
  my %date_strat_to_existance_ = ( );
  my @repeated_combined_days_ = ();
  foreach my $interval_ ( keys %intervals_to_days_ )  
  {
    push ( @repeated_combined_days_, @{ $intervals_to_days_{ $interval_ } } );
  }
  my @combined_list_days_ = GetUniqueList ( @repeated_combined_days_ );

  
  my @strats_skip_dates_ = ();
  my %strats_skip_dates_count_ = ( );
  my @last_tradingdate_strats_vec_ = ( );

  foreach my $this_date_ ( @combined_list_days_ )
  {
    my @t_strats_with_res_ = ();
    GetStratsWithGlobalResultsForShortcodeDate( $shortcode_, $this_date_, \@t_strats_with_res_, $GLOBALRESULTSDBDIR, "A", $install_from_config_pool_);
    $strats_skip_dates_count_{ $this_date_ } = 0;
    if ( $#t_strats_with_res_ >= 0 )
    {
      foreach  my $t_strat_base_name_ ( @t_strats_with_res_ )
      {
        $date_strat_to_existance_ { $this_date_.".".$t_strat_base_name_ } = 1;
      }
    }
    else
    {
      foreach my $interval_ ( keys %intervals_to_days_ ) {
        @{ $intervals_to_days_{ $interval_ } } = grep { $_ != $this_date_ } @{ $intervals_to_days_{ $interval_ } };
      }
      push ( @strats_skip_dates_, $this_date_ );
      if ( $verbose_ ) { print "WARNING: Results file not present for date ".$this_date_."\n"; }
    }
  }

  if ( $verbose_ ) { print "Number of strats in pool: ".($#strats_in_pool_ + 1)."\n\n"; }
  my $considered_count_ = 0;
# Now, we have the results and we have the list of strats.
  foreach my $strat_ ( @strats_in_pool_ ) 
  {
    if ( $strat_ eq "." || $strat_ eq ".." )
    {
      next;
    }

    my ( $st_dt , $end_dt ) = ( $strat_ =~ m/.*_(20[\d]{6})_(20[\d]{6})_.*/ );

    if ( $st_dt and $end_dt and $pickstrats_date_ <= $end_dt and $ignore_new_strats_ == 1 ) 
    {
# This strat was created after pickstrats_date, so ignore it!
      if ( $verbose_ ) { print "Ignoring Newer_than_pickstrats_date Strat: ".$strat_."\n" };
      next;
    }

    my %result_days_in_intervals = ();
    foreach my $interval_ ( keys %intervals_to_days_ )
    {
      $result_days_in_intervals { $interval_ } = 0;
    }

    my @this_strat_skip_dates_ = ();
    foreach my $interval_ ( keys %intervals_to_days_ )
    {
      foreach my $this_date_ ( @{ $intervals_to_days_ { $interval_ } } )
      {

        if ( exists $date_strat_to_existance_ { $this_date_.".".$strat_ } )
        {
          $result_days_in_intervals { $interval_ } += 1;
        }
        else  {
          push ( @this_strat_skip_dates_, $this_date_ );
        }
      }
    }

    my $good_ = GetStratProperties ( $strat_ );

    my $has_min_days_all_intv_ = 1;
    foreach my $interval_ ( keys %intervals_to_pick_from_ )
    {
      if ( $result_days_in_intervals { $interval_ } < $min_days_frac_ * ( 1 + $#{ $intervals_to_days_ { $interval_ } } ) ) {
        if ( $verbose_ ) { print "WARNING: ".$strat_." in interval ".$interval_." has results for only ".$result_days_in_intervals { $interval_ }." days OUT OF ".( 1 + $#{ $intervals_to_days_ { $interval_ } } )." days\n"; }
        $has_min_days_all_intv_ = 0;
        last;
      }
    }
    if ( $has_min_days_all_intv_ == 0 ) { next; }

    if ( $good_ == 1 ) {
      foreach my $tdate_ ( @this_strat_skip_dates_ ) {
        $strats_skip_dates_count_{ $tdate_ } += 1;
        if ( $tdate_ eq $last_trading_date_ ) {
          push ( @last_tradingdate_strats_vec_, $strat_ );
        }
      } 
      $strat_name_to_considered_ { $strat_ } = 1;
      $considered_count_ += 1;
    }
  }

  foreach my $tdate_ ( @combined_list_days_ ) {
    if ( $strats_skip_dates_count_{ $tdate_ } > 0.4 * $considered_count_ ) {
      push ( @strats_skip_dates_, $tdate_ );
    }
  }
  @strats_skip_dates_ = GetUniqueList ( @strats_skip_dates_ );

  if ( ( 1 + $#strats_skip_dates_ ) > ( (1-$common_days_frac_) * ( 1 + $#combined_list_days_ ) ) ) {
    SendErrorMailAndDie ("Common dates between filtered strats is less than ".$common_days_frac_." of the total no. of dates");
  }

  my @combined_skip_dates_ = keys %skip_dates_map_;
  push ( @combined_skip_dates_, @strats_skip_dates_ );
  
  if ( $#combined_skip_dates_ >= 0 )
  {
    if ( FindItemFromVec ( $last_trading_date_, @strats_skip_dates_ ) ) {
      my $error_string_ = "Warning: The last trading date: $last_trading_date_, ";
      if ( $#last_tradingdate_strats_vec_ < 0 ) { 
        $error_string_ .= "Results Not Available for Any Strats";
      } else {
        if ( $verbose_ ) {
          $error_string_ .= "Results Not Available for Strats:\n".join("\n", @last_tradingdate_strats_vec_);
        } else {
          $error_string_ .= "Results Not Available for ".($#last_tradingdate_strats_vec_+1)." strats";
        }
      }
      SendErrorMail ( $error_string_ );
      $mail_body_ = $mail_body_."\n$error_string_\n\n";
    }
       
    my $file_handle_ = FileHandle->new;
    $file_handle_->open ( "> $skip_dates_file_ " ) or SendErrorMailifAutomatedAndDie ( "Could not open $skip_dates_file_ for writing\n" );
    push ( @intermediate_files_, $skip_dates_file_ );
    print $file_handle_ join ( "\n", @combined_skip_dates_ );
    $file_handle_->close;

    foreach my $f_intv_ ( keys %intervals_to_file_ ) {
      my $fname_ = $intervals_to_file_ { $f_intv_ };
      my $fname_tmp_ = $fname_.".tmp";
      my $exec_cmd_ = "grep -vf $skip_dates_file_ $fname_ > $fname_tmp_";
      `$exec_cmd_`;
      `mv $fname_tmp_ $fname_`;
    }

    @combined_list_days_ = grep { ! FindItemFromVec( $_, @combined_skip_dates_ ) } @combined_list_days_;
  }
  else {
    $skip_dates_file_ = "INVALIDFILE";
  }

  { # write this list of all dates to a file
    $combined_days_file_ = GetCSTempFileName ( $PICKSTRAT_TEMP_DIR );
    push ( @intermediate_files_, $combined_days_file_ );

    open CSTF, "> $combined_days_file_" or SendErrorMailifAutomatedAndDie ( "Could not open $combined_days_file_ for writing\n" );
    print CSTF join("\n", @combined_list_days_)."\n";
    close CSTF;
  }
  
  print "# of absent_results-skip-dates: ".($#strats_skip_dates_ + 1)."\n";
  if ( $verbose_ ) { print "List of common absent-result dates_: ".join ( " ", @strats_skip_dates_ )."\n"; }

  print "# of total-skip-dates_: ".($#combined_skip_dates_ + 1).", # of total_days_to_be_considered : ".($#combined_list_days_ + 1)."\n" ; 
  if ( $verbose_ ) { print "Number of strats to be considered: ".$considered_count_."\n\n"; }
}

sub GetExponentialDecayWeights {
  my $halflife_days_ = shift;
  my $weight_vec_ref_ = shift;

  my $decay_factor_ = exp(-1 * (log(2) / $halflife_days_) );
  
  @$weight_vec_ref_ = ( );
  my $curr_weight_ = 1;

# Assuming that weight below 0.01 is insignificant
  while ( $curr_weight_ > 0.01 && $#$weight_vec_ref_ < 250 ) {
    push ( @$weight_vec_ref_, $curr_weight_ );
    $curr_weight_ *= $decay_factor_;
  }
}

sub CalcPrevWorkingDateForShortcode
{
  my $this_date_ = shift;
  my $this_shortcode_ = shift;

  $this_date_ = CalcPrevWorkingDateMult ( $this_date_, 1);
  while ( SkipWeirdDate( $this_date_ ) ||
      IsDateHoliday( $this_date_ ) ||
      IsProductHoliday( $this_date_, $this_shortcode_ ) ||
      NoDataDateForShortcode ( $this_date_, $this_shortcode_ ) ||
      !ValidDate( $this_date_ ) ||
      exists $skip_dates_map_{ $this_date_ } ) 
  {
    $this_date_ = CalcPrevWorkingDateMult ( $this_date_, 1);
  }

  return $this_date_;
}


sub GetListOfDatesFromInterval
{
  my $interval_ = shift;
  my $interval_days_weights_map_ref_ = shift;

  my @interval_args_ = split ( ' ', $interval_ );
  my $num_past_days_ = $interval_args_[0];
  my @list_of_tradingdates_in_range_ = ( );
  my %tradingdates_vec_ = ( );
  if ( $verbose_ ) { print "\nComputing for interval ".$interval_." pickstrat_date: ".$pickstrats_date_."\n"; }

  my @exp_decay_weights_ = ( );
  if ( $#interval_args_ > 0 && $interval_args_[1] eq "EXP" ) {
    GetExponentialDecayWeights ( $num_past_days_, \@exp_decay_weights_ );
    $num_past_days_ = scalar @exp_decay_weights_;
  }

# first get all trading dates in range.
  my $this_date_ = $pickstrats_date_;
  while ( @list_of_tradingdates_in_range_ < $num_past_days_ )
  {
    $this_date_ = CalcPrevWorkingDateForShortcode ( $this_date_, $shortcode_ );
    push ( @list_of_tradingdates_in_range_, $this_date_ );
  }
  my $intv_end_date_ = $list_of_tradingdates_in_range_[0];
  my $intv_start_date_ = $list_of_tradingdates_in_range_[ $#list_of_tradingdates_in_range_ ];


  if ( $#interval_args_ == 0 ) {
    %tradingdates_vec_ = map { $_ => 1 } @list_of_tradingdates_in_range_;
  }
  elsif ( $interval_args_[1] eq "EXP" ) {
    %tradingdates_vec_ = map { $list_of_tradingdates_in_range_[$_] => $exp_decay_weights_[$_] } 0..$#exp_decay_weights_;
  }
  else {
    %tradingdates_vec_ = ( );

    if ( $interval_args_[1] eq "HV_MEDIAN_VOLUME_FACTOR" ||
        $interval_args_[1] eq "LV_MEDIAN_VOLUME_FACTOR" ||
        $interval_args_[1] eq "HV_FRACTIONAL_NUMBER_OF_DAYS" ||
        $interval_args_[1] eq "LV_FRACTIONAL_NUMBER_OF_DAYS" )
    { # we will need to get volume information
      if ( $#interval_args_ < 2 ) {
        PrintErrorAndDie ( "Factor not provided with ".$interval_args_[1] );
      }

      my %tradingdate_to_volume_information_ = ( );
      my @list_of_volumes_ = ( );

      my $factor_ = $interval_args_[2];
      my $GET_AVG_VOLUME_SCRIPT = $SCRIPTS_DIR."/get_avg_volume_in_timeperiod_2.pl";
      my $exec_cmd_ = "$GET_AVG_VOLUME_SCRIPT $shortcode_ $intv_start_date_ $intv_end_date_ $exec_start_hhmm_ $exec_end_hhmm_";
      my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

      foreach my $exec_cmd_output_line_ ( @exec_cmd_output_ )
      {
        my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_line_ );
        my $tradingdate_ = $exec_cmd_output_words_[0];

        if ( $#exec_cmd_output_words_ >= 2 && ValidDate ( $tradingdate_ ) && FindItemFromVec( $tradingdate_, @list_of_tradingdates_in_range_ ) )
        {
          $tradingdate_to_volume_information_ { $exec_cmd_output_words_ [ 0 ] } = $exec_cmd_output_words_ [ 2 ];
          push ( @list_of_volumes_ , $exec_cmd_output_words_ [ 2 ] );
        }
      }

      if ( keys ( %tradingdate_to_volume_information_ ) < 1 )
      {
        print "get-avg-volume script failed , check output of this command :\n";
        print $exec_cmd_."\n";
        exit ( 0 );
      }

      my $median_volume_ = GetMedianAndSort ( \@list_of_volumes_ );
      if ( $verbose_ ) { print "median-volume for the list of days = $median_volume_\n"; }

      if ( $interval_args_[1] eq "HV_MEDIAN_VOLUME_FACTOR" )
      {
        my $cutoff_volume_ = $median_volume_ * $factor_;
        if ( $verbose_ ) { print "hv-cutoff-volume for the list of days = $cutoff_volume_\n"; }

        foreach my $tradingdate_ ( keys ( %tradingdate_to_volume_information_ ) )
        {
          if ( $tradingdate_to_volume_information_ { $tradingdate_ } >= $cutoff_volume_ )
          {
            $tradingdates_vec_ { $tradingdate_ } = 1;
          }
        }
      }
      elsif ( $interval_args_[1] eq "LV_MEDIAN_VOLUME_FACTOR" )
      {
        my $cutoff_volume_ = $median_volume_ * $factor_;
        if ( $verbose_ ) { print "lv-cutoff-volume for the list of days = $cutoff_volume_\n"; }

        foreach my $tradingdate_ ( keys ( %tradingdate_to_volume_information_ ) )
        {
          if ( $tradingdate_to_volume_information_ { $tradingdate_ } <= $cutoff_volume_ )
          {
            $tradingdates_vec_ { $tradingdate_ } = 1;
          }
        }
      }
      elsif ( $interval_args_[1] eq "HV_FRACTIONAL_NUMBER_OF_DAYS" )
      {
        my $cutoff_volume_index_ = max ( 0 , min ( ( 1 - $factor_ ) * @list_of_volumes_, $#list_of_volumes_ ) );
        my $cutoff_volume_ = $list_of_volumes_ [ $cutoff_volume_index_ ];
        if ( $verbose_ ) { print "hv-fract-cutoff-volume for the list of days = $cutoff_volume_\n"; }

        foreach my $tradingdate_ ( keys ( %tradingdate_to_volume_information_ ) )
        {
          if ( $tradingdate_to_volume_information_ { $tradingdate_ } >= $cutoff_volume_ )
          {
            $tradingdates_vec_ { $tradingdate_ } = 1;
          }
        }
      }
      elsif ( $interval_args_[1] eq "LV_FRACTIONAL_NUMBER_OF_DAYS" )
      {
        my $cutoff_volume_index_ = max ( 0 , min ( $factor_ * @list_of_volumes_, $#list_of_volumes_ ) );
        my $cutoff_volume_ = $list_of_volumes_ [ $cutoff_volume_index_ ];
        if ( $verbose_ ) { print "lv-fract-cutoff-volume for the list of days = $cutoff_volume_\n"; }

        foreach my $tradingdate_ ( keys ( %tradingdate_to_volume_information_ ) )
        {
          if ( $tradingdate_to_volume_information_ { $tradingdate_ } <= $cutoff_volume_ )
          {
            $tradingdates_vec_ { $tradingdate_ } = 1;
          }
        }
      }
    }
    elsif ( $interval_args_[1] eq "FRAC_DAYS" )
    {
      if ( $#interval_args_ < 5 ) {
        PrintErrorAndDie ( "FRAC_DAYS format: FRAC_DAYS TOP_PERCENTILE <HIGH/LOW> START_HHMM END_HHMM (VOL / STDEV / L1SZ / TREND / <CORR indep_shortcode> / <other_features> )" );
      }
      my $this_shortcode_ = $interval_args_[2];
      my $percentile_ = $interval_args_[3];
      my $highlow_ = $interval_args_[4];
      my $start_hhmm_ = $interval_args_[5];
      my $end_hhmm_ = $interval_args_[6];
      my $feature_ = $interval_args_[7];
      my @feature_aux_ = ( );
      if ( $#interval_args_ > 7 ) { @feature_aux_ = @interval_args_[8..$#interval_args_]; }

      my %date2feature_map_ = ( );
      foreach my $date_ ( @list_of_tradingdates_in_range_ ) {
        my ( $t_avg_, $is_valid_ ) = GetFeatureAverage ( $this_shortcode_, $date_, $feature_, \@feature_aux_, $start_hhmm_, $end_hhmm_, 0 );
        if ( $is_valid_ ) {
          $date2feature_map_ { $date_ } = $t_avg_;
        }
      }

      my @valid_tradingdates_sorted_ = ( );

      if ( $highlow_ eq "HIGH" ) {
        @valid_tradingdates_sorted_ = sort { $date2feature_map_{ $a } <=> $date2feature_map_{ $b } } keys %date2feature_map_;
      } elsif ( $highlow_ eq "LOW" ) {
        @valid_tradingdates_sorted_ = sort { $date2feature_map_{ $b } <=> $date2feature_map_{ $a } } keys %date2feature_map_;
      } else {
        PrintErrorAndDie ( "FRAC_DAYS format: FRAC_DAYS TOP_PERCENTILE <HIGH/LOW> START_HHMM END_HHMM (VOL / STDEV / L1SZ / TREND / <CORR indep_shortcode> / <other_features> )" );
      }

      my $cutoff_idx_ = max( 0, min( (1 - $percentile_) * @valid_tradingdates_sorted_, $#valid_tradingdates_sorted_ ) );

      %tradingdates_vec_ = map { $_ => 1 } @valid_tradingdates_sorted_[ $cutoff_idx_..$#valid_tradingdates_sorted_ ];
    }
    elsif ( $interval_args_[1] eq "ECO_EVENT" )
    {
      if ( $#interval_args_ < 2 ) {
        PrintErrorAndDie ( "EVENT NAME not provided with ".$interval_args_[1] );
      }
      my $eco_event_ = join ( ' ', @interval_args_[2..$#interval_args_] );
      my $exec_cmd_ = "grep -h \"$eco_event_\" /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_*_processed.txt";
      if ( $verbose_ ) { print $exec_cmd_."\n"; }

      my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

      foreach my $eco_line_ ( @exec_cmd_output_ )
      {
        my @eco_line_words_ = split ( ' ' , $eco_line_ ); chomp ( @eco_line_words_ );

        if ( $#eco_line_words_ >= 4 )
        {
          my $date_time_word_ = $eco_line_words_ [ $#eco_line_words_ ];

          my @date_time_words_ = split ( '_' , $date_time_word_ ); chomp ( @date_time_words_ );
          if ( $#date_time_words_ >= 0 )
          {
            my $tradingdate_ = $date_time_words_ [ 0 ];
            if ( FindItemFromVec ( $tradingdate_ , @list_of_tradingdates_in_range_ ) )
            {
              $tradingdates_vec_ { $tradingdate_ } = 1;
            }
          }
        }
      }
    }
    elsif ( $interval_args_[1] eq "ECO_EVENT_CONDITIONAL" )
    {
      if ( $#interval_args_ < 4 ) {
        PrintErrorAndDie ( "USAGE: <LOOKBACK_DAYS ECO_EVENT_CONDITIONAL CURRENCY [EVENT_NAME/ALL] [EVENT_DEGREE/ALL]" );
      }
      my $eco_event_ = $interval_args_[2];

      ### In case event name is given as ALL don't filter based on event type, similarly for event_degree ###

      if ( $interval_args_[3] ne "ALL" )
      {
        $eco_event_ = $eco_event_." ".$interval_args_[3];
      }
      else
      {
        $eco_event_ = $eco_event_." .*";
      }
      if ( $interval_args_[4] ne "ALL" )
      {
        $eco_event_ = $eco_event_." ".$interval_args_[4]." ";
      }
      else
      {
        $eco_event_ = $eco_event_." .* ";
      }

      my $exec_cmd_ = "grep -h \"$eco_event_\" /spare/local/tradeinfo/SysInfo/BloombergEcoReports/merged_eco_*_processed.txt";
      if ( $verbose_ ) { print $exec_cmd_."\n"; }
      my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

      foreach my $eco_line_ ( @exec_cmd_output_ )
      {
        my @eco_line_words_ = split ( ' ' , $eco_line_ ); chomp ( @eco_line_words_ );

        if ( $#eco_line_words_ >= 4 )
        {
          my $date_time_word_ = $eco_line_words_ [ $#eco_line_words_ ];

          my @date_time_words_ = split ( '_' , $date_time_word_ ); chomp ( @date_time_words_ );
          if ( $#date_time_words_ >= 0 )
          {
            my $tradingdate_ = $date_time_words_ [ 0 ];
            if ( FindItemFromVec ( $tradingdate_ , @list_of_tradingdates_in_range_ ) )
            {
              $tradingdates_vec_ { $tradingdate_ } = 1;
            }
          }
        }
      }
    }
    elsif ( $interval_args_[1] eq "DAYFEATURES_CLUSTER" )
    {
      if ( $#interval_args_ < 2 ) {
        PrintErrorAndDie ( "Factor not provided with ".$interval_args_[1] );
      }
      my $factor_ = $interval_args_[2];
      if ( $factor_ <= 0 || $factor_ > 1 ) {
        $factor_ = 0.3;
        print "DAYFEATURES_CLUSTER Percentile input provided: ".$interval_args_[2]." is outside the range(0-1). Using the default value: ".$factor_."\n";
      }

      my $use_weights_ = 0;
      my $weights_str_ = "";
      if ( $#interval_args_ > 2 ) {
        $use_weights_ = 1;
        $weights_str_ = $interval_args_[3];
      }

      my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
      my $features_file_ = $PICKSTRAT_TEMP_DIR."/wkodii_featuresdata_".$unique_gsm_id_;
      my $generate_features_exec_ = $HOME_DIR."/".$REPO."/WKoDii/get_day_features.pl";
      my $generate_features_cmd_ = "$generate_features_exec_ $shortcode_ $intv_end_date_ $num_past_days_ $dayfeatures_config_file_ DAY $exec_start_hhmm_ $exec_end_hhmm_ > $features_file_";
      print $generate_features_cmd_."\n";
      `$generate_features_cmd_ 2>/dev/null`;

      my $similarity_exec_ = $HOME_DIR."/".$REPO."/WKoDii/obtain_weights_on_days.py";
      my $distance_metric_ = "Mahalanobis";
      if ( $use_weights_ ) { $distance_metric_ = "WeightedEuclidean"; }

      my $similarity_exec_cmd_ = "$PYTHONEXEC $similarity_exec_ $pickstrats_date_ -1 0 $features_file_ ARIMA $distance_metric_ $dayfeatures_special_events_file_ $weights_str_ 2>/dev/null";

      if ( $verbose_ ) { print $similarity_exec_cmd_."\n"; }
      my $tradingdates_similarity_json_string_ = `$similarity_exec_cmd_ 2>/dev/null`; chomp( $tradingdates_similarity_json_string_ );
      my $tradingdates_similarity_ref_ = decode_json $tradingdates_similarity_json_string_ || 
        SendErrorMailifAutomatedAndDie ( "Error in finding_similar_days\n Json string to decode: ".$tradingdates_similarity_json_string_."\n$!" ) ;

      my $include_last_day_ = 0;
      if ( ! exists $$tradingdates_similarity_ref_{ $intv_end_date_ } ) {
        SendErrorMail ( "Warning: DayFeatures could not be generated for Yesterday: $intv_end_date_ for the interval: ".$interval_."\n Probably the SampleData for $intv_end_date_ does NOT exist\n.. Waiting for user prompt.." );
        $mail_body_ = $mail_body_."\nWarning: DayFeatures could not be generated for Yesterday: $intv_end_date_ for the interval: ".$interval_."\n";
        print "Warning: DayFeatures could not be generated for Yesterday: $intv_end_date_ for the interval: ".$interval_."\n";
        my $usr_input_ = AskUserPrompt( "Do you anyways want to include $intv_end_date_ in the interval: ".$interval_.".. Enter 0 (for NO) / 1 (for YES):", 30 );
        $include_last_day_ = 1; 
        if ( defined $usr_input_ ) {
          chomp ( $usr_input_ );
          if ( $usr_input_ eq "0" ) {
            $include_last_day_ = $usr_input_;
          }
          elsif ( $usr_input_ ne "1" ) {
            print "User Input is neither 0 nor 1. Assuming the last_trading_date $intv_end_date_ to be included in the interval.\n";
          }
        }
        else {
          print "User Input not defined. Assuming the last_trading_date $intv_end_date_ to be included in the interval.\n";
        }
      }
      my @valid_tradingdates_ = grep { $$tradingdates_similarity_ref_{ $_ } > 0 } keys %$tradingdates_similarity_ref_;
      @valid_tradingdates_ = grep { ! exists $skip_dates_map_{ $_ } } @valid_tradingdates_;
      my @valid_tradingdates_sorted_ = sort { $$tradingdates_similarity_ref_{ $a } <=> $$tradingdates_similarity_ref_{ $b } } @valid_tradingdates_;

      my $cutoff_idx_ = max( 0, min( (1 - $factor_) * @valid_tradingdates_sorted_, $#valid_tradingdates_sorted_ ) );

      %tradingdates_vec_ = map { $_ => 1 } @valid_tradingdates_sorted_[ $cutoff_idx_..$#valid_tradingdates_sorted_ ];

      if ( $include_last_day_ && ! exists $tradingdates_vec_{ $intv_end_date_ } ) {
        $tradingdates_vec_{ $intv_end_date_ } = 1;
      }
    }
    elsif ( $interval_args_[1] eq "DAYFEATURES_CLUSTER_WEIGHTED" )
    {
      my $use_weights_ = 0;
      my $weights_str_ = "";
      if ( $#interval_args_ > 1 ) {
        $use_weights_ = 1;
        $weights_str_ = $interval_args_[2];
      }

      my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
      my $features_file_ = $PICKSTRAT_TEMP_DIR."/wkodii_featuresdata_".$unique_gsm_id_;
      my $generate_features_exec_ = $HOME_DIR."/".$REPO."/WKoDii/get_day_features.pl";
      my $generate_features_cmd_ = "$generate_features_exec_ $shortcode_ $intv_end_date_ $num_past_days_ $dayfeatures_config_file_ DAY $exec_start_hhmm_ $exec_end_hhmm_ > $features_file_";
      print $generate_features_cmd_."\n";
      `$generate_features_cmd_ 2>/dev/null`;

      my $similarity_exec_ = $HOME_DIR."/".$REPO."/WKoDii/obtain_weights_on_days.py";
      my $similarity_exec_cmd_ = "$PYTHONEXEC $similarity_exec_ $pickstrats_date_ -1 0 $features_file_ ARIMA MahalanobisExp 2>/dev/null";
      if ( $use_weights_ ) {
        $similarity_exec_cmd_ = "$PYTHONEXEC $similarity_exec_ $pickstrats_date_ -1 0 $features_file_ ARIMA WeightedEuclideanExp 0 $weights_str_ 2>/dev/null";
      }
      if ( $verbose_ ) { print $similarity_exec_cmd_."\n"; }
      my $tradingdates_similarity_json_string_ = `$similarity_exec_cmd_ 2>/dev/null`; chomp( $tradingdates_similarity_json_string_ );
      my $tradingdates_similarity_ref_ = decode_json $tradingdates_similarity_json_string_ || 
        SendErrorMailifAutomatedAndDie ( "Error in finding_similar_days\n Json string to decode: ".$tradingdates_similarity_json_string_."\n$!" ) ;

      if ( ! exists $$tradingdates_similarity_ref_{ $intv_end_date_ } ) {
        $mail_body_ = $mail_body_."\nWarning: DayFeatures could not be generated for Yesterday: $intv_end_date_ for the interval: ".$interval_."\n Probably the SampleData for $intv_end_date_ does NOT exist\n";
        print "Warning: DayFeatures could not be generated for Yesterday: $intv_end_date_\n Probably the SampleData for $intv_end_date_ does NOT exist\n";
        my $usr_input_ = AskUserPrompt( "Do you anyways want to continue without including $intv_end_date_ in the interval: ".$interval_.".. Enter 0 (for NO) / 1 (for YES):", 30 );
        if ( ! defined $usr_input_ || $usr_input_ ne "1" ) {
          SendErrorMailAndDie ( "Error: DayFeatures could not be generated for Yesterday: $intv_end_date_ for the interval: ".$interval_."\nProbably the SampleData for $intv_end_date_ does NOT exist\n..Exiting" );
        }
      }

      %tradingdates_vec_ = %{$tradingdates_similarity_ref_};
    }
    else
    { # no specific criteria specified , summarize over entire set
      PrintErrorAndDie ( "Interval ".$interval_." is Undefined. Exiting..\n");
    }
  }

  if ( ! %tradingdates_vec_ ) {
    PrintErrorAndDie ( "No days in the interval ".$interval_.". Exiting..\n");
  }

  my @tradingdates_ = grep { ! exists $skip_dates_map_{ $_ } } keys %tradingdates_vec_;
  %tradingdates_vec_ = map { $_ => $tradingdates_vec_{ $_ } } @tradingdates_;

  my @days_weights_vec_ = values %tradingdates_vec_;
  my $t_sum_wt_ = GetSum ( \@days_weights_vec_ );

# logging the dates with their weights
  if ( $verbose_ ) {
    print join(", ", map { sprintf("%d:%.3f", $_, $tradingdates_vec_{ $_ }) } reverse sort keys %tradingdates_vec_ )."\n";
  }

# normalizing the weight vectors for the days
  %$interval_days_weights_map_ref_ = ( );
  foreach my $t_date_ ( keys %tradingdates_vec_ ) {
    $$interval_days_weights_map_ref_{ $t_date_ } = $tradingdates_vec_ { $t_date_ } / $t_sum_wt_;
  }
}

sub BuildListOfDates
{
  if ( %intervals_to_pick_from_weighted_ ) {
    my %weighted_days_vec_ = ( );
    foreach my $interval_ ( keys %intervals_to_pick_from_weighted_ ) {
      my %intv_tradingdates_map_ = ( );

      GetListOfDatesFromInterval ( $interval_, \%intv_tradingdates_map_ );

      foreach my $tradingdate_ ( keys %intv_tradingdates_map_ ) {
        if ( ! exists $weighted_days_vec_{ $tradingdate_ } ) {
          $weighted_days_vec_{ $tradingdate_ } = 0;
        }
        $weighted_days_vec_{ $tradingdate_ } += $intv_tradingdates_map_{ $tradingdate_ } * $intervals_to_pick_from_weighted_{ $interval_ };
      }
#      print "Interval ".$interval_." Dates: ".join(" ", keys %intv_tradingdates_map_)."\n";
    }
    my @dates_vec_ = sort keys %weighted_days_vec_;
    $intervals_to_days_ { "DAYS_WEIGHTS_VECTOR" } = \@dates_vec_;
    $intervals_start_day_ { "DAYS_WEIGHTS_VECTOR" } = $dates_vec_[0];

    my $cstempfile_ = GetCSTempFileName ( $PICKSTRAT_TEMP_DIR );
    $intervals_to_file_ { "DAYS_WEIGHTS_VECTOR" } = $cstempfile_;
    push ( @intermediate_files_, $cstempfile_ );

    open CSTF, "> $cstempfile_" or SendErrorMailifAutomatedAndDie ( "Could not open $cstempfile_ for writing\n" );
    foreach my $tradingdate_ ( @ { $intervals_to_days_ { "DAYS_WEIGHTS_VECTOR" } } )
    {
      if ( exists $weighted_days_vec_{ $tradingdate_ } ) {
        print CSTF $tradingdate_." ".$weighted_days_vec_{ $tradingdate_ }."\n";
      } else {
        print CSTF "$tradingdate_\n";
      }
    }
    close CSTF;
  }
  else {
    foreach my $interval_ ( keys %intervals_to_pick_from_ ) {
      my %intv_tradingdates_map_ = ( );

      GetListOfDatesFromInterval ( $interval_, \%intv_tradingdates_map_ );
      my @dates_vec_ = sort keys %intv_tradingdates_map_;
      $intervals_to_days_ { $interval_ } = \@dates_vec_;
      $intervals_start_day_ { $interval_ } = $dates_vec_[0];

      if ( $verbose_ ) { print "The list of tradingdates for interval ".$interval_.":\n".join ( ' ' , @{ $intervals_to_days_ { $interval_ } } )."\n"; }
      { # write this list of specific dates to a file
        my $cstempfile_ = GetCSTempFileName ( $PICKSTRAT_TEMP_DIR );
        $intervals_to_file_ { $interval_ } = $cstempfile_;
        push ( @intermediate_files_, $cstempfile_ );

        open CSTF, "> $cstempfile_" or SendErrorMailifAutomatedAndDie ( "Could not open $cstempfile_ for writing\n" );
        foreach my $tradingdate_ ( @ { $intervals_to_days_ { $interval_ } } )
        {
          if ( exists $intv_tradingdates_map_{ $tradingdate_ } ) {
            print CSTF $tradingdate_." ".$intv_tradingdates_map_{ $tradingdate_ }."\n";
          } else {
            print CSTF "$tradingdate_\n";
          }
        }
        close CSTF;
      }
    }
  }
  if ( $verbose_ ) { 
    print "List of Provided skip dates for ".$shortcode_.": ".join( " ", keys %skip_dates_map_ )."\n";
  }
  $last_trading_date_ = CalcPrevWorkingDateForShortcode ( $pickstrats_date_, $shortcode_ );
}

sub GetMUR
{
  my $t_strat_name_ = shift;
  my $end_date_ = $pickstrats_date_;
  if ($#_ >= 0 )
  {
	$end_date_ = shift;
  }
  
  my @exec_output_ = ();
  my $exec_cmd_ = "";
  
  if ($install_from_config_pool_ > 0){
  	$exec_cmd_ = $WF_SCRIPTS_DIR."/print_strat_from_config.py -c $t_strat_name_ -d $end_date_";
  	my $strat_line_ = `$exec_cmd_`;
  	my @strat_words_ = split(' ', $strat_line_);
  	if ( $#strat_words_ >= 5){
  		my $param_name_ = $strat_words_[4];
  		@exec_output_ = `cat $param_name_ | grep MAX_UNIT_RATIO`; chomp(@exec_output_);
  	}
  }else{
  	$exec_cmd_ = $SCRIPTS_DIR."/show_param_from_base.sh ".$t_strat_name_." | grep MAX_UNIT_RATIO";
  	@exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
  }
  if ( $#exec_output_ >= 0)
  {
    my @t_output_ = split ( ' ' , $exec_output_ [ 0 ] );
    return $t_output_ [ 2 ];
  }
  else
  {
# Regime
    my $t_max_mur_ = 0;
    for my $exec_output_line_ ( @exec_output_ )
    {
      if ( index ( $exec_output_line_ , "PARAMFILELIST" ) > 0 )
      {
        my @exec_output_line_words_ = split ( $exec_output_line_ , " " );
        $exec_cmd_ = "cat ".$exec_output_line_words_ [ 1 ]." | grep MAX_UNIT_RATIO";
        @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
        if ( $#exec_output_ >= 0)
        {
          my @t_output_ = split ( ' ' , $exec_output_ [ 0 ] );
          $t_max_mur_ = max ( $t_max_mur_ , $t_output_ [ 2 ] );
        }
      }
    }
    return $t_max_mur_;
  }
}

sub FilterStratsCutoff
{
  my ( $t_subset_index_, $min_pnl_cap_per_uts_ ) = @_;

  print "\nFiltering Strats on Combined Days\n\n";

  my $start_date_ = min ( values %intervals_start_day_ );
  my $end_date_ = CalcPrevWorkingDateMult ( $pickstrats_date_ , 1 );

  my %strat_to_mur_ = ( );

  my $exec_cmd_ = $SUMMARIZE_EXEC." $shortcode_ $strats_list_file_ $GLOBALRESULTSDBDIR $start_date_ $end_date_ $skip_dates_file_ ".$sort_algo_[0]." $min_pnl_cap_per_uts_ ".$combined_days_file_." 1 N 1";
  if ( $verbose_ ) { print $exec_cmd_."\n"; }
  my @ssr_results_ = `$exec_cmd_`;

  if ( $verbose_ ) { print "\@ssr_results length: ".( $#ssr_results_ + 1 )."\n"; }

  foreach my $ssr_result_line_ ( @ssr_results_ )
  {
    my @ssr_words_ = split ( ' ' , $ssr_result_line_ );
    my $t_strat_name_ = $ssr_words_ [ 1 ];

    if ( ! exists $strat_name_to_considered_{ $t_strat_name_ } ) { next; }

    my $t_volume_average_ = $ssr_words_[4];
    my $t_normalized_ttc_ = $ssr_words_[9];
    my $t_average_msg_count_ = $ssr_words_[19];
    my $t_uts_ = $ssr_words_[25];
    my $t_lotsize_ = `$get_contract_specs_exec $shortcode_ $pickstrats_date_ LOTSIZE | awk '{print \$2}' 2>/dev/null` ; chomp ( $t_lotsize_ );

    my $t_avg_vol_per_lot_ = ( $t_lotsize_ / $t_uts_ ) * $t_volume_average_;
    
    if ( $t_avg_vol_per_lot_ / $t_average_msg_count_  < $fill_ratio_ )
    {
	print "Fill ratio is low... ignoring\n";
	next;
    }

    my $t_mur_ = 0;
    if ( exists ( $strat_to_mur_ { $t_strat_name_ } ) )
    {
      $t_mur_ = $strat_to_mur_ { $t_strat_name_ };
    }
    else
    {
      $t_mur_ = GetMUR ( $t_strat_name_, $end_date_);
      $strat_to_mur_ { $t_strat_name_ } = $t_mur_;
    }

    if ( ( $#min_volume_per_strat_ >= $t_subset_index_ ) && ( $t_volume_average_ < $min_volume_per_strat_ [ $t_subset_index_ ] ) )
    {
      print "Volume $t_volume_average_ < ".$min_volume_per_strat_ [ $t_subset_index_ ]." ... ignoring ".$t_strat_name_."\n";	
      next;
    }

    if ( ( $#max_ttc_per_strat_ >= $t_subset_index_ ) && ( $t_normalized_ttc_ > $max_ttc_per_strat_ [ $t_subset_index_] ) )
    {
      print "TTC $t_normalized_ttc_ > ".$max_ttc_per_strat_ [ $t_subset_index_ ]." ... ignoring ".$t_strat_name_."\n";	
      next;
    }

    if ( ( $max_message_count_cutoff_ > 0 ) && ( $t_average_msg_count_ > $max_message_count_cutoff_ ) )
    {
      print "Message count $t_average_msg_count_ > ".$max_message_count_cutoff_." ... ignoring ".$t_strat_name_."\n";
      next;
    }

    if ( $#max_mur_ >= 0 && ( $t_mur_ > $max_mur_ [ 0 ] ) ) 
    {
      print "MUR $t_mur_ > ".$max_mur_[ 0 ]." ... ignoring ".$t_strat_name_."\n";
      next;
    }

    if ( $#exchanges_to_remove_ >= 0 )
    {
      my $flag_ = 1;
      foreach my $t_exchange_to_remove_ ( @exchanges_to_remove_ )
      {
        my $exec_cmd_ = $SCRIPTS_DIR."/check_strat_for_exchange.pl ".$t_strat_name_." ".$t_exchange_to_remove_;
        my $t_result_line_ = `$exec_cmd_`;
        chomp ( $t_result_line_ );
        my @t_result_words_ = split ' ', $t_result_line_;
        if ( $t_result_words_ [ 0 ] eq "Reject" )
        { 
          $flag_ = 0; 
          last;
        }
      }
      if ( $flag_ == 0 ) 
      {
        next; 
      }
    }

    if ( $#shortcodes_to_remove_ >= 0 )
    {
      my $flag_ = 1;
      foreach my $t_shortcode_to_remove_ ( @shortcodes_to_remove_ )
      {
        my $exec_cmd_ = $SCRIPTS_DIR."/check_strat_for_shortcode.pl ".$t_strat_name_." ".$t_shortcode_to_remove_;
        my $t_result_line_ = `$exec_cmd_`;
        chomp ( $t_result_line_ );
        my @t_result_words_ = split ' ', $t_result_line_;
        if ( $t_result_words_ [ 0 ] eq "Reject" )
        { 
          $flag_ = 0; 
          last; 
        }
      }
      if ( $flag_ == 0 ) 
      { 
        next; 
      }
    }

    if ( FindItemFromVec ( $t_strat_name_ , @strats_to_exclude_ ) )
    { 
# We were explicitly instructed not to pick this strat.
      next;
    }

    if ( FindItemFromVec ( $t_strat_name_ , @picked_strats_ ) )
    { 
# Already picked as part of an earlier subset.
      next;
    }

    $passed_strats_ { $t_strat_name_ } = 1;
  }

  open PASSED_STRAT_LIST_HANDLE, "> $passed_strats_list_file_" or SendErrorMailAndDie ( "cannot open $passed_strats_list_file_ for writing" );
  push ( @intermediate_files_, $passed_strats_list_file_ );
  print PASSED_STRAT_LIST_HANDLE join("\n", keys %passed_strats_)."\n";
  close (PASSED_STRAT_LIST_HANDLE);

  print "Number of strats that passed all cutoffs: ".(scalar keys %passed_strats_)."\n\n";
}

sub GetGlobalResults
{

  my ( $t_subset_index_ ) = @_;

# By default, it is 1.
  my $min_pnl_cap_per_uts_ = 0;
  if ( $use_min_pnl_cap_ ) 
  {
    $min_pnl_cap_per_uts_ = $use_optimal_max_loss_per_unit_size_[$t_subset_index_] > 0 ? $max_max_loss_per_unit_size_ : $max_loss_per_unit_size_[$t_subset_index_];
  }
  FilterStratsCutoff ( $t_subset_index_, $min_pnl_cap_per_uts_ ); 

  foreach my $interval_ ( keys %intervals_to_pick_from_ ) 
  {
    if ( $verbose_ ) { print "Getting global results for interval: ".$interval_." ...\n"; }

    my $start_date_ = $intervals_start_day_{ $interval_ };
    my $end_date_ = CalcPrevWorkingDateMult ( $pickstrats_date_ , 1 );


    my $exec_cmd_ = $SUMMARIZE_EXEC." $shortcode_ $strats_list_file_ $GLOBALRESULTSDBDIR $start_date_ $end_date_ $skip_dates_file_ ".$sort_algo_[0]." $min_pnl_cap_per_uts_ ".$intervals_to_file_{$interval_}." 1 N 1";

    if ( $verbose_ ) { print $exec_cmd_."\n"; }
    my @ssr_results_ = `$exec_cmd_`;

    foreach my $ssr_result_line_ ( @ssr_results_ )
    {
      my @ssr_result_words_ = split ( ' ' , $ssr_result_line_ );
      my $t_strat_name_ = $ssr_result_words_ [ 1 ];

      if ( ! exists $passed_strats_{ $t_strat_name_ } ) { next; }

      splice @ssr_result_words_, 0, 2;

      $interval_to_global_results_ { $interval_ } { $t_strat_name_ } = join(' ', @ssr_result_words_);
    }

    if ( ! exists $interval_to_global_results_{ $interval_ } ) {
      print "No results for interval ".$interval_."\n";
      next;
    }
  }
}

sub ScoreStrats
{
  my ( $t_subset_index_ ) = @_;

  print "Scoring strats ...\n";

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." # ScoreStrats ( $t_subset_index_ )\n\n";

# Remove strats that did not have results in all intervals.
  my %strat_name_to_result_count_ = ( );
  foreach my $interval_ ( keys %intervals_to_pick_from_ )
  {
    foreach my $strat_name_ ( keys %passed_strats_ )
    {
      if ( ! exists ( $strat_name_to_result_count_ { $strat_name_ } ) )
      {
        $strat_name_to_result_count_ { $strat_name_ } = 0;
      }

      if ( exists ( $interval_to_global_results_ { $interval_ } { $strat_name_ } ) )
      {
        $strat_name_to_result_count_ { $strat_name_ } ++;
      }
    }
  }

  my @all_strats_ = ( );
  my @unapproved_strats_ = ( );
  my %approved_strats_map = ( );
  GetSimulaApprovedStrats($shortcode_, \%approved_strats_map, $install_from_config_pool_);
  foreach my $strat_name_ ( keys %strat_name_to_result_count_ )
  {
    if ( ! exists ( $approved_strats_map { $strat_name_ } ) )
    {
      print "Strat $strat_name_ not approved via Simula. Ignoring\n";
      push ( @unapproved_strats_ , $strat_name_ );
    }
    elsif ( $strat_name_to_result_count_ { $strat_name_ } == keys %intervals_to_pick_from_ )
    {
      if ( ! FindItemFromVec ( $strat_name_ , @all_strats_ ) )
      {
        push ( @all_strats_ , $strat_name_ );
      }
    }
  }

  if ( $#unapproved_strats_ >= 0 )
  {
    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_."\nWARNING: The below strats were not approved from Simula. Ignoring these: \n".join('\n', @unapproved_strats_)."\n\n";
    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  }

  if ( $verbose_ ) { print "Number of strats to be scored: ".( $#all_strats_ + 1 )."\n"; }

  $mail_body_ = $mail_body_."No. of strats to be scored: ".( $#all_strats_ + 1 )."\n\n";

  if ( $pnl_prediction_algo_ ne "None"){
    print "Pnl_Predicition_Algo is ".$pnl_prediction_algo_."\n";
    my $pnl_prediction_script_ = "/home/dvctrader/basetrade/PnlPrediction/pnl_prediction_main_script.py";
    my $end_date_ = CalcPrevWorkingDateMult ( $pickstrats_date_ , 1 );
    my $start_time_ = (split('-', $fld_time_periods_[0]))[0];
    my $end_time_ = (split('-', $fld_time_periods_[0]))[1];
    my $cmd="$PYTHONEXEC $pnl_prediction_script_ -shc $shortcode_ -clist $passed_strats_list_file_ -st $start_time_ -et $end_time_ -ed $end_date_ -ldays $pnl_prediction_num_days_ -learn_predict_flag 1 -workdir $PICKSTRAT_TEMP_DIR -method '$pnl_prediction_algo_'";
    print $cmd."\n";
    my @output=`$cmd`;
    print "".join(@output);
    my $score_file_ = "None";
    foreach my $line(@output){
      if(index($line, "File with predictions is :") != -1){
        $score_file_ = (split(' ',$line))[-1];
      }
    }
    if ($score_file_ eq "None"){
      PrintErrorAndDie("Score File not found.")
    }
    else{
      open SCORE_HANDLE, "< $score_file_" or SendErrorMailAndDie ( "cannot open $score_file_ for reading" );
      while( my $row = <SCORE_HANDLE>){
        chomp $row;
        my @tokens = split(',', $row);
        if ($tokens[0] ne "Strat"){
          $strat_name_to_global_result_score_ { $tokens[0] } += int( $tokens[1] );
        }
      }
      close (SCORE_HANDLE);
    }
  }
  else {
    foreach my $interval_ (keys %intervals_to_pick_from_)
    {
      my $this_interval_strat_name_to_score_ref_;
      foreach my $strat_name_ (@all_strats_)
      {
        if (exists ( $interval_to_global_results_{ $interval_ }{ $strat_name_ } ))
        {
          my $this_interval_strat_score_ = GetScoreFromResultLineAndSortAlgo (
              $interval_to_global_results_{ $interval_ }{ $strat_name_ }, $sort_algo_[ $t_subset_index_ ] );
          $$this_interval_strat_name_to_score_ref_{ $strat_name_ } = $this_interval_strat_score_;
        }
      }

      if ($normalize_interval_scores_ ne "NA")
      {
        #Normalize Scores
        $this_interval_strat_name_to_score_ref_ = NormalizeIntervalScores( $this_interval_strat_name_to_score_ref_,
            $normalize_interval_scores_ );
      }

      if ($verbose_)
      {
        print "# Sorted Scores for Interval: ".$interval_."\n";

        foreach my $strat_name_ (sort { $$this_interval_strat_name_to_score_ref_{ $b } <=> $$this_interval_strat_name_to_score_ref_{ $a } } keys % { $this_interval_strat_name_to_score_ref_ })
        {
          print $$this_interval_strat_name_to_score_ref_{ $strat_name_ }."\t: ".$strat_name_."\n";
        }
      }

      foreach my $strat_name_ (keys % {$this_interval_strat_name_to_score_ref_})
      {
        $strat_name_to_global_result_score_{ $strat_name_ } += $$this_interval_strat_name_to_score_ref_{ $strat_name_ } * $intervals_to_pick_from_{ $interval_ };
      }

    }
  }

  foreach my $strat_name_ ( sort { $strat_name_to_global_result_score_ { $b } <=> $strat_name_to_global_result_score_ { $a } } keys %strat_name_to_global_result_score_ )
  {
    push ( @algo_sorted_strat_names_ , $strat_name_ );
    push ( @algo_sorted_strat_results_ , $strat_name_to_global_result_score_ { $strat_name_ } );
  }

  if ( $verbose_ )
  {
    print "# Final Sorted Scores\n";
    for ( my $i = 0; $i <= $#algo_sorted_strat_names_; $i++ )
    {
      print $algo_sorted_strat_results_ [ $i ]."\t: ".$algo_sorted_strat_names_ [ $i ]."\n"; 
    }
  }

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";

  return;
}

sub PickStrats
{
  my ( $t_subset_index_ ) = @_;

  print "Picking strats ...\n";

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." # PickStrats ( $t_subset_index_ )\n\n";
  $mail_body_ = $mail_body_."\n\n\t PICKED :\n";

    my $num_picked_this_subset_ = 0;

# First pick from STRATS_TO_KEEP list.
  foreach my $strat_name_ ( @strats_to_keep_ )
  {
    if ( $num_picked_this_subset_ >= $num_strats_to_install_ [ $t_subset_index_ ] )
    {
      last;
    }
#    if ( ! exists $strat_name_to_considered_ { $strat_name_ } ) {
#      print "WARNING: strat_to_keep: ".$strat_name_." is not amongst the candidate strats. Skipping it..\n";
#      next;
#    }

    if ( ! FindItemFromVec ( $strat_name_ , @picked_strats_ ) )
    {
      push ( @picked_strats_ , $strat_name_ );
      $mail_body_ = $mail_body_."\t\t ".$strat_name_."\n";
      $strat_name_to_subset_index_ { $strat_name_ } = $t_subset_index_;
      $num_picked_this_subset_++;
      print "KEEP: ".$strat_name_."\n";
      if ( $use_optimal_max_loss_per_unit_size_[ $t_subset_index_ ] ) {
        ComputeOptimalMaxLoss ( $strat_name_ , 0 );
      }
    }
  }

  if ( $automated_picking_ ) {
    if ( @algo_sorted_strat_names_ < $num_strats_to_install_ [ $t_subset_index_ ] ) {
      SendErrorMailAndDie ( "Pickstrats Failed ... not enough strats passed the cutoffs" );
    }

    my %sample_pnls_strats_vec_ = ( );
    open CSTF, "< $combined_days_file_" or SendErrorMailifAutomatedAndDie ( "Could not open $combined_days_file_ for writing\n" );
    my @combined_list_days_ = <CSTF>; chomp ( @combined_list_days_ );
    close CSTF;

    FetchPnlSamplesStrats ( $shortcode_, \@algo_sorted_strat_names_, \@combined_list_days_, \%sample_pnls_strats_vec_ );

    if ( $use_combined_metric_ == 1 )
    {
      my $lookuptonstrats_ = 7;

      my %combined_pnl_series_ = ( );

      foreach my $strat_ ( @picked_strats_ ) {
        if ( !%combined_pnl_series_ ) {
          %combined_pnl_series_ = %{$sample_pnls_strats_vec_{$strat_} };
        } else {
          CombinePnlSamples ( \%combined_pnl_series_, \%{$sample_pnls_strats_vec_{$strat_}}, \%combined_pnl_series_ );
        }
      }

      while ( $num_picked_this_subset_ < $num_strats_to_install_ [ $t_subset_index_ ] )
      {
        my $num_strats_visited_ = 0;
        my $best_metric_;
        my $best_metric_strat_;
        for ( my $i = 0; $i <= $#algo_sorted_strat_names_ ; $i++)
        {
          my $t_strat_name_ = $algo_sorted_strat_names_ [ $i ];
          if ( FindItemFromVec ( $t_strat_name_ , @picked_strats_ ) ) { next; }

          if ( $num_strats_visited_ > $lookuptonstrats_ )  { last; }

          if ( ! exists ( $strat_name_to_optimal_max_loss_ { $t_strat_name_ } ) ) { 
            ComputeOptimalMaxLoss ( $t_strat_name_ , 1 );
          }
          if ( ! exists ( $strat_name_to_optimal_max_loss_ { $t_strat_name_ } ) ) { next; }

          my %t_combined_pnl_series_ = ( );
          my %pnl_stats_map_ = ( );
          if ( !%combined_pnl_series_ ) {
            %t_combined_pnl_series_ = %{$sample_pnls_strats_vec_{$t_strat_name_}};
          } else {
            CombinePnlSamples ( \%combined_pnl_series_, \%{$sample_pnls_strats_vec_{$t_strat_name_}}, \%t_combined_pnl_series_ );
          }
          foreach my $interval_ ( keys %intervals_to_days_ ) {
            my %t_pnl_stats_map_ = ( );
            PnlSamplesGetStats ( \%t_combined_pnl_series_, \%t_pnl_stats_map_, \@{ $intervals_to_days_{ $interval_ } } );
            foreach ( keys %t_pnl_stats_map_ ) {
              if ( ! exists $pnl_stats_map_{ $_ } ) {
                $pnl_stats_map_{ $_ } = $intervals_to_pick_from_ { $interval_ } * $t_pnl_stats_map_{ $_ };
              } else {
                $pnl_stats_map_{ $_ } += $intervals_to_pick_from_ { $interval_ } * $t_pnl_stats_map_{ $_ };
              }
            }
          }
          if ( $verbose_ ) { print "Combination ".(@picked_strats_+1)." strat: ".$t_strat_name_.", ".$combined_sort_algo_.": ".$pnl_stats_map_{ $combined_sort_algo_ }."\n"; }

          if ( !defined $best_metric_ || $pnl_stats_map_{ $combined_sort_algo_ } > $best_metric_ ) {
            $best_metric_strat_ = $t_strat_name_;
            $best_metric_ = $pnl_stats_map_{ $combined_sort_algo_ };
          }

          $num_strats_visited_++;
        }

        if ( defined $best_metric_strat_ ) {
          push ( @picked_strats_, $best_metric_strat_ );
          $mail_body_ = $mail_body_."\t\t ".$best_metric_strat_."\n";
          $strat_name_to_subset_index_ { $best_metric_strat_ } = $t_subset_index_;
          $num_picked_this_subset_++;
          if ( !%combined_pnl_series_ ) {
            %combined_pnl_series_ = %{$sample_pnls_strats_vec_{$best_metric_strat_}};
          } else {
            CombinePnlSamples ( \%combined_pnl_series_, \%{$sample_pnls_strats_vec_{$best_metric_strat_}}, \%combined_pnl_series_ );
          }
          print "PICKED: ".$best_metric_strat_."\n";
        }
      }
    } else {
      for ( my $i = 0; $i <= $#algo_sorted_strat_names_; $i++)
      {
        if ( $num_picked_this_subset_ >= $num_strats_to_install_ [ $t_subset_index_ ] )
        {
          last;
        }

        my $t_strat_name_ = $algo_sorted_strat_names_ [ $i ];
# print "Samples Size of strat: ".$t_strat_name_." is ".(keys %{$sample_pnls_strats_vec_{$t_strat_name_ }})."\n";

# Check whether this strat has already been picked.
        if ( FindItemFromVec ( $t_strat_name_ , @picked_strats_ ) ) { next; }

        if ( $use_optimal_max_loss_per_unit_size_[ $t_subset_index_ ] ) {
          ComputeOptimalMaxLoss ( $t_strat_name_ , 1 );
          if ( ! exists ( $strat_name_to_optimal_max_loss_ { $t_strat_name_ } ) ) { next; } 
        }

# Check diversity.
        my $found_similar_strat_ = 0;
        foreach my $p_strat_name_ ( @picked_strats_ )
        {
          my $similarty_score_ = GetSimilarityScore ( $p_strat_name_ , $t_strat_name_, \%sample_pnls_strats_vec_ );
          if ( $similarty_score_ > $diversity_threshold_ )
          {
# This strat is too similar to the existing one, skip it.
            print "Similar to ".$p_strat_name_.", ignoring: ".$t_strat_name_." [ SIMILARITY = ".$similarty_score_." ]\n";
            $found_similar_strat_ = 1;
            last;
          }
        }
        if ( $found_similar_strat_ == 1 ) { next; }

        push ( @picked_strats_ , $t_strat_name_ );
        $mail_body_ = $mail_body_."\t\t ".$t_strat_name_."\n";
        $strat_name_to_subset_index_ { $t_strat_name_ } = $t_subset_index_;
        $num_picked_this_subset_++;
        print "PICKED: ".$t_strat_name_."\n";

        if ( $i > 25 ) {
          print "WARNING: Low ranked strats are getting picked ... consider increasing your DIVERSITY_THRESHOLD\n";
        }
      }
    }
  }

  if ( $num_picked_this_subset_ < $num_strats_to_install_ [ $t_subset_index_ ] ) {
    SendErrorMailAndDie ( "Pickstrats Failed... not enough strats picked.." );
  }

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
}

sub AssignTotalSizes
{
  if ( ! $use_total_size_from_max_loss_ ) {
    return;
  }
  my $lotsize_ = `$get_contract_specs_exec $shortcode_ $pickstrats_date_ LOTSIZE | awk '{print \$2}' 2>/dev/null` ; chomp ( $lotsize_ );
  if ( $lotsize_ eq "" ) {
    print "Warning: Could not determine the min_order_size for $shortcode_.. Assuming min_order_size=1..\n";
    $lotsize_ = 1;
  }
  
  if ( $verbose_ ) { print "Determining the Total Size for the Strats ...\n"; }
  my $combined_max_loss_per_uts_ = ComputeOptimalMaxLossCombined ( \@picked_strats_ );
  print "Combined Optimal MaxLoss: ".$combined_max_loss_per_uts_."\n";
   $mail_body_ = $mail_body_."\nCombined Optimal MaxLoss: ".$combined_max_loss_per_uts_."\n";

  my $sum_num_strats_to_install_ = GetSum ( \@num_strats_to_install_ );
  my $sum_total_size_to_run_ = int ( 0.5 + $global_max_loss_ / ( $lotsize_ * $combined_max_loss_per_uts_ ) );

  my $rem_size_ = $sum_total_size_to_run_;
  my $rem_strats_ = $sum_num_strats_to_install_;
  @total_size_to_run_ = map { 0 } @num_strats_to_install_;
  my $this_strat_size_ = 0;
  my @remove_strats_ = ( );
  foreach my $t_picked_strat_ ( @picked_strats_ ) {
    if ( $rem_size_ == 0 ) {
      push ( @remove_strats_, $t_picked_strat_ );
    }
    $this_strat_size_ = ceil ( $rem_size_ / $rem_strats_ );
    $total_size_to_run_ [ $strat_name_to_subset_index_ { $t_picked_strat_ } ] += $lotsize_ * $this_strat_size_;
    $rem_size_ -= $this_strat_size_;
    $rem_strats_--;
  }
  $sum_total_size_to_run_ = GetSum ( \@total_size_to_run_ );
  print "TOTAL_SIZE_TO_RUN ".$sum_total_size_to_run_."\n";
  $mail_body_ = $mail_body_."\nTotal Size to Run: ".$sum_total_size_to_run_."\n";

  foreach my $t_strat_ ( @remove_strats_ ) {
    print "Removing due to 0 UTS allocation: ".$t_strat_."\n";
    $mail_body_ = $mail_body_."\nRemoving due to 0 UTS allocation: ".$t_strat_."\n";
    @picked_strats_ = grep { $_ ne $t_strat_ } @picked_strats_;
    $num_strats_to_install_ [ $strat_name_to_subset_index_{ $t_strat_ } ] -= 1;
    delete $strat_name_to_subset_index_{ $t_strat_ };
  }

  $mail_body_ = $mail_body_."\n";
}

sub InstallStrats
{
  if ( $install_picked_strats_ )
  {
    my $create_lock_numtrials_ = 10;
    my $create_lock_timeout_secs_ = 30;

    my $lock_exists_ = `ssh $install_location_ "if [ -f $remote_lockfile_ ]; then echo 1; else echo 0; fi"`; chomp($lock_exists_);

    while ( $lock_exists_ && $create_lock_numtrials_ > 0 ) {
      print "lockfile $remote_lockfile_ exists at $install_location_..\n Retrying after $create_lock_timeout_secs_ seconds..\n";
      sleep $create_lock_timeout_secs_;

      $lock_exists_ = `ssh $install_location_ "if [ -f $remote_lockfile_ ]; then echo 1; else echo 0; fi"`; chomp($lock_exists_);
      $create_lock_numtrials_--;
    }

    if ( $lock_exists_ ) {
      SendErrorMailAndDie("lockfile $remote_lockfile_ exists at $install_location_\nEither already a pick_strats is running or 
          somebody messed up in last pick_strats for this server.\n" );
    }
    
    print "creating lock $install_location_:$remote_lockfile_. Make sure it is deleted after the run.\n";
    `ssh $install_location_ "touch $remote_lockfile_"`;
    $is_remote_lock_created_ = 1;

    if ( ! -d $LOCAL_PRODUCTION_STRAT_LIST_DIR )
    {
      `mkdir -p $LOCAL_PRODUCTION_STRAT_LIST_DIR`;
    }

    print "Installing strats ...\n";
    
    VerifyDBRecords ( );

    MakeLocalStratCopy ( );

    ScaleInstalledParamsStrats ( );

# Combine muliple strat-lines into single strat-files.
    CombineProductionStrats ( );

    CheckORSLimits ( );

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." # InstallStrats\n\n";

    foreach my $queryid_ ( keys %queryid_to_stratbase_ )
    {
       $mail_body_ = $mail_body_."\t\t [ $queryid_ ]  ".$queryid_to_stratbase_{ $queryid_ }."\n";
    }

    if ( ExistsWithSize ( $local_single_strat_file_ ) )
    {
      my $remove_cmd_ = "rm ".$local_single_strat_file_;
      `$remove_cmd_`
    }

    SyncStrats ( );

    UpdateProductionCrontab ( );

    AddRecordtoDB ( );
  }
  else
  {
    $mail_body_ = $mail_body_."\n NO INSTALLS SINCE INSTALL_PICKED_STRATS=".$install_picked_strats_." INSTALL_LOCATION='".$install_location_."' PROD_QUERY_START_ID='".$prod_query_start_id_."' EXCHANGE='".$exchange_."' EXEC_START_HHMM='".$exec_start_hhmm_."' EXEC_END_HHMM='".$exec_end_hhmm_."'\n";
  }
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}

sub GetQueryConfigIdsAndUTSinOrder
{
  my @query_config_ids_arr_ = ();
  foreach my $strat_ ( @picked_strats_ ) {
  push(@query_config_ids_arr_, GetIdFromName($strat_, "wf_config"));
  }
  my $query_config_ids_str_ = join(",", @query_config_ids_arr_);
  my $uts_in_order_str_ = join(",",map {$strat_to_uts_or_notional_{$_}} @picked_strats_);
  return $query_config_ids_str_, $uts_in_order_str_;
}

sub VerifyDBRecords
{
  my $configname_ = basename ( $config_file_ );
  
  my ($config_id_, $start_id_, $end_id_ ) = GetPickstratConfigId ( $configname_ );

  # Check if the start and stop ids match with DB Records
  if (defined $config_id_) {
    if ( $start_id_ != $prod_query_start_id_ ) {
      SendErrorMailAndDie("PROD_QUERY_START_ID of the configfile does not match with the DB Records. DB-start_id: $start_id_ config-start_id: $prod_query_start_id_\n".
        "Please revert to the DB start_id OR.. ask RiskAlloc Team to update the DB records\n");
    }
    elsif ( $end_id_ != $prod_query_stop_id_ ) {
      SendErrorMailAndDie("PROD_QUERY_STOP_ID of the configfile does not match with the DB Records. DB-stop_id: $end_id_ config-stop_id: $prod_query_stop_id_\n".
        "Please revert to the DB end_queryid OR.. ask RiskAlloc Team to update the DB records\n");
    }
  }

  # Check if the query_ids don't overlap with any other configs in DB
  my $matching_configid_ = GetPickstratConfigIdWithMatchingQueryIds([$prod_query_start_id_..$prod_query_stop_id_], $config_id_);
  if ($matching_configid_ > 0) {
    my $matching_configname_ = GetPickstratConfigNameForId($matching_configid_);

    SendErrorMailAndDie("PROD_QUERY_START_ID to PROD_QUERY_STOP_ID of the configfile overlaps with query-ids of another config\n".
      "Config_id: $matching_configid_ Name: $matching_configname_ \nPlease update the query_ids");
  }

  # If the confid does not already exist in DB, insert it and verify that it's inserted
  if (! defined $config_id_) {
    $config_id_ = InsertPickstratConfigId ( $configname_, $prod_query_start_id_, $prod_query_stop_id_ );

    if (! defined $config_id_) {
      SendErrorMailAndDie("ERROR: DB_config_id could not fetched/inserted for config $configname_\n");
    }
    print "Inserted Pickstrat Config to DB: config_id: $config_id_ start_query_id: $prod_query_start_id_ end_queryid $prod_query_stop_id_\n";
  }
}

 
sub AddRecordtoDB
{
  my $configname_ = basename ( $config_file_ );
  my $tcurr_hhmm_ = `date +%H%M`; chomp ( $tcurr_hhmm_ );
  my $tcurr_date_ = `date +%Y%m%d`; chomp ( $tcurr_date_ );
  my $num_queries_ = scalar (@picked_strats_);
  my $sum_maxlosses_ = GetSum ( [ @strat_to_maxlosses_{ @picked_strats_ } ] );
  my ($query_config_ids_, $uts_in_order_) = GetQueryConfigIdsAndUTSinOrder();
  my $pick_strats_logic_ = "";

  if ($automated_picking_){
    if ( %intervals_to_pick_from_weighted_ ) {
      $pick_strats_logic_  = "itpfw;";
      $pick_strats_logic_ .= join(",", map {$_."^".$intervals_to_pick_from_weighted_{$_}} keys %intervals_to_pick_from_weighted_);
    } else {
      $pick_strats_logic_  = "itpf;";
      $pick_strats_logic_ .= join(",", map {",".$_."^".$intervals_to_pick_from_{$_}} keys %intervals_to_pick_from_);
    }
    $pick_strats_logic_ .= ";" . join(",", @sort_algo_) . ";$diversity_threshold_";
  } else {
    $pick_strats_logic_ = "MANUAL";
  }

  my ($config_id_, $start_id_, $end_id_ ) = GetPickstratConfigId ( $configname_ );
  if ( ! defined $config_id_ ) {
    print "WARN: DB_config_id could not fetched/inserted for config $configname_\n";
    return;
  }

  InsertPickstratRecord ( $config_id_, $tcurr_date_, $tcurr_hhmm_, $num_queries_, $global_max_loss_, $sum_maxlosses_,
                          $computed_global_maxloss_, $query_config_ids_, $uts_in_order_, $pick_strats_logic_);
}

sub MakeLocalStratCopy
{
  foreach my $strat_ ( @picked_strats_ ) {
    my $PRINT_STRAT_NAME_FROM_BASE = $SCRIPTS_DIR."/print_strat_from_base.sh";

    my $exec_cmd_ = $PRINT_STRAT_NAME_FROM_BASE." $strat_";
    my @t_strat_name_ = `$exec_cmd_`; chomp ( @t_strat_name_ );

    if ( $#t_strat_name_ < 0 )
    {
      SendErrorMailifAutomatedAndDie ( "$exec_cmd_ returned ".@t_strat_name_ );
    }

    my $full_path_strat_name_ = $t_strat_name_ [ 0 ];

	#in case of configs, create the strategy file for this date, and then install that.
	my $new_full_strat_path_ = GetLocalStratFromConfig($full_path_strat_name_,$pickstrats_date_, $LOCAL_PRODUCTION_STRAT_LIST_DIR);
	
    my $strat_copy_path_ = MakeLocalCopyStrategy ( $new_full_strat_path_, $LOCAL_PRODUCTION_STRAT_LIST_DIR );

    $strat_copy_path_map_ { $strat_ } = $strat_copy_path_;
    $strat_localpath_to_base_{ $strat_copy_path_ } = $strat_;
  }
}

sub SetStratUTS
{
  my @rem_size_to_run_ = ( );
  my @rem_strats_to_install_ = ( );

# For the list of installed strats , get the param files to scale.
  for ( my $i = 0 ; $i <= $#num_strats_to_install_ ; $i ++ )
  {
    push ( @rem_size_to_run_, $total_size_to_run_ [ $i ] );
    push ( @rem_strats_to_install_, $num_strats_to_install_[ $i ] );
  }
  
  foreach my $strat_ ( @picked_strats_ ) {
    my $t_subset_index_ = $strat_name_to_subset_index_ { $strat_ };
    my $t_uts_ = ceil ( $rem_size_to_run_ [ $t_subset_index_ ] / $rem_strats_to_install_ [ $t_subset_index_ ] );

    $strat_to_uts_or_notional_ { $strat_ } = $t_uts_;
    $rem_size_to_run_ [ $t_subset_index_ ] -= $t_uts_;
    $rem_strats_to_install_ [ $t_subset_index_ ] --;
#    print "UTS for $strat_: ".$strat_to_uts_ { $strat_ }."\n";

    if ( $shortcode_ =~ /^(NSE_|BSE_)/ ) {
      my $t_uts1_ = `$NOTIONAL_TO_UTS_NSE $shortcode_ $pickstrats_date_ $t_uts_`;
      chomp ( $t_uts1_ );
      $strat_to_uts_{ $strat_ } = $t_uts1_;
    }
    else {
      $strat_to_uts_{ $strat_ } = $t_uts_;
    }

    $strat_to_ceiled_uts_or_notional_{ $strat_ } = $t_uts_;
    if ( $shortcode_ =~ /^(NSE_|BSE_)/ ) {
      my $t_uts1_ = `$NOTIONAL_TO_UTS_NSE $shortcode_ $pickstrats_date_ $t_uts_ 1`;
      chomp ( $t_uts1_ );
      $strat_to_ceiled_uts_or_notional_{ $strat_ } = $t_uts1_;
    }
  }
}

sub ScaleInstalledParamsStrats
{
  my %param_file_to_parent_strat_ = ( );
  my %strat_file_to_content_ = ( );
  my %param_files_to_parent_strat_to_regime_ = ( );
  my $regime_index = -1;

  foreach my $full_path_strat_name_ ( values %strat_copy_path_map_ )
  {
    push ( @intermediate_files_, $full_path_strat_name_ );

    my $exec_cmd_ = "cat $full_path_strat_name_";

    my @t_strat_contents_ = `$exec_cmd_`; chomp ( @t_strat_contents_ );

    if ( $#t_strat_contents_ < 0 )
    {
      SendErrorMailifAutomatedAndDie ( "$full_path_strat_name_ is empty" );
    }

    $strat_file_to_content_ { $full_path_strat_name_ } = $t_strat_contents_ [ 0 ];

    my @t_strat_words_ = split ( ' ' , $t_strat_contents_ [ 0 ] );
    if ( $#t_strat_words_ < 7 )
    {
      SendErrorMailifAutomatedAndDie ( "Malformed strat file ($full_path_strat_name_): ".$t_strat_contents_ [ 0 ] );
    }

    push ( @intermediate_files_, $t_strat_words_ [ 3 ] );
    push ( @intermediate_files_, $t_strat_words_ [ 4 ] );

    my $t_param_file_full_path_ = $t_strat_words_ [ 4 ];
    push ( @{ $param_file_to_parent_strat_ { $t_param_file_full_path_ } } , $full_path_strat_name_ );
  }

# if the SET_BUFFERED_END_TIMES is set then set the buffered times
  if ( $set_diff_end_times_ > 0 ) {
    my $buffer_mins_ = $set_diff_end_times_;

    my %endtime2strat_file_ = ( );
    foreach my $full_path_strat_name_ ( values %strat_copy_path_map_ )
    {
      my @strat_file_words_ = split ( ' ' , $strat_file_to_content_ { $full_path_strat_name_ } );
      if ( $#strat_file_words_ >= 6 ) {
        push ( @{ $endtime2strat_file_{ $strat_file_words_[6] } }, $full_path_strat_name_ );
      }
    }

    foreach my $end_time_ ( keys %endtime2strat_file_ )
    {
      my $hhmm_pref_ = `echo $end_time_ | egrep -o '[a-zA-Z]*_'`; chomp ( $hhmm_pref_ ); 
      my $hhmm_ = `echo $end_time_ | sed 's/[a-zA-Z]*_//g'`; chomp( $hhmm_ );
      my $t_mins_ = (int($hhmm_/100))*60 + ($hhmm_%100);
    
      if ( $hhmm_ eq "" ) {
        print "Warning: END_TIME for some strats incorrectly mentioned.. Skipping SET_BUFFERED_END_TIMES for those strats..\n";
        next;
      }
      foreach my $strat_file_ ( @{ $endtime2strat_file_{ $end_time_ } } )
      {
        my @strat_file_words_ = split ( ' ' , $strat_file_to_content_ { $strat_file_ } );
        my $t_hhmm_ = (int($t_mins_/60))*100 + ($t_mins_%60);
        $strat_file_words_[6] = $hhmm_pref_.$t_hhmm_;
        $strat_file_to_content_ { $strat_file_ } = join( ' ', @strat_file_words_ );
        $t_mins_ -= $buffer_mins_;
      }
    }
  }

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." # GetOpenTradeLossPerUnitSizeWithinBounds\n\n";

  my %unscaled_strat_file_to_regime_to_scaled_param_file_ = ( );

  foreach my $param_file_ ( keys %param_file_to_parent_strat_ )
  {
    foreach my $parent_strat_ ( @ { $param_file_to_parent_strat_ { $param_file_ } } )
    {
      $unscaled_strat_file_to_regime_to_scaled_param_file_{ $parent_strat_ } = ScaleParamFile ( $parent_strat_ , $param_file_ );
    }

# Remove the remote copy of this param file.
    my $exec_cmd_ = "ssh -l dvctrader $install_location_ 'rm -f $param_file_'";
    `$exec_cmd_`;
  }

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";

# Edit each strat file to use the scaled param file.
  foreach my $strat_file_ ( values %strat_copy_path_map_ )
  {
    my @strat_file_words_ = split ( ' ' , $strat_file_to_content_ { $strat_file_ } );
    $strat_file_words_ [ 4 ] = $unscaled_strat_file_to_regime_to_scaled_param_file_ { $strat_file_ };
    
    open ( STRAT_FILE , ">" , $strat_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $strat_file_" );
    print STRAT_FILE join ( ' ' , @strat_file_words_ )."\n";
    close ( STRAT_FILE );
  }
  return;
}

sub ScaleParamFileContents
{
  my $base_strat_name_ = shift;
  my $pf_contents_ref_ = shift;
  my $t_pf_contents_ref_ = shift;

  my @param_file_contents_ = @$pf_contents_ref_;
  
  my $unit_trade_size_actual_ = $strat_to_uts_or_notional_{ $base_strat_name_ };
  my $unit_trade_size_ = $strat_to_ceiled_uts_or_notional_{ $base_strat_name_ };

## Reading Param UTS ( orig_uts_ )
  my @uts_line_ = grep ( /\b$uts_string_\b/, @param_file_contents_ );
  
  if ( $#uts_line_ != 0 ) {
    SendErrorMailAndDie ("There is either no UnitTradeSize line or multiple lines. @uts_line_. $#uts_line_\nCannot proceed!!!."); 
  }
  my @t_words = split( /\s+/ , $uts_line_[0] );
  my $orig_uts_ = $t_words[2] ;

## for NSE, use ceiled_up value of notional_UTS for scaling of the parameters
  if ( $shortcode_ =~ /^(NSE_|BSE_)/ ) {
    $orig_uts_ = `$NOTIONAL_TO_UTS_NSE $shortcode_ $pickstrats_date_ $orig_uts_ 1`;
    chomp ( $orig_uts_ );
  }

## Reading and Setting OpentradeLoss ( Always keep OTL >= Param_OTL )
  my @otl_line_ = grep ( /MAX_OPENTRADE_LOSS/, @param_file_contents_ ) ;
  
  if ( $#otl_line_ != 0 ) {
    SendErrorMailAndDie ("There is either no MAX_OPENTRADE_LOSS line or multiple lines. @otl_line_. $#otl_line_\nCannot proceed!!!.");
  }
  @t_words = split( / / , $otl_line_[0] );
  my $param_otl_per_uts_ = $t_words[2] / $orig_uts_ ;
  
  if ( $use_paramfile_opentrade_loss_per_unit_size_ ) {
    $opentrade_loss_per_unit_size_ = $param_otl_per_uts_;
    print "opentrade_per_uts: ".$opentrade_loss_per_unit_size_."\n";
  } else {
    $opentrade_loss_per_unit_size_ = max ( $param_otl_per_uts_, $opentrade_loss_per_unit_size_ );
  }
  
  $mail_body_ = $mail_body_." ".$opentrade_loss_per_unit_size_." ".$base_strat_name_."\n";

  my $opentrade_loss_ = int ( ceil ( $opentrade_loss_per_unit_size_ * $unit_trade_size_ ) );
  my $shortterm_loss_ = $short_term_loss_per_unit_size_ * $unit_trade_size_;
  my $max_loss_ = $strat_to_maxlosses_{ $base_strat_name_ };
  my $maxposition_;
  my $maxunitratio_;

  my $filled_max_global_position_ = 0;
  my $filled_global_max_loss_ = 0;

  foreach my $param_file_line_ ( @param_file_contents_ )
  {
    my @param_line_words_ = split ( ' ' , $param_file_line_ );

    given ( $param_line_words_ [ 1 ] )
    {
      when ( $uts_string_ )
      {
        $param_line_words_ [ 2 ] = $unit_trade_size_actual_;
      }
      when ( "MAX_LOSS" )
      {
        $param_line_words_ [ 2 ] = $max_loss_;
      }
      when ( "MAX_POSITION" )
      {
        $maxposition_ = ( ( $param_line_words_ [ 2 ] * $unit_trade_size_ ) / $orig_uts_ );
        $param_line_words_ [ 2 ] = $maxposition_;
      }
      when( "WORST_CASE_POSITION" )
      {
        $param_line_words_ [ 2 ] = ( ( $param_line_words_ [ 2 ] * $unit_trade_size_ ) / $orig_uts_ );		
      }
      when ( "ZEROPOS_LIMITS" )
      {
        if ( $orig_uts_ > 1 &&
            $param_line_words_ [ 2 ] >= $orig_uts_ )
        { # this is a case where the zeropos-limits is >= than the uts.
# most likely the zeropos-limits was meant to be an absolute specification.
# this also needs to be scaled when installed in production.
          $param_line_words_ [ 2 ] = ( ( $param_line_words_ [ 2 ] * $unit_trade_size_ ) / $orig_uts_ );
        }
      }
      when ( "HIGHPOS_LIMITS" )
      {
        chomp($param_line_words_ [ 2 ]);
        if ( $orig_uts_ > 1 &&
            $param_line_words_ [ 2 ] >= $orig_uts_ )
        { # this is a case where the highpos-limits is >= than the uts.
# most likely the highpos-limits was meant to be an absolute specification.
# this also needs to be scaled when installed in production.
          $param_line_words_ [ 2 ] = ( ( $param_line_words_ [ 2 ] * $unit_trade_size_ ) / $orig_uts_ );
        }
      }
      when ( "MAX_OPENTRADE_LOSS" )
      {
        $param_line_words_ [ 2 ] = $opentrade_loss_;
      }

      when ( "MAX_SHORT_TERM_LOSS" )
      {
        $param_line_words_ [ 2 ] = $shortterm_loss_;
      }

      when ( "MAX_GLOBAL_POSITION" )
      {
        if ( $max_global_position_ > 0 )
        {
          $filled_max_global_position_ = 1; # flag to indicate that this param file had a pre-existing max-global-position field.
            $param_line_words_ [ 2 ] = ( $max_global_position_ );
        }
      }

      when ( "GLOBAL_MAX_LOSS" )
      {
        if ( $global_max_loss_ > 0 )
        {
          $filled_global_max_loss_ = 1; # flag to indicate that this param file had a pre-existing global-max-loss field.
            $param_line_words_ [ 2 ] = ( $global_max_loss_ );
        }
      }
      when ( "USE_ONLINE_UTS" )
      {
        $param_line_words_ [ 2 ] = ( 0 );
      }
      when ( "MAX_UNIT_RATIO" )
      {
        $maxunitratio_ = $param_line_words_ [ 2 ];
      }
      when ( "FEATURE_MODELFILE" )
      {
        $feature_modelfile_ = $param_line_words_[2];
        $dest_feature_modelfile_ = "/spare/local/feature_models/".basename($feature_modelfile_);
        $dest_feature_modelfile_ = CopyFileCreateDir($feature_modelfile_, $dest_feature_modelfile_, "dvctrader", $install_location_);
        $param_line_words_[2] = ( $dest_feature_modelfile_ );
      }
      default
      {
      }
    }

    push ( @$t_pf_contents_ref_, join ( ' ' , @param_line_words_ ) );
  }

  if ( ! $filled_max_global_position_ && $max_global_position_ > 0 )
  { 
# if provided a max-global-position value in the config , but the
# param file is missing a max-global-position tag , explicitly add one
    push ( @$t_pf_contents_ref_, "PARAMVALUE MAX_GLOBAL_POSITION $max_global_position_" );
  }

  if ( ! $filled_global_max_loss_ && $global_max_loss_ > 0 )
  { 
# if provided a global-max-loss value in the config , but the
# param file is missing a global-max-loss tag , explicitly add one

    if ( ! $ignore_global_max_loss_ ) {      
      push ( @$t_pf_contents_ref_, "PARAMVALUE GLOBAL_MAX_LOSS $global_max_loss_" );
    }
  }

  if ( $should_get_combined_flat_ && $combined_get_flat_model_ ne "" )
  { 
# if provided a global-max-loss value in the config , but the
# param file is missing a global-max-loss tag , explicitly add one
    push ( @$t_pf_contents_ref_, "PARAMVALUE SHOULD_GET_COMBINE_GET_FLAT $should_get_combined_flat_" );
    push ( @$t_pf_contents_ref_, "PARAMVALUE COMBINED_GET_FLAT_MODEL $combined_get_flat_model_" );
    push ( @$t_pf_contents_ref_, "PARAMVALUE NUMBER_OF_STRATS $num_strats_to_install_[0]" );
  }

# add RELEASE_CORE_PREMATURE to paramfile only if it is provided in the config-file and is either 0/1
  if ( defined $release_core_premature_ ) {
    push ( @$t_pf_contents_ref_, "PARAMVALUE RELEASE_CORE_PREMATURE $release_core_premature_" );
  }

  if ( ! defined $maxposition_ && ! defined $maxunitratio_ ) {
    PrintErrorAndDie ( "Param for strat: $base_strat_name_ has neither MAX_UNIT_RATIO nor MAX_POSITION defined" );
  } 

  if ( defined $maxposition_ ) {
    $strat_to_maxposition_{ $base_strat_name_ } = $maxposition_;
  }
  else {
    $strat_to_maxposition_{ $base_strat_name_ } = $maxunitratio_ * $strat_to_uts_{ $base_strat_name_ };
  }

  my $paramfile_prefix_ = ".".$unit_trade_size_actual_.".".$max_loss_.".".$opentrade_loss_.".".$shortterm_loss_;
  return $paramfile_prefix_;
}

sub ScaleParamFile
{
  my ( $parent_strat_ , $param_file_ ) = @_;
  my $base_strat_name_ = $strat_localpath_to_base_{ $parent_strat_ };

## Reading Param Contents
  open PARAM_FILE, "< $param_file_" or SendErrorMailifAutomatedAndDie ( "Could not open $param_file_" );
  my @param_file_contents_ = <PARAM_FILE>; chomp ( @param_file_contents_ );
  close ( PARAM_FILE );

  my @scaled_param_file_contents_ = ( );
 
## if paramfile is regime, break its lines into constituents
## for each constituent, call ScaleParamFileContents
## append the output to the @scaled_param_file_contents_
  my @param_file_names_vec_ = ( );
  my @constituent_param_contents_ = ( );
  foreach my $pline_ ( @param_file_contents_ )
  {
    if ( ( $pline_ =~ /^PARAMFILELIST\b/ or $pline_ =~ /^INDICATOR\b/ )
          and $#constituent_param_contents_ >= 0 ) 
    {
      my @scaled_constituent_param_contents_ = ( );

      my $constituent_prefix_ = 
          ScaleParamFileContents ( $base_strat_name_, \@constituent_param_contents_ , \@scaled_constituent_param_contents_ );

      push ( @scaled_param_file_contents_, @scaled_constituent_param_contents_ );
      push ( @scaled_param_file_contents_, $pline_ );
      push ( @param_file_names_vec_, $param_file_.$constituent_prefix_ );
      @constituent_param_contents_ = ( );
    }
    else {
      push ( @constituent_param_contents_, $pline_ );
    }
  }

  my $param_file_name_;
  $param_file_name_ = $param_file_names_vec_[0] if $#param_file_names_vec_ >= 0; 


## output_contents_ is empty, it means that it's not regime paramfile
## so, call ScaleParamFileContents on input paramlines
  if ( $#scaled_param_file_contents_ < 0 and $#constituent_param_contents_ >= 0 )
  {
    my $paramfile_prefix_ = ScaleParamFileContents ( $base_strat_name_, \@constituent_param_contents_ , \@scaled_param_file_contents_ );
    $param_file_name_ = $param_file_.$paramfile_prefix_;
  }

  open ( PARAM_FILE , ">" , $param_file_name_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $param_file_name_" );
  print PARAM_FILE $_."\n" foreach @scaled_param_file_contents_;
  close ( PARAM_FILE );

  push ( @intermediate_files_ , $param_file_name_ );

  return $param_file_name_;
}

sub CombineProductionStrats
{
# Combine the strats to install according to the combing key.

  my $num_strats_per_file_ = $num_strat_files_to_install_;
  my $current_prod_id_index_ = 0; # Start with the 1st id.

  my %queryid_to_strat_ = ( );

  while ( $current_prod_id_index_ <= $#picked_strats_ )
  {
    my @combined_strat_ids_ = ( );
    my $group_combining_key_ = undef;

    while ( ( $#combined_strat_ids_ + 1 ) < $num_strats_per_file_ &&
        $current_prod_id_index_ <= $#picked_strats_ )
    {
# Collect a group of $num_strat_files_to_install_ strategy lines
# or till we've reached the end of all picked strategy lines
# or if we run into a strategy which has a different combining key than the group in which it is about to be added.

      my $strat_ = $strat_copy_path_map_ { $picked_strats_ [ $current_prod_id_index_ ] };

      my $tstrat_key_ = GetCombinableKeyForStrat ( $strat_, 0 );
      $group_combining_key_ = $tstrat_key_ if ! defined $group_combining_key_;

      # The picked-strat being considered has a base-px different from the base-px of this group.
      last if $group_combining_key_ ne $tstrat_key_;

      my $query_id_ = $prod_query_start_id_ + $current_prod_id_index_;
      $queryid_to_strat_{ $query_id_ } = $strat_;
      $queryid_to_stratbase_{ $query_id_ }  = $picked_strats_ [ $current_prod_id_index_ ];

      push ( @combined_strat_ids_ , $query_id_ );

      $current_prod_id_index_ ++;
    }

    if ( $#combined_strat_ids_ >= 0 )
    {
      my $combined_strategy_file_name_ = $queryid_to_strat_{ $combined_strat_ids_[ 0 ] };

      push ( @intermediate_files_ , $combined_strategy_file_name_ );

      for ( my $i = 1 ; $i <= $#combined_strat_ids_ ; $i ++ )
      {
        $combined_strategy_file_name_ = $combined_strategy_file_name_.".".$combined_strat_ids_ [ $i ];
      }

      my $combined_strat_content_ = "";
      foreach my $query_id_ ( @combined_strat_ids_ )
      {
        my $strat_ = $queryid_to_strat_{ $query_id_ };

        open STRATH, "< $strat_" or PrintStacktraceAndDie( "ould not open $strat_ for reading" );
        my @stratlines_ = <STRATH>; chomp ( @stratlines_ );
        close STRATH;

        if ( $#stratlines_ >= 0 ) {
          $combined_strat_content_ .= $stratlines_[0]."\n";
        }
      }

      open COMBSTRATH, "> $combined_strategy_file_name_" or PrintStacktraceAndDie( "Could not open $combined_strategy_file_name_ for writing" );
      print COMBSTRATH $combined_strat_content_;
      close COMBSTRATH;

      $queryid_to_combined_stratpath_{ $combined_strat_ids_[ 0 ] } = $combined_strategy_file_name_;

      if ( ! FindItemFromVec( $combined_strategy_file_name_, @intermediate_files_ ) ) {
        push ( @intermediate_files_, $combined_strategy_file_name_ );
      }
    }
  }
}

sub SyncStrats
{
# Need to maintain a list of all query ids , should we need to combine
# multiple strat lines into a single strat file.
  foreach my $query_id_ ( keys %queryid_to_combined_stratpath_ )
  {
    my $combined_strategy_file_name_ = $queryid_to_combined_stratpath_{ $query_id_ };

    $queryid_to_remote_stratpath_{ $query_id_ } = InstallStrategyProduction ( $combined_strategy_file_name_, $install_location_ , $query_id_, "dvctrader", \%queryid_to_stratbase_, $LOCAL_PRODUCTION_STRAT_LIST_DIR );

# If add_to_crontab_ is set to 0, then write the contents of this strat_ to the local_single_strat_file_ and remove this_strat_ from the server
    if ( ! $add_to_crontab_ ) {
      my $write_to_single_strat_file_cmd_ = "ssh $install_location_ -l dvctrader \'cat ".$queryid_to_remote_stratpath_{ $query_id_ }." \' >> $local_single_strat_file_";
      `$write_to_single_strat_file_cmd_`;
      my $remove_remote_stratfile_cmd_ = "ssh $install_location_ -l dvctrader \'rm ".$queryid_to_remote_stratpath_{ $query_id_ }." \'";
      `$remove_remote_stratfile_cmd_`;
    }
  }
  if ( ! $add_to_crontab_ ) {
    print "SINGLE_STRAT_FILE_NAME: $local_single_strat_file_ \n";
  }
}

sub SendReportMail
{
  if ( ! $install_picked_strats_ )
  {
    return;
  }
  if ( ! $email_address_ )
  {
    print STDOUT $mail_body_;
    return;
  }
  else
  {
    if ( $email_address_ && $mail_body_ )
    {
      open ( MAIL , "|/usr/sbin/sendmail -t" );
      print MAIL "To: $email_address_\n";
      print MAIL "From: $email_address_\n";
      print MAIL "Subject: pick_strats_and_install ( $config_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
      print MAIL $mail_body_ ;
      close(MAIL);
    }
  }
}

sub UpdateProductionCrontab
{
  if ( $add_to_crontab_ == 1 )
  {
    ClearProductionStratList ( );
    $mail_body_ = $mail_body_."\n\n\t INSTALLED :\n";

    foreach my $query_id_ ( keys %queryid_to_remote_stratpath_ ) 
    {
      print "Adding to Crontab: $query_id_\n";
      AddToProductionStratList ( $queryid_to_remote_stratpath_{ $query_id_ }, $query_id_ );
    }
# `cat $local_production_strat_list_file_`;

# Backup existing crontab on destination server.
    BackupExistingProductionInstall ( );

    FixProductionCrontab ( );

    InstallProductionCrontab ( );
  }
}

sub ClearProductionStratList
{
  open ( LOCAL_PRODUCTION_FILE , ">" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );
  print LOCAL_PRODUCTION_FILE "##################################################################################################\n";
# Print a tiny header into the crontab , maybe it will help
# understand this install at trading time.
  print LOCAL_PRODUCTION_FILE "## [ $shortcode_ $exchange_ ] [ ";
  for ( my $i = 0 ; $i <= $#num_strats_to_install_ ; $i ++ )
  {
    print LOCAL_PRODUCTION_FILE " ( ".$num_strats_to_install_ [ $i ]." ".$total_size_to_run_ [ $i ]." ) ";
  }
  print LOCAL_PRODUCTION_FILE "LOSS ( $min_max_loss_per_unit_size_ $max_max_loss_per_unit_size_ ) ".join(" ",@max_loss_per_unit_size_)." ] ";
  for ( my $i = 0 ; $i <= $#num_strats_to_install_ ; $i ++ )
  {
    print LOCAL_PRODUCTION_FILE "VOL-SORT [ ".$min_volume_per_strat_ [ $i ]." ".$sort_algo_[ $i ]." ] ";
  }
  print LOCAL_PRODUCTION_FILE "[ $email_address_ $pickstrats_date_ : $hhmmss_ ] ##\n";
  print LOCAL_PRODUCTION_FILE "##################################################################################################\n";
  close ( LOCAL_PRODUCTION_FILE );
  return;
}

sub AddToProductionStratList
{
  my ( $remote_full_path_strat_name_, $t_dest_id_ ) = @_;

  my $ONLOAD_START_REAL_TRADING = "/home/pengine/prod/live_scripts/onload_start_real_trading.sh";
  my $fpga_arg_ = "";

#Should we enable FPGA
  if (($exchange_ eq "CME" && $fpga_flag_ == 1) ||
      ($exchange_ eq "BMF" && $fpga_flag_ == 1)) {
    $fpga_arg_ = "FPGA";
  }

  my $STOP_REAL_TRADING = "/home/dvctrader/LiveExec/ModelScripts/stop_real_trading.sh";
  my $QUERYOUTPUT_FILE = "/spare/local/logs/tradelogs/queryoutput/sms.".$t_dest_id_;

  open ( LOCAL_PRODUCTION_FILE , ">>" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );

  my $onload_switch_token_ = "OFF";
  if ( $onload_trade_exec_ ) { $onload_switch_token_ = "ON"; }

  my $vma_switch_token_ = "OFF";
  if ( $vma_trade_exec_ ) { $vma_switch_token_ = "ON"; }

  my $affinity_switch_token_ = "NF";
  if ( $affinity_trade_exec_ ) { $affinity_switch_token_ = "AF"; }

  print LOCAL_PRODUCTION_FILE "\n";

# Tagging the US strat instlocal_production_strat_list_file_alls with "US_MORN_DAY"
# and the EU strat installs with "EU_MORN_DAY"
# This will allow EU and US to co-exist.
  my $time_period_comment_tag_ = "# ".$timeperiod_;

  my $weekdays_st_range_ = "1-5";
  my $weekdays_et_range_ = "1-5";
  if ( $exec_start_hhmm_ >= 2100 ) {
    $weekdays_st_range_ = "0-4";
    if ( $exec_end_hhmm_ < $exec_start_hhmm_ ) {
      $weekdays_et_range_ = "1-5";
    } else {
      $weekdays_et_range_ = "0-4";
    }
  }

  print LOCAL_PRODUCTION_FILE substr ( $exec_start_hhmm_ , 2 , 2 )." ".substr ( $exec_start_hhmm_ , 0 , 2 )." * * ".$weekdays_st_range_." ".$ONLOAD_START_REAL_TRADING." ".$exchange_." ".$t_dest_id_." ".$remote_full_path_strat_name_." ".$onload_switch_token_." ".$affinity_switch_token_." ".$vma_switch_token_." ".$risk_tags_." ".$fpga_arg_." >> ".$QUERYOUTPUT_FILE." 2>&1 ".$time_period_comment_tag_."\n";
  print LOCAL_PRODUCTION_FILE substr ( $exec_end_hhmm_ , 2 , 2 )." ".substr ( $exec_end_hhmm_ , 0 , 2 )." * * ".$weekdays_et_range_." ".$STOP_REAL_TRADING." ".$exchange_." ".$t_dest_id_." 1>/dev/null 2>/dev/null $time_period_comment_tag_\n";

  close ( LOCAL_PRODUCTION_FILE );

  return;
}

sub SortPickedStratsByCombingKey
{
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." # SortPickedStratsByCombingKey\n\n";

  my $combining_key_to_strats_ref_ = FilterCombinableConfigsForDate (\@picked_strats_, $pickstrats_date_, 0);

  if ( $num_strat_files_to_install_ > 1 ) { 
# There will be grouping multi-strats involved , clear the list and sort by base-px.
    @picked_strats_ = ( );
  }

  foreach my $comb_key_ ( keys %$combining_key_to_strats_ref_ ) {
    $mail_body_ = $mail_body_." ".$comb_key_." -> \n";

    foreach my $t_strat_ ( @ { $$combining_key_to_strats_ref_{$comb_key_} } ) {
      $mail_body_ = $mail_body_."\t $t_strat_\n";

      if ( $num_strat_files_to_install_ > 1 ) { 
# There will be grouping involved , so this order will need to be maintained while grouping.
        push ( @picked_strats_ , $t_strat_ ); # push them in order of price types
      }
    }
  }
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}

sub DisplayMsgCountInfo
{
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." # DisplayMsgCountInfo\n\n";

  my $end_yyyymmdd_ = CalcPrevWorkingDateMult ( $pickstrats_date_ , 10 );

  my $total_msg_count_ = 0;

  foreach my $t_strat_name_ ( @picked_strats_ )
  {
    my $exec_cmd_ = "$SUMMARIZE_SINGLE_STRATEGY_RESULTS $shortcode_ $t_strat_name_ $GLOBALRESULTSDBDIR $end_yyyymmdd_ $pickstrats_date_";
    my @t_results_ = `$exec_cmd_`; chomp ( @t_results_ );
    my $msg_count_ = 0;

    foreach my $t_line_ ( @t_results_ )
    {
      my @line_words_ = split ( ' ' , $t_line_ );
      if ( $#line_words_ >= 15 && $line_words_ [ 0 ] eq "STATISTICS" )
      {
        $msg_count_ = $line_words_ [ 19 ];
      }
    }

    $mail_body_ = $mail_body_."-> ".$t_strat_name_." ".$msg_count_."\n";
    if ( $msg_count_ > 0 )
    {
      $total_msg_count_ += $msg_count_;
    }
  }
  $mail_body_ = $mail_body_."\n Total msg count of set : $total_msg_count_\n";
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}

sub FillRatioCheck
{

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  $mail_body_ = $mail_body_." # FillRatioInfo\n\n";

  my $end_yyyymmdd_ = CalcPrevWorkingDateMult ( $pickstrats_date_ , 10 );
  my $t_lot_size_ = 0;
  my $t_lotsize_ = `$get_contract_specs_exec $shortcode_ $pickstrats_date_ LOTSIZE | awk '{print \$2}' 2>/dev/null` ; chomp ( $t_lotsize_ );

  foreach my $t_strat_name_ ( @picked_strats_ )
  {
    my $exec_cmd_ = "$SUMMARIZE_SINGLE_STRATEGY_RESULTS $shortcode_ $t_strat_name_ $GLOBALRESULTSDBDIR $end_yyyymmdd_ $pickstrats_date_";
    my @t_results_ = `$exec_cmd_`; chomp ( @t_results_ );
    my $msg_count_ = 0;
    my $volume_per_uts_ = 0;

    foreach my $t_line_ ( @t_results_ )
    {
      my @line_words_ = split ( ' ' , $t_line_ );
      if ( $#line_words_ >= 26 && $line_words_ [ 0 ] eq "STATISTICS" )
      {	
        $msg_count_ = $line_words_ [ 19 ];
        $volume_per_uts_ = $line_words_[ 3 ] / $line_words_[ 26 ];
        my $t_avg_vol_per_lot_ = $t_lotsize_ * $volume_per_uts_;

        print $msg_count_." ".$volume_per_uts_." ".$t_avg_vol_per_lot_."\n";
        my $t_fill_ratio_ = $t_avg_vol_per_lot_ / $msg_count_; 
        if ( $t_fill_ratio_  < $fill_ratio_ )
        {
          print "Fill ratio is low... $t_strat_name_\n";
          $mail_body_ = $mail_body_."\nWARNING: FillRatioCheckFailed: \n".$t_strat_name_."\n\n";
          #SendErrorMailAndDie("Fill Ratio Check Failed\n");
        }

        $mail_body_ = $mail_body_."->  $t_fill_ratio_ $t_strat_name_\n";
      }
    }
  }
}


sub GetLocalStratFromConfig
{
  #given a configfile, date and local destination dir, it prints the strategy file
  my ( $configfile_, $trade_date_, $local_destination_dir_) = @_;

  my $strategy_path_ = $configfile_;

  try {
    #mofing this part to try catch statement, for now, just in case there's issue with running the script
    #assumes configfile is full path
    my $baseconfigname_ = basename($configfile_);
    my $config_dir_ = dirname($configfile_);
    my $exec_cmd_ = $WF_SCRIPTS_DIR."/print_strat_from_config.py -c $baseconfigname_ -d $trade_date_ ";
    my $strategy_content_ = `$exec_cmd_`; chomp ( $strategy_content_);
    #this solely depneds on fact that we don't print anything in case the configfile is not a valid config
    if ( $strategy_content_){
      my $strategy_dir_ = $local_destination_dir_."/".$config_dir_;
      `mkdir -p $strategy_dir_`;
      $strategy_path_ = $strategy_dir_."/"."$baseconfigname_".".stratname";
      open STRATEGYPATH, " > $strategy_path_" or PrintStacktraceAndDie("Could not open temporary local file $strategy_path_\n");
      print STRATEGYPATH $strategy_content_."\n";
      close(STRATEGYPATH);
      push(@intermediate_files_, $strategy_path_);
      return $strategy_path_;
    } 
    else {
      return $strategy_path_;
    }
  }
  catch {
    return $strategy_path_;
  }
}

sub SanityCheckGlobalMaxloss
{
  $computed_global_maxloss_ = ComputeGlobalMaxloss ( );

  if ( $computed_global_maxloss_ != -1 ) {
    if ( $global_max_loss_ < 0.8 * $computed_global_maxloss_ ) {
      SendErrorMail ("Global_MaxLoss:".$global_max_loss_." is much lower than the computed value:".$computed_global_maxloss_."..\n");
      print "WARN: Global_MaxLoss:".$global_max_loss_." is much lower than the computed value:".$computed_global_maxloss_."..\n";
      $mail_body_ = $mail_body_."\nWARN: Global_MaxLoss:".$global_max_loss_." is much lower than the computed value:".$computed_global_maxloss_."..\n";
    }
    elsif ( $global_max_loss_ > 2 * $computed_global_maxloss_ ) {
      print "WARN: Global_MaxLoss:".$global_max_loss_." is much higher than the computed value:".$computed_global_maxloss_."..\n";
      $mail_body_ = $mail_body_."\nWARN: Global_MaxLoss:".$global_max_loss_." is much higher than the computed value:".$computed_global_maxloss_."..\n";
    }
  }
}

sub BackupExistingProductionInstall
{
  my $backup_remote_production_strat_list_file_ = $remote_production_strat_list_file_.".".$pickstrats_date_.".".$hhmmss_;
  my $exec_cmd_ = "ssh $install_location_ -l dvctrader 'mkdir -p $REMOTE_PRODUCTION_STRAT_LIST_DIR'";
  `$exec_cmd_`;
  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l \> $backup_remote_production_strat_list_file_'";
  `$exec_cmd_`;
  return;
}

sub FixProductionCrontab
{
# Only remove installs for same timeperiod.
  my $time_period_comment_tag_ = "# ".$timeperiod_;

  my $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep -w $shortcode_ | grep start_real_trading | grep -w $exchange_ | grep \"$time_period_comment_tag_\"'";
  my @existing_queries_ = `$exec_cmd_`; chomp ( @existing_queries_ );

  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep stop_real_trading | grep -w $exchange_ | grep \"$time_period_comment_tag_\"'";
  my @existing_stops_ = `$exec_cmd_`; chomp ( @existing_stops_ );

# Match starts with stops , so we don't end up messing with
# cron jobs for different products.
  my %start_query_ids_ = ( );

  foreach my $existing_query_line_ ( @existing_queries_ )
  {
    my @existing_query_words_ = split ( ' ' , $existing_query_line_ );
    my $t_query_id_ = 0;

    for ( my $i = 0 ; $i <= $#existing_query_words_ ; $i ++ )
    {
      if ( $existing_query_words_ [ $i ] eq $exchange_ )
      {
        $t_query_id_ = $existing_query_words_ [ $i + 1 ];
        last;
      }
    }

    $start_query_ids_ { $t_query_id_ } = 1;
  }

  my @stop_lines_to_remove_ = ( );

  foreach my $stop_query_line_ ( @existing_stops_ )
  {
    my @existing_stop_words_ = split ( ' ' , $stop_query_line_ );
    my $t_query_id_ = 0;

    for ( my $i = 0 ; $i <= $#existing_stop_words_ ; $i ++ )
    {
      if ( $existing_stop_words_ [ $i ] eq $exchange_ )
      {
        $t_query_id_ = $existing_stop_words_ [ $i + 1 ];
        last;
      }
    }

    if ( exists ( $start_query_ids_ { $t_query_id_ } ) )
    {
# There isn't a start corresponding to this query id that matches
# the shortcode-exchange pair we are about to install.
# This is either a stray stop or a stop for a different shortcode and / or exchange.
# Preserve it.
      push ( @stop_lines_to_remove_ , $stop_query_line_ );
    }
  }

# Need to remove all starts for this shortcode and exchange ( all in  @existing_queries_ )
# Need to remove only the stops which were for this shortcode & exchange ( all in @stop_lines_to_remove_ )

  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l'";
  my @crontab_lines_ = `$exec_cmd_`; chomp ( @crontab_lines_ );

  open ( LOCAL_PRODUCTION_FILE , "<" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );
  my @local_production_lines_ = <LOCAL_PRODUCTION_FILE>;
  close ( LOCAL_PRODUCTION_FILE );

  open ( LOCAL_PRODUCTION_FILE , ">" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );

# Skip lines from a pervious install.
  my @lines_to_skip_ = ( );
  push ( @lines_to_skip_ , "################################################################################################" );
  push ( @lines_to_skip_ , "## [ $shortcode_ $exchange_ ] [" );

  foreach my $crontab_line_ ( @crontab_lines_ )
  {
    #Remove existing installs for the same query ID (required here to filter matching installs with different tags)
    if ( ( index ( $crontab_line_ , "start_real_trading" ) >= 0 ) || ( index ( $crontab_line_ , "stop_real_trading" ) >= 0 ) ) {
      my @crontab_line_words_ = split ( ' ' , $crontab_line_ );
      my $t_query_id_ = 0;

      for ( my $cron_ctr = 0 ; $cron_ctr <= $#crontab_line_words_ ; $cron_ctr ++ )
      {
        if ( $crontab_line_words_ [ $cron_ctr ] eq $exchange_ )
        {
          $t_query_id_ = $crontab_line_words_ [ $cron_ctr + 1 ];
          last;
        }
      }

      if ( ( $t_query_id_ >= $prod_query_start_id_ ) && ( $t_query_id_ < ( $prod_query_start_id_ + $sum_num_strats_to_install_ ) ) ) {
        #Duplicate query id already installed => Remove this
        next;
      }
    }

    #Remove existing installs for the same shortcode and tag
    if ( FindItemFromVec ( $crontab_line_ , @existing_queries_ ) || FindItemFromVec ( $crontab_line_ , @stop_lines_to_remove_  ) )
    {
      next;
    }

    my $to_skip_ = 0;
    foreach my $t_line_to_skip_ ( @lines_to_skip_ )
    {
      if ( index ( $crontab_line_ , $t_line_to_skip_ ) >= 0 )
      {
        $to_skip_ = 1;
      }
    }
    if ( $to_skip_ )
    {
      next;
    }

    chomp ( $crontab_line_ );

    if ( $crontab_line_ )
    {
      print LOCAL_PRODUCTION_FILE $crontab_line_."\n\n";
    }
  }

# Now append the generated crontab for this shortcode.
  print LOCAL_PRODUCTION_FILE @local_production_lines_;
  close ( LOCAL_PRODUCTION_FILE );
  return;
}

sub InstallProductionCrontab
{
  my $exec_cmd_ = "rsync -avz --delete $local_production_strat_list_file_ dvctrader\@$install_location_:$remote_production_strat_list_file_";
  `$exec_cmd_`;
  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab $remote_production_strat_list_file_'";
  `$exec_cmd_`;
  return;
}

sub RemoveIntermediateFiles
{
  foreach my $intermediate_file_ ( @intermediate_files_ )
  {
    my $exec_cmd_ = "rm -f $intermediate_file_";
#	print $exec_cmd_."\n";
    `$exec_cmd_`;
  }
  return;
}

sub IsTooInsample
{
  my ( $t_strat_name_ ) = @_;

  my $last_insample_date_ = GetInsampleDate ( $t_strat_name_ );

  my $total_weight_insample_ = 0;

  if ( $last_insample_date_ > 20110101 )
  {
    foreach my $interval_ ( keys %intervals_to_pick_from_ )
    {
      my $outsample_barrier_ = CalcPrevWorkingDateMult ( $pickstrats_date_ , ( ( $interval_ * 2 ) / 3 ) );

      if ( $last_insample_date_ > $outsample_barrier_ )
      {
        $total_weight_insample_ += $intervals_to_pick_from_ { $interval_ };
      }
    }
  }

  return ( $total_weight_insample_ >= 0.5 );
}

sub SetOMLSettings
{
  if ( ! $read_OML_settings_ )
  {
    $OML_lookback_days_ = 90;
    $OML_hit_ratio_ = 0.10;
    $OML_number_top_loss_ = 10;
  }
}

sub ComputeOptimalMaxLossCombined
{
  my $strat_vec_ref_ = shift;
  
  my $picked_strats_list_file_ = $PICKSTRAT_TEMP_DIR."/strat_list_picked_".$shortcode_."_".$timeperiod_."_".$unique_gsm_id_ ;
  open LISTHANDLE, "> $picked_strats_list_file_" or SendErrorMailifAutomatedAndDie ( "Cannot open $picked_strats_list_file_ for writing..\n" );
  print LISTHANDLE join("\n", @$strat_vec_ref_)."\n";
  close LISTHANDLE;

  my $global_hit_ratio_ = min ( 0.15, $OML_hit_ratio_ );

  $mail_body_ = $mail_body_."Using Global OML_settings\n $OML_lookback_days_ $global_hit_ratio_ $OML_number_top_loss_ \n";

  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss_combined.pl";
  my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeperiod_ $OML_lookback_days_ $global_hit_ratio_ $OML_number_top_loss_ $picked_strats_list_file_ $pickstrats_date_ $skip_dates_file_";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

  my $combined_max_loss_ = -1;
# Consider the 
  my $t_oml_check_str_ = "";
  foreach my $max_loss_line_ ( @exec_output_ )
  {
    if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
    {
      next;
    }
    my @max_loss_words_ = split ( ' ' , $max_loss_line_ );

    if ( $#max_loss_words_ >= 2 )
    {
      my $max_loss_ = $max_loss_words_ [ 0 ];
#      my $num_max_loss_hits_ = $max_loss_words_ [ 2 ];

      if ( $optimal_max_loss_check_ > 0 && $max_loss_ > 1.2 * $max_max_loss_per_unit_size_ )
      {
        $t_oml_check_str_ = $t_oml_check_str_."$max_loss_ > 1.2 * $max_max_loss_per_unit_size_, ";
        next;
      }

      $combined_max_loss_ = $max_loss_;
      last;
    }
  }
  if ( $t_oml_check_str_ ne "" ) { print "Failed OML_CHECK ".$t_oml_check_str_; }

  if ( $combined_max_loss_ == -1 )
  {    ## combined_optimal_max_loss script failed
    my $err_str_ = "Failure: Couldn't compute Combined OML\n";
    if ( $t_oml_check_str_ ne "" ) { $err_str_ = $err_str_."Try increasing the MAX_MAX_LOSS_PER_UNIT_SIZE..\n"; }
    SendErrorMailAndDie ( $err_str_ );
  }

  $mail_body_ = $mail_body_."Combined Optimal MaxLoss: ".$combined_max_loss_."\t";
  print "Combined Optimal MaxLoss: ".$combined_max_loss_."\t";

  if ( $max_max_loss_per_unit_size_ && ($combined_max_loss_ > $max_max_loss_per_unit_size_) ) {
    $mail_body_ = $mail_body_."higher than max_max_loss_per_unit_size_ resetting to ".$max_max_loss_per_unit_size_."\n";
    print "WARN : combined optimal loss is higher than given max_max_loss_per_unit_size_ check email resetting \n";
    $combined_max_loss_ = $max_max_loss_per_unit_size_;
  }
  elsif ( $min_max_loss_per_unit_size_ && ($combined_max_loss_ < $min_max_loss_per_unit_size_) ) {
    $mail_body_ = $mail_body_."lower than min_max_loss_per_unit_size_ resetting to ".$min_max_loss_per_unit_size_."\n";
    print "WARN : combined optimal loss is lower than given min_max_loss_per_unit_size_ check email resetting \n";
    $combined_max_loss_ = $min_max_loss_per_unit_size_;
  }
  if ( $max_loss_sim_real_bias_per_unit_size_ != 0 )
  {
    $combined_max_loss_ += $max_loss_sim_real_bias_per_unit_size_;
    $mail_body_ = $mail_body_."due to max loss sim real bias value: ".$max_loss_sim_real_bias_per_unit_size_.", combined_max_loss update to ".$combined_max_loss_."\n";
  }

  return $combined_max_loss_;
}

sub ComputeGlobalMaxloss
{
  my $picked_strats_list_file_ = $PICKSTRAT_TEMP_DIR."/strat_list_picked_".$shortcode_."_".$timeperiod_."_".$unique_gsm_id_ ;
  open LISTHANDLE, "> $picked_strats_list_file_" or SendErrorMailifAutomatedAndDie ( "Cannot open $picked_strats_list_file_ for writing..\n" );

  foreach my $strat_ ( keys %strat_to_uts_or_notional_ ) {
    my $tline_ = $strat_." ".$strat_to_uts_or_notional_{ $strat_ };
    if ( defined $strat_to_maxlosses_{ $strat_ } ) {
      $tline_ .= " ".$strat_to_maxlosses_{ $strat_ };
    }
    print LISTHANDLE $tline_."\n";
  }
  close LISTHANDLE;

  my $global_hit_ratio_ = min ( 0.15, $OML_hit_ratio_ );

  $mail_body_ = $mail_body_."Using Global OML_settings\n $OML_lookback_days_ $global_hit_ratio_ $OML_number_top_loss_ \n";

  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss_combined.pl";
  my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeperiod_ $OML_lookback_days_ $global_hit_ratio_ $OML_number_top_loss_ $picked_strats_list_file_ $pickstrats_date_ $skip_dates_file_";
#  print "$exec_cmd_\n";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

  my $combined_max_loss_ = -1;
# Consider the 
  foreach my $max_loss_line_ ( @exec_output_ )
  {
    if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
    {
      next;
    }
    my @max_loss_words_ = split ( ' ' , $max_loss_line_ );

    if ( $#max_loss_words_ >= 2 )
    {
      my $max_loss_ = $max_loss_words_ [ 0 ];
#      my $num_max_loss_hits_ = $max_loss_words_ [ 2 ];

      $combined_max_loss_ = $max_loss_;
      last;
    }
  }

  if ( $combined_max_loss_ == -1 )
  {    ## combined_optimal_max_loss script failed
    print "WARN: Couldn't compute Combined OML\n";
    $mail_body_ = $mail_body_."\nWARN: Couldn't compute Combined OML\n";
    return -1;
  }

  my @uts_vec_ = values %strat_to_uts_or_notional_;
  my $total_size_ = GetSum ( \@uts_vec_ );

  $combined_max_loss_ += $max_loss_sim_real_bias_per_unit_size_;
  $combined_max_loss_ *= $total_size_;

  if ( %strat_to_maxlosses_ ) {
    my $sum_maxlosses_ = 0;
    foreach my $strat_ ( keys %strat_to_maxlosses_ ) {
      $sum_maxlosses_ += $strat_to_maxlosses_{ $strat_ };
    }

    if ( $combined_max_loss_ > $sum_maxlosses_ ) {
      $combined_max_loss_ = $sum_maxlosses_;
    }
    elsif ( $combined_max_loss_ < 0.6 * $sum_maxlosses_ ) {
#    print "WARN: Combined OML (".$combined_max_loss_.") < 0.6 * Sum_maxlosses (".$sum_maxlosses_.").. Resetting it to ".(0.6 * $sum_maxlosses_)."\n";
      $combined_max_loss_ = 0.6 * $sum_maxlosses_;
    }
  }

  return $combined_max_loss_;
}

sub ComputeStdevMaxloss
{
  my ( $t_strat_ , $t_auto_pick_ ) = @_;
  if ( exists ( $strat_name_to_optimal_max_loss_ { $t_strat_ } ) ) { return; }

  my $FIND_OPTIMAL_STDEV_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_stdev_max_loss.pl";
  my $exec_cmd_ = $FIND_OPTIMAL_STDEV_MAX_LOSS." $shortcode_ $timeperiod_ $OSML_lookback_days_ $OSML_hit_ratio_ $OSML_high_volatile_percentile_ $t_strat_ $pickstrats_date_ $OSML_is_volatile_ $skip_dates_file_";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

  my $max_loss_;
  foreach my $line_ ( @exec_output_ ) {
    if ( $line_ =~ /Max Loss for / ) {
      my @lwords_ = split( ' ', $line_ );
      $max_loss_ = $lwords_[ $#lwords_ ];
    }
  }

  if ( ! defined $max_loss_ ) {
      print "Couldn't compute Stdev_OML for ".$t_strat_." . Computing normal OML\n";
      ComputeOptimalMaxLossAllDays ( $t_strat_ , $t_auto_pick_ );
      return;
  }
  else {
    if ( $t_auto_pick_ )
    {
      if ( $optimal_max_loss_check_ > 0 && $max_loss_ > 1.2 * $max_max_loss_per_unit_size_ )
      {
        print "OML WARNING for $t_strat_: $max_loss_ > 1.2 * $max_max_loss_per_unit_size_.. ";
        $max_loss_ = 1.2 * $max_max_loss_per_unit_size_;
        print "Capping Maxloss at $max_loss_\n";
      }
    }
    $strat_name_to_optimal_max_loss_ { $t_strat_ } = $max_loss_;
  }
}

sub ComputeOptimalMaxLoss
{
  my ( $t_strat_ , $t_auto_pick_ ) = @_;

  if ( $read_OSML_settings_ ) {
    ComputeStdevMaxloss ( $t_strat_ , $t_auto_pick_ );
  }
  else {
    ComputeOptimalMaxLossAllDays ( $t_strat_ , $t_auto_pick_ );
  }
}

sub ComputeOptimalMaxLossAllDays
{
  my ( $t_strat_ , $t_auto_pick_ ) = @_;

  if ( exists ( $strat_name_to_optimal_max_loss_ { $t_strat_ } ) ) { return; }

  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl";
  my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeperiod_ $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ $t_strat_ $pickstrats_date_ $skip_dates_file_";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

# Consider the 
  my $t_oml_check_str_ = "";
  my $t_mlp_check_str_ = "";
  foreach my $max_loss_line_ ( @exec_output_ )
  {
    if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
    {
      next;
    }

    my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
    if ( $#max_loss_words_ >= 2 )
    {
      my $max_loss_ = $max_loss_words_ [ 0 ];
      my $avg_pnl_ = $max_loss_words_ [ 1 ];
#      my $num_max_loss_hits_ = $max_loss_words_ [ 2 ];
      if ( $t_auto_pick_ )
      {
        my $t_max_loss_by_pnl_ = int($max_loss_/$avg_pnl_);
        if ( $max_loss_pnl_cutoff_ > 0 &&
           ( $avg_pnl_ <= 0 || $t_max_loss_by_pnl_ > $max_loss_pnl_cutoff_ ) )
        {
          $t_mlp_check_str_ = $t_mlp_check_str_."$t_max_loss_by_pnl_ > $max_loss_pnl_cutoff_, ";
          next;
        }
        if ( $optimal_max_loss_check_ > 0 && $max_loss_ > 1.2 * $max_max_loss_per_unit_size_ )
        {
          $t_oml_check_str_ = $t_oml_check_str_."$max_loss_ > 1.2 * $max_max_loss_per_unit_size_, ";
          next;
        }
      }
      $strat_name_to_optimal_max_loss_ { $t_strat_ } = $max_loss_ ;
      last;
    }
  }

  if ( ! exists ( $strat_name_to_optimal_max_loss_ { $t_strat_ } ) )
  {
    if ( $t_auto_pick_ ) 
    { 
      print "Ignoring $t_strat_ . "; 
      if ( $t_oml_check_str_ ne "" ) { print "Failed OML_CHECK ".$t_oml_check_str_; } 
      if ( $t_mlp_check_str_ ne "" ) { print "Failed MAXLOSS_BY_PNL_CHECK ".$t_mlp_check_str_; } 
      print "\n";
    }
    else
    {
      print "Couldn't compute OML for ".$t_strat_." . Using min_max_loss_per_unit_size_ = $min_max_loss_per_unit_size_\n"; 
      $strat_name_to_optimal_max_loss_ { $t_strat_ } = $min_max_loss_per_unit_size_;
    }
  }
}

sub CheckResetMaxLosses
{
# Use user provided max-losses.
# Purge computed max-losses.
  $mail_body_ = $mail_body_."Using OML_settings\n $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ \n";
  if ( $print_OML_ ) { print "\nOML_Setting $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ \n"; }
  
  if ( $print_OML_ && $max_max_loss_per_unit_size_ ) { print "\nMAX_MAX_LOSS $max_max_loss_per_unit_size_\n"; }
  if ( $print_OML_ && $min_max_loss_per_unit_size_ ) { print "MIN_MAX_LOSS $min_max_loss_per_unit_size_\n"; }
  
  foreach my $strat_name_ ( @picked_strats_ )
  {
    if ( exists $strat_name_to_optimal_max_loss_ { $strat_name_ } )
    {
      $mail_body_ = $mail_body_." ".$strat_name_to_optimal_max_loss_ { $strat_name_ }." ".$strat_name_."\n";
    }
    my $t_max_loss_per_unit_size_ = $max_loss_per_unit_size_ [ $strat_name_to_subset_index_ { $strat_name_ } ];

    $mail_body_ = $mail_body_."for strategy: ".$strat_name_." ";
    if ( defined $t_max_loss_per_unit_size_ && $t_max_loss_per_unit_size_ > 0
      && ! $use_optimal_max_loss_per_unit_size_ [ $strat_name_to_subset_index_ { $strat_name_ } ] )
    {
      $mail_body_ = $mail_body_."using user specified max_loss ".$t_max_loss_per_unit_size_."\n" ;
      $strat_name_to_optimal_max_loss_ { $strat_name_ } = $t_max_loss_per_unit_size_;
    }
    elsif ( ! exists $strat_name_to_optimal_max_loss_ { $strat_name_ } )
    {
      print "WARN: Optimal max_loss NOT present.. using user specified max_loss ".$t_max_loss_per_unit_size_."\n";
      $strat_name_to_optimal_max_loss_ { $strat_name_ } = $t_max_loss_per_unit_size_;
    }
    elsif ( ( $max_max_loss_per_unit_size_ && $strat_name_to_optimal_max_loss_ { $strat_name_ } > $max_max_loss_per_unit_size_ ) )
    {
      $mail_body_ = $mail_body_."optimal loss is higher than given max_max_loss_per_unit_size_ resetting to ".$max_max_loss_per_unit_size_."\n";
      print "WARN : optimal loss is higher than given max_max_loss_per_unit_size_ check email resetting \n";
      $strat_name_to_optimal_max_loss_ { $strat_name_ } = $max_max_loss_per_unit_size_;
    }
    elsif ( ( $min_max_loss_per_unit_size_ && $strat_name_to_optimal_max_loss_ { $strat_name_} < $min_max_loss_per_unit_size_ ) ) 
    {
      $mail_body_ = $mail_body_."optimal loss is lower than given min_max_loss_per_unit_size_ resetting to ".$min_max_loss_per_unit_size_."\n";
      print "WARN : optimal loss is lower than given min_max_loss_per_unit_size_ check email resetting \n";
      $strat_name_to_optimal_max_loss_ { $strat_name_ } = $min_max_loss_per_unit_size_ ;
    }
    else
    {
      $mail_body_ = $mail_body_."using optimal loss ".$strat_name_to_optimal_max_loss_ { $strat_name_ }."\n";
    }
    if ( $max_loss_sim_real_bias_per_unit_size_ != 0 )
    {
      $strat_name_to_optimal_max_loss_ { $strat_name_ } = $strat_name_to_optimal_max_loss_ { $strat_name_ } + $max_loss_sim_real_bias_per_unit_size_ ;
      $mail_body_ = $mail_body_."due to max loss sim real bias value: ".$max_loss_sim_real_bias_per_unit_size_." , max loss updated to ".$strat_name_to_optimal_max_loss_ { $strat_name_ }."\n";
    }

## naming the local and remote paramfile
## scaling the Parameters and writing them to the local_paramfile
    my $max_loss_ = sprintf ( "%d" , $strat_name_to_optimal_max_loss_ { $strat_name_ } * $strat_to_ceiled_uts_or_notional_{ $strat_name_ } );
## capping the total max_loss for the strat (currently enabled only for NSE/BSE)  
    if ( $shortcode_ =~ /^(NSE_|BSE_)/ ) {
      if ( defined $min_max_loss_per_strat_ ) {
        $max_loss_ = max( $max_loss_, $min_max_loss_per_strat_ );
      }
      if ( defined $max_max_loss_per_strat_ ) {
        $max_loss_ =  min( $max_loss_, $max_max_loss_per_strat_ );
      }
    }
    $strat_to_maxlosses_{ $strat_name_ } = $max_loss_;
  }
  return;
}

sub CheckORSLimits
{
  print "Loading the Limits for $shortcode_ from server: $install_location_\n";

  my $max_order_size_limit_;
  my $max_position_limit_;


  if ( $shortcode_ =~ /^(NSE_|BSE_)/ ) {
    print "Ignoring Limits Check for NSE products\n";
    return;
  }
  elsif ( $exchange_ eq "HONGKONG" || $exchange_ eq "HKEX" ) {
    my $get_limits_cmd_ = "ssh dvcinfra\@$install_location_ \'cat /home/newedge/limit.csv\'";
    my @symbol_limit_lines_ = `$get_limits_cmd_ 2>/dev/null`; chomp( @symbol_limit_lines_ );
    if ( $#symbol_limit_lines_ >= 0 ) {
      @symbol_limit_lines_ = grep { !( $_ =~ /^#/) } @symbol_limit_lines_;

      foreach my $limit_line_ ( @symbol_limit_lines_ ) {
        my @limit_words_ = split( " ", $limit_line_ );
        if ( $#limit_words_ < 3 ) {
          next;
        }
        if ( $limit_words_[0] eq $shortcode_ ) {
          if ( ! defined $max_order_size_limit_ || $max_order_size_limit_ > $limit_words_[3] ) {
            $max_order_size_limit_ = $limit_words_[3];
          }
          if ( ! defined $max_position_limit_ || $max_position_limit_ > $limit_words_[2] ) {
            $max_position_limit_ = $limit_words_[2];
          }
        }
      }
    } else {
      my $limits_logfile_ = `ssh dvcinfra\@$install_location_ \'ls -1 /var/TradingSystemLog/log.201\* | sort | tail -1\'`; chomp ( $limits_logfile_ );
      if ( defined $limits_logfile_ && $limits_logfile_ ne "" ) {
        my @symbol_limit_lines_ = `ssh dvcinfra\@$install_location_ \'grep ADDTRADINGSYMBOL $limits_logfile_\' 2>/dev/null`; chomp ( @symbol_limit_lines_ );

        foreach my $limit_line_ ( @symbol_limit_lines_ ) {
          $limit_line_ =~ s/^.*ADDTRADINGSYMBOL/ADDTRADINGSYMBOL/g;
          my @limit_words_ = split( " ", $limit_line_ );
          if ( $#limit_words_ < 3 ) {
            next;
          }
          if ( $limit_words_[1] eq $shortcode_ ) {
            if ( ! defined $max_order_size_limit_ || $max_order_size_limit_ > $limit_words_[3] ) {
              $max_order_size_limit_ = $limit_words_[3];
            }
            if ( ! defined $max_position_limit_ || $max_position_limit_ > $limit_words_[2] ) {
              $max_position_limit_ = $limit_words_[2];
            }
          }
        }
      }
    }
  }
  else {
    my $get_limits_cmd_ = "ssh dvcinfra\@$install_location_ \'cat /home/pengine/prod/live_configs/\`hostname\`_addts.cfg\'";
    my $t_shortcode_ = $shortcode_;
    if($shortcode_ eq "XTE_0") {
      $t_shortcode_= "XT_0";
    }
      elsif($shortcode_ eq "YTE_0") {
        $t_shortcode_= "YT_0";
    }
    if ( $exchange_ eq "NSE" || $shortcode_ =~ /^(NSE_|BSE_)/ ){
      $get_limits_cmd_ = "ssh dvcinfra\@$install_location_ \'cat /home/dvcinfra/LiveExec/config/AddTradingSymbolConfig\'";
    }
    my @symbol_limit_lines_ = `$get_limits_cmd_ 2>/dev/null`; chomp( @symbol_limit_lines_ );
    @symbol_limit_lines_ = grep { !( $_ =~ /^#/) } @symbol_limit_lines_;

    foreach my $limit_line_ ( @symbol_limit_lines_ ) {
      my @limit_words_ = split( " ", $limit_line_ );
      if ( $#limit_words_ < 6 ) {
        next;
      }
      if ( $limit_words_[2] eq "ADDTRADINGSYMBOL" ) {
        my $shc_ = $limit_words_[3];
        if ( $limit_words_[3] eq $t_shortcode_ ) {
          if ( ! defined $max_order_size_limit_ || $max_order_size_limit_ > $limit_words_[5] ) {
            $max_order_size_limit_ = $limit_words_[5];
          }
          if ( ! defined $max_position_limit_ || $max_position_limit_ > $limit_words_[4] ) {
            $max_position_limit_ = $limit_words_[4];
          }
        }
      }
      elsif ( $limit_words_[2] eq "ADDCOMBINEDTRADINGSYMBOL" ) {
        my $shcgrp_ = $limit_words_[3];
        my $shc_weight_;

        for ( my $lword_idx_ = 8; $lword_idx_ < $#limit_words_; $lword_idx_ += 2 ) {
          if ( $limit_words_[ $lword_idx_ ] eq $t_shortcode_ ) {
            $shc_weight_ = $limit_words_[ $lword_idx_ + 1 ];
            last;
          }
        }

        if ( defined $shc_weight_ ) {
          my $t_ordersize_ = $limit_words_[5] / $shc_weight_;
          my $t_maxpos_ = $limit_words_[4] / $shc_weight_;

          if ( ! defined $max_order_size_limit_ || $max_order_size_limit_ > $t_ordersize_ ) {
            $max_order_size_limit_ = $t_ordersize_;
          }
          if ( ! defined $max_position_limit_ || $max_position_limit_ > $t_maxpos_ ) {
            $max_position_limit_ = $t_maxpos_;
          }
        }
      }
    }
  }
  if ( defined $max_order_size_limit_ && defined $max_position_limit_ ) {
    print "OrderSizeLimit: ".$max_order_size_limit_.", MaxPosLimit: ".$max_position_limit_."\n";

    my $total_maxpos_ = 0;
    foreach my $strat_ ( keys %strat_to_uts_ ) {
      if ( $strat_to_uts_{ $strat_ } > $max_order_size_limit_ ) {
        SendErrorMailAndDie ( "UTS for strat $strat_ exceeds the OrderSizeLimit $max_order_size_limit_" );
      }
      $total_maxpos_ += $strat_to_maxposition_{ $strat_ };
    }
    if ( $total_maxpos_ > $max_position_limit_ ) {
      SendErrorMailAndDie ( "Total MaxPos of all the queries exceeeds the MaxPosLimit $max_position_limit_" );
    }
  }
  else {
    SendErrorMailAndDie ( "MaxPosLimit and OrderSizeLimit could not fetched for ".$shortcode_ );
  }
}

sub NormalizeIntervalScores
{
  my $strat_name_to_score_map_ref_ = shift;
  my $normalize_string_ = shift;

  return $strat_name_to_score_map_ref_ if ( ! $strat_name_to_score_map_ref_ );

  if ( $verbose_ ) { print "Normalizing Scores [$normalize_string_] ... \n"; }
  
  my $strat_name_to_norm_score_map_ref_ ;
  given ( $normalize_string_ )
  {
    when ( "Rank" )
    {
      my @sorted_strat_names_ = sort { $$strat_name_to_score_map_ref_{$b} <=> $$strat_name_to_score_map_ref_{$a} } keys % {$strat_name_to_score_map_ref_};
      my $last_norm_score_ = -1;
      foreach my $t_strat_ (@sorted_strat_names_)
      {
        $$strat_name_to_norm_score_map_ref_{$t_strat_} = $last_norm_score_--;
      }
    }
    when ( "BucketRank" )
    {
      my @sorted_strat_names_ = sort { $$strat_name_to_score_map_ref_{$b} <=> $$strat_name_to_score_map_ref_{$a} } keys % {$strat_name_to_score_map_ref_};
      my $first_strat_ = shift @sorted_strat_names_;
      $$strat_name_to_norm_score_map_ref_{$first_strat_} = -1;
      my $last_score_ = $$strat_name_to_score_map_ref_{$first_strat_};
      my $last_norm_score_ = -1;
      for ( my $s_idx_=0; $s_idx_<=$#sorted_strat_names_; $s_idx_++ )
      {
        my $t_strat_ = $sorted_strat_names_[$s_idx_];
        my $this_score_  = $$strat_name_to_score_map_ref_{$t_strat_};
        if ( abs($last_score_ - $this_score_) > 0.05*abs($last_score_) ) 
        {
          $last_norm_score_ = -1*($s_idx_+2);
          $last_score_ = $this_score_;
        }
        $$strat_name_to_norm_score_map_ref_{$t_strat_} = $last_norm_score_;
      }
    }
    default
    {
      $strat_name_to_norm_score_map_ref_ = $strat_name_to_score_map_ref_;
    }
  }
  return $strat_name_to_norm_score_map_ref_;
}

sub GetScoreFromResultLineAndSortAlgo
{
  my ( $t_result_line_ , $t_sort_algo_ ) = @_;
  my @t_result_words_ = split ( ' ' , $t_result_line_ );

  my $t_pnl_ = $t_result_words_ [ 0 ];
  my $t_pnl_stdev_ = $t_result_words_ [ 1 ];
  my $t_vol_ = $t_result_words_ [ 2 ];
  my $t_pnl_sharpe_ = $t_result_words_ [ 3 ];
  my $t_ttc_ = $t_result_words_ [ 7 ];
  my $t_dd_ = 1.0 / $t_result_words_ [ 14 ];
  my $t_msg_count_ = $t_result_words_ [ 17 ];
  my $t_gain_pain_ratio_ = $t_result_words_ [ 18 ];
  my $kcna_score_ = $t_result_words_ [ $#t_result_words_ ];

  my $score_ = 0;

  given ( $t_sort_algo_ )
  {
    when ( "PNL" )
    {
      $score_ = $t_pnl_;
    }
    when ( "GAIN_PAIN" )
    {
      $score_ = $t_gain_pain_ratio_;
    }
    when ( "PNL_ADJ" )
    {
      $score_ = $t_pnl_ - 0.30 * $t_pnl_stdev_;
    }
    when ( "PNL_SHARPE_AVG" )
    {
      $score_ = $t_pnl_stdev_==0 ? $t_pnl_ : $t_pnl_ * min ( 1.2, abs ( $t_pnl_sharpe_ ) );
    }
    when ( "PNL_SQRT" )
    {
      $score_ = SqrtSign ( $t_pnl_ );
    }
    when ( "PNL_VOL" )
    {
      $score_ = $t_pnl_ * $t_vol_;
    }
    when ( "PNL_SQRT_VOL" )
    {
      $score_ = $t_pnl_ * sqrt ( $t_vol_ );
    }
    when ( "PNL_VOL_SQRT" )
    {
      $score_ = SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ );
    }
    when ( "PNL_DD" )
    {
      $score_ = $t_pnl_ * abs ( $t_dd_ );
    }
    when ( "PNL_SQRT_DD" )
    {
      $score_ = $t_pnl_ * sqrt ( abs ( $t_dd_ ) );
    }
    when ( "PNL_VOL_SQRT_BY_DD" )
    {
      $score_ = $t_pnl_ * sqrt ( abs ( $t_vol_ ) ) * abs ( $t_dd_ ) ;
    }
    when ( "PNL_VOL_SQRT_BY_DD_SQRT" )
    {
      $score_ = SqrtSign ( $t_pnl_ ) * sqrt ( abs ( $t_vol_ ) ) * sqrt ( abs ( $t_dd_ ) ) ; # sign(pnl)*sqrt(abs(pnl)) * sqrt(abs(volume)) * sqrt(max(0,pnl_dd_ratio))
    }
    when ( "PNL_VOL_SQRT_BY_TTC_BY_DD_SQRT" )
    {
      if ( $t_ttc_ == 0 ) 
      {
        $score_ = -9999;
      }
      else 
      {
        $score_ = ( SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / $t_ttc_; 
      }
    }
    when ( "PNL_VOL_SQRT_BY_TTC_SQRT_BY_DD_SQRT" )
    {
      $score_ = ( SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / sqrt ( $t_ttc_ );
    }
    when ( "PNL_SHARPE" )
    {
      $score_ = $t_pnl_sharpe_;
    }
    when ( "PNL_SHARPE_SQRT" )
    {
      $score_ = SqrtSign ( $t_pnl_sharpe_ );
    }
    when ( "PNL_SHARPE_VOL" )
    {
      $score_ = $t_pnl_sharpe_ * $t_vol_;
    }
    when ( "PNL_SHARPE_VOL_SQRT" )
    {
      $score_ = SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ );
    }
    when ( "PNL_SHARPE_DD" )
    {
      $score_ = $t_pnl_sharpe_ * abs ( $t_dd_ );
    }
    when ( "PNL_SHARPE_SQRT_DD" )
    {
      $score_ = $t_pnl_sharpe_ * sqrt ( abs ( $t_dd_ ) );
    }
    when ( "PNL_SHARPE_VOL_SQRT_BY_DD" )
    {
      $score_ = $t_pnl_sharpe_ * sqrt ( $t_vol_ ) * abs ( $t_dd_ ) ;
    }
    when ( "PNL_SHARPE_VOL_SQRT_BY_DD_SQRT" )
    {
      $score_ = SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ; # sign(pnl_sharpe)*sqrt(abs(pnl_sharpe)) * sqrt(abs(volume)) * sqrt(max(0,pnl_sharpe_dd_ratio))
    }
    when ( "PNL_SHARPE_VOL_SQRT_BY_TTC_BY_DD_SQRT" )
    {
      $score_ = ( SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / $t_ttc_;
    }
    when ( "PNL_SHARPE_VOL_SQRT_BY_TTC_SQRT_BY_DD_SQRT" )
    {
      $score_ = ( SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / sqrt ( $t_ttc_ );
    }
    when ( /^kCNA/ )
    {
      $score_ = $kcna_score_;
    }
    default
    {
      SendErrorMailifAutomatedAndDie ( "SORT_ALGO=".$t_sort_algo_." NOT AVAILABLE" );
    }
  }

  if ( $normalize_score_by_message_count_ && $t_msg_count_)
  {
    $score_ = $score_ / $t_msg_count_ ;
  }
  if ( $normalize_score_by_sqrt_message_count_ && $t_msg_count_)
  {
    $score_ = $score_ / sqrt ($t_msg_count_) ;
  }

  return $score_ ;
}

sub SendErrorMail
{
  my $error_string_ = "@_";
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $email_address_\n";
  print MAIL "From: $email_address_\n";
  print MAIL "Subject: Error- pick_strats_and_install ( $config_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
  print MAIL $error_string_ ;
  close(MAIL);

  print STDERR "\n", colored($error_string_, 'red'), "\n";
}

sub SendErrorMailAndDie
{
  my $error_string_ = "@_";
  if ( $install_picked_strats_ ) {
    open ( MAIL , "|/usr/sbin/sendmail -t" );
    print MAIL "To: $email_address_\n";
    print MAIL "From: $email_address_\n";
    print MAIL "Subject: Error- pick_strats_and_install ( $config_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
    print MAIL $error_string_ ;
    close(MAIL);
  }

  PrintErrorAndDie ( $error_string_ );
}

sub SendErrorMailifAutomatedAndDie
{
  my $error_string_ = "@_";
  if ( $automated_picking_ && $install_picked_strats_ ) {
    open ( MAIL , "|/usr/sbin/sendmail -t" );
    print MAIL "To: $email_address_\n";
    print MAIL "From: $email_address_\n";
    print MAIL "Subject: Error- pick_strats_and_install ( $config_file_ ) $pickstrats_date_ $hhmmss_ \n\n";
    print MAIL $error_string_ ;
    close(MAIL);
  }

  PrintErrorAndDie ( $error_string_ );
}

sub PrintErrorAndDie
{
  my $error_string_ = "@_";

  print STDERR "\n", colored("ERROR: $error_string_", 'bold red'), "\n";

  die;
}

#avoid multiple simultaneous runs for same query id
sub CreateRunningLocks
{
  my @t_lock_keys_ = ( );
#Iterate over each query id
  for ( my $strat_counter = 0 ; $strat_counter < $sum_num_strats_to_install_ ; $strat_counter ++ )
  {
    my $temp_query_id_ = $prod_query_start_id_ + $strat_counter;
    push ( @t_lock_keys_, $temp_query_id_ );
  }
  my $production_file_ = basename ($local_production_strat_list_file_);
  push ( @t_lock_keys_, $production_file_ );

  if ( $install_picked_strats_ ) {
    my @done_keys_ = ( );
    my @running_keys_ = ( );

    my $pickstrat_lock_file_ = "/home/dvctrader/.pickstrats_locks/running_locks_module";

    if ( TakeLock ( $pickstrat_lock_file_ ) ) {

      foreach my $temp_query_id_ ( @t_lock_keys_ )
      {
        `$lock_manager_script $temp_query_id_ START 2>/dev/null`;
        my $lock_error_code_ = $?>>8;

        if ( $lock_error_code_ ne "0" ) { #Lock creation failed
          if ( $lock_error_code_ eq "2" ) {
            push ( @done_keys_, $temp_query_id_ );
          }
          else {
            push ( @running_keys_, $temp_query_id_ );
          }
        }
        else {
          push ( @lock_keys_, $temp_query_id_ );
        }
      }

      if ( $#done_keys_ >= 0 ) {
        print "\nLooks like duplicate install for ids ".join(",",@done_keys_)." is Done.\n";
        my $user_input = AskUserPrompt( "Are you sure you want to go ahead? Press <Enter> to continue, Ctrl+C to exit installation: ", 2 );
        if ( ! defined $user_input ) {
          print "No User Input.. Continuing with the installation..\n";
        }
      }

      if ( $#running_keys_ >= 0 ) {
        print "\nWARNING! Simultaneous installation for ids ".join(",",@running_keys_)." is Running.\n";
        my $user_input;
        if ( $exit_on_simultaneous_run_ ) {
          $user_input = "N";
        }
        else {
          $user_input = AskUserPrompt( "Are you sure you want to go ahead? Press Y to continue, N to exit installation: ", 30);
          if ( defined $user_input ) { chomp($user_input); }
        }

        if ( ! defined $user_input || $user_input ne "Y" ) {
          PrintErrorAndDie ( "Aborting Installation.." );
        }

        print "\nThis might result in Simultaneous installations for the same query ids..\n";
        $user_input = AskUserPrompt( "Please confirm if you want to go ahead? Press Y to continue, N to exit installation: ", 30 );
        if ( defined $user_input ) { chomp($user_input); }

        if ( ! defined $user_input || $user_input ne "Y" ) {
          PrintErrorAndDie ( "Aborting Installation.." );
        }

        print "\nContinuing the Installation! I hope you know what you just did!\n\n";
      }

      foreach my $temp_query_id_ ( @done_keys_, @running_keys_ ) {
        `$lock_manager_script $temp_query_id_ START_FORCE 2>/dev/null`;
        my $lock_error_code_ = $?>>8;

        if ( $lock_error_code_ ne "0" ) { #Lock creation failed
#Could not create lock dir even after removing existing one
          PrintErrorAndDie ( "Cannot create running lock file for query id ".$temp_query_id_." even after removing the existing one. Probably permission issue. Aborting install!\n");
        }
        else {
          push ( @lock_keys_, $temp_query_id_ );
        }
      }

      RemoveLock ( $pickstrat_lock_file_ );
    }
  }
}

#remove running locks for given query ids
sub RemoveRunningLocks
{
  if ( $install_picked_strats_ ) {
    #Remove running lock before exiting
    foreach my $strat_counter ( 0..$#lock_keys_ )
    {
      my $temp_query_id_ = $lock_keys_[ $strat_counter ];
      `$lock_manager_script $temp_query_id_ STOP 2>/dev/null`;
    }
  }
}

sub AskUserPrompt {
  my ($question_, $timeout_seconds_, $default_) = @_;

  my $answer = $default_;
  print colored($question_." (enter before $timeout_seconds_ seconds): ", 'blue');
  eval {
    local $SIG{ALRM} = sub { die "timeout reading from keyboard\n" };
    alarm $timeout_seconds_;
    $answer = <STDIN>;
    alarm 0;
    chomp $answer;
  };
  if ($@) {
    if ( $@ eq "timeout reading from keyboard\n" ) {
      print "\n", colored("timeout reading from keyboard", 'red'), "\n";
    } else {
      die colored($@, 'red');
    }
  }
  return $answer;
}

#To avoid reusing same query id again on same day
sub CreateDoneLocks
{
  if ( $install_picked_strats_ ) {
    #Installed successfully => Mark this query_start_id as done for today (to avoid multiple installs for same id)
    foreach my $strat_counter ( 0..$#lock_keys_ )
    {
      my $temp_query_id_ = $lock_keys_[ $strat_counter ];
      `$lock_manager_script $temp_query_id_ COMPLETE 2>/dev/null`;
      if ( $? ne "0" ) {
        #Operation failed
        SendErrorMail( "Could not create done lock for ".$prod_query_start_id_." ; exit code: ".$? );
      }
    }
  }
}

sub signal_handler
{
  RemoveIntermediateFiles ( );
  RemoveRunningLocks ( );
  if ( $is_remote_lock_created_ && $install_location_ ne "" )
  {
    `ssh $install_location_ "rm -f $remote_lockfile_"`;
    print "removed lock  $install_location_:$remote_lockfile_\n";
  }
  die "Caught a signal $!\n";
}

sub END
{
  RemoveIntermediateFiles ( );
  RemoveRunningLocks ( );
  if ( $is_remote_lock_created_ && $install_location_ ne "" )
  {
    `ssh $install_location_ "rm -f $remote_lockfile_"`;
    print "removed lock  $install_location_:$remote_lockfile_\n";
  }
}
