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
my $SHARED_LOC = "/media/shared/ephemeral17/temp_options/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";
my $FBPA_SHARED_WORK_DIR = $SHARED_LOC."FBPA/";
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

my $MAX_STRAT_FILES_IN_ONE_SIM = 1;
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );
my $USE_DISTRIBUTED = 1; #Accept argument
my $DISTRIBUTED_EXECUTOR = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py";
my $SHARED_LOCATION = "/media/shared/ephemeral17/commands";
my $DEBUG = 0;

# start
my $USAGE="$0 param_file_with_permutations start_date end_date strategy_file_name index" ;

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $orig_param_filename_ = $ARGV[0];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $strategy_file_name_ = $ARGV[3] ;
my $index_ = $ARGV[4];

my ( $trading_start_hhmm_, $trading_end_hhmm_ ) = GetStratStartEndHHMM ( $strategy_file_name_ ) ;
my ( $model_filename_, $t_param_filename_ ) = GetModelAndParamFileNames ( $strategy_file_name_ ) ;
my ( $shortcode_ , $strategyname_ ) = GetTradingExec ( $strategy_file_name_ ) ;


my $param_file_list_basename_ = basename ( $orig_param_filename_ );
$FBPA_WORK_DIR = $FBPA_WORK_DIR.$shortcode_."/".$trading_start_hhmm_."-".$trading_end_hhmm_."/".$strategyname_."/".$param_file_list_basename_."/";
$FBPA_SHARED_WORK_DIR = $FBPA_SHARED_WORK_DIR.$shortcode_."/".$trading_start_hhmm_."-".$trading_end_hhmm_."/".$strategyname_."/".$param_file_list_basename_."/";

my @trading_days_vec_ = GetDatesFromStartDate( $shortcode_, $trading_start_yyyymmdd_, $trading_end_yyyymmdd_, "INVALIDFILE", 400 );

my $delete_intermediate_files_ = 1;
my @intermediate_files_ = ();

my @model_filevec_ = ();
my @param_filevec_ = ();
my @strategy_filevec_ = ();
my @independent_parallel_commands_ = ( );

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_; 
my $shared_work_dir_ = $FBPA_SHARED_WORK_DIR.$unique_gsm_id_; ######Change this

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

my $results_base_dir = $shared_work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $local_params_dir_ = $work_dir_."/params_dir";
my $local_models_dir_ = $work_dir_."/model_dir";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $results_base_dir ) ) { `mkdir -p $results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }
if ( ! ( -d $local_models_dir_ ) ) { `mkdir -p $local_models_dir_`; }

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
RunSimulationOnCandidates ( );

print "Results Dir: $results_base_dir \n";
ExecuteCommands();

# Remove intermediate files
for ( my $idx_=0; $idx_ <= $#intermediate_files_; $idx_++){
	my $cmd_ = "rm -rf $intermediate_files_[$idx_]";
	print $main_log_file_handle_ "$cmd_\n";
	`$cmd_`;	
}

# close log file
$main_log_file_handle_->close;

exit ( 0 );
# end script


sub MakeParamAndModelFiles
{
    @param_filevec_ = PermuteParams ( $orig_param_filename_, $local_params_dir_ );    

    for ( my $t_paramfile_idx_ = 0 ; $t_paramfile_idx_ < scalar ( @param_filevec_ ) ; $t_paramfile_idx_ ++ )
    {
	my $t_model_filename_ = $local_models_dir_."/model_".$t_paramfile_idx_;
	`$MODELSCRIPTS_DIR/create_options_modelfile.pl $t_model_filename_ $model_filename_ $param_filevec_[$t_paramfile_idx_] $index_`;
	push ( @model_filevec_, $t_model_filename_ );
    }
}

# For each model file, we create a strategy file
sub MakeStrategyFiles 
{
  print $main_log_file_handle_ "\nMakeStrategyFiles\n\n";

  my $strategy_progid_ = 100;
  my $ivadaptor_ = "kMethodBlackScholes";

  for (my $model_file_index_ = 0; $model_file_index_ <= $#model_filevec_ ; $model_file_index_++, $strategy_progid_++) 
  {
      my $this_model_filename_ = $model_filevec_[$model_file_index_];

      if ( ExistsWithSize ( $this_model_filename_ ) )
      {
          my $this_strategy_filebase_ = "strat_".$model_file_index_."_".$strategy_progid_ ;
          my $this_strategy_filename_ = $local_strats_dir_."/".$this_strategy_filebase_;

          my $exec_cmd="$MODELSCRIPTS_DIR/create_options_strategyfile.pl $this_strategy_filename_ $strategyname_ $this_model_filename_ $ivadaptor_ $trading_start_hhmm_ $trading_end_hhmm_ $strategy_progid_";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;

	  if ( ExistsWithSize (  $this_strategy_filename_ ) )
	  {
	      push ( @strategy_filevec_, $this_strategy_filename_ ); 
	  }
      }
      else
      {
          print $main_log_file_handle_ "Skipping model ".$this_model_filename_."\n";
      }
  }
}


# stats per underlying NSE_SHC_FUT0 # Primary
# stats per option NSE_SHC_C0_A # to add any contract specific params
# stats per strategy NSE_SHC_OPT0 # if we use same param for all strats

sub RunSimulationOnCandidates
{
  print $main_log_file_handle_ "\n\n RunSimulationOnCandidates\n\n";

  my @non_unique_results_filevec_ = ( );

  my @unique_sim_id_list_ = ( );
  
  my @tradingdate_list_ = ( );
  my @temp_strategy_list_file_index_list_ = ( );
  my @temp_strategy_list_file_list_ = ( );
  my @temp_strategy_cat_file_list_ = ( );
  my @temp_strategy_output_file_list_ = ( );
  my @temp_strategy_log_file_list_ = ( );
  my @temp_strategy_pnl_stats_file_list_ = ( );

# generate a list of commands which are unique , 
# independent from each other and can be safely run in parallel.

  my $tradingdate_ = $trading_end_yyyymmdd_;
  for ( my $i = 0 ; $i < scalar ( @trading_days_vec_ ) ; $i ++ )
  {
      $tradingdate_ = $trading_days_vec_[$i];

      my $strategy_filevec_index_ = 0;
      while ( $strategy_filevec_index_ <= $#strategy_filevec_ )
      {
	      my $this_strategy_filename_ = $strategy_filevec_ [ $strategy_filevec_index_ ];

# this one has strategy file name in it
	      my $temp_strategy_list_file_ = $shared_work_dir_."/temp_strategy_list_file_".$tradingdate_."_".$strategy_filevec_index_.".txt";
	      open ( TSLF , ">" , $temp_strategy_list_file_ ) or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );
	      print TSLF $this_strategy_filename_."\n";
	      close TSLF;
	      my $unique_sim_id_ = GetGlobalUniqueId (); # Get a concurrency safe id.
# this one has strategy line in it ( why ? )
		      my $temp_strategy_catted_file_ = $shared_work_dir_."/temp_strategy_cat_file_".$tradingdate_."_".$strategy_filevec_index_.".txt";
	      `cat $this_strategy_filename_ | awk '{ \$5 = $unique_sim_id_ ; print \$0 }' > $temp_strategy_catted_file_`;

	      $strategy_filevec_index_ ++;
	      my ($tradingdateyyyy_, $tradingdatemm_, $tradingdatedd_) = BreakDateYYYYMMDD ( $tradingdate_ );
	      my $resultsfilename_ = $results_base_dir."/".$tradingdateyyyy_."/".$tradingdatemm_."/".$tradingdatedd_."/results_database.txt";

	      my $resultsfilepathdir_ = dirname ( $resultsfilename_ ); chomp ( $resultsfilepathdir_ );
	      if ( ! ( -d $resultsfilepathdir_ ) ) 
	      {
		      `mkdir -p $resultsfilepathdir_`; 
	      }
	      if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

	      my $exec_cmd_ = "$MODELSCRIPTS_DIR/single_options_permute.pl $temp_strategy_catted_file_ $temp_strategy_list_file_ $unique_sim_id_ $tradingdate_ $results_base_dir ";
	      #print "$exec_cmd_ \n";
	      #my @output_lines_ = `$exec_cmd_`;
	      #print join("\n", @output_lines_)."\n";
	      push(@independent_parallel_commands_, $exec_cmd_);

      }
  }
}

sub ExecuteCommands {
	if ( $USE_DISTRIBUTED ) {
		my $commands_file_id_  = `date +%N`;
		chomp( $commands_file_id_ );
		my $commands_file_ = $SHARED_LOCATION."/".$commands_file_id_;
		my $commands_file_handle_ = FileHandle->new;
		$commands_file_handle_->open ( "> $commands_file_ " ) or PrintStacktraceAndDie ( "Could not open $commands_file_ for writing\n" );
		print $commands_file_handle_ "$_ \n" for @independent_parallel_commands_;
		close $commands_file_handle_;
		my $dist_exec_cmd_ = "";
		$dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $commands_file_ -s 1";

		print "Command executed: $dist_exec_cmd_ \n";
		my @output_lines_ = `$dist_exec_cmd_`;
		chomp( @output_lines_ );
		print "$_ \n" for @output_lines_;
	}
	else {
		foreach my $command ( @independent_parallel_commands_ ) {
			my @output_lines_ = `$command`;
			my $return_val_ = $?;
			if ( $return_val_ != 0 || $DEBUG == 1 ) {
				print "Output: \n".join ( "", @output_lines_ )."\n";
			}
		}
	}
}
