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
use Getopt::Long;
use Pod::Usage;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use Fcntl qw (:flock);
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use Term::ReadKey;
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

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to s

my ($feature_, $feature_aux_, $lb_perc_, $hb_hl_) = ("NA","NA",0,0); # AvgPrice NA 0 16; STDEV NA 0.2 0.3; VOL NA 0.3 HIGH

# references:
# http://perltricks.com/article/195/2015/10/21/Professional-scripts-are-a-snap-with-Getopt--Long/
# http://perldoc.perl.org/Getopt/Long.html

my $getopt_success = GetOptions (
				 "pfile|p=s"  => \(my $orig_param_filename_ ),
				 "sfile|s=s"  => \(my $strategy_file_name_ = ''),
				 "sdate|sd=s" => \(my $trading_start_yyyymmdd_str_ ),
				 "edate|ed=s" => \(my $trading_end_yyyymmdd_str_ = ''),
				 "evstr:s"    => \(my $event_string_ = "INVALID"),
				 "regindx:i"  => \(my $regime_index_ = -1),
				 "skipdf:s"   => \(my $skip_days_file_ = "INVALID"),
				 "feat:s"     => \$feature_,
				 "faux:s"     => \$feature_aux_,
				 "lbperc:i"   => \$lb_perc_,
				 "hdhl:i"     => \$hb_hl_,
				 "dist!"      => \(my $distributed_ = 0),
				 "grid!"      => \(my $using_grid_ = 1),
				 'help|?'     => \(my $help = 0),
				 'man'        => \(my $man = 0)) ;
pod2usage(-exitval => 0, -verbose => 1) if $help;
pod2usage(-exitval => 0, -verbose => 2) if $man;

=head1 NAME

This utility will try out many permutations of a paramfile and then it wioll see which one is the best.
  
=head1 SYNOPSIS

parllel_params_permute.pl --pfile pfile --sfile sfile --sdate sdate --edate edate [Options] 
  
=head1 OPTIONS
  
=over 8
  
=item B<-help>
  
Print a brief help message and exits.
  
=item B<-man>
  
Prints the manual page and exits.
  
=item B<-pfile>
  
This is the paramfile that we are processing
  
=item B<-sfile>
  
This is the stratfile that we are processing
  
=back
  
=head1 REQUIRED

-p paramfile -s stratfile -sd startdate -ed enddate

=head1 DESCRIPTION
  
B<This program> will read the given input param file, and from the startdate to enddate see which param works best for the given stratfile.
  
=cut

pod2usage(-verbose => 99, -sections => "REQUIRED") unless $orig_param_filename_;
#die ("$0 requires --pfile param_file_name") unless $orig_param_filename_;
#die ("$0 requires --sfile strategy_file_name") unless $strategy_file_name_;
#die ("$0 requires --sdate start date of analysis") unless $trading_start_yyyymmdd_str_;
#die ("$0 requires --edate end date of analysis") unless $trading_end_yyyymmdd_str_;

require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
require "$GENPERLLIB_DIR/search_script.pl"; # SearchScript
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; #GetFilteredDays
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
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/get_strat_traded_ezone.pl"; #GetStratTradedEzone
require "$GENPERLLIB_DIR/get_trading_exec.pl"; # GetTradingExec
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_base_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratBaseExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort
require "$GENPERLLIB_DIR/strat_utils.pl"; #GetParam
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/best_worker_pool.pl"; # GetBestWorkerPool
require "$GENPERLLIB_DIR/compute_pnl_grid.pl"; # ComputePnlGridStartEndDate ComputePnlGridDatesVec
require "$GENPERLLIB_DIR/date_utils.pl"; # GetTrainingDatesFromDB

my $MAX_STRAT_FILES_IN_ONE_SIM = 577; #why 577 ? generally we have multiples of 2 and 3 in params, 576 has only 2 and 3 as his factors
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $trading_start_yyyymmdd_str_ );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $trading_end_yyyymmdd_str_ );

if($using_grid_){
  if(not defined $ENV{'GRID_USERNAME'} or not defined $ENV{'GRID_PASSWORD'}){
    print "Enter username\n";
    my $grid_user = <STDIN>;
    chomp $grid_user;
    print "Enter password\n";
    ReadMode('noecho');
    chomp(my $grid_pass = <STDIN>);
    ReadMode('normal');
    $ENV{'GRID_USERNAME'}=$grid_user;
    $ENV{'GRID_PASSWORD'}=$grid_pass;
  }
}


#Variables for distributed (celery) version
my $DISTRIBUTED_EXECUTOR = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py";
my $max_sim_timeout = 1200;
my $celery_shared_location = "/media/shared/ephemeral17/temp_strats/";

my ( $trading_start_hhmm_, $trading_end_hhmm_ ) = GetStratStartEndHHMM ( $strategy_file_name_ ) ;
my ( $model_filename_, $t_param_filename_ ) = GetModelAndParamFileNames ( $strategy_file_name_ ) ;
my ( $shortcode_ , $strategyname_ ) = GetTradingExec ( $strategy_file_name_ ) ;
my $strategy_line_type_ = `cat $strategy_file_name_ | awk '{print \$1}' | head -n1`;
$strategy_line_type_ = substr($strategy_line_type_, 0, -1);


my $param_file_list_basename_ = basename ( $orig_param_filename_ );
$FBPA_WORK_DIR = $FBPA_WORK_DIR.$shortcode_."/".$trading_start_hhmm_."-".$trading_end_hhmm_."/".$strategyname_."/".$param_file_list_basename_."/";

my @trading_days_vec_ = ( );
if ( ! IsValidStartEndDate($trading_start_yyyymmdd_, $trading_end_yyyymmdd_) ) {
  if ( -f $trading_start_yyyymmdd_ ) {
    open DATESHANDLE, "< $trading_start_yyyymmdd_" or PrintStacktraceAndDie ( "Could not open file $trading_start_yyyymmdd_ for reading" );
    my @dates_from_file_ = <DATESHANDLE>;
    chomp @dates_from_file_;
    close DATESHANDLE;
    my %dates_from_file_map_ =map{$_ =>1} @dates_from_file_;
    @trading_days_vec_ = grep ( $dates_from_file_map_{$_}, GetTrainingDatesFromDB($dates_from_file_[0], $dates_from_file_[-1], "", "INVALIDFILE"));
  }
  else {
    print STDERR "Either Start/End Dates are invalid or start date is after end-date\n";
    exit(0);
  }
}
else {
  @trading_days_vec_ = GetTrainingDatesFromDB($trading_start_yyyymmdd_, $trading_end_yyyymmdd_, "", "INVALIDFILE");
  #@trading_days_vec_ = GetDatesFromStartDate( $shortcode_, $trading_start_yyyymmdd_, $trading_end_yyyymmdd_, "INVALIDFILE", 2000 );
}

if ( $feature_ ne "NA" ) {
  my @filtered_trading_day_vec_ = ();
  if ( $hb_hl_ eq "HIGH" || $hb_hl_ eq "LOW" ) {
    GetFilteredDays ( $shortcode_, \@trading_days_vec_, $lb_perc_, $hb_hl_, $feature_, $feature_aux_, \@filtered_trading_day_vec_, $trading_start_hhmm_, $trading_end_hhmm_ );
  } else {
    GetFilteredDaysOnSampleBounds ( $shortcode_, \@trading_days_vec_, $lb_perc_, $hb_hl_, $feature_, $feature_aux_, \@filtered_trading_day_vec_, $trading_start_hhmm_, $trading_end_hhmm_ );
    print $shortcode_." ".$lb_perc_." ".$hb_hl_." ".$feature_." ".$feature_aux_." ".$trading_start_hhmm_." ".$trading_end_hhmm_."\n";
  }
  @trading_days_vec_ = @filtered_trading_day_vec_;
}

if ( $event_string_ ne "INVALID" ) {
  my $events_filename_ = "~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2015_processed.txt";
  my $t_exec_cmd_ = "cat $events_filename_ | grep $event_string_ | awk '{print \$5}' | awk -F '_' '{print \$1}' | xargs";
  my $event_dates_line_ = `$t_exec_cmd_`;
  my @event_dates_ = split ( ' ', $event_dates_line_ );
  my %evdates_map_ = map { $_=>1 } @event_dates_;
  @trading_days_vec_ = grep { defined $evdates_map_{ $_ } } @trading_days_vec_;
}

my @skip_days_ = ( );
if ( $skip_days_file_ ne "INVALIDFILE" ) {
  if ( -f $skip_days_file_ ) {
    open DATESHANDLE, "< $skip_days_file_" or PrintStacktraceAndDie ( "Could not open file $skip_days_file_ for reading" );
    @skip_days_ = <DATESHANDLE>;
    chomp @skip_days_;
    close DATESHANDLE;
  }
}

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
for ( my $i = 0 ; $i < 30 ; $i ++ ) {
  if ( -d $work_dir_ ) {
    print STDERR "Surprising but this dir exists\n";
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    $work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_; 
  }
  else { last; }
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
for ( my $idx_=0; $idx_ <= $#intermediate_files_; $idx_++){
	my $cmd_ = "rm -rf $intermediate_files_[$idx_]";
	print $main_log_file_handle_ "$cmd_\n";
	`$cmd_`;	
}

$main_log_file_handle_->close;

exit ( 0 );

sub ProcessListArgs
{
#model
  push ( @model_filevec_ , $model_filename_ ) ;

#params
  if ( $regime_index_ <= 0 ) {    
    open (PARAM_FILE, $orig_param_filename_);
    my $is_regime_param_ = 0;
    if ( grep {/PARAMFILELIST/} <PARAM_FILE>) {
      $is_regime_param_ = 1;
    }
    close PARAM_FILE;

    if ( $is_regime_param_ > 0 ) {
     @param_filevec_ = PermuteParamsRegime ( $orig_param_filename_, $local_params_dir_ );
    } else {
     @param_filevec_ = PermuteParams ( $orig_param_filename_, $local_params_dir_ );
    } 
  }
  else {
    @param_filevec_ = ParallelPermuteParamsRegime ( $regime_index_, $t_param_filename_, $orig_param_filename_, $local_params_dir_ );
  } 
  print $main_log_file_handle_ "param_filevec_ = ".$#param_filevec_."\n";
}

# For each param file & for each model file, we create a strategy file
# Arguments:
# $main_log_file_handle_
# @model_filevec_
# $distributed_
# $celery_shared_location
# @param_filevec_
# $traded_ezone_
#
# Output:
# %strat2param_
# @strategy_filevec_
# TODO: change this to a submoduke that can be called. It is now a subroutine that depends on shared variables.
sub MakeStrategyFiles 
{
  print $main_log_file_handle_ "\nMakeStrategyFiles\n\n";

  my $strategy_progid_ = 1001;

  foreach my $model_file_index_ ( 0 .. $#model_filevec_ ) {
    my $this_model_filename_ = $model_filevec_[$model_file_index_];

    if ( ! ExistsWithSize ( $this_model_filename_ ) ) {
      print $main_log_file_handle_ "Skipping model ".$this_model_filename_."\n";
      next;
    }

    if ( $using_grid_ || $distributed_ == 1 ) {
	  #	Copy model file to a shared location (for distributed runs)
      my $shared_model_filename = $celery_shared_location.$this_model_filename_;
      my $copy_model = "mkdir -p `dirname $shared_model_filename`; cp $this_model_filename_ $shared_model_filename";
      `$copy_model`;
      $this_model_filename_ = $shared_model_filename;
    }

    foreach my $param_file_index_ ( 0 .. $#param_filevec_ ) {
      my $this_param_filename_ = $param_filevec_[$param_file_index_];

      if ( ! ExistsWithSize ( $this_param_filename_ ) ) {
        print $main_log_file_handle_ "Skipping param ".$this_param_filename_."\n";
        next;
      }

      if ( $using_grid_ || $distributed_ == 1) {
#Copy param file to a shared location (for distributed runs)
        my $shared_param_filename = $celery_shared_location.$this_param_filename_;
        my $copy_param = "mkdir -p `dirname $shared_param_filename`; cp $this_param_filename_ $shared_param_filename";
        `$copy_param`;
        $this_param_filename_ = $shared_param_filename;
      }

      my $this_strategy_filebase_ = "strat_".$param_file_index_."_".$model_file_index_."_".$strategy_progid_ ;
      my $this_strategy_filename_ = $local_strats_dir_."/".$this_strategy_filebase_;

      my $create_strategy_file_script = SearchScript ( "create_strategy_file.pl", () );
      my $exec_cmd = "$create_strategy_file_script $this_strategy_filename_ $shortcode_ $strategyname_ $this_model_filename_ $this_param_filename_ $trading_start_hhmm_ $trading_end_hhmm_ $strategy_progid_";
      $exec_cmd = $exec_cmd." $traded_ezone_" if ( $traded_ezone_ ne "INVALID" );
      $exec_cmd = $exec_cmd." PORT" if ( $strategy_line_type_ eq "PORT_STRATEGYLINE" );

      print $main_log_file_handle_ "$exec_cmd\n";
      `$exec_cmd`;
# uniqueness of progid ensures that we can run them in sim together
      $strategy_progid_++; 

      if ( ExistsWithSize (  $this_strategy_filename_ ) ) {
        push ( @strategy_filevec_, $this_strategy_filename_ ); 
        $strat2param_ { $this_strategy_filebase_ } = $this_param_filename_ ;
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
  my @temp_strategy_pnl_stats_file_list_ = ( );
  my @temp_strategy_log_file_list_ = ( );

# generate a list of commands which are unique , 
# independent from each other and can be safely run in parallel.

  my $tradingdate_ = $trading_end_yyyymmdd_;
  if ( $using_grid_ == 1 )
  {
    ComputePnlGridDatesVec($work_dir_, $local_strats_dir_, $local_results_base_dir, @trading_days_vec_);
    return;
  }

  foreach my $tradingdate_ ( @trading_days_vec_ ) {

    next if ( $tradingdate_ ~~ @skip_days_ );

    my $temp_strategy_list_file_index_ = 0;
    for ( my $strategy_filevec_index_ = 0 ; $strategy_filevec_index_ <= $#strategy_filevec_ ; ) {

      my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
      my $temp_strategy_cat_file_ = $work_dir_."/temp_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
      my $temp_strategy_output_file_ = $work_dir_."/temp_strategy_output_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
      my $temp_strategy_pnl_stats_file_ = $work_dir_."/temp_strategy_pnl_stats_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

      open ( TSLF , ">" , $temp_strategy_list_file_ ) or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );

      for ( my $num_files_ = 0 ; ( $num_files_ < $MAX_STRAT_FILES_IN_ONE_SIM ) && ( $strategy_filevec_index_ <= $#strategy_filevec_ ) ; $num_files_ ++ ) {
        my $this_strategy_filename_ = $strategy_filevec_ [ $strategy_filevec_index_ ];
        $strategy_filevec_index_ ++;

        print TSLF $this_strategy_filename_."\n";
        `cat $this_strategy_filename_ >> $temp_strategy_cat_file_`;
      }
      close ( TSLF );

# Get a concurrency safe id.
      my $unique_sim_id_ = GetGlobalUniqueId ( );
      my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
      my $this_log_filename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".$unique_sim_id_;

      my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
      my $exec_cmd_ = "";
      my $get_pnl_stats_2_script = "";
      
      if ($strategy_line_type_ eq "PORT_STRATEGYLINE") {
        $get_pnl_stats_2_script = SearchScript ( "get_pnl_stats_stir_2.pl", () );
      }
      else {
        $get_pnl_stats_2_script = SearchScript ( "get_pnl_stats_2.pl", () );
      }

      if ( ! $distributed_ ) {
        my $SIM_STRATEGY_EXEC = SearchExec ("sim_strategy", ());
        $exec_cmd_ = $SIM_STRATEGY_EXEC . " SIM " . $temp_strategy_cat_file_ . " " . $unique_sim_id_ . " " . $tradingdate_ . " " . $market_model_index_ . " ADD_DBG_CODE -1 > " . $temp_strategy_output_file_ . " 2>/dev/null ; " . $get_pnl_stats_2_script . " $this_trades_filename_ > $temp_strategy_pnl_stats_file_ 2>/dev/null; rm -f $this_trades_filename_; rm -f $this_log_filename_ ";
      }
      else {
        my $temporary_strat_location = $celery_shared_location.$temp_strategy_cat_file_;
        my $copy_cmd = "mkdir -p `dirname $temporary_strat_location`; cp $temp_strategy_cat_file_ $temporary_strat_location";
        `$copy_cmd`;
        my $celery_channel_name = GetBestWorkerPool($shortcode_);

	    my $SIM_STRATEGY_EXEC = SearchExec ( "sim_strategy", () ) ;
        $exec_cmd_ = "$DISTRIBUTED_EXECUTOR $max_sim_timeout $temp_strategy_output_file_ $celery_channel_name $shortcode_ bash -c \"sim_output=\\`$SIM_STRATEGY_EXEC SIM $temporary_strat_location $unique_sim_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 2>&1 | tr '\\n' '\\002'\\`; pnl_stats=\\`$get_pnl_stats_2_script $this_trades_filename_ 2>&1 | sed 's/^/PNLSTATS /g' | tr '\\n' '\\002'\\`; echo -n \\\$sim_output\\\$'\\n'\\\$pnl_stats | tr '\\002' '\\n'; \"";

        push ( @intermediate_files_, $temporary_strat_location);
      }


      push ( @unique_sim_id_list_ , $unique_sim_id_ );
      push ( @independent_parallel_commands_ , $exec_cmd_ );

      push ( @tradingdate_list_ ,$tradingdate_ );
      push ( @temp_strategy_list_file_index_list_ , $temp_strategy_list_file_index_ ); $temp_strategy_list_file_index_ ++;

      push ( @temp_strategy_list_file_list_ , $temp_strategy_list_file_ );
      push ( @temp_strategy_cat_file_list_ , $temp_strategy_cat_file_ );
      push ( @temp_strategy_output_file_list_ , $temp_strategy_output_file_ );
      push ( @temp_strategy_pnl_stats_file_list_ , $temp_strategy_pnl_stats_file_ );
      push ( @temp_strategy_log_file_list_ , $this_log_filename_ );
      push ( @intermediate_files_, $temp_strategy_list_file_);
      push ( @intermediate_files_, $temp_strategy_cat_file_ );
      push ( @intermediate_files_, $temp_strategy_output_file_);
      push ( @intermediate_files_, $temp_strategy_pnl_stats_file_);
      push ( @intermediate_files_, $this_log_filename_);
    }

  }

# process the list of commands , processing MAX_CORES_TO_USE_IN_PARALLEL at once
  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; ) {
    my @output_files_to_poll_this_run_ = ( );
    my @logfiles_files_to_poll_this_run_ = ( );

    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = $MAX_CORES_TO_USE_IN_PARALLEL;

    if ( ! $distributed_ ) {
      $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    }

    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ ) {
      push ( @output_files_to_poll_this_run_ , $temp_strategy_output_file_list_[ $command_index_ ] );
      push ( @logfiles_files_to_poll_this_run_ , $temp_strategy_output_file_list_[ $command_index_ ] );

      if ( -f $temp_strategy_output_file_list_[ $command_index_ ] ) {
        `rm -f $temp_strategy_output_file_list_[ $command_index_ ]`;
      }

      print $main_log_file_handle_ $independent_parallel_commands_[ $command_index_ ]."\n";
      my $exec_cmd_ = $independent_parallel_commands_[ $command_index_ ];
      my $temp_script_ = $work_dir_."/temp_script_".$command_index_;
      open SCRIPT, "> $temp_script_" or PrintStacktraceAndDie ( "Could not open $temp_script_ for writing\n" );
      print SCRIPT "$exec_cmd_\n";
      close ( SCRIPT );

      my $pid_ = fork();
      die "unable to fork $!" unless defined($pid_);

# child process
      if ( ! $pid_ ) {
        exec("sh $temp_script_");
      }

#back to parent process
      print $main_log_file_handle_ "PID of $temp_script_ is $pid_\n";
      $command_index_ ++;
      sleep ( 1 );
    }

    my $t_pid_ = 9999;
    while ( $t_pid_ > 0 ) {
# there are still some child processes running, wait returns -1 when no child process is left
      $t_pid_ = wait();
      print $main_log_file_handle_ "PID of completed process: $t_pid_\n";
    }
  }

  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ ) {
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
    chomp ( @tradeinit_output_lines_ );

    if ( ! $distributed_ ) {
      if ( ExistsWithSize ( $temp_strategy_pnl_stats_file_ ) ) {
        my $exec_cmd_ = "cat $temp_strategy_pnl_stats_file_";
        print $main_log_file_handle_ $exec_cmd_."\n";
        my @pnlstats_output_lines_ = `$exec_cmd_`;

        foreach my $pnlstats_line_ ( @pnlstats_output_lines_ ) {
          my @rwords_ = split ( ' ', $pnlstats_line_ );

          if( $#rwords_ >= 1 ) {
            my $unique_strat_id_ = $rwords_[0];
# remove the first word since it is unique_strat_id_
            splice ( @rwords_, 0, 1 );
            $unique_id_to_pnlstats_map_{$unique_strat_id_} = join ( ' ', @rwords_ );
          }
        }
      }
    }
    else {
      my @remote_sim_output_lines_ = @tradeinit_output_lines_;
      @tradeinit_output_lines_ = ( );

      my @pnlstats_output_lines_ = ( );

      foreach my $outline_ ( @remote_sim_output_lines_ ) {
#Distribute the output lines to sim_strategy_output_lines_ and pnlstats_output_lines_
        if ( $outline_ =~ /PNLSTATS/ ) {
          push( @pnlstats_output_lines_, $outline_ );
        }
        else {
# This line was printed by sim_strategy (stdout/stderr)
          push ( @tradeinit_output_lines_, $outline_ );
        }
      }

      foreach my $pnlstats_line_ ( @pnlstats_output_lines_ ) {
        my @rwords_ = split ( ' ', $pnlstats_line_ );

        if( $#rwords_ >= 2 ) {
          my $unique_strat_id_ = $rwords_[1];
# remove the first word since it is unique_strat_id_
          splice ( @rwords_, 0, 2 );
          $unique_id_to_pnlstats_map_{$unique_strat_id_} = join ( ' ', @rwords_ );
        }
      }
    }

    my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

    open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );

    my $psindex_ = 0;
    foreach my $tradeinit_outline_ ( @tradeinit_output_lines_ ) {

      if ( $tradeinit_outline_ =~ /SIMRESULT/ ) {
        $tradeinit_outline_ =~ s/SIMRESULT\s+//g;

        my ($pnl_, $vol_) = split ( /\s+/, $tradeinit_outline_ );

# volume >= 0 ... changed to allow 0 since some bax queries did not trade all day
        if ( $vol_ > 0 || ( ( $shortcode_ =~ /BAX/ ) && ( $vol_ >= 0 ) ) ) {

          my $unique_strat_id_ = GetUniqueSimIdFromCatFile ( $temp_strategy_cat_file_, $psindex_ );

          if ( ! exists $unique_id_to_pnlstats_map_{$unique_strat_id_} ) {
            $unique_id_to_pnlstats_map_{$unique_strat_id_} = "0 0 0 0 0 0 0 0 0 0 0";
          }

          printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$tradeinit_outline_, $unique_id_to_pnlstats_map_{$unique_strat_id_}, $unique_strat_id_ ;
          printf TRLF "%s %s %s\n",$tradeinit_outline_,$unique_id_to_pnlstats_map_{$unique_strat_id_}, $unique_strat_id_;
        }
        $psindex_ ++;
      }
    }
    close TRLF;

    if ( ExistsWithSize ( $temp_results_list_file_ ) ) {
      my $add_results_to_local_database_script = SearchScript ( "add_results_to_local_database.pl", () );
      my $exec_cmd="$add_results_to_local_database_script $temp_strategy_list_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir";
      print $main_log_file_handle_ "$exec_cmd\n";
      my $this_local_results_database_file_ = `$exec_cmd`;
      push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
    }
  }

  @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ );
}
