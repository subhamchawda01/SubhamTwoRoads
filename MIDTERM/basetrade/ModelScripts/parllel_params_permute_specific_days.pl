#!/usr/bin/perl

# \file ModelScripts/parallel_params_permute.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
#
# SHORTCODE
# TIMEPERIOD
# BASEPX
# PARAMFILE_WITH_PERMUTATIONS
# TRADING_START_YYYYMMDD 
# TRADING_END_YYYYMMDD

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use Fcntl qw (:flock);
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

package ResultLine;
use Class::Struct;

# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );

package main;

sub MakeStrategyFiles ; # takes input_stratfile and makes strategyfiles corresponding to different scale constants
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "ankit" || $USER eq "anshul")
{
  $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to s

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_strat_traded_ezone.pl"; #GetStratTradedEzone
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/get_trading_exec.pl"; # GetTradingExec
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_base_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratBaseExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort

require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_STRAT_FILES_IN_ONE_SIM = 40; # please work on optimizing this value
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

# start
my $USAGE="$0 param_file_with_permutations datefile strategy_file_name ";

if ( $#ARGV != 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $orig_param_filename_ = $ARGV[0];
my $datefile = $ARGV[1];
my $strategy_file_name_ = $ARGV[2] ;

my ( $trading_start_hhmm_, $trading_end_hhmm_ ) = GetStratStartEndHHMM ( $strategy_file_name_ ) ;
my ( $model_filename_, $t_param_filename_ ) = GetModelAndParamFileNames ( $strategy_file_name_ ) ;
my ( $shortcode_ , $strategyname_ ) = GetTradingExec ( $strategy_file_name_ ) ;

my $param_file_list_basename_ = basename ( $orig_param_filename_ );
$FBPA_WORK_DIR = $FBPA_WORK_DIR.$shortcode_."/".$trading_start_hhmm_."-".$trading_end_hhmm_."/".$strategyname_."/".$param_file_list_basename_."/";

my $traded_ezone_ = GetStratTradedEzone ( $strategy_file_name_ );

my $delete_intermediate_files_ = 1;

my $num_files_to_choose_ = 1;
my $min_pnl_per_contract_to_allow_ = -1.10 ;
my $min_volume_to_allow_ = 100; 
my $max_ttc_to_allow_ = 120;

my %strat2param_ ;
my @model_filevec_ = ();
my @param_filevec_ = ();
my @strategy_filevec_ = ();
my %param_to_resultvec_;
my %param_to_cmedpnl_;
my %param_to_cmedvolume_;

my @intermediate_files_ = ();

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
  if ( -d $work_dir_ )
  {
    print STDERR "Surprising but this dir exists\n";
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    $work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_; 
  }
  else
  {
    last;
  }
}

print "$strategyname_ $strategy_file_name_ $work_dir_ \n";

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $local_params_dir_ = $work_dir_."/params_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

open DATEFILE, "< $datefile" or PrintStacktraceAndDie ( "Could not open $datefile for writing\n" );
my @all_dates_ = <DATEFILE>;
chomp ( @all_dates_ );
close ( DATEFILE );

# Get param files and model files
ProcessListArgs ( );
if ( $#param_filevec_ < 0 )
{
  print "Exiting due to 0 param_filevec_\n";
  print $main_log_file_handle_ "Exiting due to 0 param_filevec_\n";
}
else
{
# From the given arguments create the strategy files for simulations
  MakeStrategyFiles ( );


# Run simulations on the created strategy files
  RunSimulationOnCandidates ( );

}
# end script
$main_log_file_handle_->close;

exit ( 0 );

sub ProcessListArgs
{

#model
  push ( @model_filevec_ , $model_filename_ ) ;
#params
  @param_filevec_ = PermuteParams ( $orig_param_filename_, $local_params_dir_ );
  print $main_log_file_handle_ "param_filevec_ = ".$#param_filevec_."\n";
}

# For each param file & for each model file, we create a strategy file
sub MakeStrategyFiles 
{
  print $main_log_file_handle_ "\nMakeStrategyFiles\n\n";

  my $strategy_progid_ = 1001;

  for (my $model_file_index_ = 0; $model_file_index_ <= $#model_filevec_; $model_file_index_++) 
  { # For each model

    my $this_model_filename_ = $model_filevec_[$model_file_index_];
    if ( ExistsWithSize ( $this_model_filename_ ) )
    {
      for (my $param_file_index_ = 0; $param_file_index_ <= $#param_filevec_ ; $param_file_index_++) 
      { # For each param

        my $this_param_filename_ = $param_filevec_[$param_file_index_];

        if ( ExistsWithSize ( $this_param_filename_ ) )
        {
          my $this_strategy_filebase_ = "strat_".$param_file_index_."_".$model_file_index_."_".$strategy_progid_ ;
          my $this_strategy_filename_ = $local_strats_dir_."/".$this_strategy_filebase_;

          my $exec_cmd="$MODELSCRIPTS_DIR/create_strategy_file.pl $this_strategy_filename_ $shortcode_ $strategyname_ $this_model_filename_ $this_param_filename_ $trading_start_hhmm_ $trading_end_hhmm_ $strategy_progid_";
          $exec_cmd = $exec_cmd." $traded_ezone_" if ( $traded_ezone_ ne "INVALID" );
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
          $strategy_progid_++; # uniqueness of progid ensures that we can run them in sim together

              if ( ExistsWithSize (  $this_strategy_filename_ ) )
              {
                push ( @strategy_filevec_, $this_strategy_filename_ ); 
                $strat2param_ { $this_strategy_filebase_ } = $this_param_filename_ ;
              }
        }
        else
        {
          print $main_log_file_handle_ "Skipping param ".$this_param_filename_."\n";
        }
      }
    }
    else
    {
      print $main_log_file_handle_ "Skipping model ".$this_model_filename_."\n";
    }
  }
}

sub RunSimulationOnCandidates
{
  print $main_log_file_handle_ "\n\n RunSimulationOnCandidates\n\n";

  my @non_unique_results_filevec_ = ( );

  my @unique_sim_id_list_ = ( );
  my @independent_parallel_commands_ = ( );
  my @tradingdate_list_ = ( );
  my @temp_strategy_list_file_index_list_ = ( );
  my @temp_strategy_list_file_list_ = ( );
  my @temp_strategy_cat_file_list_ = ( );
  my @temp_strategy_output_file_list_ = ( );
  my @temp_strategy_pnl_stats_file_list_ = ( );
  my @temp_strategy_log_file_list_ = ( );

# generate a list of commands which are unique , 
# independent from each other and can be safely run in parallel.

  my $max_days_at_a_time_ = 2000;
  foreach my $tradingdate_  ( @all_dates_ )
  {
    if ( SkipWeirdDate ( $tradingdate_ ) ||
        IsDateHoliday ( $tradingdate_ ) ||
        IsProductHoliday ( $tradingdate_ , $shortcode_ ) ||
        NoDataDate ( $tradingdate_ ) )
    {
      next;
    }

    if ( ! ValidDate ( $tradingdate_ ) )
    {
      last;
    }

    my $temp_strategy_list_file_index_ = 0;
    for ( my $strategy_filevec_index_ = 0 ; $strategy_filevec_index_ <= $#strategy_filevec_ ; )
    {
      my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
      my $temp_strategy_cat_file_ = $work_dir_."/temp_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
      my $temp_strategy_output_file_ = $work_dir_."/temp_strategy_output_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
      my $temp_strategy_pnl_stats_file_ = $work_dir_."/temp_strategy_pnl_stats_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

      open ( TSLF , ">" , $temp_strategy_list_file_ ) or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );
      for ( my $num_files_ = 0 ; $num_files_ < $MAX_STRAT_FILES_IN_ONE_SIM && $strategy_filevec_index_ <= $#strategy_filevec_ ; $num_files_ ++ )
      {
        my $this_strategy_filename_ = $strategy_filevec_ [ $strategy_filevec_index_ ];
        $strategy_filevec_index_ ++;

        print TSLF $this_strategy_filename_."\n";
        `cat $this_strategy_filename_ >> $temp_strategy_cat_file_`;
      }
      close ( TSLF );

      my $unique_sim_id_ = GetGlobalUniqueId ( ); # Get a concurrency safe id.
          my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
      my $this_log_filename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".$unique_sim_id_;

      my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
      my $exec_cmd_ = $LIVE_BIN_DIR."/sim_strategy SIM ".$temp_strategy_cat_file_." ".$unique_sim_id_." ".$tradingdate_." ".$market_model_index_." ADD_DBG_CODE -1 > ".$temp_strategy_output_file_." 2>/dev/null ; ".$MODELSCRIPTS_DIR."/get_pnl_stats_2.pl $this_trades_filename_ > $temp_strategy_pnl_stats_file_ 2>/dev/null; rm -f $this_trades_filename_; rm -f $this_log_filename_ ";

      push ( @unique_sim_id_list_ , $unique_sim_id_ );
      push ( @independent_parallel_commands_ , $exec_cmd_ );

      push ( @tradingdate_list_ ,$tradingdate_ );
      push ( @temp_strategy_list_file_index_list_ , $temp_strategy_list_file_index_ ); $temp_strategy_list_file_index_ ++;

      push ( @temp_strategy_list_file_list_ , $temp_strategy_list_file_ );
      push ( @temp_strategy_cat_file_list_ , $temp_strategy_cat_file_ );
      push ( @temp_strategy_output_file_list_ , $temp_strategy_output_file_ );
      push ( @temp_strategy_pnl_stats_file_list_ , $temp_strategy_pnl_stats_file_ );
      push ( @temp_strategy_log_file_list_ , $this_log_filename_ );
    }

    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_ , 1 );
  }

# process the list of commands , processing MAX_CORES_TO_USE_IN_PARALLEL at once
  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; )
  {
    my @output_files_to_poll_this_run_ = ( );
    my @logfiles_files_to_poll_this_run_ = ( );

    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ )
    {
      push ( @output_files_to_poll_this_run_ , $temp_strategy_output_file_list_ [ $command_index_ ] );
      push ( @logfiles_files_to_poll_this_run_ , $temp_strategy_output_file_list_ [ $command_index_ ] );

      { # empty the output result file.
        my $exec_cmd_ = "> ".$temp_strategy_output_file_list_ [ $command_index_ ];
        `$exec_cmd_`;
      }

      print $main_log_file_handle_ $independent_parallel_commands_ [ $command_index_ ]."\n";
      my $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ];
      my $temp_script_ = $work_dir_."/temp_script_".$command_index_;
      open SCRIPT, "> $temp_script_" or PrintStacktraceAndDie ( "Could not open $temp_script_ for writing\n" );
      print SCRIPT "$exec_cmd_\n";
      close ( SCRIPT );

      my $pid_ = fork();
      die "unable to fork $!" unless defined($pid_);
      if ( !$pid_ )
      {
#child process has pid 0
        exec("sh $temp_script_");
      }

#back to parent process
      print $main_log_file_handle_ "PID of $temp_script_ is $pid_\n";
      $command_index_ ++;
      sleep ( 1 );
    }

    my $t_pid_ = 9999;
    while ( $t_pid_ > 0 )
    { 
# there are still some child processes running, wait returns -1 when no child process is left
      $t_pid_ = wait();
      print $main_log_file_handle_ "PID of completed process: $t_pid_\n";
    }
  }

  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
  {
    my %unique_id_to_pnlstats_map_ = ( );

    my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];
    my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];
    my $temp_strategy_list_file_index_ = $temp_strategy_list_file_index_list_ [ $command_index_ ];
    my $temp_strategy_list_file_ = $temp_strategy_list_file_list_ [ $command_index_ ];
    my $temp_strategy_cat_file_ = $temp_strategy_cat_file_list_ [ $command_index_ ];
    my $temp_strategy_output_file_ = $temp_strategy_output_file_list_ [ $command_index_ ];
    my $temp_strategy_pnl_stats_file_ = $temp_strategy_pnl_stats_file_list_ [ $command_index_ ];
    my $exec_cmd_ = "cat ".$temp_strategy_output_file_list_ [ $command_index_ ];
    my @tradeinit_output_lines_ = `$exec_cmd_`;

    if ( ExistsWithSize ( $temp_strategy_pnl_stats_file_ ) )
    {
      my $exec_cmd_ = "cat $temp_strategy_pnl_stats_file_";
      print $main_log_file_handle_ $exec_cmd_."\n";
      my @pnlstats_output_lines_ = `$exec_cmd_`;

      for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
      {
        my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
        if( $#rwords_ >= 1 )
        {
          my $unique_strat_id_ = $rwords_[0];
          splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_strat_id_
              $unique_id_to_pnlstats_map_{$unique_strat_id_} = join ( ' ', @rwords_ );
        }
      }
    }

    my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

    open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
    for ( my $t_tradeinit_output_lines_index_ = 0, my $psindex_ = 0; $t_tradeinit_output_lines_index_ <= $#tradeinit_output_lines_; $t_tradeinit_output_lines_index_ ++ )
    {
      if ( $tradeinit_output_lines_ [ $t_tradeinit_output_lines_index_ ] =~ /SIMRESULT/ )
      { # SIMRESULT pnl volume                                                                                                                                                                                               
        my @rwords_ = split ( ' ', $tradeinit_output_lines_[$t_tradeinit_output_lines_index_] );
        splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc                                                                                     
            my $remaining_simresult_line_ = join ( ' ', @rwords_ );
        if ( ( $rwords_[1] > 0 ) || # volume > 0                                                                                                                                                                           
            ( ( $shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day                                                                      
        {
          my $unique_strat_id_ = GetUniqueSimIdFromCatFile ( $temp_strategy_cat_file_, $psindex_ );
          if ( ! exists $unique_id_to_pnlstats_map_{$unique_strat_id_} )
          {
            $unique_id_to_pnlstats_map_{$unique_strat_id_} = "0 0 0 0 0 0 0 0 0 0 0";
          }
          printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_strat_id_}, $unique_strat_id_ ;
          printf TRLF "%s %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_strat_id_}, $unique_strat_id_;
        }
        $psindex_ ++;
      }
    }
    close TRLF;

    if ( ExistsWithSize ( $temp_results_list_file_ ) )
    {
      my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database.pl $temp_strategy_list_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir";
      print $main_log_file_handle_ "$exec_cmd\n";
      my $this_local_results_database_file_ = `$exec_cmd`;
      push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
    }
  }

  @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ );
}
