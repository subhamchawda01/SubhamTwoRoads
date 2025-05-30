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
use List::Util qw/max min sum/; # for max
use List::MoreUtils qw(uniq); # for uniq
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

sub LoadConfigFileMRT;
sub SanityCheckConfigParamsMRT;
sub PickStrats;
sub CheckResetMaxLosses;
sub InstallStrats;
sub ClearProductionStratList;
sub AddToProductionStratList;
sub BackupExistingProductionInstall;
sub FixProductionCrontab;
sub InstallProductionCrontab;
sub CombineProductionStrats;
sub RemoveIntermediateFiles;
sub TimeInMin;
sub GetShortcodeList;
sub GetAllShortcodesforPickedStrats;
sub DevelopShortcodeStratCustomScalingMap;
sub CheckORSLimits;
sub ScaleParamFile;
sub SendReportMail;
sub PrintErrorAndDie;
sub SendErrorMail;
sub SendErrorMailAndDie;
sub SendErrorMailifAutomatedAndDie;
sub CreateRunningLocks;
sub RemoveRunningLocks;
sub CreateDoneLocks;
sub AskUserPrompt;

my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list";
my $local_production_strat_list_file_ = "";
my $REMOTE_PRODUCTION_STRAT_LIST_DIR = "/home/dvctrader/production_strat_list";
my $remote_production_strat_list_file_ = "";
my $local_single_strat_file_ = "";

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
require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; # GetBasepxStratFirstModel
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

my @ADDITIONAL_EXEC_PATHS=();
my $get_contract_specs_exec = SearchExec ( "get_contract_specs", @ADDITIONAL_EXEC_PATHS ) ;
my $SUMMARIZE_EXEC = SearchExec ( "summarize_strategy_results", @ADDITIONAL_EXEC_PATHS ) ;

if ( ! $get_contract_specs_exec && ! $SUMMARIZE_EXEC ) {
  exit(0);
}

my $verbose_ = 0 ;
my $add_to_crontab_ = 1;    # if zero this argument would not change the crontab file on the server and put all the strategies in a single local file. Only works whem install is set to 1.

my $exit_on_simultaneous_run_ = 0; # if 0: ask for user prompt, if 1: directly exit
my $print_OML_ = 0;

my $USAGE="$0 SHORTCODE TIMEPERIOD CONFIGFILE [ VERBOSE = 0/1 ] [ print_OML = 0/1 ] [add_to_crontab = 1/0, default:1] [exit_on_simultaneous_run = 1/0, default:0]";

my $sum_num_strats_to_install_ = 0;
my $prod_query_start_id_ = 0;
my $prod_query_stop_id_ = 0;
my $fpga_flag_ = 1;  # Only for CME
my $risk_tags_ = "MRT";  # Tags for risk monitor computations

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
my %underlier_shortcodes_strat_uts_ = ( );
my %underlier_shortcodes_strat_cs_ = ( );
my %underlier_shortcodes_strat_maxpos_ = ( );
my @all_underlier_shortcodes_ = ();
my %strat_localpath_to_base_ = ( );
my %strat_to_param_scaled_ = ( );
my %queryid_to_remote_stratpath_ = ( );
my %queryid_to_stratbase_ = ( );
my %strat_to_maxlosses_ = ( );
my %strat_to_new_uts_ = ( );

my $pickstrats_date_ = `date +%Y%m%d`; chomp ( $pickstrats_date_ );
my $curr_hh_ = `date +%H%M`; chomp ( $curr_hh_ );
if ( $curr_hh_ >= 2200 )
{
  $pickstrats_date_ = CalcNextDate ( $pickstrats_date_ );
}
my $curr_date_ = $pickstrats_date_;
my @total_size_to_run_ = ( );
my @max_loss_per_strat_ = ( );
my @strats_to_keep_ = ( );
my $email_address_ = "";
my $install_picked_strats_ = 0;
my $install_location_ = "";
my $exec_start_hhmm_ = 0;
my $exec_end_hhmm_ = 0;
my $remote_basefolder_path_ = "";
my $onload_trade_exec_ = 0;
my $vma_trade_exec_ = 0;
my $affinity_trade_exec_ = 0;
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
`mkdir -p $PICKSTRAT_TEMP_DIR`;
my $remote_lockfile_ = "/home/dvctrader/pick_strat_lock";
my $is_remote_lock_created_ = 0;
my @lock_keys_ = ( );

LoadConfigFileMRT ( $config_file_ );

SanityCheckConfigParamsMRT ( );

#create running locks (to avoid multiple simultaneous runs across servers)
CreateRunningLocks ( );

my @picked_strats_ = ( );

PickStrats ( );

SetStratUTS ( );

CheckResetMaxLosses ( );

if ( $curr_date_ eq $pickstrats_date_ ) {
  InstallStrats ( );
}

SendReportMail ( );

#Installed successfully => Mark this query_start_id as done for today (to avoid multiple installs for same id)
CreateDoneLocks ( );

exit ( 0 );

sub LoadConfigFileMRT
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
        }

        when ( "EXCHANGE" )
        {
          $exchange_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > EXCHANGE=".$exchange_."\n";
        }

        when ( "NUM_STRATS_TO_INSTALL" )
        {
          $sum_num_strats_to_install_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > NUM_STRATS_TO_INSTALL=".$sum_num_strats_to_install_."\n";
        }

        when ( "TOTAL_SIZE_TO_RUN" )
        {
          my $t_total_size_to_run_ = $t_words_ [ 0 ];
          push ( @total_size_to_run_ , $t_total_size_to_run_ );
          $mail_body_ = $mail_body_." \t > TOTAL_SIZE_TO_RUN=".$t_total_size_to_run_."\n";
        }

        when ( "EMAIL_ADDRESS" )
        {
          $email_address_ = $t_words_ [ 0 ];
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

        when ( "INSTALL_PICKED_STRATS" )
        {
          $install_picked_strats_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_picked_strats_."\n";
        }

        when ( "INSTALL_LOCATION" )
        {
          $install_location_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > INSTALL_LOCATION=".$install_location_."\n";
        }

        when ( "REMOTE_BASEFOLDER_PATH" )
        {
          $remote_basefolder_path_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > REMOTE_BASEFOLDER_PATH=".$remote_basefolder_path_."\n";
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

        when ( "MAX_LOSS_PER_STRAT" )
        {
          my $t_max_loss_per_strat_ = $t_words_ [ 0 ];
          push ( @max_loss_per_strat_, $t_max_loss_per_strat_ );
          $mail_body_ = $mail_body_." \t > MAX_LOSS_PER_strat=".$t_max_loss_per_strat_."\n";
        }

        when ( "RISK_TAGS" )
        {
          $risk_tags_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > RISK_TAGS=".$risk_tags_."\n";
        }

        when ( "FPGA" )
        {
          $fpga_flag_ = $t_words_ [ 0 ];
          $mail_body_ = $mail_body_." \t > FPGA_FLAG=".$fpga_flag_."\n";
        }
      }
    }
  }

  $mail_body_ = $mail_body_." \t > DATE=".$pickstrats_date_."\n";

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}

sub SanityCheckConfigParamsMRT
{
  print "Sanity Checking ...\n";

  if ( $sum_num_strats_to_install_ < 0 )
  {
    SendErrorMailAndDie ( "SUM_NUM_STRATS_TO_INSTALL=".$sum_num_strats_to_install_ );
  }

  if ( $sum_num_strats_to_install_ == 0 )
  {
    print "Warning no strat has been given to run for the given config\n";
  }

  if ( $sum_num_strats_to_install_ != $#max_loss_per_strat_ + 1 )
  {
    SendErrorMailAndDie ( "Number of maxloss's specified not equal to number of strats to install" );
  }

  if ( $sum_num_strats_to_install_ != $#total_size_to_run_ + 1 )
  {
    SendErrorMailAndDie ( "Number of strats for which uts was specified not equal to number of strats to install" );
  }

  $local_production_strat_list_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".basename ( $config_file_ );

  push ( @intermediate_files_ , $local_production_strat_list_file_ );

  $local_single_strat_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".basename ( $config_file_ ).".single_strat_file";

  $remote_production_strat_list_file_ = $REMOTE_PRODUCTION_STRAT_LIST_DIR."/".basename ( $config_file_ );

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
   elsif ( $prod_query_stop_id_ < $prod_query_start_id_ + $sum_num_strats_to_install_ - 1) {
     PrintErrorAndDie ( "Queries to install are more than query id's provided" );
   }
   elsif ( ! defined $risk_tags_ || $risk_tags_ eq "" ) {
     PrintErrorAndDie ( "INSTALL_PICKED_STRATS is set, but RISK_TAGS is not provided" );
   }

  }

  return;
}

sub TimeInMin
{
  my $utc_hhmm_ = shift;
  my $utc_hhmm_min_ = substr($utc_hhmm_, 0, 2)*60 + substr($utc_hhmm_, 2, 4);
  return $utc_hhmm_min_;
}

sub PickStrats
{
    print "Picking strats ...\n";

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." # PickStrats \n\n";
    $mail_body_ = $mail_body_."\n\n\t PICKED :\n";

    foreach my $strat_name_ (@strats_to_keep_)
    {
        if (!FindItemFromVec ( $strat_name_, @picked_strats_ ))
        {
            push ( @picked_strats_, $strat_name_ );
            $mail_body_ = $mail_body_."\t\t ".$strat_name_."\n";
            print "KEEP: ".$strat_name_."\n";
        }
    }

    if ( $sum_num_strats_to_install_ != $#strats_to_keep_ + 1 )
    {
      SendErrorMailAndDie ( "Number of strats specified not equal to unmber of strats to keep" );
    }

  my @start_time_utc_hhmm_ = ();
  my @end_time_utc_hhmm_ = ();

  if ( $sum_num_strats_to_install_ > 0 )
  {
    foreach my $strat_ (@picked_strats_)
    {
      my $full_path_strat_ = GetFullPathofStrat($strat_);
      my @strat_contents_ = GetStratContents($full_path_strat_);

      my $start_time_strat_utc_hhmm_ = GetUTCHHMMStr( $strat_contents_[5], $pickstrats_date_);
      push( @start_time_utc_hhmm_, $start_time_strat_utc_hhmm_ );

      my $end_time_strat_utc_hhmm_ = GetUTCHHMMStr( $strat_contents_[6], $pickstrats_date_);
      push( @end_time_utc_hhmm_, $end_time_strat_utc_hhmm_ );
    }

    my $min_start_time_strat_min_ = TimeInMin( min( @start_time_utc_hhmm_) );
    my $max_end_time_strat_min_ = TimeInMin( min( @end_time_utc_hhmm_) );
    my $exec_start_hhmm_min_ = TimeInMin( int($exec_start_hhmm_) );
    my $exec_end_hhmm_min_ = TimeInMin( int($exec_end_hhmm_) );

    if ($min_start_time_strat_min_ < $exec_start_hhmm_min_ + 30)
    {
      PrintErrorAndDie("Minimum start time of strat should be at least 30 minutes after exec start time.
                        Minimum start time of strats = ".min( @start_time_utc_hhmm_ ).
          "\nExec_Start_hhmm = ".int($exec_start_hhmm_) );
    }

    if ($max_end_time_strat_min_ + 10 > $exec_end_hhmm_min_)
    {
      PrintErrorAndDie("Maximum end time of strat should be at least 10 minutes before exec end time
                        Maximum end time of strats = ".max( @end_time_utc_hhmm_ ).
          "\nExec_End_hhmm = ".int($exec_end_hhmm_) );
    }
  }
}

sub InstallStrats
{
  if ( $install_picked_strats_ )
  {
    my $create_lock_numtrials_ = 1;
    my $create_lock_timeout_secs_ = 1;

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

    ScaleInstalledParamsStrats ( );

    CombineProductionStrats ( );

    CheckORSLimits( );

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
  }
  else
  {
    $mail_body_ = $mail_body_."\n NO INSTALLS SINCE INSTALL_PICKED_STRATS=".$install_picked_strats_." INSTALL_LOCATION='".$install_location_."' PROD_QUERY_START_ID='".$prod_query_start_id_."' EXCHANGE='".$exchange_."' EXEC_START_HHMM='".$exec_start_hhmm_."' EXEC_END_HHMM='".$exec_end_hhmm_."'\n";
  }
  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}


sub CheckORSLimits
{
  GetAllShortcodesforPickedStrats();
  DevelopShortcodeStratCustomScalingMap();
  foreach my $t_shortcode_ (@all_underlier_shortcodes_)
  {
    print "Loading the Limits for $t_shortcode_ from server: $install_location_\n";
    if ( $t_shortcode_ =~ /^(NSE_|BSE_)/ ) {
      print "Ignoring Limits Check for NSE products\n";
      next;
    }

    my $max_order_size_limit_;
    my $max_position_limit_;
    my $get_limits_cmd_ = "ssh dvcinfra\@$install_location_ \'cat /home/pengine/prod/live_configs/\`hostname\`_addts.cfg\'";
    my $tt_shortcode_ = $t_shortcode_;

    if($t_shortcode_ eq "XTE_0") {
      $tt_shortcode_= "XT_0";
    }
      elsif($t_shortcode_ eq "YTE_0") {
        $tt_shortcode_= "YT_0";
    }

    if ( $exchange_ eq $t_shortcode_ =~ /^(NSE_|BSE_)/ ){
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
        if ( $limit_words_[3] eq $tt_shortcode_ ) {
          if ( ! defined $max_order_size_limit_ || $max_order_size_limit_ > $limit_words_[5] ) {
            $max_order_size_limit_ = $limit_words_[5];
          }
          if ( ! defined $max_position_limit_ || $max_position_limit_ > $limit_words_[4] ) {
            $max_position_limit_ = $limit_words_[4];
          }
        }
      }
      elsif ( $limit_words_[2] eq "ADDCOMBINEDTRADINGSYMBOL" ) {
        my $shc_weight_;

        for ( my $lword_idx_ = 8; $lword_idx_ < $#limit_words_; $lword_idx_ += 2 ) {
          if ( $limit_words_[ $lword_idx_ ] eq $tt_shortcode_ ) {
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

    if ( !defined $max_order_size_limit_ || !defined $max_position_limit_ ) {
      SendErrorMailAndDie ( "MaxPosLimit and OrderSizeLimit could not fetched for ".$shortcode_ );
    }
    print "OrderSizeLimit: ".$max_order_size_limit_.", MaxPosLimit: ".$max_position_limit_."\n";
    my $total_maxpos_ = sum(values %{$underlier_shortcodes_strat_maxpos_{$t_shortcode_}});
    my $max_order_size_ = max(values %{$underlier_shortcodes_strat_uts_{$t_shortcode_}});

    if ( $total_maxpos_ > $max_position_limit_) {
      SendErrorMailAndDie ( "Total MaxPos of all the queries exceeeds the MaxPosLimit $max_position_limit_" );
    }

    if ( $max_order_size_> $max_order_size_limit_) {
      SendErrorMailAndDie ( "UTS exceeds the OrderSizeLimit $max_order_size_limit_" );
    }
  }
}

sub ReadUts
{
  my $pf_contents_ref_ = shift;
  my $base_strat_name_ = shift;

  my @param_file_contents_ = @$pf_contents_ref_;
  my $existing_uts_ = 0;

  foreach my $param_file_line_ ( @param_file_contents_ )
  {
    my @param_line_words_ = split ( ' ', $param_file_line_ );
    if ( $param_line_words_[1] eq "INST_UTS" )
    {
      $existing_uts_ = int($param_line_words_[2]);
      last;
    }
  }

  if ( $existing_uts_ == 0 && GetFullPathofStrat($base_strat_name_) !~ /fgbmlx/ ) #for fgb* we dont have read uts for now
  {
    PrintErrorAndDie("Not able to read UTS for $base_strat_name_");
  }

  if ( GetFullPathofStrat($base_strat_name_) =~ /fgbmlx/)
  {
    $existing_uts_ = 1;
  }

  return $existing_uts_;
}
sub ScaleParamFileContents
{
  my $base_strat_name_ = shift;
  my $pf_contents_ref_ = shift;
  my $t_pf_contents_ref_ = shift;

  my @param_file_contents_ = @$pf_contents_ref_;

  my $max_loss_ = $strat_to_maxlosses_{ $base_strat_name_ };
  my $new_uts_ = $strat_to_new_uts_{ $base_strat_name_ };
  my $existing_uts_ = ReadUts($pf_contents_ref_, $base_strat_name_);
  my $new_maxloss = ceil($max_loss_*$new_uts_/$existing_uts_);

  foreach my $param_file_line_ ( @param_file_contents_ )
  {
    my @param_line_words_ = split ( ' ' , $param_file_line_ );

    given ( $param_line_words_ [ 1 ] )
    {
      when ( "STRAT_MAX_LOSS" )
      {
          $param_line_words_ [ 2 ] = $new_maxloss;
          $strat_to_maxlosses_{ $base_strat_name_ } = $new_maxloss;
      }
      when ( "PRODUCT_MAXLOSS" )
      {
          $param_line_words_ [ 2 ] = max(20000, int($param_line_words_ [ 2 ]));
      }
      when ( "HISTFILE_PREFIX" )
      {
          if ( substr($param_line_words_ [ 2 ], 0, 27) ne "/spare/local/MeanRevertPort" )
          {
            PrintErrorAndDie("Histfile Prefix does not begin with /spare/local/MeanRevertPort")
          }
      }
      when ( "INST_UTS" )
      {
          $param_line_words_ [ 2 ] = $new_uts_;
      }
      when ( "INST_MAXLOTS" )
      {
          my $new_inst_maxlots_ = $param_line_words_[2]*$new_uts_/$existing_uts_;
          if ( $new_inst_maxlots_ =~ /^\d+$/)
          {
            $param_line_words_[2] = $new_inst_maxlots_;
          }
          else
          {
            PrintErrorAndDie( "Recheck. New Inst Maxlots not coming out to be a whole number for ", $base_strat_name_);
          }
      }
      default
      {
      }
    }

    push ( @$t_pf_contents_ref_, join ( ' ' , @param_line_words_ ) );
  }

  my $paramfile_prefix_ = ".corrected_ml_$new_maxloss";
  return $paramfile_prefix_;
}

sub ScaleParamFile
{
  my ( $parent_strat_ , $param_file_ ) = @_;

## Reading Param Contents
  open PARAM_FILE, "< $param_file_" or SendErrorMailifAutomatedAndDie ( "Could not open $param_file_" );
  my @param_file_contents_ = <PARAM_FILE>; chomp ( @param_file_contents_ );
  close ( PARAM_FILE );

  my @scaled_param_file_contents_ = ( );

## if paramfile is regime, break its lines into constituents
## for each constituent, call ScaleParamFileContents
## append the output to the @scaled_param_file_contents_
  my @constituent_param_contents_ = @param_file_contents_;

  my $paramfile_prefix_ = ScaleParamFileContents ( $parent_strat_, \@constituent_param_contents_ , \@scaled_param_file_contents_ );
  my $param_file_name_ = $param_file_.$paramfile_prefix_;

  open ( PARAM_FILE , ">" , $param_file_name_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $param_file_name_" );
  print PARAM_FILE $_."\n" foreach @scaled_param_file_contents_;
  close ( PARAM_FILE );

  push ( @intermediate_files_ , $param_file_name_ );

  return $param_file_name_;
}

sub GetFullPathofStrat
{
    my ( $strat_ ) = @_;
    my $PRINT_STRAT_NAME_FROM_BASE = $SCRIPTS_DIR."/print_strat_from_base.sh";

    my $exec_cmd_ = $PRINT_STRAT_NAME_FROM_BASE." $strat_";
    my @t_strat_name_ = `$exec_cmd_`; chomp ( @t_strat_name_ );

    if ( $#t_strat_name_ < 0 )
    {
      SendErrorMailifAutomatedAndDie ( "$exec_cmd_ returned ".@t_strat_name_ );
    }

    my $full_path_strat_name_ = $t_strat_name_ [ 0 ];

    return $full_path_strat_name_ ;
}

sub GetStratContents
{
    my ( $full_path_strat_name_ ) = @_;
    my $exec_cmd_ = "cat $full_path_strat_name_";
    my @t_strat_contents_ = `$exec_cmd_`; chomp ( @t_strat_contents_ );

    if ( $#t_strat_contents_ < 0 )
    {
      SendErrorMailifAutomatedAndDie ( "$full_path_strat_name_ is empty" );
    }
    my @t_strat_words_ = split ( ' ' , $t_strat_contents_ [ 0 ] );
    return @t_strat_words_
}

sub ScaleInstalledParamsStrats
{
  foreach my $strat_ ( @picked_strats_ ) {
      my $full_path_strat_name_ = GetFullPathofStrat( $strat_ );

    my @t_strat_words_ = GetStratContents( $full_path_strat_name_);
    if ( $#t_strat_words_ < 7 )
    {
      SendErrorMailifAutomatedAndDie ( "Malformed strat file ($full_path_strat_name_): ".join(" ",@t_strat_words_) );
    }
    if ( $t_strat_words_[2] ne "IndexFuturesMeanRevertingTrading" )
    {
      SendErrorMailifAutomatedAndDie ( "Strat File not MRT. Please check the strat files before running install mrt script" );
    }

    my $t_param_file_full_path_ = $t_strat_words_ [ 4 ];
    $strat_to_param_scaled_{ $strat_ } = ScaleParamFile ( $strat_ , $t_param_file_full_path_ );
  }

  $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
  return;
}

sub GetShortcodeList
{
  my $strat_ = shift;

  my $full_path_strat_name_ = GetFullPathofStrat( $strat_ );
  my @t_strat_words_ = GetStratContents( $full_path_strat_name_);
  my $t_shortcode_file_full_path_ = $t_strat_words_[3];
  my @shortcode_list_strat_ = ( );

  ## Reading ShortCode File Contents
  open SHORTCODE_FILE, "< $t_shortcode_file_full_path_" or SendErrorMailifAutomatedAndDie ( "Could not open $t_shortcode_file_full_path_" );
  my @shortcode_file_contents_ = <SHORTCODE_FILE>; chomp ( @shortcode_file_contents_ );
  close ( SHORTCODE_FILE );

  foreach my $shortcode_lines_ ( @shortcode_file_contents_ )
  {
    my @t_words_ = split ( ' ' , $shortcode_lines_ );
    if ( $#t_words_ >= 1 && $t_words_[1] eq "NoTrade" )
    {
      next;
    }
    else
    {
      push @shortcode_list_strat_, $t_words_[0]
    }
  }
  return @shortcode_list_strat_;
}

sub GetAllShortcodesforPickedStrats
{
  foreach my $strat_ ( @picked_strats_)
  {
    push(@all_underlier_shortcodes_, GetShortcodeList($strat_) );
  }

  @all_underlier_shortcodes_ = uniq(@all_underlier_shortcodes_);
}

sub DevelopShortcodeStratCustomScalingMap
{
  foreach my $strat_ ( @picked_strats_ )
  {
    my $full_path_strat_name_ = GetFullPathofStrat( $strat_ );
    my @t_strat_words_ = GetStratContents( $full_path_strat_name_);
    my $t_param_file_full_path_ = $t_strat_words_ [ 4 ];
    my @shortcode_list_strat_ = GetShortcodeList( $strat_ );

    my $customscaling_line_ = `cat $t_param_file_full_path_ | grep USE_CUSTOM_SCALING`;
    my $maxlots_line_ = `cat $t_param_file_full_path_ | grep INST_MAXLOTS | awk '{print \$3}'`;
    my $uts_line_ = `cat $t_param_file_full_path_ | grep INST_UTS | awk '{print \$3}'`;
    my $maxlots_ = int($maxlots_line_);
    my $uts_ = 1;
    if ( $uts_line_ ne "" )
    {
      $uts_ = int($uts_line_);
    }

    if ( $customscaling_line_ eq "" )
    {
      foreach my $t_shortcode_ ( @shortcode_list_strat_ )
      {
        $underlier_shortcodes_strat_cs_{$t_shortcode_}{$strat_} = 1;
        $underlier_shortcodes_strat_maxpos_{$t_shortcode_}{$strat_} = $maxlots_;
        $underlier_shortcodes_strat_uts_{$t_shortcode_}{$strat_} = $uts_;
      }
    }
    else
    {
      my @cs_vector_ = split(",", (split(" ", $customscaling_line_))[2]);
      if ($#cs_vector_ ne $#shortcode_list_strat_)
      {
        PrintErrorAndDie("Length of CustomScaling in paramfile is not the same as number of shortcodes provided")
      }
      else
      {
        for( my $i=0; $i<=$#shortcode_list_strat_; $i++)
        {
          my $t_shortcode_=$shortcode_list_strat_[$i];
          $underlier_shortcodes_strat_cs_{$t_shortcode_}{$strat_} = int($cs_vector_[$i]);
          $underlier_shortcodes_strat_maxpos_{$t_shortcode_}{$strat_} = int($cs_vector_[$i])*$maxlots_;
          $underlier_shortcodes_strat_uts_{$t_shortcode_}{$strat_} = int($cs_vector_[$i])*$uts_;
        }
      }
    }
  }
  return;
}

sub CombineProductionStrats
{
# Combine the strats to install according to the combing key.

  my $current_prod_id_index_ = 0; # Start with the 1st id.

  while ( $current_prod_id_index_ <= $#picked_strats_ )
  {
      my $query_id_ = $prod_query_start_id_ + $current_prod_id_index_;
      $queryid_to_stratbase_{ $query_id_ }  = $picked_strats_ [ $current_prod_id_index_ ];
      $current_prod_id_index_ ++;
  }
}

sub InstallStrategyProductionMRT
{
  my ( $t_strat_, $prod_machine_ip_, $strat_id_, $user_ ) = @_;

  my $local_strat_full_path_ = GetFullPathofStrat($t_strat_);
  my @strat_contents_ =  GetStratContents( $local_strat_full_path_ );

  my $scaled_paramfile_ = $strat_to_param_scaled_{ $t_strat_ } ;
  my $remote_paramfile_path_ = $remote_basefolder_path_.basename($scaled_paramfile_);
  my $create_remote_paramfile_cmd_ = "scp $scaled_paramfile_ $user_\@$prod_machine_ip_:$remote_basefolder_path_";
  `$create_remote_paramfile_cmd_`;

  my $modelfile_ =   $strat_contents_[3];
  my $remote_modelfile_path_ =  $remote_basefolder_path_.basename($modelfile_);
  my $create_remote_modelfile_cmd_ = "scp $modelfile_ $user_\@$prod_machine_ip_:$remote_basefolder_path_";
  `$create_remote_modelfile_cmd_`;

  my $stratfile_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$t_strat_.".final_prod_";
  $strat_contents_[3] =   $remote_modelfile_path_;
  $strat_contents_[4] =   $remote_paramfile_path_;
    $strat_contents_[7] =   $strat_id_;
  $strat_contents_[8] = "INVALID";
  $strat_contents_[9] = $t_strat_;

  my $remote_stratfile_ = $remote_basefolder_path_.$t_strat_."_".$strat_id_;

  open ( STRAT_FILE , ">" , $stratfile_ ) or SendErrorMailifAutomatedAndDie ( "Could not open $stratfile_ for writing" );
  print STRAT_FILE join(" ", @strat_contents_), "\n";
  close ( STRAT_FILE );

  my $create_remote_stratfile_cmd = "scp $stratfile_ $user_\@$prod_machine_ip_:$remote_stratfile_";
  `$create_remote_stratfile_cmd`;

    return $remote_stratfile_
}

sub SyncStrats
{
# Need to maintain a list of all query ids , should we need to combine
# multiple strat lines into a single strat file.
  foreach my $query_id_ ( keys %queryid_to_stratbase_ )
  {
      my $strat_ = $queryid_to_stratbase_{ $query_id_ };

      $queryid_to_remote_stratpath_{ $query_id_ } = InstallStrategyProductionMRT ( $strat_, $install_location_,
          $query_id_, "dvctrader" );
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
  print LOCAL_PRODUCTION_FILE "## [ MRT $exchange_ $shortcode_ ] ";
  print LOCAL_PRODUCTION_FILE "[ LOSS ";
  foreach my $strat_ ( @picked_strats_ )
  {
    my $t_maxloss_ = $strat_to_maxlosses_{$strat_};
    print LOCAL_PRODUCTION_FILE $t_maxloss_, " ";
  }
  print LOCAL_PRODUCTION_FILE "] ";
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
  if ($exchange_ eq "CME" && $fpga_flag_ == 1) {
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
  push ( @lines_to_skip_ , "## [ MRT $exchange_ $shortcode_ ] [" );

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
	#print $exec_cmd_."\n";
    `$exec_cmd_`;
  }
  return;
}

sub SetStratUTS
{
# Use user provided max-losses.
# Purge computed max-losses.

  my $iterator=0;

  foreach my $strat_name_ ( @picked_strats_ )
  {
    $strat_to_new_uts_{ $strat_name_ } = $total_size_to_run_ [ $iterator ];
    $iterator = $iterator+1;
  }
  return;
}

sub CheckResetMaxLosses
{
# Use user provided max-losses.
# Purge computed max-losses.

  my $iterator=0;

  foreach my $strat_name_ ( @picked_strats_ )
  {
    $strat_to_maxlosses_{ $strat_name_ } = $max_loss_per_strat_ [ $iterator ];
    $iterator = $iterator+1;
  }
  return;
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
