#!/usr/bin/perl

# \file ModelScripts/stir_parallel_find_best_params_permute_for_strat.pl
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
struct ( 'ResultLine', { pnl_ => '$', pnl_adj_dd_ => '$' , volume_ => '$' } );
struct ( 'MetricLine', { cmedpnl_ => '$', cmedvol_ => '$' , sharpe_ => '$' } );

package main;

sub MakeStrategyFiles ; # takes input_stratfile and makes strategyfiles corresponding to different scale constants
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub SummarizeLocalResultsAndChoose ; # from the files created in the local_results_base_dir choose the best ones to send to pool
sub FindMapStringFromStrat ;  # generate map string from the strat name

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR="/home/dvctrader/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
#my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $LIVE_BIN_DIR=$BIN_DIR;

if ( $USER eq "ankit" || $USER eq "anshul")
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

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
require "$GENPERLLIB_DIR/get_unique_sim_id_from_stir_cat_file.pl"; # GetUniqueSimIdFromStirCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/permute_params_stir.pl"; # PermuteParams
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_base_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratBaseExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_STRAT_FILES_IN_ONE_SIM = 10; # please work on optimizing this value
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

# start
my $USAGE="$0 shortcode param_file_permutations_list start_date end_date strategy_file_name [max_iterations]";

sub GetExecLogicFromCode
{
    my $exec_logic_code_ = shift;
    my @StandardExecLogics = ( "DirectionalAggressiveTrading",
			       "DirectionalInterventionAggressiveTrading",
			       "DirectionalInterventionLogisticTrading",
			       "DirectionalLogisticTrading",
			       "DirectionalPairAggressiveTrading",
			       "PriceBasedAggressiveTrading",
			       "PriceBasedInterventionAggressiveTrading",
			       "PriceBasedSecurityAggressiveTrading",
			       "PriceBasedTrading",
			       "PriceBasedVolTrading",
			       "PriceBasedScalper",
			       "PricePairBasedAggressiveTrading", 
                               "ReturnsBasedAggressiveTrading" ) ;
    foreach my $st_exec_logic_( @StandardExecLogics ) {
	my $t_ = $st_exec_logic_; 
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
	$t_ =~ s/[a-z]//g ;
	# print "$t_ $st_exec_logic_\n" ;
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
    }
    PrintStacktraceAndDie ( "Wrong ExecLogic Code: $exec_logic_code_\n" );
}

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $permute_param_list_ = $ARGV[1];
#my $orig_param_filename_ = $ARGV[1];
#my $param_file_index_in_strat_ = $ARGV[2];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[3] );
my $strategy_file_name_ = $ARGV[4] ;
my $MAXITER = 30;
if ( $#ARGV >= 5 )
{
  $MAXITER = $ARGV [ 5 ];
}

#my $min_volume_ = -1;
#my $sort_algo_ = "kCNAPnlAverage";
#if ( $#ARGV >= 5 )
#{
#    $min_volume_ = $ARGV [ 5 ];
#    $sort_algo_ = $ARGV [ 6 ];
#}

$FBPA_WORK_DIR = $FBPA_WORK_DIR.$shortcode_."/";

my $trading_start_hhmm_ = "";
my $trading_end_hhmm_ = "";

my $delete_intermediate_files_ = 1;

my $num_files_to_choose_ = 1;
my $min_pnl_per_contract_to_allow_ = -1.10 ;
my $min_volume_to_allow_ = 100; 

my $iter = 0;
my %strat2param_;
my %param2string_;
my @params_tobe_permuted_ = ();
my @params_tobe_permuted_index_ = ();
my $params_count_ = 0;
my @param_filevec_stir_ = ();
my @new_strategy_filevec_ = ();
my @computed_strategy_filevec_ = ();
my %param_to_resultvec_;
my %param_to_cmedpnl_;
my %param_to_cmedvolume_;
my %param_to_sharpe_;
my $next_strategy_filename_ = "INVALID";
my @intermediate_files_ = ();
my $tgt_tmp_dir_ = "0";

my %strat_pnl_map_;
my %strat_string2path_;
my @strats_as_iter_orig = ();

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

print "$work_dir_\n";
my $local_results_base_dir = ""; 
my $local_strats_dir_ = "";
my $temp_strat_filename_ = "";
my $local_params_dir_ = "";
my $full_strategy_filename_ = "";
my $same_strat_param_ = "";
my $local_orig_iter_param_ = $work_dir_."/orig_iter_param";
my $local_orig_iter_strat_ = $work_dir_."/orig_iter_strat";
my $this_iter_directory = "";
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_orig_iter_param_ ) )  { `mkdir -p $local_orig_iter_param_`; }
if ( ! ( -d $local_orig_iter_strat_ ) )  { `mkdir -p $local_orig_iter_strat_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

# Get param files and model files
ProcessListArgs ( );
GetParamsToBePermuted ( );

RunPermuteOptimizationIter ( );
# among the candidates choose the best
# end script
$main_log_file_handle_->close;

exit ( 0 );

sub ProcessListArgs
{
    print $main_log_file_handle_ "\nProcessListArgs\n\n";

    print $main_log_file_handle_ "\n$strategy_file_name_\n";
    #models
    my $full_stir_strategy_filename_ = "";
    if ( ExistsWithSize ( $strategy_file_name_ ) )
    {
	$full_stir_strategy_filename_ = $strategy_file_name_;
    }   
    else
    {
	print $main_log_file_handle_ "Strategy not found\n";
        exit ( 1 );        
    }

    print $main_log_file_handle_ "\n$full_stir_strategy_filename_\n";  
	
    
    my $exec_cmd_ = "cat $full_stir_strategy_filename_ | awk '{ print \$2}'";
    $full_strategy_filename_ = `$exec_cmd_`;
    chomp( $full_strategy_filename_ ); 
}

sub GetParamsToBePermuted
{
  print $main_log_file_handle_ "\nReadingParamLists $permute_param_list_ \n";
  
  open PARAMLIST, "+< $permute_param_list_ " or PrintStacktraceAndDie ( "$0 could not open $permute_param_list_\n");

  while ( my $thisline_ = <PARAMLIST> )
  {
    chomp ( $thisline_ );
    print $main_log_file_handle_ "$thisline_\n"; #logging for later

    my @param_and_index_ = split ( ' ', $thisline_ );

    push ( @params_tobe_permuted_ , $param_and_index_[0] );
    push ( @params_tobe_permuted_index_ , $param_and_index_[1] );
  }
  $params_count_ = @params_tobe_permuted_index_ ;
}

sub RunPermuteOptimizationIter
{
  $iter = 0;
  $next_strategy_filename_ = $full_strategy_filename_;  # "INVALID" ;

  while ( $iter < $MAXITER )
  {
    %param2string_ = ();
    %strat2param_ = ();
    @param_filevec_stir_ = ();
    @new_strategy_filevec_ = ();
    @computed_strategy_filevec_ = ();
    %param_to_resultvec_ = ();
    %param_to_cmedpnl_ = ();
    %param_to_cmedvolume_ = ();
    %param_to_sharpe_ = ();
    @intermediate_files_ = ();

    print $main_log_file_handle_ "\n\n============= ITER $iter ========================\n\n";
    
    $this_iter_directory = $work_dir_."/iter_".$iter;
    if ( ! ( -d $this_iter_directory ) ) { `mkdir -p $this_iter_directory `; }
    $tgt_tmp_dir_ = $this_iter_directory."/tmp_files";
    if ( !( -d $tgt_tmp_dir_ ) ) { `mkdir -p $tgt_tmp_dir_`; }


    $local_results_base_dir = $this_iter_directory."/local_results_base_dir";
    $local_strats_dir_ = $this_iter_directory."/strats_dir";
    $temp_strat_filename_ = $local_strats_dir_."/temp_strat_file";
    $local_params_dir_ = $this_iter_directory."/params_dir";
    if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
    if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
    if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }

    my $permute_orig_strat_ = $next_strategy_filename_ ;
    if ( $iter == 0 ) { $permute_orig_strat_ = "INVALID"; }

    @param_filevec_stir_ = PermuteParamsStir ( $permute_orig_strat_, \@params_tobe_permuted_index_, \@params_tobe_permuted_, $local_params_dir_ );
    print $main_log_file_handle_ "param_filevec_stir_ = ".$#param_filevec_stir_."\n";

    if ( $#param_filevec_stir_ < 0 )
    {
      print "Exiting due to 0 param_filevec_stir_\n";
      print $main_log_file_handle_ "Exiting due to 0 param_filevec_stir_\n";
      return;
    }

    for ( my $i = 0 ; $i < $params_count_ ; $i ++ )
    {
      my @param_filevec_  = @{$param_filevec_stir_ [ $i ] };
      print $main_log_file_handle_ "index: $params_tobe_permuted_[$i] , param_filevec_ = ".$#param_filevec_."\n";
    }

# From the given arguments create the strategy files for simulations
    MakeStrategyFiles ( );

# Run simulations on the created strategy files
    RunSimulationOnCandidates ( );

# among the candidates choose the best
    $next_strategy_filename_ = SummarizeLocalResultsAndChoose ( );

#    print $main_log_file_handle_ "Moving to next iter with strat: $next_strategy_filename_ \n";
#    `sed -i s,$work_dir_/params_dir,$this_iter_directory/params_dir,g $local_strats_dir_/*`;
#    `sed -i s,$work_dir_/strats_dir,$this_iter_directory/strats_dir,g $local_strats_dir_/*`;
#    `mv $local_strats_dir_ $this_iter_directory/`;
#    `mv $local_params_dir_ $this_iter_directory/`;
#    `mv $local_results_base_dir $this_iter_directory/`;
#    if ( ! $delete_intermediate_files_ ) { `sed -i s,$work_dir_,$this_iter_directory,g $tgt_tmp_dir_/*`; }
    $iter++;
  }

# Set the original params as the result
}

# For each param file & for each model file, we create a strategy file
sub MakeStrategyFiles 
{
    print $main_log_file_handle_ "\nMakeStrategyFiles\n\n";

    my $strategy_progid_ = 1001;

    my @orig_param_string_ = ();

    if ( $iter == 0 )
    {
      my $next_strat1_ = $local_strats_dir_."/orig_strat";
      `cp $next_strategy_filename_ $next_strat1_`;
      $next_strategy_filename_ = $next_strat1_ ;
    }
    for ( my $i = 0 ; $i < $params_count_ ; $i ++ )
    {
      my @param_filevec_  = @{$param_filevec_stir_ [ $i ] };  
      my $this_param_filename_ = $param_filevec_[0];
      my $this_param_string_ = ParamToString ( $this_param_filename_ );
      my $param_file_index_in_strat_ = $params_tobe_permuted_index_[$i];
      if ( $iter == 0 )  ## copying the 1st params of the permute _params into the orig_strat for iter 0
      {
        my $exec_cmd_ = "cat $next_strategy_filename_ | awk '{if( NR == ($param_file_index_in_strat_ + 1 ) ) { \$4= \"$this_param_filename_\" } print \$0 }' > $temp_strat_filename_";
        `$exec_cmd_`;
        `cp $temp_strat_filename_ $next_strategy_filename_`;
      }
      push ( @orig_param_string_ , $this_param_string_ );
    }

    for ( my $i = 0 ; $i < $params_count_ ; $i ++ )
    {
      my $param_file_index_in_strat_ = $params_tobe_permuted_index_[$i];		
      my @param_filevec_  = @{$param_filevec_stir_ [ $i ] };
      my $start_index_ = 1;
      if ($i == 0) { $start_index_ = 0; $same_strat_param_ = $param_filevec_[0]; }
      
      for (my $param_file_index_ = $start_index_; $param_file_index_ <= $#param_filevec_ ; $param_file_index_++) 
      { # For each param
  		
  	my $this_param_filename_ = $param_filevec_[$param_file_index_];

        my $this_param_string_ = ParamToString ( $this_param_filename_ );
        my @this_strat_str_vec_ = @orig_param_string_ ;
        $this_strat_str_vec_ [$params_count_] = $this_param_string_;
        my $this_strat_string_ = join ( '\|' , @this_strat_str_vec_ );

  	if ( ExistsWithSize ( $this_param_filename_ ) )
  	{
          my $this_strategy_im_filebase_ = "strat_im_i".$i."p".$param_file_index_."_".$strategy_progid_ ;
          my $this_strategy_im_filename_ = $local_strats_dir_."/".$this_strategy_im_filebase_;

  	  my $this_strategy_filebase_ = "strat_i".$i."p".$param_file_index_."_".$strategy_progid_;
  	  my $this_strategy_filename_ = $local_strats_dir_."/".$this_strategy_filebase_;
  	          
  	  my $exec_cmd_ = "cat $next_strategy_filename_ | awk '{if( NR == ($param_file_index_in_strat_ + 1 ) ) { \$4= \"$this_param_filename_\" } print \$0 }' > $temp_strat_filename_";
#  print $main_log_file_handle_ "$exec_cmd_\n";
  	  `$exec_cmd_`;
  	  $exec_cmd_ = "cat $temp_strat_filename_ | awk '{if ( NR == 1 ) { \$7=$strategy_progid_ } print \$0 }' > $this_strategy_im_filename_";
# 	  print $main_log_file_handle_ "$exec_cmd_\n";
  	  `$exec_cmd_`;
  	
          my $random_id_ = "$strategy_progid_";	
          $exec_cmd_ = "echo STRUCTURED_STRATEGYLINE $this_strategy_im_filename_ $random_id_ > $this_strategy_filename_";
  	  `$exec_cmd_`; 
  	  $strategy_progid_++; # uniqueness of progid ensures that we can run them in sim together
  
  	  if ( ExistsWithSize (  $this_strategy_filename_ ) )
  	  {
  	      $strat2param_ { $this_strategy_im_filebase_ } = $this_param_filename_ ;
              $param2string_ { $this_param_filename_ } = $this_strat_string_ ;
              if ( ! exists $strat_pnl_map_{$this_strat_string_} ) {
                push ( @new_strategy_filevec_ , $this_strategy_filename_ );
                $strat_string2path_{$this_strat_string_} = $this_strategy_filename_ ;
              }
              else { push ( @computed_strategy_filevec_ , $this_strategy_im_filebase_ ); }
  	  }
  	}
  	else
  	{
  	    print $main_log_file_handle_ "Skipping param ".$this_param_filename_."\n";
  	}
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

    # generate a list of commands which are unique , 
    # independent from each other and can be safely run in parallel.

    my $tradingdate_ = $trading_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;

    for ( my $i = 0 ; $i < $max_days_at_a_time_ ; $i ++ )
    {
      print $main_log_file_handle_ "RunSimulation date: $tradingdate_ , noOfStrategies: $#new_strategy_filevec_\n";      
      if ( SkipWeirdDate ( $tradingdate_ ) ||
	     IsDateHoliday ( $tradingdate_ ) ||
	     IsProductHoliday ( $tradingdate_ , $shortcode_ ) ||
	     NoDataDate ( $tradingdate_ ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_ , 1 );
	    next;
	}

	if ( ( 0 && ! ValidDate ( $tradingdate_ ) ) ||
	     $tradingdate_ < $trading_start_yyyymmdd_ )
	{
          last;
	}

        my $temp_strategy_list_file_index_ = 0;

	for ( my $strategy_filevec_index_ = 0 ; $strategy_filevec_index_ <= $#new_strategy_filevec_ ; )
	{
            my $temp_strategy_list_file_ = $this_iter_directory."/temp_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
	    my $temp_strategy_cat_file_ = $this_iter_directory."/temp_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
            my $temp_strategy_output_file_ = $this_iter_directory."/temp_strategy_output_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

            open ( TSLF , ">" , $temp_strategy_list_file_ ) or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );
            for ( my $num_files_ = 0 ; $num_files_ < $MAX_STRAT_FILES_IN_ONE_SIM && $strategy_filevec_index_ <= $#new_strategy_filevec_ ; $num_files_ ++ )
            {
                my $this_strategy_filename_ = $new_strategy_filevec_ [ $strategy_filevec_index_ ];
                $strategy_filevec_index_ ++;

                print TSLF $this_strategy_filename_."\n";
                `cat $this_strategy_filename_ >> $temp_strategy_cat_file_`;
            }
	    
            close ( TSLF );

            my $unique_sim_id_ = GetGlobalUniqueId ( ); # Get a concurrency safe id.

            my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
            my $exec_cmd_ = $LIVE_BIN_DIR."/sim_strategy SIM ".$temp_strategy_cat_file_." ".$unique_sim_id_." ".$tradingdate_." ".$market_model_index_." ADD_DBG_CODE -1 > ".$temp_strategy_output_file_." 2>/dev/null";
#            my $exec_cmd_ = $HOME_DIR."/sim_strategy5 SIM ".$temp_strategy_cat_file_." ".$unique_sim_id_." ".$tradingdate_." ".$market_model_index_." ADD_DBG_CODE -1 > ".$temp_strategy_output_file_." 2>/dev/null";

	    print $main_log_file_handle_ "$exec_cmd_\n";

            push ( @unique_sim_id_list_ , $unique_sim_id_ );
            push ( @independent_parallel_commands_ , $exec_cmd_ );

            push ( @tradingdate_list_ ,$tradingdate_ );
            push ( @temp_strategy_list_file_index_list_ , $temp_strategy_list_file_index_ ); $temp_strategy_list_file_index_ ++;

            push ( @temp_strategy_list_file_list_ , $temp_strategy_list_file_ );
            push ( @temp_strategy_cat_file_list_ , $temp_strategy_cat_file_ );
            push ( @temp_strategy_output_file_list_ , $temp_strategy_output_file_ );
	}

	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_ , 1 );
    }

    # process the list of commands , processing MAX_CORES_TO_USE_IN_PARALLEL at once
    for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; )
    {
	my @output_files_to_poll_this_run_ = ( );

	my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL  );
	for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ )
	{
	    push ( @output_files_to_poll_this_run_ , $temp_strategy_output_file_list_ [ $command_index_ ] );

	    { # empty the output result file.
		my $exec_cmd_ = "> ".$temp_strategy_output_file_list_ [ $command_index_ ];
		`$exec_cmd_`;
	    }

	    print $main_log_file_handle_ $independent_parallel_commands_ [ $command_index_ ]."\n";
	    my $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ]." &";
	    `$exec_cmd_`;

            my $t_unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ]; 
            $exec_cmd_ = "rm -f /spare/local/logs/tradelogs/logs*$t_unique_sim_id_"; 

	    $command_index_ ++;
	    sleep ( 1 );
	}

	while ( ! AllOutputFilesPopulatedLocal ( @output_files_to_poll_this_run_ ) )
	{ # there are still some sim-strats which haven't output SIMRESULT lines
	    my $result_ = AllOutputFilesPopulatedLocal ( @output_files_to_poll_this_run_ );	
	    sleep ( 1 );
	}
    }

    for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
    {
        my %unique_id_to_pnlstats_map_ = ( );
	
	my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];
	my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];

        my $temp_strategy_list_file_ = $temp_strategy_list_file_list_ [ $command_index_ ];        
	my $temp_strategy_cat_file_ = $temp_strategy_cat_file_list_ [ $command_index_ ];
	my $exec_cmd_ = "cat ".$temp_strategy_cat_file_;
	my $strategy_cat_file_lines_ = `$exec_cmd_`; chomp($strategy_cat_file_lines_);
	my @strategy_cat_file_line_vec_ = split('\n', $strategy_cat_file_lines_);


	my $temp_strategy_output_file_ = $temp_strategy_output_file_list_ [ $command_index_ ];
	$exec_cmd_ = "cat ".$temp_strategy_output_file_;
	my $tradeinit_output_lines_ = `$exec_cmd_`; chomp($tradeinit_output_lines_);
	my @tradeinit_output_line_vec_ = split('\n', $tradeinit_output_lines_);

	my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
	my @pnlstats_output_line_vec_ = ();
        my %pnlstats_output_map_ = ();
	if ( ExistsWithSize ( $this_trades_filename_ ) )
	{
	    my $exec_cmd_ = $MODELSCRIPTS_DIR."/get_pnl_stats_stir_2.pl $this_trades_filename_";
	    print $main_log_file_handle_ $exec_cmd_."\n";
	    my $pnlstats_output_lines_ = `$exec_cmd_`; chomp ( $pnlstats_output_lines_ );
	    @pnlstats_output_line_vec_ = split('\n', $pnlstats_output_lines_);
            foreach my $pnlstats_line ( @pnlstats_output_line_vec_ )
            {
              my @pwords_ = split ( ' ', $pnlstats_line ); chomp ( @pwords_ );
              my $id_ = $pwords_[0];
              splice ( @pwords_ , 0 , 1 );
              $pnlstats_output_map_{$id_} = join( ' ' , @pwords_ );
            }
            $exec_cmd_ = "echo $this_trades_filename_ | sed 's/trades/log/g'";
            my $t_logfilename_ = `$exec_cmd_`;
            chomp ( $t_logfilename_ );
            $exec_cmd_ = "rm -f $this_trades_filename_ $t_logfilename_";
            `$exec_cmd_`;
	}

	{
	    my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int ( $unique_sim_id_ );
	    `rm -f $this_tradeslogfilename_`;
	}

	#to check if pnlstats_output_line_vec_, tradeinit_output_line_vec_, strategy_cat_file_line_vec_ have same size
	if ( ($#tradeinit_output_line_vec_ != $#pnlstats_output_line_vec_) || ($#pnlstats_output_line_vec_ != $#strategy_cat_file_line_vec_) )
	{
		print $main_log_file_handle_ "somehow pnlstats_output_line_vec_, tradeinit_output_line_vec_, strategy_cat_file_line_vec_ dont't have same size, check files $temp_strategy_output_file_, $temp_strategy_cat_file_, $this_trades_filename_\n";
		next;
	}


	my $temp_results_list_file_ = `echo $temp_strategy_output_file_ | sed 's\/temp_strategy_output_file_\/temp_results_list_file_\/g'`; chomp($temp_results_list_file_);
	open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );

	for (my $t_strat_idx_ = 0 ; $t_strat_idx_ <= $#tradeinit_output_line_vec_ ; $t_strat_idx_++ )
	{
		my $strategy_cat_file_line_ = $strategy_cat_file_line_vec_[$t_strat_idx_]; chomp($strategy_cat_file_line_);
		my @strategy_cat_file_line_words_ = split(' ', $strategy_cat_file_line_);
		my $unique_strat_id_ = 	$strategy_cat_file_line_words_[2];

		my $pnlstats_output_line_ = $pnlstats_output_map_{$unique_strat_id_};
		my $tradeinit_output_line_ = $tradeinit_output_line_vec_[$t_strat_idx_]; chomp($tradeinit_output_line_);
		
		{
		    if ( $tradeinit_output_line_ =~ /SIMRESULT/ )
		    { # SIMRESULT pnl volume S B A I                                                                                                                                                                                               
			my @rwords_ = split ( ' ', $tradeinit_output_line_ );
			splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc                                                                                     
			my $remaining_simresult_line_ = join ( ' ', @rwords_ );
			if ( ( $rwords_[1] > 0 ) || # volume > 0                                                                                                                                                  
			     ( ( $shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day                                                                      
			{
			    printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $pnlstats_output_line_, $unique_strat_id_ ;
			    printf TRLF "%s %s %s\n",$remaining_simresult_line_,$pnlstats_output_line_, $unique_strat_id_;
			}
		    }
		}
	}

	close TRLF;		

	if ( ExistsWithSize ( $temp_results_list_file_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database.pl $temp_strategy_cat_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir";
	    print $main_log_file_handle_ "$exec_cmd\n";
	    my $this_local_results_database_file_ = `$exec_cmd`;
	    push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
	}

         if ( $delete_intermediate_files_ )
         {
           if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
           if ( -e $temp_strategy_list_file_ ) { `rm -f $temp_strategy_list_file_`; }
           if ( -e $temp_results_list_file_ ) { `rm -f $temp_results_list_file_`; }
           if ( -e $temp_strategy_output_file_ ) { `rm -f $temp_strategy_output_file_`; } 
         }
         else {
           if ( -e $temp_strategy_cat_file_ ) { `mv $temp_strategy_cat_file_ $tgt_tmp_dir_`; }
           if ( -e $temp_strategy_list_file_ ) { `mv $temp_strategy_list_file_ $tgt_tmp_dir_`; }
           if ( -e $temp_results_list_file_ ) { `mv $temp_results_list_file_ $tgt_tmp_dir_`; }
           if ( -e $temp_strategy_output_file_ ) { `mv $temp_strategy_output_file_ $tgt_tmp_dir_`; }
         }
    }

    @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ );
}

sub SummarizeLocalResultsAndChoose
{
  print $main_log_file_handle_ "\n\n SummarizeLocalResultsAndChoose \n\n No. of Unique Results Files: ".($#unique_results_filevec_ + 1)."\n"; 
  my %param_to_stratpath_ = (); 
  for ( my $i = 0 ; $i <= $#unique_results_filevec_; $i++ )
    {
	my $t_results_filename_ = $unique_results_filevec_[$i];
#        print $main_log_file_handle_ "ResultFile: $t_results_filename_\n";
	my $is_open_ = open RESULTSFILEHANDLE, "< $t_results_filename_ ";
	if ( ! $is_open_ )
	{
	    print STDERR "$0 SummarizeLocalResultsAndChoose: Could not open results_file $t_results_filename_\n" ;
	}
	else
	{
	    while ( my $result_line_ = <RESULTSFILEHANDLE> )
	    {
		my @result_line_words_ = split ( ' ', $result_line_ );
		if ( $#result_line_words_ >= 17 )
		{ # dependant on format in results
		    # 0 is stratfilename
		    # 2 is pnl
		    # 3 is volume
		    # $#result_line_words_ - 4 is drawdown
		    my $strategy_filebase_ = $result_line_words_[0];
		    my $pnl_ = $result_line_words_[2];
		    my $volume_ = $result_line_words_[3];
		    my $drawdown_ = $result_line_words_[$#result_line_words_ - 4];

#                    print $main_log_file_handle_ "StratFile: $strategy_filebase_\n";
		    if ( exists $strat2param_{$strategy_filebase_} )
		    {
			my $this_param_file_ = $strat2param_{$strategy_filebase_} ;
#                        print $main_log_file_handle_ "ParamFile: $this_param_file_\n";

			my $new_result_line_ = new ResultLine;
			$new_result_line_->pnl_ ( $pnl_ ); # absolute pnl
			$new_result_line_->pnl_adj_dd_ ( $pnl_ - ( 0.33 * $drawdown_ ) ); # conservative pnl
			$new_result_line_->volume_ ( $volume_ );
			
			if ( exists $param_to_resultvec_{$this_param_file_} ) 
			{ 
			    my $scalar_ref_ = $param_to_resultvec_{$this_param_file_};
			    push ( @$scalar_ref_, $new_result_line_ );
			} 
			else 
			{ # seeing this param for the first time
			    my @result_vec_ = ();
			    push ( @result_vec_, $new_result_line_ );
			    $param_to_resultvec_{$this_param_file_} = [ @result_vec_ ] ;
                            $param_to_stratpath_{$this_param_file_} = $local_strats_dir_."/".$strategy_filebase_;
			}
			
		    }
		}
	    }
	    close RESULTSFILEHANDLE ;
	}
    }

    foreach my $this_param_file_ ( keys %param_to_resultvec_ )
    {
	my @pnl_vec_ = ();
        my @pnl_adj_dd_vec_ = ();
	my @volume_vec_ = ();
	
	my $scalar_ref_resultvec_ = $param_to_resultvec_{$this_param_file_};
	for ( my $resultvec_index_ = 0 ; $resultvec_index_ <= $#$scalar_ref_resultvec_ ; $resultvec_index_ ++ )
	{ # foreach result vec item
	    my $this_result_line_ = $$scalar_ref_resultvec_[$resultvec_index_];
 	    push ( @pnl_vec_, $this_result_line_->pnl_() ) ;
            push ( @pnl_adj_dd_vec_, $this_result_line_->pnl_adj_dd_() );
	    push ( @volume_vec_, $this_result_line_->volume_() ) ;
	}

	my $cmed_pnl_ = GetAverage ( \@pnl_adj_dd_vec_ );
	$param_to_cmedpnl_{$this_param_file_} = $cmed_pnl_;

	my $cmed_volume_ = GetAverage ( \@volume_vec_ );
	$param_to_cmedvolume_{$this_param_file_} = $cmed_volume_;

        my $avg_pnl_ = GetAverage ( \@pnl_vec_ );
        my $sd_pnl_ = GetStdev ( \@pnl_vec_ );
        my $sharpe_ = $avg_pnl_ / $sd_pnl_ ;
        $param_to_sharpe_{$this_param_file_} = $sharpe_;

        my $strat_metric_ = new MetricLine;
        $strat_metric_->cmedpnl_ ( $cmed_pnl_ );
        $strat_metric_->cmedvol_ ( $cmed_volume_ );
        $strat_metric_->sharpe_ ( $sharpe_ );

        my $param_stratstring_ = $param2string_{$this_param_file_};
        $strat_pnl_map_{$param_stratstring_} = $strat_metric_;
    }

    foreach my $this_old_strat_ ( @computed_strategy_filevec_ )
    {
      my $this_param_file_ = $strat2param_{$this_old_strat_};
      my $param_stratstring_ = $param2string_{$this_param_file_};
      my $strat_metric_ = $strat_pnl_map_{$param_stratstring_};
      $param_to_cmedpnl_{$this_param_file_} = $strat_metric_->cmedpnl_();
      $param_to_cmedvolume_{$this_param_file_} = $strat_metric_->cmedvol_();
      $param_to_sharpe_{$this_param_file_} = $strat_metric_->sharpe_();
    }

    my @params_sorted_by_cmedpnl_ = sort { $param_to_cmedpnl_{$b} <=> $param_to_cmedpnl_{$a} } keys %param_to_cmedpnl_;

    printf $main_log_file_handle_ "#PARAM    CMEDPNL   CMEDVOL SHARPE  ### This IterPerformance\n";
    for my $this_param_file_ ( @params_sorted_by_cmedpnl_ ) 
    {
	printf $main_log_file_handle_ "%s %d %d %.2f\n", $this_param_file_, $param_to_cmedpnl_{$this_param_file_}, $param_to_cmedvolume_{$this_param_file_}, $param_to_sharpe_{$this_param_file_};
    }

    my @strats_sorted_by_pnl_ = sort { $strat_pnl_map_{$b}->cmedpnl_() <=> $strat_pnl_map_{$a}->cmedpnl_() } keys %strat_pnl_map_;

    printf $main_log_file_handle_ "\n\n#STRATPATH CMEDPNL CMEDVOL SHARPE  ### TotalPerformance\n";
    for my $istr ( 0..9 )
    {
      my $stratstr = $strats_sorted_by_pnl_[$istr];
      my $strat_metric_ = $strat_pnl_map_{$stratstr};
      printf $main_log_file_handle_ "%s %d %d %.2f\n", $strat_string2path_ {$stratstr}, $strat_metric_->cmedpnl_(), $strat_metric_->cmedvol_(), $strat_metric_->sharpe_();
    }

    my $next_stir_strat_path_ = "";
    my $next_strat_string_ = "";

    foreach my $stratstr ( @strats_sorted_by_pnl_ )
    {
      if ( ! ( $stratstr ~~ @strats_as_iter_orig ) ) {
        $next_strat_string_ = $stratstr;
        $next_stir_strat_path_ = $strat_string2path_ {$stratstr};
        last;
      }
    }

    if ( $next_stir_strat_path_ eq "" )
    {
      printf $main_log_file_handle_ "No other non traversed Strategy left. Exiting... \n";
      exit(0);
    }

    push ( @strats_as_iter_orig , $next_strat_string_ );
    print "Next Strategy: $next_stir_strat_path_ \n";

    my $next_strategy_path_ = `awk '{print \$2}' $next_stir_strat_path_`; chomp( $next_strategy_path_ );

    print $main_log_file_handle_ "\nChoosing the next untraversed Strategy: $next_stir_strat_path_, $next_strategy_path_, pnl: $strat_pnl_map_{$next_strat_string_} for the next iteration \n\n";
    print $main_log_file_handle_ "The param_set_string for the $next_stir_strat_path_ :\n $next_strat_string_ \n";

    return $next_strategy_path_;
#    if ( $next_best_param_ eq $same_strat_param_ && $#params_sorted_by_cmedpnl_ > 0)
#    {
#      print $main_log_file_handle_ "Skipping best param set because that was the iter-original param set.\n ITER: $iter BEST_PARAMFILE: $next_best_param_ \n";
#      $next_best_param_ = $params_sorted_by_cmedpnl_[1];
#    }
##    print join("\n",keys %param_to_stratpath_)."\n";
#    print "Next param: ".$next_best_param_."\n";
#
#    my $next_strategy_filename_ = $param_to_stratpath_{$next_best_param_};
#    my $param_basename_ = `basename $next_best_param_`; chomp($param_basename_);
#    print $main_log_file_handle_ "\n\nchoosing PARAMFILE: $param_basename_ for the next iteration , strat: $next_strategy_path_ \n\n";
#    my $param_dirname_ = `dirname $next_best_param_`; chomp($param_dirname_);
#    my $param_fld_indx_ = `basename $param_dirname_`; chomp($param_fld_indx_);
#    my $param_tgt_dirname_ = $local_orig_iter_param_."/".$param_fld_indx_;
#    if ( ! ( -d $param_tgt_dirname_ ) ) { `mkdir -p $param_tgt_dirname_`; }
#
#    my $strat_basename_ = `basename $next_strategy_filename_`; chomp($strat_basename_);
#    my $next_strategy_path_ = $local_orig_iter_strat_."/".$strat_basename_;
#    `cp $next_best_param_ $param_tgt_dirname_`;
#    `cp $next_strategy_filename_ $local_orig_iter_strat_`;
#    my $exec_cmd_ = "sed -i s,$param_dirname_,$param_tgt_dirname_,g $next_strategy_path_" ;
#    `$exec_cmd_`;
}

sub ParamToString
{
  my $paramfile_ = $_[0];
  my $output_string_ = "";
  if ( -e $paramfile_ )
  {
    open ORIG_SRC_FILE_, "<", $paramfile_ or PrintStacktraceAndDie ( "Could not open file : $paramfile_\n" );

    while (my $line_ = <ORIG_SRC_FILE_>)
    {
      if (substr ($line_, 0, 1) eq '#')
      {
        next;
      }

      my @words_ = split (' ', $line_);

      if ($#words_ < 2)
      {
        next;
      }

      $output_string_ = $output_string_.",".$words_[1].":".$words_[2];
    }
  }
  return $output_string_;
}

sub AllOutputFilesPopulatedLocal 
{	
        my ( @output_files_to_poll_this_run_ ) = @_;
	for ( my $output_file_index_ = 0; $output_file_index_ <= $#output_files_to_poll_this_run_ ; $output_file_index_ ++ )
	{
		my $exec_cmd_ = "grep SIMRESULT ".$output_files_to_poll_this_run_ [ $output_file_index_ ]." 2>/dev/null | wc -l 2>/dev/null";
		my @exec_cmd_output_ = `$exec_cmd_`;

		chomp ( @exec_cmd_output_ );

		if ( $#exec_cmd_output_ >= 0 )
		{
			my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] );
			chomp ( @exec_cmd_output_words_ );
			if ( $#exec_cmd_output_words_ >= 0 )
			{				
				my $simresult_count_ = $exec_cmd_output_words_[ 0 ];
				if ( $simresult_count_ <= 0 )
				{
					return 0;
					last;
				}
			}
			else
			{
				return 0;
				last;
			}
		}
		else
		{
			return 0;
			last;
		}
	}	
	return 1;
}

