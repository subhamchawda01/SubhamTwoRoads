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
sub MakeClubbedStrateyFiles;
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database


my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";
my $SHARED_FOLDER="/media/shared/ephemeral17/".$USER."/";
my $SHARED_LOCATION = "/media/shared/ephemeral17/commands";
my $DISTRIBUTED_EXECUTOR = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";
my $SHARED_FBPA_WORK_DIR = $SHARED_FOLDER."FBPA/";

my $REPO="basetrade";

my $SCRIPTS_BASE_DIR=$HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/basetrade_install/bin";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to s

require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; #GetFilteredDays
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
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
require "$GENPERLLIB_DIR/strat_utils.pl"; #GetParam
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_CORES_TO_USE_IN_PARALLEL = 5;
my $MAX_STRATEGIES_IN_ONE_SIM = 1;
my $DEBUG = 0;

# start
my $USAGE="$0 param_file_with_permutations start_date end_date strategy_file_name index [debug]" ;

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $orig_param_filename_ = $ARGV[0];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $strategy_file_name_ = $ARGV[3] ;
my $index_ = $ARGV[4];

if ( $#ARGV >= 5  )
{
    my $MAX_CORES_TO_USE_IN_PARALLEL = $ARGV[5];
}

if ( $#ARGV >= 6 )
{
    my $MAX_STRATEGIES_IN_ONE_SIM = $ARGV[6];
}

if ( $#ARGV >= 7 )
{
    $DEBUG = $ARGV[7];
}

my ( $trading_start_hhmm_, $trading_end_hhmm_ ) = GetStratStartEndHHMM ( $strategy_file_name_ ) ;
my ( $model_filename_, $t_param_filename_ ) = GetModelAndParamFileNames ( $strategy_file_name_ ) ;
my ( $shortcode_ , $strategyname_ ) = GetTradingExec ( $strategy_file_name_ ) ;


my $param_file_list_basename_ = basename ( $orig_param_filename_ );
$SHARED_FBPA_WORK_DIR = $SHARED_FBPA_WORK_DIR.$shortcode_."/".$trading_start_hhmm_."-".$trading_end_hhmm_."/".$strategyname_."/".$param_file_list_basename_."/";

my @trading_days_vec_ = GetDatesFromStartDate( $shortcode_, $trading_start_yyyymmdd_, $trading_end_yyyymmdd_, "INVALIDFILE", 400 );

my $delete_intermediate_files_ = 1;
my @intermediate_files_ = ();

my @model_filevec_ = ();
my @param_filevec_ = ();
my @strategy_filevec_ = ();
my %strategy_filemap_ = ();
my @independent_parallel_commands_ = ();
my @clubbed_strategy_filevec_ = (); # they already have uniqids
my @id_to_strategy = ();

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = $unique_gsm_id_ + 1;
my $shared_work_dir_ = $SHARED_FBPA_WORK_DIR.$unique_gsm_id_;

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
  if ( -d $shared_work_dir_ )
  {
    print STDERR "Surprising but this dir exists\n";
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = $unique_gsm_id_ + 1;
    $shared_work_dir_ = $SHARED_FBPA_WORK_DIR.$unique_gsm_id_;
  }
  else
  {
    last;
  }
}

my $results_base_dir = $shared_work_dir_."/results_base_dir";

my $shared_strats_dir_ = $shared_work_dir_."/strats_dir";
my $shared_params_dir_ = $shared_work_dir_."/params_dir";
my $shared_models_dir_ = $shared_work_dir_."/model_dir";

my $main_log_file_ = $shared_work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;


# start
if ( ! ( -d $shared_work_dir_ ) ) { `mkdir -p $shared_work_dir_`; }
if ( ! ( -d $results_base_dir ) ) { `mkdir -p $results_base_dir`; }

if ( ! ( -d $shared_strats_dir_ ) ) { `mkdir -p $shared_strats_dir_`; }
if ( ! ( -d $shared_params_dir_ ) ) { `mkdir -p $shared_params_dir_`; }
if ( ! ( -d $shared_models_dir_ ) ) { `mkdir -p $shared_models_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

# Get param files and model files
MakeParamAndModelFiles ( );

if ( $#param_filevec_ < 0 || $#param_filevec_ != $#model_filevec_ )
{
  print "could not permute params or create model files \n";
  print $main_log_file_handle_ "Exiting, couldnot crate param_filevec_ or model_filevec_ \n";
  exit(0);
}

# From the given arguments create the strategy files for simulations
MakeStrategyFiles ( );
if ( $#strategy_filevec_ < 0 )
{
    print "could not create any strategy files \n";
    print $main_log_file_handle_ "Exiting, couldnot crate any strategys\n";
    exit(0);
}

# Run simulations on the created strategy files
# collect primary and secondary pnl stats
MakeClubbedStrateyFiles();
RunSimulationOnCandidates();
`chmod -R a+rw $shared_work_dir_`;
ExecuteCommands();

# Remove intermediate files
print "removing intermediate files \n";
for ( my $idx_=0; $idx_ <= $#intermediate_files_; $idx_++) {
    `rm -rf $intermediate_files_[$idx_]`;
}

# close log file
$main_log_file_handle_->close;

exit ( 0 );
# end script


sub MakeParamAndModelFiles
{
    print $main_log_file_handle_ "MakingParamAndModelFiles\n";
    @param_filevec_ = PermuteParams ( $orig_param_filename_, $shared_params_dir_ );

    for ( my $t_paramfile_idx_ = 0 ; $t_paramfile_idx_ < scalar ( @param_filevec_ ) ; $t_paramfile_idx_ ++ )
    {
	# paramfilename indexing starting from 1
	my $t_model_filename_ = $shared_models_dir_."/model_".$t_paramfile_idx_;
	`$MODELSCRIPTS_DIR/create_options_modelfile.pl $t_model_filename_ $model_filename_ $param_filevec_[$t_paramfile_idx_] $index_`;
	push ( @model_filevec_, $t_model_filename_ );
    }
    print $main_log_file_handle_ "Total Paramfiles ".scalar(@model_filevec_)."\n";
}

# For each model file, we create a strategy file
sub MakeStrategyFiles 
{
  print $main_log_file_handle_ "MakingStrategyFiles\n";

  # strategy start id 
  my $strategy_progid_ = $unique_gsm_id_;
  my $ivadaptor_ = "kMethodBlackScholes";

  for (my $model_file_index_ = 0; $model_file_index_ <= $#model_filevec_ ; $model_file_index_++, $strategy_progid_++) 
  {
      my $this_model_filename_ = $model_filevec_[$model_file_index_];

      if ( ExistsWithSize ( $this_model_filename_ ) )
      {
          my $this_strategy_filebase_ = "strat_".$model_file_index_;
          my $this_strategy_filename_ = $shared_strats_dir_."/".$this_strategy_filebase_;

          my $exec_cmd="$MODELSCRIPTS_DIR/create_options_strategyfile.pl $this_strategy_filename_ $strategyname_ $this_model_filename_ $ivadaptor_ $trading_start_hhmm_ $trading_end_hhmm_ $strategy_progid_";
          `$exec_cmd`;

	  if ( ExistsWithSize (  $this_strategy_filename_ ) )
	  {
	      $strategy_filemap_{$strategy_progid_} = basename $this_strategy_filename_;
	      print $main_log_file_handle_ "Adding into strategy_map_ ".$strategy_progid_.$strategy_filemap_{$strategy_progid_}."\n";
	      push (@strategy_filevec_, $this_strategy_filename_);
	  }
      }
      else
      {
          print $main_log_file_handle_ "Skipping model ".$this_model_filename_."\n";
      }
  }
  print $main_log_file_handle_ "Total Strategy Files ".scalar(@strategy_filevec_)."\n";
}


# stats per underlying NSE_SHC_FUT0 # Primary
# stats per option NSE_SHC_C0_A # to add any contract specific params
# stats per strategy NSE_SHC_OPT0 # if we use same param for all strats
# make Y sets of combined strategy files ( X clubbed together ) ( Y uniq ids )
# run each of Y for a given trading_date in parallel, collect stats and print into results file
# Y is number of cores to used

# X == $MAX_STRATEGIES_IN_ONE_SIM
# Y == $MAX_CORES_TO_USE_IN_PARALLEL

## non-temp files ##
# model_dir
# param_dir
# strats_dir
# results_base_dir
# main_log_file.txt
## 

sub MakeClubbedStrateyFiles
{
    print $main_log_file_handle_ "ClubbingStrateyFiles\n";


# clubbing of strategies is independent
# we need to change stratid used inside each of strategyline
    if ( $MAX_STRATEGIES_IN_ONE_SIM > 1 )
    {
	my $strategy_idx_ = 0;

	my $catted_strategy_filename_ = $shared_work_dir_."/catted_strat_".$strategy_idx_;

#	my $id_to_strategy_filename_ = $work_dir_."id_to_strategy_".$strategy_idx_;

	my $strat_count_for_this_catted_file_ = 0;
	while ( $strategy_idx_ < scalar ( @strategy_filevec_ ) )
	{
	    cat $strategy_filevec_[$strategy_idx_] >> $catted_strategy_filename_;
	    $strat_count_for_this_catted_file_ ++;
	    $strategy_idx_++;
	    if ( $strat_count_for_this_catted_file_ == $MAX_STRATEGIES_IN_ONE_SIM )
	    {
		push(@clubbed_strategy_filevec_, $catted_strategy_filename_);
		push(@intermediate_files_, $catted_strategy_filename_);
		$catted_strategy_filename_ = $shared_work_dir_."/catted_strat_".$strategy_idx_;
		$strat_count_for_this_catted_file_ = 0;
	    }
	}
    }
    else
    {
	# review : copy reference instead ?
	@clubbed_strategy_filevec_ = @strategy_filevec_;
    }
    print $main_log_file_handle_ "Total Clubbed Strategies ".scalar(@clubbed_strategy_filevec_)."\n";
}


# we run in parallel for date and once done for a date, we dump all them into one file at a go, 
sub RunSimulationOnCandidates
{
    print $main_log_file_handle_ "\nRunningSimulations\n\n";
    my $unique_sim_id_ = $unique_gsm_id_;
    # sim_strategy_options + get_pnl_stats_options will yeild runtime_id_ shc_ result_line_
    # tradingdate_ stratname_shc_ result_line_
    for ( my $date_index_ = 0; $date_index_ <= $#trading_days_vec_; $date_index_ ++ )
    {
	#strat_shc_ 
	my @resultline_filenames_ = ();
	my $tradingdate_ = $trading_days_vec_[$date_index_];
	print $main_log_file_handle_ $tradingdate_."\n";
	my $strategy_id_ = $unique_gsm_id_;

	my ($tradingdateyyyy_, $tradingdatemm_, $tradingdatedd_) = BreakDateYYYYMMDD ( $tradingdate_ );
	my $resultsfilename_ = $results_base_dir."/".$tradingdateyyyy_."/".$tradingdatemm_."/".$tradingdatedd_."/results_database.txt";
	my $resultsfilepathdir_ = dirname ( $resultsfilename_ ); chomp ( $resultsfilepathdir_ );
	if ( ! ( -d $resultsfilepathdir_ ) )
	{
	    `mkdir -p $resultsfilepathdir_`;
	}

	for ( my $catted_file_index_ = 0; $catted_file_index_ < scalar(@clubbed_strategy_filevec_); )
	{
		my $strat_file_ = $clubbed_strategy_filevec_[$catted_file_index_];
		my $exec_cmd_ = "/home/dvctrader/basetrade/ModelScripts/options_single_run_sim.pl $strat_file_ $unique_sim_id_ $tradingdate_ $resultsfilename_ $strategy_filemap_{$strategy_id_}";
		push ( @independent_parallel_commands_ , $exec_cmd_ ); #Declare this

		$catted_file_index_++;
		$unique_sim_id_++;
		$strategy_id_++;
	}
    }
}

sub ExecuteCommands
{
    my $commands_file_id_  = `date +%N`;
    chomp( $commands_file_id_ );
    my $commands_file_ = $SHARED_LOCATION."/".$commands_file_id_;
    my $commands_file_handle_ = FileHandle->new;
    $commands_file_handle_->open ( "> $commands_file_ " ) or PrintStacktraceAndDie ( "Could not open $commands_file_ for writing\n" );
    print $commands_file_handle_ "$_ \n" for @independent_parallel_commands_;
    close $commands_file_handle_;
    my $dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $commands_file_ -s 1";
    print "Command executed: $dist_exec_cmd_ \n";
    my @output_lines_ = `$dist_exec_cmd_`;
    chomp( @output_lines_ );
    print "$_ \n" for @output_lines_;
		print $main_log_file_handle_ "$_ \n" for @output_lines_;
}
