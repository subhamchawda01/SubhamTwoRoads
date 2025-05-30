#!/usr/bin/perl

# \file ModelScripts/pick_option_strats_and_install.pl
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
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);

# Given Risk Number: 2000
# look into list of potential underlyings and corresponding strats
        # figure out max_loss per contract
        # figure out max loss per underlying
        # sum(max(x, eod_pnl)), find x that maximizes sum. We can restrict x with upper_cap of k*RiskNumber
        # then zip files and push as 2897/2898/2899 strategy_id_

sub LoadListOfUnderlyings;
sub LoadConfig;
sub SanityCheckConfigParams;
sub ComputeOptimalMaxLoss;
sub CreateRunningLocks;
sub RemoveRunningLocks;
sub CreateDoneLocks;
sub AskUserPrompt;
sub CreateSimStatisicMaps;
sub ShortlistStrategiesToRun;
sub GroupStrategies;
sub InstallStrategies;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BASETRADE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $GLOBALRESULTSDBDIR = "DB";
my $MODELLING_BASE_DIR="/home/dvctrader/modelling";
my $MODELLING_STRATS_DIR=$MODELLING_BASE_DIR."/NSEOptionsMM"; # this directory is used to store the chosen strategy files
my $NSE_FILES_DIR="/spare/local/tradeinfo/NSE_Files/";
my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list/";
my $REMOTE_PRODUCTION_STRAT_LIST_DIR = "/home/dvctrader/Options";
my $local_production_strat_list_file_ = "";
my $remote_production_strat_list_file_ = ""; 

my $PICKSTRATS_DIR = "/spare/local/pickstrats_logs";
my $PICKSTRAT_TEMP_DIR = $PICKSTRATS_DIR."/temp_dir";

my $lock_manager_script = $HOME_DIR."/".$REPO."/scripts/pick_strats_lock.sh";
#my $lock_manager_script = $HOME_DIR."/hrishav/pick_strats_lock.sh";

my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
my @intermediate_files_ = ( );

require "$GENPERLLIB_DIR/sqrt_sign.pl"; # SqrtSign
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
require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; # GetBasepxStratFirstModel
require "$GENPERLLIB_DIR/strat_utils.pl"; # GetRegimeInd,IsStagedStrat
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetStratsWithGlobalResultsForShortcodeDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # FetchPnlSamplesStrats 
require "$GENPERLLIB_DIR/read_shc_machine_mapping.pl"; # GetMachineForProduct
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # GetPickstratConfigId InsertPickstratConfigId InsertPickstratRecord
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; # FetchCorrelationForPair
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl"; # GetPnlSamplesCorrelation
require "$GENPERLLIB_DIR/install_strategy_production.pl";
require "$GENPERLLIB_DIR/copy_strategy.pl";
require "$GENPERLLIB_DIR/lock_utils.pl";
require "$GENPERLLIB_DIR/option_strat_utils.pl";

# start
my $add_to_crontab_ = 1;
my $exit_on_simultaneous_run_ = 0; # if 0: ask for user prompt, if 1: directly exit

my $USAGE="$0 SEGMENT TIMEPERIOD CONFIGFILE ";

my $num_strats_to_install_ = 0;
my $prod_query_start_id_ = 0;
my $prod_query_stop_id_ = 0;
my $risk_tags_;  # Tags for risk monitor computations

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $segment_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $config_file_ = $ARGV [ 2 ];

my $exchange_ = "NSE";

my $lookback_days_ = 40;

my $pickstrats_date_ = `date +%Y%m%d`; chomp ( $pickstrats_date_ );
my $curr_hh_ = `date +%H%M`; chomp ( $curr_hh_ );
if ( $curr_hh_ >= 1800 )
{
  $pickstrats_date_ = CalcNextDate ( $pickstrats_date_ );
}
my $curr_date_ = $pickstrats_date_;
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $min_max_loss_per_contract_ = -1;
my $max_max_loss_per_contract_ = -1;
my $min_max_loss_per_underlying_ = -1;
my $max_max_loss_per_underlying_ = -1;
my $global_risk_number_ = -1;

my $OML_lookback_days_ = 0 ;
my $OML_hit_ratio_ = 0 ;
my $OML_number_top_loss_ = 0 ;
my $global_OML_lookback_days_ = 100 ;
my $global_OML_hit_ratio_ = 0.02 ;
my $global_OML_number_top_loss_ = 4 ;
my $underlying_max_loss_bias_ = 1;
my $max_loss_pnl_cutoff_ = 0;

my @strats_to_keep_ = ( );
my @strats_to_exclude_ = ( );
my @shc_to_keep_ = ( );
my @shc_to_remove_ = ( );
my @banned_shc_ = ( );
my @earnings_shc_ = ( );
my $max_message_count_cutoff_ = 0 ;
my $min_pnl_by_maxloss_cutoff_global__ = 0.15 ;
my @strats_in_pool_ = ( );
my @sort_algo_ = ( );
my $email_address_ = "";

my $install_picked_strats_ = 0;
my $install_location_ = "";
my $exec_start_hhmm_ = 0;
my $exec_end_hhmm_ = 0;
my $onload_trade_exec_ = 0;
my $vma_trade_exec_ = 0;
my $affinity_trade_exec_ = 0;

my %skip_dates_map_ = ( );
my $last_trading_date_ = "";

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
`mkdir -p $PICKSTRAT_TEMP_DIR`;
my $skip_dates_file_ = $PICKSTRAT_TEMP_DIR."/skip_dates_file_".$segment_."_".$timeperiod_."_".$unique_gsm_id_ ;
my $strats_list_file_ = $PICKSTRAT_TEMP_DIR."/strat_list_".$segment_."_".$timeperiod_."_".$unique_gsm_id_ ;
my $combined_strats_list_file_ = $PICKSTRAT_TEMP_DIR."/combined_strat_list_".$segment_."_".$timeperiod_."_".$unique_gsm_id_ ;

my $remote_lockfile_ = "/home/dvctrader/pick_strat_lock"; 
my $is_remote_lock_created_ = 0;
my @lock_keys_ = ( );

my @picked_strats_ = ( );
my %passed_strats_ = ( );
my %interval_to_global_results_ = ( );
my %strat_name_to_global_result_score_ = ( );
my @algo_sorted_strat_names_ = ( );
my @algo_sorted_strat_results_ = ( );
my %strat_name_to_optimal_max_loss_ = ( );
my %strat_to_sim_results_ = ( );
my %strat_to_maxlosses_ = ( );
my %strat_to_score_ = ( );
my @combined_strats_list_to_run_ = ();

my $end_date_ = CalcPrevDate ( $pickstrats_date_ );
my $start_date_ = $end_date_;

my %strat_name_shc_to_param_ = ();
my %strat_name_to_model_ = ();
my %query_id_to_remote_strat_path_ = ();

my @files_to_sync_ = ();

LoadConfigFile($config_file_);

SanityCheckConfigParams();

CreateRunningLocks();

CreateSimStatisicMaps();

ShortlistStrategiesToRun();

InstallStrats();

CreateDoneLocks();

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
        when ( "SEGMENT" )
        {
          my $t_segment_ = $t_words_ [ 0 ];
          if ( $t_segment_ ne $segment_ )
          {
            PrintErrorAndDie ( "$t_segment_ in config file != $segment_" );
          }
          $mail_body_ = $mail_body_." \t > SEGMENT=".$t_segment_."\n";
        }

        when ( "EXCHANGE" )
        {
          $exchange_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > EXCHANGE=".$exchange_."\n";
        }

        when ( "NUM_STRATS_TO_INSTALL" )
        {
          my $t_num_strats_to_install_ = $t_words_ [ 0 ];
          $num_strats_to_install_ = $t_num_strats_to_install_ ;
          $mail_body_ = $mail_body_." \t > NUM_STRATS_TO_INSTALL=".$t_num_strats_to_install_."\n";
        }

        when ( "DATE" )
        {
          my $t_pickstrats_date_ = $t_words_ [ 0 ];
          if ( $t_pickstrats_date_ ne "TODAY" ) {
            $pickstrats_date_ = GetIsoDateFromStrMin1 ( $t_pickstrats_date_ );
          }
        }

        when ( "SHC_TO_KEEP" )
        {
          my $t_shc_to_keep_ = $t_words_ [ 0 ];
          push ( @shc_to_keep_ , $t_shc_to_keep_ );
          $mail_body_ = $mail_body_." \t > SHC_TO_KEEP=".$t_shc_to_keep_."\n";
        }

        when ( "SHC_TO_KEEP_FILE" )
        {
	    my $shc_to_keep_file_ = $t_words_[0];
	    open FH, "< $shc_to_keep_file_ " or PrintStacktraceAndDie ( "Could not open $shc_to_keep_file_\n" );
	    while(my $thisline_ = <FH>)
	    {
		if ( $thisline_ =~ '/^#/' ) {
		    next;
		}
		my @this_words_ = split(' ', $thisline_);
		my $t_shc_to_keep_ = $this_words_[ 0 ];
		push ( @shc_to_keep_ , $t_shc_to_keep_ );
		$mail_body_ = $mail_body_." \t > SHC_TO_KEEP=".$t_shc_to_keep_."\n";
	    }
	    close FH;
	}
	    
        when ( "SHC_TO_REMOVE" )
        {
          my $t_shc_to_remove_ = $t_words_ [ 0 ];
          push ( @shc_to_remove_ , $t_shc_to_remove_ );
          $mail_body_ = $mail_body_." \t > SHC_TO_REMOVE=".$t_shc_to_remove_."\n";
        }

        when ( "SHC_TO_REMOVE_FILE" )
        {
	    my $shc_to_remove_file_ = $t_words_[0];
	    open FH, "< $shc_to_remove_file_ " or PrintStacktraceAndDie ( "Could not open $shc_to_remove_file_\n" );
	    while(my $thisline_ = <FH>)
	    {
		if ( $thisline_ =~ '/^#/' ) {
		    next;
		}
		my @this_words_ = split(' ', $thisline_);
		my $t_shc_to_remove_ = $this_words_[ 0 ];
		push ( @shc_to_remove_ , $t_shc_to_remove_ );
		$mail_body_ = $mail_body_." \t > SHC_TO_REMOVE=".$t_shc_to_remove_."\n";
	    }
	    close FH;
        }

        when ( "MIN_MAX_LOSS_PER_CONTRACT" )
        {
          $min_max_loss_per_contract_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MIN_MAX_LOSS_PER_CONTRACT=".$min_max_loss_per_contract_."\n";
        }

        when ( "MAX_MAX_LOSS_PER_CONTRACT" )
        {
          $max_max_loss_per_contract_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_MAX_LOSS_PER_CONTRACT=".$max_max_loss_per_contract_."\n";
        }

        when ( "MIN_MAX_LOSS_PER_UNDERLYING" )
        {
          $min_max_loss_per_underlying_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MIN_MAX_LOSS_PER_UNDERLYING=".$min_max_loss_per_underlying_."\n";
        }

        when ( "MAX_MAX_LOSS_PER_UNDERLYING" )
        {
          $max_max_loss_per_underlying_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_MAX_LOSS_PER_UNDERLYING=".$max_max_loss_per_contract_."\n";
        }

        when ( "GLOBAL_RISK_NUMBER" )
        {
          $global_risk_number_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > GLOBAL_RISK_NUMBER=".$global_risk_number_."\n";
        }

        when ( "OPTIMAL_MAX_LOSS_SETTINGS" )
        { 
          $OML_lookback_days_ = max ( 10, $t_words_ [ 0 ] ) ;
          $OML_hit_ratio_ = min ( 0.2, $t_words_ [ 1 ] ) ; #greater than 20% is not sane
          $OML_number_top_loss_ = 10; #Since it odes not matter here
        }

        when ( "GLOBAL_OPTIMAL_MAX_LOSS_SETTINGS" )
        { 
          $global_OML_lookback_days_ = max ( 10, $t_words_ [ 0 ] ) ;
          $global_OML_hit_ratio_ = min ( 0.2, $t_words_ [ 1 ] ) ; #greater than 20% is not sane
          $global_OML_number_top_loss_ = 4; #Since it odes not matter here
        }

	when ( "UNDERLYING_MAX_LOSS_BIAS" )	    
	{
	    $underlying_max_loss_bias_ = max(1, $t_words_[0]);
	}

        when ( "STRATS_TO_KEEP" )
        {
          if ( ! FindItemFromVec ( $t_words_ [ 0 ] , @strats_to_keep_ ) )
          {
            push ( @strats_to_keep_ , $t_words_ [ 0 ] );
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
        
        when ( "SORT_ALGO" )
        {
          my $t_sort_algo_ = $t_words_ [ 0 ];
          push ( @sort_algo_ , $t_sort_algo_ );
          $mail_body_ = $mail_body_." \t > SORT_ALGO=".$t_sort_algo_."\n";
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

        when ( "RISK_TAGS" )
        {
          $risk_tags_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > RISK_TAGS=".$risk_tags_."\n";
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

        when ( "MAX_MSG_COUNT_CUTOFF" )
        {
          $max_message_count_cutoff_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MAX_MSG_COUNT_CUTOFF=".$max_message_count_cutoff_."\n";
        }

        when ( "MIN_PNL_BY_MAXLOSS_CUTOFF_GLOBAL" )
        {
          $min_pnl_by_maxloss_cutoff_global__ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > MIN_PNL_BY_MAXLOSS_CUTOFF_GLOBAL=".$min_pnl_by_maxloss_cutoff_global__."\n";
        }

        when ( "LOOKBACK_DAYS" )
        {
          $lookback_days_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > LOOKBACK_DAYS=".$lookback_days_."\n";
          $start_date_ = CalcPrevWorkingDateMult ( $pickstrats_date_ , $lookback_days_ );
        }

      }      
    }
  }

  if ( ! -d $LOCAL_PRODUCTION_STRAT_LIST_DIR )
  {
    `mkdir -p $LOCAL_PRODUCTION_STRAT_LIST_DIR`;
  }

  my $banned_shc_dated_file_ = $NSE_FILES_DIR."/SecuritiesUnderBan/fo_secban_".$pickstrats_date_.".csv";
  my $earnings_shc_dated_file_ = $NSE_FILES_DIR."/Earnings/earnings_".$pickstrats_date_;

  if (-e $banned_shc_dated_file_)
  {
    open BANHANDLE, "< $banned_shc_dated_file_" or PrintStacktraceAndDie ( "$0 Could not open $banned_shc_dated_file_\n" );
    @banned_shc_ = <BANHANDLE>; chomp ( @banned_shc_ );
    $mail_body_ = $mail_body_." \t > BANNED SHORTCODES=".join(',', @banned_shc_)."\n";
    close BANHANDLE; 
  }

  if (-e $earnings_shc_dated_file_)
  {
    open ERNHANDLE, "< $earnings_shc_dated_file_" or PrintStacktraceAndDie ( "$0 Could not open $earnings_shc_dated_file_\n" );
    @earnings_shc_ = <ERNHANDLE>; chomp ( @earnings_shc_ );
    $mail_body_ = $mail_body_." \t > EARNINGS SHORTCODES=".join(',', @earnings_shc_)."\n";
    close ERNHANDLE; 
  }

  push(@shc_to_remove_,@banned_shc_);
  push(@shc_to_remove_,@earnings_shc_);

  if ( $install_location_ eq "" ) {
    my $base_shc_ = "NSE_NIFTY_FUT0";
    if ($segment_ eq "CUR") { $base_shc_ = "NSE_USDINR_FUT0"; }
    $install_location_ = GetMachineForProduct ( "NSE_NIFTY_FUT0" );
    if ( ! defined $install_location_ ) {
      PrintErrorAndDie ( "INSTALL_LOCATION could NOT be fetched for NSE_NIFTY_FUT0" );
    }
    $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_location_."\n";
  }

  $mail_body_ = $mail_body_." \t > DATE=".$pickstrats_date_."\n";

  my @strat_paths_in_pool_ = ();
  
  if ($#strats_to_keep_ < 0)
  {  
    my @strats_in_dir_ = MakeStratVecFromDir($MODELLING_STRATS_DIR."/".$segment_."Strats/");
    push ( @strat_paths_in_pool_, @strats_in_dir_ );
  }
  else 
  {
    foreach my $t_strat_name_ (@strats_to_keep_)
    {
      my $full_path_ =  GetFullStratPathFromBaseOptions($t_strat_name_);
      push(@strat_paths_in_pool_,$full_path_);  
    }
  }

  @strat_paths_in_pool_ = GetUniqueList ( @strat_paths_in_pool_ ); 
  foreach my $strat_path_ ( @strat_paths_in_pool_ ) {
    my $strat_ = `basename $strat_path_`; chomp ( $strat_ );
    push ( @strats_in_pool_, $strat_ );
  }

  open STRAT_LIST_HANDLE, "> $strats_list_file_" or SendErrorMailAndDie ( "cannot open $strats_list_file_ for writing" );
  push ( @intermediate_files_, $strats_list_file_ );
  print STRAT_LIST_HANDLE join("\n", @strats_in_pool_)."\n";
  close (STRAT_LIST_HANDLE);

  my @combined_skip_dates_ = keys %skip_dates_map_;
  my $file_handle_ = FileHandle->new;
  $file_handle_->open ( "> $skip_dates_file_ " ) or SendErrorMailifAutomatedAndDie ( "Could not open $skip_dates_file_ for writing\n" );
  push ( @intermediate_files_, $skip_dates_file_ );
  print $file_handle_ join ( "\n", @combined_skip_dates_ );
  $file_handle_->close;


  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}

# Create SIM statistics for all underlying and choose which products to run together based on the risk number
# Need to figure out a way to look to find out best combination 
# Optimal max loss is used for underlying only (Do we need per contract basis ?) 

sub CreateSimStatisicMaps
{

  # Banned shc_to_remove from shortcode list 
  my $shc_to_remove_file_ = "/spare/local/tradeinfo/NSE_Files/SecuritiesUnderBan/fo_secban_".$pickstrats_date_.".csv";
  open FH, "< $shc_to_remove_file_ " or PrintStacktraceAndDie ( "Could not open $shc_to_remove_file_\n" );
  while(my $thisline_ = <FH>)
  {
      if ( $thisline_ =~ '/^#/' ) {
	  next;
      }
      my @this_words_ = split(' ', $thisline_);
      my $t_shc_to_remove_ = $this_words_[ 0 ];
      push ( @shc_to_remove_ , $t_shc_to_remove_ );
      $mail_body_ = $mail_body_." \t > SHC_TO_REMOVE=".$t_shc_to_remove_."\n";
  }
  close FH;
  #

  my %shortcodes;
  @shortcodes{ @shc_to_keep_ } = ();            # All shc are the keys.
  print "all shcs: ";
  print join(' ', @shc_to_keep_ );
  print "\n";
  delete @shortcodes{ @shc_to_remove_ }; # Remove the shc_to_remove_ (Here we can also include banned stocks)
  print "shortcodes removed: ";
  print join(' ', @shc_to_remove_);
  print "\n";
  @shc_to_keep_ = keys %shortcodes;
  print "final shortcodes: ";
  print join(' ', @shc_to_keep_);
  print "\n";


  my $SUMMARIZE_EXEC = $MODELSCRIPTS_DIR."/summarize_strats_for_options.pl";
  foreach my $shc_ (@shc_to_keep_) 
  {
    my $exec_cmd_ = $SUMMARIZE_EXEC." $shc_ $strats_list_file_ $GLOBALRESULTSDBDIR $start_date_ $end_date_ $skip_dates_file_ ".$sort_algo_[0]." 0 IF 1 | head -n2";
    my @ssr_results_ = `$exec_cmd_`;

    foreach my $ssr_result_line_ ( @ssr_results_ )
    {
      my @ssr_result_words_ = split ( /\s+/ , $ssr_result_line_ );
      if($#ssr_result_words_ < 0) { next } ;
      my $t_strat_name_ = $ssr_result_words_ [ 1 ];
      $strat_to_score_ {$t_strat_name_} = $ssr_result_words_ [ 17 ];
      my $string_to_remove_  = "_".$shc_."\$";
      $t_strat_name_ =~ s/$string_to_remove_//g;
      ComputeOptimalMaxLoss($t_strat_name_, $shc_);
      splice @ssr_result_words_, 0, 2;
      $strat_to_sim_results_ { $t_strat_name_ } { $shc_ } = join(' ', @ssr_result_words_);
    }
  }
}


sub ShortlistStrategiesToRun 
{
  my $avg_pnl_ = 0;
  my $max_loss_ = 0;
  my $score = 0;
  my %shc_to_included_ = ();
  my $current_pnl_ = 0;
  my $current_max_loss_ = 0;
  my $num_products_ = 0;

  my $temp_strats_list_file_ = $PICKSTRAT_TEMP_DIR."/temp_strat_list_".$segment_."_".$timeperiod_."_".$unique_gsm_id_ ;
  push ( @intermediate_files_, $temp_strats_list_file_ );
  foreach my $strat_name_ (sort { $strat_to_score_{$b} <=> $strat_to_score_{$a} or $a cmp $b } keys %strat_to_score_) {

    my $t_strat_name_ = $strat_name_;
    my($shc_) = $strat_name_ =~ /.*_(.*)/;
    my $string_to_remove_  = "_".$shc_."\$";
    $t_strat_name_ =~ s/$string_to_remove_//g;
     
    if(exists($shc_to_included_{$shc_}))
    {
      if (! $install_picked_strats_ )      
      {
        print "Ignoring ".$shc_." for strat : ".$t_strat_name_." since shortcode already selected.\n";
      }
      next;
    }

    my $combined_strat_string_to_add_ = $strat_name_." 1 2000 ".$shc_."\n";
    open STRAT_LIST_HANDLE, "> $temp_strats_list_file_" or SendErrorMailAndDie ( "cannot open $temp_strats_list_file_ for writing" );  
    print STRAT_LIST_HANDLE join("\n", @combined_strats_list_to_run_)."\n";
    print STRAT_LIST_HANDLE $combined_strat_string_to_add_;
    close (STRAT_LIST_HANDLE); 
    # Here if we cross global risk number then discard the startegy
    my ($t_avg_pnl_,$t_max_loss_) = ComputeCombinedMaxLoss($temp_strats_list_file_);

    $t_avg_pnl_ *= ($num_products_ + 1);   
    $t_max_loss_ *= ($num_products_ + 1);   
 
    if( ($max_loss_ > 0.75*$global_risk_number_) || (($t_avg_pnl_/$t_max_loss_ < $min_pnl_by_maxloss_cutoff_global__)))
    {
      if (! $install_picked_strats_ )      
      {
        print "Ignoring ".$shc_." for strat: ".$t_strat_name_." Pnl Loss Score : ".$t_avg_pnl_." ".$t_max_loss_." ".$t_avg_pnl_/$t_max_loss_." Current Pnl Loss Score : ".$current_pnl_." ".$current_max_loss_." ".$score."\n";
      }
      next;
    }   
   
    if( (($t_avg_pnl_/$t_max_loss_ > $score) && ($t_avg_pnl_ > 0.9*$current_pnl_)) || ( ($t_avg_pnl_/$t_max_loss_ > 0.75*$score) && ($t_avg_pnl_ > $current_pnl_) ) )  # Bit conservative estimates
    {
      print "Selecting ".$shc_." for strat: ".$t_strat_name_." Pnl Loss Score : ".$t_avg_pnl_." ".$t_max_loss_." ".$t_avg_pnl_/$t_max_loss_." Prev Pnl Loss Score : ".$current_pnl_." ".$current_max_loss_." ".$score."\n";
      $num_products_ += 1; 
      $current_pnl_ = $t_avg_pnl_;
      $current_max_loss_ = $t_max_loss_;
      $score = $t_avg_pnl_/$t_max_loss_;
      $shc_to_included_{$shc_} = 1;
      push(@combined_strats_list_to_run_,$combined_strat_string_to_add_);
      my $full_strat_path_name_ = GetFullStratPathFromBaseOptions($t_strat_name_);
      my $model_file_ = GetModelFileName($full_strat_path_name_);
      my $param_file_ = GetParamFileName($full_strat_path_name_,"NSE_".$shc_."_FUT0");

      if(! exists $strat_name_to_model_{$t_strat_name_})
      {
        $strat_name_to_model_{$t_strat_name_} = $model_file_;
      }
      
      if (! exists $strat_name_shc_to_param_{$t_strat_name_}{$shc_} )
      {
        $strat_name_shc_to_param_{$t_strat_name_}{$shc_} = $param_file_;
      }
    }
    else
    {
      print "Ignoring ".$shc_." for strat: ".$t_strat_name_." Pnl Loss Score : ".$t_avg_pnl_." ".$t_max_loss_." ".$t_avg_pnl_/$t_max_loss_." Current Pnl Loss Score : ".$current_pnl_." ".$current_max_loss_." ".$score."\n";
    }
  }
  open STRAT_LIST_HANDLE, "> $combined_strats_list_file_" or SendErrorMailAndDie ( "cannot open $combined_strats_list_file_ for writing" );
  push ( @intermediate_files_, $combined_strats_list_file_ );
  print STRAT_LIST_HANDLE join("", @combined_strats_list_to_run_)."\n";
  close (STRAT_LIST_HANDLE);
  #print $num_products_."\n";
}

# for each model we will make a separate strategy
sub CombineStrategies
{
  my $current_id_ = $prod_query_start_id_;
  foreach my $strat_name_ (keys %strat_name_shc_to_param_) 
  {
    foreach my $shc_ (keys %{$strat_name_shc_to_param_{$strat_name_}})
    {
      my $t_param_file_ = $strat_name_shc_to_param_{$strat_name_}{$shc_};
      my $base_param_name_ = `basename $t_param_file_` ; chomp($base_param_name_);
      $base_param_name_ = $base_param_name_."_".$shc_."_".$current_id_;
      `cp $t_param_file_ $LOCAL_PRODUCTION_STRAT_LIST_DIR/$base_param_name_`;
      push(@files_to_sync_, $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$base_param_name_);
      $strat_name_shc_to_param_{$strat_name_}{$shc_} = $base_param_name_;
      my $max_loss_per_underlying_ = ceil($underlying_max_loss_bias_ * $strat_name_to_optimal_max_loss_{$strat_name_}{$shc_});

      SetParamValues($LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$base_param_name_,"GLOBAL_MAX_LOSS",$max_loss_per_underlying_);
    }
    my $t_model_file_ = $strat_name_to_model_{$strat_name_};
    my $base_model_name_ = `basename $t_model_file_` ; chomp($base_model_name_);
    $base_model_name_ = $base_model_name_."_".$current_id_;
    `cp $t_model_file_ $LOCAL_PRODUCTION_STRAT_LIST_DIR/$base_model_name_`;
    push(@files_to_sync_, $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$base_model_name_);
    $strat_name_to_model_{$strat_name_} = $base_model_name_;
    SetModelValues($strat_name_,$LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$base_model_name_);
    my $full_strat_path_name_ = GetFullStratPathFromBaseOptions($strat_name_);
    my $base_strat_name_ = $strat_name_."_".$current_id_;
    chomp($full_strat_path_name_);
    `cp $full_strat_path_name_ $LOCAL_PRODUCTION_STRAT_LIST_DIR/$base_strat_name_`;
    push(@files_to_sync_, $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$base_strat_name_);
    $query_id_to_remote_strat_path_{$current_id_} = $REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$base_strat_name_;
    SetStratValues($LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$base_strat_name_,$REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$base_model_name_,$current_id_);
    $current_id_ = $current_id_ + 1;
  } 
}

sub ComputeCombinedMaxLoss
{
  my ( $t_strat_file_ ) = @_;

  my $avg_pnl_ = 0;
  my $max_loss_ = 0;

  $mail_body_ = $mail_body_."Using Global OML_settings\n $global_OML_lookback_days_ $global_OML_hit_ratio_ $global_OML_number_top_loss_ \n";

  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss_combined.pl";
  my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." NSE_NIFTY_FUT0 $timeperiod_ $global_OML_lookback_days_ $global_OML_hit_ratio_ $global_OML_number_top_loss_ $t_strat_file_ $pickstrats_date_ $skip_dates_file_";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

  foreach my $max_loss_line_ ( @exec_output_ )
  {
    if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
    {
      next;
    }

    my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
    if ( $#max_loss_words_ >= 2 )
    {
      $avg_pnl_ = $max_loss_words_ [ 1 ];
      $max_loss_ = $max_loss_words_ [ 0 ];
    }
  }

  return ($avg_pnl_,$max_loss_);

}

sub ComputeOptimalMaxLoss
{
  my ( $t_strat_ , $shc_ ) = @_;
  if ( exists ( $strat_name_to_optimal_max_loss_ { $t_strat_ } { $shc_ } ) ) { return; }

  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl";
  my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shc_ $timeperiod_ $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ $t_strat_ $pickstrats_date_ $skip_dates_file_";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

  my $best_max_loss_pnl_ = $max_max_loss_per_underlying_;
  foreach my $max_loss_line_ ( @exec_output_ )
  {
    if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
    {
      next;
    }

    # We will go through all and pick the one which is not in our range and has best pnl by max loss
    my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
    if ( $#max_loss_words_ >= 2 )
    {
      my $avg_pnl_ = $max_loss_words_ [ 1 ];
      my $max_loss_ = $max_loss_words_ [ 0 ];
      $max_loss_ = max($max_loss_,$min_max_loss_per_underlying_);
      if(($max_loss_ > $max_max_loss_per_underlying_) || ($avg_pnl_ <= 0))
      {
        next;
      }
      
      my $t_max_loss_by_pnl_ = int($max_loss_/$avg_pnl_);
      if ( $t_max_loss_by_pnl_ < $best_max_loss_pnl_ )
      {
        $strat_name_to_optimal_max_loss_ { $t_strat_ } { $shc_ } = $max_loss_ ;
        $best_max_loss_pnl_ = $t_max_loss_by_pnl_;
      }
      next;
    }
  }

  if ( ! exists ( $strat_name_to_optimal_max_loss_ { $t_strat_ } { $shc_} ) )
  {
    #print "Couldn't compute OML for ".$t_strat_." . Using min_max_loss_per_underlying_ = $min_max_loss_per_underlying_\n"; 
    $strat_name_to_optimal_max_loss_ { $t_strat_ } { $shc_ } = $min_max_loss_per_underlying_;
  }
  
  #print $t_strat_." ".$shc_." ".$strat_name_to_optimal_max_loss_ { $t_strat_ } { $shc_}."\n";

}

sub SanityCheckConfigParams
{
  print "Sanity Checking ...\n";


  if (($min_max_loss_per_contract_ < 0) || ($max_max_loss_per_contract_ < 0) ||
       ($min_max_loss_per_underlying_ < 0) || ($max_max_loss_per_underlying_ < 0) ||
       ($global_risk_number_ < 0)) {
    SendErrorMailAndDie ( "MAX_LOSSES  not set \n" );
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
   elsif ( ! defined $risk_tags_ || $risk_tags_ eq "" ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but RISK_TAGS is not provided" );
   }
  }


  $local_production_strat_list_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$segment_.".".$timeperiod_.".".basename ( $config_file_ );

  #push ( @intermediate_files_ , $local_production_strat_list_file_ );

  $remote_production_strat_list_file_ = $REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$segment_.".".$timeperiod_.".".basename ( $config_file_ );

  return;
}

sub SetParamValues
{
  my $t_param_file_ = shift;
  my $field_to_change_ = shift;
  my $value_ = shift;

  my $edited_param_file_ = $t_param_file_.$unique_gsm_id_;  

  open ( EDITED_PARAM_FILE , ">" , $edited_param_file_ ) or PrintStacktraceAndDie ( "Could not open param file $edited_param_file_" );
  open ( CONFIG_FILE , "<" , $t_param_file_ ) or PrintStacktraceAndDie ( "Could not open param file $t_param_file_" );

  while ( my $line_ = <CONFIG_FILE> )
  {
    chomp ( $line_ );
    my @t_words_ = split ( ' ' , $line_ );
    
    if ( $#t_words_ < 2 )
    {
      print EDITED_PARAM_FILE $line_."\n";
      next;
    }

    if ($t_words_[1] eq $field_to_change_) # Found the field to edit
    {
      print EDITED_PARAM_FILE $t_words_[0]." ".$t_words_[1]." ".$value_."\n";
    }
    else 
    {
      print EDITED_PARAM_FILE join(' ',@t_words_)."\n";
    }
  }
 
  close(EDITED_PARAM_FILE);

  `mv $edited_param_file_ $t_param_file_`;
}


sub SetModelValues
{
  my $t_strat_name_ = shift;
  my $t_model_file_ = shift;

  my $edited_model_file_ = $t_model_file_.$unique_gsm_id_;  

  open ( EDITED_MODEL_FILE , ">" , $edited_model_file_ ) or PrintStacktraceAndDie ( "Could not open model file $edited_model_file_" );
  open ( MODEL_FILE , "<" , $t_model_file_ ) or PrintStacktraceAndDie ( "Could not open model file $t_model_file_" );

  my $current_param_ = "";
  my $found_ = 0;
  while ( my $thismline_ = <MODEL_FILE> ) 
  {
    chomp($thismline_);


    if ($thismline_ eq "INDVIDUAL_INDICATORS" || $thismline_ eq "GLOBAL_INDICATORS" || $thismline_ eq "IINDICATORS" || $thismline_ eq "GINDICATORS")
    {
      $found_ = 0; 
      print EDITED_MODEL_FILE $thismline_."\n";
      next;
    }
     
    if ( $thismline_ eq "PARAMSHCPAIR" ) {
      $found_ = 1; 
      print EDITED_MODEL_FILE $thismline_."\n";
      next;
    }  

    if ( $found_ == 1 ) {
      my @words_ = split ( ' ', $thismline_ );
      if ($#words_ > 2) 
      {
        my @uwords_ = split ( '_', $words_[0] );
        my $shc_ = $uwords_[1];
        if (exists $strat_name_shc_to_param_{$t_strat_name_}{$shc_})
        {
          print EDITED_MODEL_FILE $words_[0]." ".$REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$strat_name_shc_to_param_{$t_strat_name_}{$shc_}." ".join(' ',@words_[2..$#words_])."\n";
        }
        next;
      }
    }

    print EDITED_MODEL_FILE $thismline_."\n";
  }

  close(EDITED_MODEL_FILE);
  `mv $edited_model_file_ $t_model_file_`;
}

sub SetStratValues
{
  my $t_strat_file_ = shift;
  my $t_model_file_ = shift;
  my $id_ = shift;

  my $edited_strat_file_ = $t_strat_file_.$unique_gsm_id_;  

  open ( EDITED_STRAT_FILE , ">" , $edited_strat_file_ ) or PrintStacktraceAndDie ( "Could not open model file $edited_strat_file_" );
  open ( STRAT_FILE , "<" , $t_strat_file_ ) or PrintStacktraceAndDie ( "Could not open model file $t_strat_file_" );

  while ( my $thisline_ = <STRAT_FILE> ) 
  {
    my @words_ = split ( ' ', $thisline_ );
    next if $#words_ < 6; 
    print EDITED_STRAT_FILE $words_[0]." ".$words_[1]." ".$t_model_file_." ".$words_[3]." ".$id_." ".$words_[5]." ".$words_[6]."\n";
  }

  close(EDITED_STRAT_FILE);
  `mv $edited_strat_file_ $t_strat_file_`;
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

    CombineStrategies();

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." # InstallStrats\n\n";

    SyncStrats ( );

    UpdateProductionCrontab ( );

  }
  else
  {
    $mail_body_ = $mail_body_."\n NO INSTALLS SINCE INSTALL_PICKED_STRATS=".$install_picked_strats_." INSTALL_LOCATION='".$install_location_."' PROD_QUERY_START_ID='".$prod_query_start_id_."' EXCHANGE='".$exchange_."' EXEC_START_HHMM='".$exec_start_hhmm_."' EXEC_END_HHMM='".$exec_end_hhmm_."'\n";
  }
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}

sub SyncStrats
{
  foreach my $file_ ( @files_to_sync_ )
  {
    # scp file over to newly created directory
    my $scp_cmd_ = ("scp -q ".$file_." dvctrader@".$install_location_.":".$REMOTE_PRODUCTION_STRAT_LIST_DIR);
    system ($scp_cmd_) == 0 or PrintStacktraceAndDie ( "system $scp_cmd_ failed : $?\n" );
  }
}

sub UpdateProductionCrontab
{
  ClearProductionStratList ( );
  $mail_body_ = $mail_body_."\n\n\t INSTALLED :\n";

  foreach my $query_id_ ( keys %query_id_to_remote_strat_path_ ) 
  {
    print "Adding to Crontab: $query_id_\n";
    AddToProductionStratList ( $query_id_to_remote_strat_path_{ $query_id_ }, $query_id_ );
  }
  BackupExistingProductionInstall ( );
  FixProductionCrontab ( );
  InstallProductionCrontab ( );
}

sub AddToProductionStratList
{
  my ( $remote_full_path_strat_name_, $t_dest_id_ ) = @_;

  my $ONLOAD_START_REAL_TRADING = "/home/dvctrader/LiveExec/ModelScripts/onload_start_options_real_trading.sh";

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

  print LOCAL_PRODUCTION_FILE substr ( $exec_start_hhmm_ , 2 , 2 )." ".substr ( $exec_start_hhmm_ , 0 , 2 )." * * ".$weekdays_st_range_." ".$ONLOAD_START_REAL_TRADING." ".$exchange_." ".$t_dest_id_." ".$remote_full_path_strat_name_." ".$onload_switch_token_." ".$affinity_switch_token_." ".$vma_switch_token_." ".$risk_tags_." >> ".$QUERYOUTPUT_FILE." 2>&1 ".$time_period_comment_tag_."\n";
  print LOCAL_PRODUCTION_FILE substr ( $exec_end_hhmm_ , 2 , 2 )." ".substr ( $exec_end_hhmm_ , 0 , 2 )." * * ".$weekdays_et_range_." ".$STOP_REAL_TRADING." ".$exchange_." ".$t_dest_id_." 1>/dev/null 2>/dev/null $time_period_comment_tag_\n";

  close ( LOCAL_PRODUCTION_FILE );

  return;
}


sub ClearProductionStratList
{
  open ( LOCAL_PRODUCTION_FILE , ">" , $local_production_strat_list_file_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );
  print LOCAL_PRODUCTION_FILE "##################################################################################################\n";
# Print a tiny header into the crontab , maybe it will help
# understand this install at trading time.
  print LOCAL_PRODUCTION_FILE "### [ Options Strategy $exchange_ ]";
  print LOCAL_PRODUCTION_FILE "[ $email_address_ $pickstrats_date_ : $hhmmss_ ] ##\n";
  print LOCAL_PRODUCTION_FILE "##################################################################################################\n";
  close ( LOCAL_PRODUCTION_FILE );
  return;
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
  my $timeperiod_prefix_ = substr ( $timeperiod_ , 0 , 2 );

# Only remove installs for same timeperiod.
  my $time_period_comment_tag_ = "# ".$timeperiod_;

  my $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep start_options_real_trading | grep $exchange_ | grep \"$time_period_comment_tag_\"'";
  my @existing_queries_ = `$exec_cmd_`; chomp ( @existing_queries_ );

  $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep stop_real_trading | grep $exchange_ | grep \"$time_period_comment_tag_\"'";
  my @existing_stops_ = `$exec_cmd_`; chomp ( @existing_stops_ );

# Match starts with stops , so we don't end up messing with
# cron jobs for different products.
  my %start_query_ids_ = ( );
  my %is_valid_stop_id_ = ( );

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
  push ( @lines_to_skip_ , "### [ Options Strategy $exchange_ ]" );

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

      if ( ( $t_query_id_ == $prod_query_start_id_ ) ) {
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

sub CreateRunningLocks
{
  my @t_lock_keys_ = ( );

  for ( my $strat_counter = 0 ; $strat_counter < $num_strats_to_install_ ; $strat_counter ++ )
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
        my $user_input = AskUserPrompt( "Are you sure you want to go ahead? Press <Enter> to continue, Ctrl+C to exit installation: ", 30 );
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
  print $question_." (enter before $timeout_seconds_ seconds): ";
  eval {
    local $SIG{ALRM} = sub { die "timeout reading from keyboard\n" };
    alarm $timeout_seconds_;
    $answer = <STDIN>;
    alarm 0;
    chomp $answer;
  };
  if ($@) {
    die $@ if $@ ne "timeout reading from keyboard\n";
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

