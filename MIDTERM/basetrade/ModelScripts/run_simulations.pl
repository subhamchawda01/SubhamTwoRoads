#!/usr/bin/perl

# \file ModelScripts/run_simulations_distributed.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input :
# directory name ( runs simulations for all files below directory )
# or strategy_list_filename ( a file with lines with just 1 word corresponding to the fullpath of a strategyfile )
# trading_start_yyyymmdd
# trading_end_yyyymmdd
# [ localresults_base_dir ] ( if not specified then use Globalresultsdbdir )
#
# for each valid_date in the period :
#   make a temporary_this_day_strategy_list_file of only the entries which do not have data in the results_base_dir
#   breaks the temporary_this_day_strategy_list_file into blocks of 50 entries
#   for each block of <= 50 entries : create temp_strategy_block_file ( with just the names of files in that block )
#     runs a sim_strategy on this tradingdate and this block and collects the results in a temp_results_block_file
#     then uses add_results_to_local_database ( temp_strategy_block_file, temp_results_block_file, results_base_dir )
#
# At the end looking at the original strategy_list_filename and results_base_dir
# build a summary_results.txt of the following format :
# STRATEGYFILENAME strategy_file_name
# date_1 pnl_1 volume_1
# date_2 pnl_2 volume_2
# .
# .
# date_n pnl_n volume_n
# STATISTICS pnl_average_ pnl_stdev_ volume_average_ pnl_sharpe_ pnl_conservative_average_ pnl_median_average_

use strict;
use warnings;
use POSIX;
use feature "switch";
use List::Util qw/max min/;
use Term::ReadKey;

sub RunChildrenAndWait;
sub process_each_shortcode;
sub CheckValidShortcode;
sub CheckValidExchange;
sub RunSimulationForShortCode;
sub ExecuteCommands;
sub SelectPairStrats;
sub LivingEachDay;
sub SanityCheckInputArguments;
sub ReadArgs;
sub Exceptions;

#Variables for exec location
my $USER=$ENV{'USER'};
#print "USER ".$USER;
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $WF_SCRIPTS_DIR=$HOME_DIR."/".$REPO."/walkforward";

require "$GENPERLLIB_DIR/s3_utils.pl"; # S3PutFilePreservePath
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult # IsDateHoliday
require "$GENPERLLIB_DIR/get_files_pending_sim.pl"; # GetFilesPendingSim
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/make_combinable_strat_vecs_from_list.pl"; # MakeCombinableStratVecsFromList
require "$GENPERLLIB_DIR/make_combinable_strat_vecs_from_dir.pl"; # MakeCombinableStratVecsFromDir
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; # GetDatesFromStartDate
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; # MakeStratVecFromDir
require "$GENPERLLIB_DIR/filter_combinable_strategies_for_date.pl"; # FilterCombinableStrategiesForDate 
require "$GENPERLLIB_DIR/make_strat_vec_from_list_matchbase.pl"; #MakeStratVecFromListMatchBase
require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
require "$GENPERLLIB_DIR/compute_pnl_grid.pl"; # ComputePnlGridStratDateVec

Exceptions();

my $USAGE="$0 shortcode/ALL/exchange_name strat_directory/stratlist_file/DEF trading_start_yyyymmdd ".
		"trading_end_yyyymmdd [GLOBALRESULTSDBDIR|DB] | [--GLOBALPNLSAMPLESDIR|-p DIRNAME/INVALIDDIR]".
		"  [--MKT_MODEL|-m  DEF] [--USE_DEFAULT_START_TIME|-s 0/1] [--USE_DISTRIBUTED|-d 0/1] [--DATELIST| ".
		"-dtlist DATELIST_FILE] [--REMOVE_RESULTS|-r 0/1] [--DEBUG|-dbg 0/1] [--BASEPX|-b BASEPX] [--STIR|-st 0/1]".
		" [--COMPUTEINDIVIDUAL|-ci 0/1] [--EMAIL|-e mail_address] [--QMODE/-qm airflow/manual] [-se sim_exec] [--backtest]";
		

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $GET_CONTRACT_SPECS = SearchExec("get_contract_specs");
#Initialize Arguments

my $shortcode_ = $ARGV[0];
my $parent_dir = $ARGV[1];
if ( $parent_dir eq "DEF" ) { $parent_dir = "/home/dvctrader/modelling/strats"; }

my $trading_start_yyyymmdd_ = max ( 20110901, $ARGV[2] );
my $trading_end_yyyymmdd_ = $ARGV[3];

my $GLOBALRESULTSDBDIR = "DB";
my $GLOBALPNLSAMPLESDIR = "INVALIDDIR";
if ( $#ARGV >= 4 ) { $GLOBALRESULTSDBDIR = $ARGV[4]; }

my $MKT_MODEL = "DEF";
my $USE_DEFAULT_START_TIME = 0;
my $USE_DISTRIBUTED = 0;
my $USE_GRID = 1;
my $REMOVE_RESULTS = 0;
my $DEBUG = 0;
my $BASEPX = "ALL";
my $LOG_DURATION_TAG = "";
my $DATELIST_FILE = "";
my $COMPUTEINDIVIDUAL = 0;
my $STIR = 0;
my $mail_address_ = "";
my $queue_mode_ = "manual";  # default
my $CUSTOM_EXEC = "";
my $USE_BACKTEST = 0;

$shortcode_ = "ALL" if $shortcode_ eq "EQIALL";

ReadArgs( );

if (defined $ENV{'USE_BACKTEST'}) {
  $USE_BACKTEST = 1;
}
if ($USE_BACKTEST) {
  $ENV{'USE_BACKTEST'}="1";
  SetBacktest();
}

if($USE_GRID){
  my $grid_user;
  my $grid_pass;
  if(not defined $ENV{'GRID_USERNAME'} or not defined $ENV{'GRID_PASSWORD'}){
    print "Enter username\n";
    $grid_user = <STDIN>;
    chomp $grid_user;
    print "Enter password\n";
    ReadMode('noecho');
    chomp($grid_pass = <STDIN>);
    ReadMode('normal');
    $ENV{'GRID_USERNAME'}=$grid_user;
    $ENV{'GRID_PASSWORD'}=$grid_pass;
  }
}

#Global Variables

#Variables for distributed (celery) version
my $DISTRIBUTED_EXECUTOR = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py";
my $DISTRIBUTED_STATUS_SCRIPT = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/view_job_status.py";
my $DISTRIBUTED_REVOKE_SCRIPT = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/revoke_my_job.py";

my $SHARED_LOCATION = "/media/shared/ephemeral21";

#Vectors for intermediate commands
my @independent_parallel_commands_ = ( );
my @all_strategy_filename_vec_ = ( );
my @strategy_filevec_ = ( );
my @pair_strategy_filevec_ = ( );
my @regular_strategy_filevec_ = ( );
my @all_valid_exchanges = ( );
my @all_valid_shortcodes = ( );
my $is_pair_strategy_ = -1;
my $base_dir = $parent_dir;
my $work_dir_;
my $main_log_file_handle_;
my $strat_listfile_index_ = 0;
my $is_config_or_strat_ = 0;

my %config_date_to_strat_ = ();
my %stratline_to_date_vec_map_ = ();
my %stratline_to_strat_map_ = ();
my %json_to_date_maps = ();
my %json_to_strat_maps = ();
  
my $MAX_STRAT_FILES_IN_ONE_SIM = 200;
my $is_db_ = int($GLOBALRESULTSDBDIR eq "DB") ;
my $dump_pnl_samples_into_dir_ = int($GLOBALPNLSAMPLESDIR ne "INVALIDDIR") ;
my $check_pnl_samples_ = int($is_db_ || $dump_pnl_samples_into_dir_); #check for pnl_samples only if they will be dumped into db or dir

my %QUEUES_MAP = ( );
$QUEUES_MAP{"airflow"} = ["autoscalegroup", "fast"];
$QUEUES_MAP{"manual"} = ["autoscalegroupmanual", "slow", "duration"];

if ( $queue_mode_ ne "airflow" && $queue_mode_ ne "manual" ) {
  print "Error: QMODE has to be either airflow or manual\n";
  exit ( 1 );
}

my $mail_body_ = "";

#Print Warning
if ( $is_db_ && $USER ne "dvctrader" ) {
  print "Warning! This script will write to the DB\n";
}
#Checks global results folder is shared folder incase of distributed version
CheckIsDistributedSharedResultsFolder();
UseCustomExec();

my $load_nse = 0;
if (index($shortcode_, "NSE") != -1 || index($shortcode_, "BSE") != -1) {
   $load_nse = 1;
}

GetValidShortcodesAndExchanges($load_nse);
SanityCheckInputArguments( );

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
$work_dir_ = $SPARE_HOME."RS/".basename($shortcode_)."/".$unique_gsm_id_;
$work_dir_ = $SHARED_LOCATION.$work_dir_ if ( $USE_DISTRIBUTED || $USE_GRID);

if ( -d $work_dir_ ) { `rm -rf $work_dir_`; }
`mkdir -p $work_dir_`;

my $main_log_file_ = $work_dir_."/main_log_file.txt";
print "Log File: $main_log_file_ \n";

$main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

my @all_shortcodes = ();

if ( $shortcode_ ne "ALL" ) {
  if ( -f $shortcode_ ) {
    @all_shortcodes = `cat $shortcode_`; chomp @all_shortcodes;
  }
  elsif ( CheckValidShortcode( $shortcode_ ) ) {
    push (@all_shortcodes, $shortcode_);
  }
  elsif ( CheckValidExchange( $shortcode_ ) ) {
    my @all_shortcodes = `$GET_CONTRACT_SPECS ALL $trading_end_yyyymmdd_ EXCHANGE $load_nse | grep -w $shortcode_ | cut -d' ' -f1`;
    chomp @all_shortcodes;
  }
  else {
    print "Invalid shortcode. Currently: $shortcode_ \n";
    exit(1);
  }
}

ProcessEachShortcode ( @all_shortcodes );

`chmod -R go+w $work_dir_ &> /dev/null`;
if ($USE_GRID==0) {
  ExecuteCommands ( );
}

SendMail ( );

exit( 0 );


###### subs ######

sub ProcessEachShortcode {
  my @all_shortcodes = @_;

# if remove_results flag is passed, remove the results first
  CheckAndRemoveResults();
 
# get the entire strategies list (across shortcodes)
  my  @all_shc_strategy_filename_vec_ = ();

  if ( -d $parent_dir) {
    @all_shc_strategy_filename_vec_ = MakeStratVecFromDir($parent_dir);
  } elsif ( -f $parent_dir) {
    @all_shc_strategy_filename_vec_ = MakeStratVecFromListMatchBase($parent_dir, "ALL");
  }

# 0 for strat 1 for config
  $is_config_or_strat_ = FindConfigOrStrat (\@all_shc_strategy_filename_vec_); 
  return if $is_config_or_strat_ < 0;

# in case wf_configs, get strats for all dates from start_date to end_date 
# note: in case of configs, we work with basenames
#       in case of strats, we work with paths
  if ( $is_config_or_strat_ ) {
    @all_shc_strategy_filename_vec_ = map { basename($_) } @all_shc_strategy_filename_vec_;
    PrintStratForConfigs (\@all_shc_strategy_filename_vec_, \%config_date_to_strat_);
  }
  
# fetch shortcodes to which the configs belong (no filter in case of "ALL")
  my %shortcode_to_configs = ( );
  GetShortcodeForConfigs (\@all_shc_strategy_filename_vec_, \%shortcode_to_configs);
  
# if shortcode as input_arg was passed as "ALL", no filter on the list of unique shortcodes from config
  if ( $#all_shortcodes < 0 ) {
    @all_shortcodes = keys %shortcode_to_configs;
  } else {
    @all_shortcodes = grep { defined $shortcode_to_configs{$_} } @all_shortcodes;
  }
  
# call RunSimulationForShortCode for all the shortcodes
  foreach (@all_shortcodes) {
    $shortcode_ = $_;
    @all_strategy_filename_vec_ = @{ $shortcode_to_configs{$shortcode_} };
    RunSimulationForShortCode( );
  }
}


sub RunSimulationForShortCode {
  $mail_body_.= "Shortcode: $shortcode_\n";
  if ( $MKT_MODEL eq "DEF" ) {
    $MKT_MODEL = GetMarketModelForShortcode ( $shortcode_ );
  }


  if ( $is_config_or_strat_ == 1 ) {
    LivingEachDayConfigs();
  } 
  elsif ( $is_config_or_strat_ == 0 ) {
    LivingEachDayStrats();
  }
}

sub FindConfigOrStrat
{
  my $stratvec_ref_ = shift;

# for the first config/strat, fetch whether it's config or strat and pass the output
  my $is_config_or_strat_ = -1;

  if ( $#$stratvec_ref_ >= 0 ) {
    foreach my $strategy_filename_ ( @$stratvec_ref_ ) {
      my $valid_config = `$WF_SCRIPTS_DIR/is_valid_config.py -c $strategy_filename_`; chomp($valid_config);
      if ($valid_config && $valid_config >= 0) {
        $is_config_or_strat_ = 1;
        last;
      }
      else {
        my ($t_msg_, $valid_strat) = IsStratValid ( $strategy_filename_ );
        if ( $valid_strat == 0 ) {
          $is_config_or_strat_ = 0;
          last;
        }
      }
    }
  }
  return $is_config_or_strat_;
}

sub PrintStratForConfigs
{
  my $configvec_ref_ = shift;
  my $config_to_strat_ref_ = shift;

  my $stratlistfile_ = GetCSTempFileName ( $work_dir_ );
  open STLIST, "> $stratlistfile_" or PrintStacktraceAndDie("Could not open $stratlistfile_ to read");
  print STLIST $_."\n" foreach @$configvec_ref_;
  close STLIST;

# print_strat_from_multiple_config_multiple_date.py prints strats for the list of configs all all dates b/w sd and ed
  my $cmd_ = "$WF_SCRIPTS_DIR/print_strat_from_multiple_config_multiple_date.py -cfile $stratlistfile_ -sd $trading_start_yyyymmdd_ -ed $trading_end_yyyymmdd_";
  my $stratdir_ = `$cmd_ 2>/dev/null | grep OUTPUT_DIR: | cut -d' ' -f2`; 
  chomp $stratdir_;
  $stratdir_ =~ s/\/$//g; # remove trailing /

  if ( ! defined $stratdir_ or $stratdir_ eq "" or ! -d $stratdir_ ) {
    print "Failed $cmd_\n";
    exit(1);
  }

# save the mapping of config->date->stratpath
  if (opendir my $dh1, $stratdir_) {
    while (my $date_ = readdir $dh1) {
      next if $date_ eq '.' or $date_ eq '..';

      if (opendir my $dh2, $stratdir_."/".$date_) {
        while (my $config_ = readdir $dh2) {
          next if $config_ eq '.' or $config_ eq '..';

          $$config_to_strat_ref_{$config_}{$date_} = $stratdir_."/".$date_."/".$config_;
        }
        closedir $dh2;
      }
    }
    closedir $dh1;
  }
}


sub GetShortcodeForConfigs
{
  my $stratvec_ref_ = shift;
  my $shc_to_config_ref_ = shift;

# fetch list of unique dependent shortcodes from the strats/configs
  foreach my $cfg_ ( @$stratvec_ref_ ) {
    my $sfile_ = $cfg_;
  
    if ( $is_config_or_strat_ == 1 ) {
      next if ! defined $config_date_to_strat_{$cfg_}; 
      $sfile_ = (values %{$config_date_to_strat_{$cfg_}})[0];
    }

    my $strat_content_ = `cat $sfile_ 2>/dev/null | head -1`;
    chomp $strat_content_;
    my @stratwords_ = split(/\s+/, $strat_content_);

    if ( ($stratwords_[0] =~ /^STRATEGYLINE/ or $stratwords_[0] =~ /^PORT_STRATEGYLINE/) and $#stratwords_ > 6 ) {
      push(@{ $$shc_to_config_ref_{$stratwords_[1]} }, $cfg_);
    }
    elsif ( $stratwords_[0] =~ /^STRUCTURED_STRATEGYLINE/ and $#stratwords_ > 1 ) {
      my $tsfile_ = $stratwords_[1];
      if ( -f $tsfile_ ) {
        my $str = `grep STRUCTURED_TRADING $stratwords_[1]`; chomp ( $str);
	my @tstratwords_ = split / /,$str;
        if ( $#tstratwords_ > 3 ) {
          push(@{ $$shc_to_config_ref_{$tstratwords_[1]} }, $cfg_);
        }
      }
    }
}
}

## Make the local results/pnlsamples files from the grid artifacts
sub ReadGridPnlAndSamplesAndMakeFiles
{
  my $results_dir = shift;
  my $list_file_vec = shift;
  my $result_date_vec = shift;
  my $pnls_file_vec = shift;
  my $samples_file_vec = shift;

  return if ( ! -d $results_dir."/pnls" or ! -d $results_dir."/samples" );

# read the pnl-files-basenames and samples-files-basenames
# take their intersection, we'll consider only the lists that have both pnls and samples
  opendir(DH, $results_dir."/pnls");
  my @pnl_results_files_ = sort readdir(DH);
  closedir(DH);

  opendir(DH, $results_dir."/samples");
  my %samples_results_map_ = map { $_ => 1 } readdir(DH);
  closedir(DH);

# intersection
  my @results_files_ = grep { defined $samples_results_map_{$_} and $_ ne "." and $_ ne ".." } @pnl_results_files_;

# read the dated files from grid folder
# parse the configname and the result from each line 
# and add them into the corresponding output files
  foreach my $result_file (@results_files_) {
    my $pnl_file_ = $results_dir."/pnls/".$result_file;
    my $samples_file_ = $results_dir."/samples/".$result_file;
    next if ! -f $pnl_file_ or ! -f $samples_file_;
    
    open GRID_PNL_FILE, " < $pnl_file_" or PrintStacktraceAndDie("Could not open $pnl_file_ to read\n");
    open GRID_SAMPLES_FILE, " < $samples_file_" or PrintStacktraceAndDie("Could not open $samples_file_ to read\n");
    my @pnls_lines = <GRID_PNL_FILE>; chomp @pnls_lines;
    my @samples_lines = <GRID_SAMPLES_FILE>; chomp @samples_lines;
    close GRID_PNL_FILE;
    close GRID_SAMPLES_FILE;

    my %strat_to_pnl_ = ( );
    my %strat_to_samples_ = ( );
    my $tradingdate_;

    foreach my $pline_ (@pnls_lines) {
      my @words = split(/\s+/, $pline_);
      next if $#words < 2;

      $strat_to_pnl_{ $words[0] } = join(' ', @words[2..$#words]);
      $tradingdate_ = $words[1] if ! defined $tradingdate_;
    }

    foreach my $pline_ (@samples_lines) {
      my @words = split(/\s+/, $pline_);
      next if $#words < 2;

      $strat_to_samples_{ $words[0] } = join(' ', @words[2..$#words]);
    }

# intersection, we'll consider only the configs that have both pnls and samples
    my @stratvec_ = sort grep { defined $strat_to_samples_{$_} } keys %strat_to_pnl_;

    my $strat_list_file = $work_dir_."/".$result_file."_list";
    my $strat_pnls_file = $work_dir_."/".$result_file."_pnls";
    my $strat_samples_file = $work_dir_."/".$result_file."_samples";

    open STRATEGY_LIST, " > $strat_list_file" or PrintStacktraceAndDie("Could not open $strat_list_file to write\n");
    open STRATEGY_PNLS, " > $strat_pnls_file" or PrintStacktraceAndDie("Could not open $strat_pnls_file to write\n");
    open STRATEGY_SAMPLES, " > $strat_samples_file" or PrintStacktraceAndDie("Could not open $strat_samples_file to write\n");

    foreach my $strat_ ( @stratvec_ ) {
      print STRATEGY_LIST $strat_."\n";

      print STRATEGY_PNLS $strat_to_pnl_{$strat_}."\n";
      print STRATEGY_SAMPLES $strat_to_samples_{$strat_}."\n";
    }

    close STRATEGY_LIST;
    close STRATEGY_PNLS;
    close STRATEGY_SAMPLES;

    push (@$list_file_vec, $strat_list_file);
    push (@$pnls_file_vec, $strat_pnls_file);
    push (@$samples_file_vec, $strat_samples_file);
    push (@$result_date_vec, $tradingdate_);
  }
}

sub SingleGridJob
{
  my $stratline_to_date_vec_map_t = shift;
  my $stratline_to_strat_map_t = shift;

  my $results_dir = ComputePnlGridStratDateVec($work_dir_, \%$stratline_to_date_vec_map_t, \%$stratline_to_strat_map_t);
print $results_dir."\n";

  my @strategy_list_file_vec = ();
  my @pnls_file_vec = ();
  my @samples_file_vec = ();
  my @result_date_vec = ();

  ReadGridPnlAndSamplesAndMakeFiles($results_dir, \@strategy_list_file_vec, \@result_date_vec, \@pnls_file_vec, \@samples_file_vec);

  my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$shortcode_;
  my $pnlsamples_database_dir_ = !$dump_pnl_samples_into_dir_ ? "INVALIDDIR" : $GLOBALPNLSAMPLESDIR."/".$shortcode_;
  for ( my $index=0; $index<= $#strategy_list_file_vec; $index++)
  {
    my $exec_cmd = "$MODELSCRIPTS_DIR/add_results_to_global_database.pl $strategy_list_file_vec[$index] $pnls_file_vec[$index]".
        " $result_date_vec[$index] $results_database_dir_ $is_db_ $samples_file_vec[$index] $pnlsamples_database_dir_";
    `$exec_cmd`;
  }
}

sub AllGridJobs
{
  foreach my $json_num (keys %{json_to_date_maps})
  {
    SingleGridJob($json_to_date_maps{$json_num}, $json_to_strat_maps{$json_num});
  }
}


sub ExecuteCommands
{
	if ( $USE_DISTRIBUTED ) {
    if ( $#independent_parallel_commands_ < 0 ) { return; }

    my $commands_file_id_  = "commands_".`date +%N`;
    chomp( $commands_file_id_ );
    my $commands_file_ = $work_dir_."/".$commands_file_id_;
    my $commands_file_handle_ = FileHandle->new;
    $commands_file_handle_->open ( "> $commands_file_ " ) or PrintStacktraceAndDie ( "Could not open $commands_file_ for writing\n" );
    print $commands_file_handle_ "$_ \n" for @independent_parallel_commands_;
    close $commands_file_handle_;
    my $dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $commands_file_ -s 1";
    if ( $CUSTOM_EXEC ne "" ) {
      $dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $commands_file_ -s 1 -e $CUSTOM_EXEC";
    }
    if ( $LOG_DURATION_TAG ne "" ) {
      $dist_exec_cmd_ = $dist_exec_cmd_." -d sim_strategy_$shortcode_";
    }

    print "Executing Command: ".$dist_exec_cmd_."\n";
    print $main_log_file_handle_ "Executing Command: ".$dist_exec_cmd_."\n";

    my $groupid_;
    foreach my $queue_ ( @{$QUEUES_MAP{$queue_mode_}} ) {
      my $this_dist_exec_cmd_ = $dist_exec_cmd_." -q $queue_";
      print $this_dist_exec_cmd_."\n";
      my @output_lines_ = `$this_dist_exec_cmd_ 2>/dev/null | grep -v "Log_group ID"`; chomp ( @output_lines_ );
      chomp( @output_lines_ );
      $mail_body_ .= $dist_exec_cmd_."\n";
      $mail_body_ .= join("\n", @output_lines_);
      print $main_log_file_handle_ "$_ \n" for @output_lines_;
      print "$_ \n" for @output_lines_;

      my ($groupid_line_) = grep { $_ =~ /Group ID:/ } @output_lines_;

      if ( ! defined $groupid_line_ ) {
        my ($queue_full_line_) = grep { $_ =~ /Queue Full!/ } @output_lines_;
      }
      else {
		$groupid_ = (split /\s+/, $groupid_line_)[2];
        last;
      }
    }

    if ( ! defined $groupid_ ) {
      if ( $queue_mode_ eq "airflow" ) {
        print "Error: All distributed queues full!!\n Scheduling to Non-distributed!!\n\n";
        print $main_log_file_handle_ "Error: All distributed queues full!!\n Scheduling to Non-distributed!!\n\n";
        $USE_DISTRIBUTED = 0;
      }
      else {
        print STDERR "Error: All distributed queues full!! Could not schedule the tasks!!\n";
        print $main_log_file_handle_ "Error: All distributed queues full!! Could not schedule the tasks!!\n";
        exit ( 1 );
      }
    }
    else {
      my @task_ids_ = `$DISTRIBUTED_STATUS_SCRIPT -g $groupid_ | grep ^ID: | awk '{print \$2}'`;
      chomp ( @task_ids_ );
      print "Task_id: ".$_."\n" foreach @task_ids_;
      my %task_to_status_ = map { $_ => 0 } @task_ids_;

      my $schedule_seconds_ = `date +%s`; chomp ( $schedule_seconds_ );
      my $last_update_seconds_ = $schedule_seconds_;
      my $last_pending_count_ = scalar @task_ids_;

      my $POLLING_PERIOD = 20; # repolling after every 120 seconds
      my $REVOKE_TIMEOUT = 7200; # celery timeout of 3 hours

      while (1) {
        sleep $POLLING_PERIOD;
        foreach my $taskid_ ( @task_ids_ ) {
          my $task_state_ = `$DISTRIBUTED_STATUS_SCRIPT -i $taskid_ 2>/dev/null | grep STATE: | awk '{print \$2}'`;
          chomp ( $task_state_ );
          if ( $task_state_ eq "SUCCESS" ) {
            my $task_logfile_ = `$DISTRIBUTED_STATUS_SCRIPT -i $taskid_ 2>/dev/null | grep LOGFILE: | awk '{print \$2}'`;
            chomp ( $task_logfile_ );
            if ( -f $task_logfile_ ) {
              my @output_lines_ = `cat $task_logfile_`; chomp ( @output_lines_ );
              ProcessChildOutput ( "Task_id: $taskid_", \@output_lines_, 0 );
            }
            $task_to_status_{ $taskid_ } = 1;
          }
          elsif ( $task_state_ eq "FAILURE" ) {
            my @t_output_lines_ = `$DISTRIBUTED_STATUS_SCRIPT -i $taskid_ 2>/dev/null`;
            my @output_lines_ = ( );
            my $lflag_ = 0;
            foreach my $line_ ( @t_output_lines_ ) {
              if ( $line_ =~ /STDOUT|STDERR/ ) {
                $lflag_ = 1;
              }
              elsif ( $line_ =~ /None|ID|STATE|STATE|RETURN|TIME/ ) {
                $lflag_ = 0;
              }
              elsif ( $lflag_ == 1 ) {
                push ( @output_lines_, $line_ );
              }
            }
            chomp ( @output_lines_ );
            ProcessChildOutput ( "Task_id: $taskid_", \@output_lines_, 1 );
            $task_to_status_{ $taskid_ } = 2;
          }
          elsif ( $task_state_ eq "REVOKED" ) {
            print $main_log_file_handle_ "Task_id: $taskid_  REVOKED\n\n";
            $mail_body_ .= "Task_id: $taskid_  REVOKED\n\n";
            $task_to_status_{ $taskid_ } = 2;
          }
        }

        my $update_seconds_ = `date +%s`; chomp ( $update_seconds_ );
        my $pending_count_ = grep { $task_to_status_{$_} == 0 } @task_ids_;

        if ( $update_seconds_ - $last_update_seconds_ > 600 || abs($last_pending_count_ - $pending_count_) > 0 ) {
          my $hhmmss_ = `date`; chomp ( $hhmmss_ );
          print $hhmmss_." No. of pending tasks: ".$pending_count_."\n";
          print $main_log_file_handle_ $hhmmss_." No. of pending tasks: ".$pending_count_."\n";
        }
        last if $pending_count_ == 0;

        if ( $update_seconds_ - $schedule_seconds_ > $REVOKE_TIMEOUT ) {
          print "Celery Schedule TIMEOUT: ".($update_seconds_-$schedule_seconds_)." seconds since tasks schedule\n";
          print $main_log_file_handle_ "Celery Schedule TIMEOUT: ".($update_seconds_-$schedule_seconds_)." seconds since tasks schedule\n";
          my $revoke_cmd_ = "$DISTRIBUTED_REVOKE_SCRIPT -g $groupid_ 2>/dev/null";
          my @output_lines_ = `$revoke_cmd_`; chomp (  @output_lines_ );
          print $revoke_cmd_."\n".join("\n", @output_lines_)."\n"; 
          print $main_log_file_handle_ $revoke_cmd_."\n".join("\n", @output_lines_)."\n"; 
          exit ( 1 );
        }
      }
    }
  }
  if ( ! $USE_DISTRIBUTED ) {
    foreach my $command ( @independent_parallel_commands_ ) {
      my @output_lines_ = `$command`; chomp ( @output_lines_ );
      my $return_val_ = $?;
      ProcessChildOutput ( $command, \@output_lines_, $return_val_ );
    }
  }
}

sub ProcessChildOutput
{
  my ($command, $olines_ref_, $return_val_) = @_;
  my @output_lines_ = @$olines_ref_;

  my $success_ = 1;

  my @stats_count_vec_ = grep { $_ =~ /\#strats/ } @output_lines_;
  if ( $#stats_count_vec_ < 0 ) { $success_ = 0; }
  else {
    my @ltoks_ = split(" ", $stats_count_vec_[0]);
    if ( ( $ltoks_[2] != $ltoks_[6] || $ltoks_[2] != $ltoks_[10] )
        || grep {$_ =~ /result_generation failed/} @output_lines_ 
        || grep {$_ =~ /add_results_to_global_database failed/} @output_lines_
        || $return_val_ != 0 ) {
      $success_ = 0;
    }
  }

  if ( $success_ == 0 || $DEBUG == 1 ) {
    print $main_log_file_handle_ $command."\n";
    print $main_log_file_handle_ join("\n", @output_lines_)."\n\n";
    $mail_body_ .= $command."\n";
    $mail_body_ .= join("\n", @output_lines_)."\n\n";
  }
}

sub SelectPairStrats 
{
  foreach my $t_strat  ( @strategy_filevec_ )
  {
    my $t_exec_cmd_ = "cat $t_strat | head -n1 | awk '{if ( \$3==\"NikPricePairAggressiveTrading\" ) { print 1 } else { print -1 } }'";
    my $is_pair_strat_ = `$t_exec_cmd_`;
    chomp ( $is_pair_strat_ ) ;
    $is_pair_strat_ = $is_pair_strat_ + 0;
    if ( $is_pair_strat_ > 0 )
    {
      push ( @pair_strategy_filevec_, $t_strat );
    }
    else
    {
      push ( @regular_strategy_filevec_, $t_strat );
    }
  }
}

sub RunSimForDay
{
  my $tradingdate_ = shift;
  my $this_day_work_dir_ = $work_dir_."/".$tradingdate_;
  if ( ! -d $this_day_work_dir_ ) { `mkdir -p $this_day_work_dir_`; }

# only the files that do not have results in the global database
  my @this_day_strategy_filevec_ = ();
  if ( $check_pnl_samples_ ) {
    GetFilesPendingSimAndPnlSamplesFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR , $GLOBALPNLSAMPLESDIR, $is_config_or_strat_); 
  }
  else {
    GetFilesPendingSimFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR, $is_config_or_strat_);
  }
  
  if ( $#this_day_strategy_filevec_ >= 0 ) 
  {
    my $this_day_strategy_filevec_front_ = 0;
    while ( $this_day_strategy_filevec_front_ <= $#this_day_strategy_filevec_ )
    {
      my $this_day_strategy_filevec_back_ = min ( ( $this_day_strategy_filevec_front_ + $MAX_STRAT_FILES_IN_ONE_SIM - 1 ), $#this_day_strategy_filevec_ ) ;

      my $temp_strategy_cat_file_ = $this_day_work_dir_."/temp_strategy_list_file_".$strat_listfile_index_.".txt" ;
      my $temp_strategy_list_filenames_ = $this_day_work_dir_."/temp_strategy_list_filenames_".$strat_listfile_index_.".txt" ;
      open TSLF, "> $temp_strategy_list_filenames_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_ for writing\n" );
      open TSCF, "> $temp_strategy_cat_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_cat_file_ for writing\n" );
      my $tscf_line_cnt_ = 0;
      my $this_strat_bunch_next_strat_id_ = 10011;

      foreach my $this_strat_filename_ ( @this_day_strategy_filevec_[ $this_day_strategy_filevec_front_ .. $this_day_strategy_filevec_back_ ] )
      {
        my $stratline_ = "";
        $stratline_ = `cat $this_strat_filename_ 2>/dev/null | grep -v "^#" | head -1`;
        chomp $stratline_;
        next if $stratline_ eq "";

        my @tsl_words_ = split ( ' ', $stratline_ );
        if ( $STIR == 0 && $#tsl_words_ >= 7 ) {
          $tsl_words_[7] = $this_strat_bunch_next_strat_id_++;
        }
        elsif ( $STIR == 1 ) {
          $tsl_words_[2] = $this_strat_bunch_next_strat_id_ ++; 
        }
        else { next; }

        my $this_strat_line = join(' ', @tsl_words_);
        print TSCF $this_strat_line."\n";
        
        # set the vector of date
        push(@{$stratline_to_date_vec_map_{$this_strat_line}}, $tradingdate_);
        # set the strat line to configname map
        my $strat_name = basename($this_strat_filename_);
        $stratline_to_strat_map_{$this_strat_line} = $strat_name;
        
        print TSLF "$strat_name\n";
        $tscf_line_cnt_ ++ ;
      }

      close TSCF;
      close TSLF;
      if ( $tscf_line_cnt_ < 1) { 
        print $main_log_file_handle_ "Strategy file empty. Skipping current iteration\n";
        $this_day_strategy_filevec_front_ = $this_day_strategy_filevec_back_ + 1;
        next;
      }

      {
        my $script_dir_ = $SCRIPTS_DIR;
        $script_dir_ = "/home/dvctrader/basetrade_install/scripts" if $USE_DISTRIBUTED;

        my $script_path = $script_dir_."/run_single_strat_file.pl";
        my $exec_cmd_ = "$script_path $shortcode_ $temp_strategy_cat_file_ $tradingdate_ $temp_strategy_list_filenames_ $this_day_work_dir_ $GLOBALRESULTSDBDIR $GLOBALPNLSAMPLESDIR DEF ";
        if ( $STIR == 1 ) {
          $script_path = $script_dir_."/run_single_strat_file_stir.pl";
          $exec_cmd_ = "$script_path $shortcode_ $temp_strategy_cat_file_ $tradingdate_ $temp_strategy_list_filenames_ $this_day_work_dir_ $GLOBALRESULTSDBDIR $GLOBALPNLSAMPLESDIR DEF ";
        }

        if ( $CUSTOM_EXEC ne "" ) {
          $exec_cmd_ = $exec_cmd_." ".$CUSTOM_EXEC;
        }
        push ( @independent_parallel_commands_ , $exec_cmd_ );
      }

      $this_day_strategy_filevec_front_ = $this_day_strategy_filevec_back_ + 1;
      $strat_listfile_index_++;
    } # while
  }
}

sub GetStratlineMapForDay
{
  #In grid we do not usw folders so removing the step where we create folders
  my $tradingdate_ = shift;

# only the files that do not have results in the global database
  my @this_day_strategy_filevec_ = ();
  if ( $check_pnl_samples_ ) {
    GetFilesPendingSimAndPnlSamplesFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR , $GLOBALPNLSAMPLESDIR, $is_config_or_strat_);
  }
  else {
    GetFilesPendingSimFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR, $is_config_or_strat_);
  }

  if ( $#this_day_strategy_filevec_ >= 0 )
  {
    my $this_day_strategy_filevec_front_ = 0;
    while ( $this_day_strategy_filevec_front_ <= $#this_day_strategy_filevec_ )
    {
      my $this_day_strategy_filevec_back_ = min ( ( $this_day_strategy_filevec_front_ + $MAX_STRAT_FILES_IN_ONE_SIM - 1 ), $#this_day_strategy_filevec_ ) ;

      my $tscf_line_cnt_ = 0;
      my $this_strat_bunch_next_strat_id_ = 10011;

      foreach my $this_strat_filename_ ( @this_day_strategy_filevec_[ $this_day_strategy_filevec_front_ .. $this_day_strategy_filevec_back_ ] )
      {
        my $stratline_ = "";
        $stratline_ = `cat $this_strat_filename_ 2>/dev/null | grep -v "^#" | head -1`;
        chomp $stratline_;
        next if $stratline_ eq "";

        my @tsl_words_ = split ( ' ', $stratline_ );
        if ( $STIR == 0 && $#tsl_words_ >= 7 ) {
          $tsl_words_[7] = $this_strat_bunch_next_strat_id_++;
        }
        elsif ( $STIR == 1 ) {
          $tsl_words_[2] = $this_strat_bunch_next_strat_id_ ++;
        }
        else { next; }

        my $this_strat_line = join(' ', @tsl_words_);

        # set the vector of date
        push(@{$stratline_to_date_vec_map_{$this_strat_line}}, $tradingdate_);
        # set the strat line to configname map
        my $strat_name = basename($this_strat_filename_);
        $stratline_to_strat_map_{$this_strat_line} = $strat_name;

        $tscf_line_cnt_ ++ ;
      }
      if ( $tscf_line_cnt_ < 1) {
        print $main_log_file_handle_ "Strategy file empty. Skipping current iteration\n";
        $this_day_strategy_filevec_front_ = $this_day_strategy_filevec_back_ + 1;
        next;
      }

      $this_day_strategy_filevec_front_ = $this_day_strategy_filevec_back_ + 1;
      $strat_listfile_index_++;
    }
  }
}

sub GetDatesVec
{
  my @dates_vec_ = GetDatesFromStartDate ( $shortcode_, $trading_start_yyyymmdd_, $trading_end_yyyymmdd_ );

  if ( defined $DATELIST_FILE && $DATELIST_FILE ne "" ) {
    if ( -f $DATELIST_FILE ) {
      open DATELIST_HANDLE, "< $DATELIST_FILE" or PrintStacktraceAndDie ( "Could not open $DATELIST_FILE for reading" );
      my @file_dates_vec_ = <DATELIST_HANDLE>;
      chomp ( @file_dates_vec_ );
      close DATELIST_HANDLE;

      my %file_dates_map_ = map { $_=>1 } @file_dates_vec_;
      @dates_vec_ = grep { defined $file_dates_map_{ $_ } } @dates_vec_;
    }
  }
  return @dates_vec_;
}

sub LivingEachDay
{
  %stratline_to_date_vec_map_ = ();
  %stratline_to_strat_map_ = ();
  my @dates_vec_ = GetDatesVec ( );
  
  if ($USE_GRID==1){
    foreach my $tradingdate_ (@dates_vec_) {
      GetStratlineMapForDay ( $tradingdate_ );
    }
  } else {
    foreach my $tradingdate_ (@dates_vec_) {
      RunSimForDay ( $tradingdate_ );
    }
  }
  if($USE_GRID==1) {
    SingleGridJob(\%stratline_to_date_vec_map_, \%stratline_to_strat_map_);
  }
}

sub LivingEachDayConfigs
{
  my @dates_vec_ = GetDatesVec ( );
  my $json_num = 0;
  foreach my $tradingdate_ ( @dates_vec_ ) {
    my @combinable_strat_vecs_ = ( );

    if ( $COMPUTEINDIVIDUAL eq 0 ) {
# for all the configs for which we have the strategy defined for tradingdate, 
# push the strategy to all_sfiles_ list
# thus, all_sfiles_ is a list of legacy strats (not configs)
# call FilterCombinableStrategies on all_sfiles_
      my @all_sfiles_ = ();
      push (@all_sfiles_, $config_date_to_strat_{$_}{$tradingdate_}) foreach 
          grep { defined $config_date_to_strat_{$_}{$tradingdate_} } @all_strategy_filename_vec_;
      my $combinable_map_ref_ = FilterCombinableStrategies ( \@all_sfiles_ );
      
      push ( @combinable_strat_vecs_, $$combinable_map_ref_{$_} ) foreach keys %$combinable_map_ref_;
      
    }
    else {
# if COMPUTEINDIVIDUAL = 1, each element of all_sfiles is passed as individual set (to be called sim_strategy on)
      push ( @combinable_strat_vecs_, [$config_date_to_strat_{$_}{$tradingdate_}] ) foreach 
          grep { defined $config_date_to_strat_{$_}{$tradingdate_} } @all_strategy_filename_vec_;
    }

    $json_num = 0;
    foreach my $stratvec_ref_ ( @combinable_strat_vecs_ ) {
      %stratline_to_date_vec_map_ = ();
      %stratline_to_strat_map_ = ();
      @strategy_filevec_ = @$stratvec_ref_;
      if($USE_GRID==1) {
        GetStratlineMapForDay( $tradingdate_ );
      } else {
        RunSimForDay( $tradingdate_ );
      }
      push (@{$json_to_date_maps{$json_num}{$_}}, @{$stratline_to_date_vec_map_{$_}}) foreach keys %stratline_to_date_vec_map_;
      $json_to_strat_maps{$json_num}{$_} = $stratline_to_strat_map_{$_} foreach keys %stratline_to_strat_map_;
      $json_num++;
    }
    $strat_listfile_index_ = 0;
  }
  if($USE_GRID==1) {
    AllGridJobs();
  }
}

sub LivingEachDayStrats
{
  my @combinable_strat_vecs_ = ( );

  if ( $COMPUTEINDIVIDUAL eq 0 ) {
    my $combinable_map_ref_ = FilterCombinableStrategies ( \@all_strategy_filename_vec_ );
    push ( @combinable_strat_vecs_, $$combinable_map_ref_{$_} ) foreach keys %$combinable_map_ref_;
  }
  else {
    push ( @combinable_strat_vecs_, [$_] ) foreach @all_strategy_filename_vec_;
  }

  foreach my $stratvec_ref_ ( @combinable_strat_vecs_ ) {
    @strategy_filevec_ = ( );
    @pair_strategy_filevec_ = ( );
    @regular_strategy_filevec_ = ( );

    @strategy_filevec_ = @$stratvec_ref_;

    SelectPairStrats ( );
    if ( $#pair_strategy_filevec_ >= 0 ) {
      $is_pair_strategy_ = 1;
      if ( $STIR eq 1 ) {
        $is_pair_strategy_++;
        $is_pair_strategy_++ if $COMPUTEINDIVIDUAL eq 1;
      }
      @strategy_filevec_ = @pair_strategy_filevec_;
      LivingEachDay ( );
    } 

      @strategy_filevec_ = @regular_strategy_filevec_;
      $is_pair_strategy_ = - 1;
      LivingEachDay ( );
    
  }
}

sub SanityCheckInputArguments
{
  if ( ! ( ( -d $base_dir ) || ( -f $base_dir ) ) )
  { 
    print STDERR "$base_dir isn't a file or directory";
    exit( 0 );
  }

  if ( ! ( $trading_start_yyyymmdd_ ) )
  {
    print STDERR "TRADING_START_YYYYMMDD missing\n";
    exit ( 0 );
  }

  if ( ! ( ValidDate ( $trading_start_yyyymmdd_ ) ) )
  {
    print STDERR "TRADING_START_YYYYMMDD not Valid\n";
    exit ( 0 );
  }

  if ( ! ( $trading_end_yyyymmdd_ ) )
  {
    print STDERR "TRADING_END_YYYYMMDD missing\n";
    exit ( 0 );
  }

  if ( ! ( ValidDate ( $trading_end_yyyymmdd_ ) ) )
  {
    print STDERR "TRADING_END_YYYYMMDD not Valid\n";
    exit ( 0 );
  }
}

sub ReadArgs
{
  my $current_mode_ = "";
  foreach my $this_arg_ ( @ARGV[ 5..$#ARGV  ] ) {
    given ( $this_arg_ ) {
      when ( "=" ) { next; };
      when ( "--GLOBALPNLSAMPLESDIR" ) { $current_mode_ = "GLOBALPNLSAMPLESDIR"; next; }
      when ( "--MKT_MODEL" ) { $current_mode_ = "MKT_MODEL"; next; }
      when ( "--USE_DEFAULT_START_TIME" ) { $current_mode_ = "USE_DEFAULT_START_TIME"; next; }
      when ( "--USE_DISTRIBUTED" ) { $current_mode_ = "USE_DISTRIBUTED"; next; }
      when ( "--REMOVE_RESULTS" ) { $current_mode_ = "REMOVE_RESULTS"; next; }
      when ( "--DEBUG" ) { $current_mode_ = "DEBUG"; next; }
      when ( "--BASEPX" ) { $current_mode_ = "BASEPX"; next; }
      when ( "--LOG_DURATION_TAG" ) { $current_mode_ = "LOG_DURATION_TAG"; next; }
      when ( "--DATELIST" ) { $current_mode_ = "DATELIST"; next; }
      when ( "--COMPUTEINDIVIDUAL" ) { $current_mode_ = "COMPUTEINDIVIDUAL"; next; }
      when ( "--STIR" ) { $current_mode_ = "STIR"; next; }
      when ( "--EMAIL" ) { $current_mode_ = "EMAIL"; next; }
      when ( "--QMODE" ) { $current_mode_ = "QMODE"; next; }
      when ( "-p" ) { $current_mode_ = "GLOBALPNLSAMPLESDIR"; next; }
      when ( "-m" ) { $current_mode_ = "MKT_MODEL"; next; }
      when ( "-s" ) { $current_mode_ = "USE_DEFAULT_START_TIME"; next; }
      when ( "-d" ) { $current_mode_ = "USE_DISTRIBUTED"; next; }
      when ( "-r" ) { $current_mode_ = "REMOVE_RESULTS"; next; }
      when ( "-dbg" ) { $current_mode_ = "DEBUG"; next; }
      when ( "-b" ) { $current_mode_ = "BASEPX"; next; }
      when ( "-dt" ) { $current_mode_ = "LOG_DURATION_TAG"; next; }
      when ( "-dtlist" ) { $current_mode_ = "DATELIST"; next; }
      when ( "-ci" ) { $current_mode_ = "COMPUTEINDIVIDUAL"; next; }
      when ( "-st" ) { $current_mode_ = "STIR"; next; }
      when ( "-e" ) { $current_mode_ = "EMAIL"; next; }
      when ( "-qm" ) { $current_mode_ = "QMODE"; next; }
      when ( "-se" ) { $current_mode_ = "SE"; next; }
      when ( "--grid") { $USE_GRID = 1; next; }
      when ( "--nogrid") { $USE_GRID = 0; next; }
      when ( "--backtest") { $USE_BACKTEST = 1; next; }
      default {
        if ( $current_mode_ eq "GLOBALPNLSAMPLESDIR" ) {
          $GLOBALPNLSAMPLESDIR = $this_arg_;
        }
        elsif ( $current_mode_ eq "MKT_MODEL" ) {
          $MKT_MODEL = $this_arg_;
        }
        elsif ( $current_mode_ eq "USE_DEFAULT_START_TIME" ) {
          $USE_DEFAULT_START_TIME = $this_arg_;
        }
        elsif ( $current_mode_ eq "USE_DISTRIBUTED" ) {
          $USE_DISTRIBUTED = $this_arg_;
          $USE_GRID = 0;
        }
        elsif ( $current_mode_ eq "REMOVE_RESULTS" ) {
          $REMOVE_RESULTS = 1;
        }
        elsif ( $current_mode_ eq "DEBUG" ) {
          $DEBUG = $this_arg_;
        }
        elsif ( $current_mode_ eq "BASEPX" ) {
          $BASEPX = $this_arg_;
        }
        elsif ( $current_mode_ eq "LOG_DURATION_TAG" ) {
          $LOG_DURATION_TAG = $this_arg_;
        }
        elsif ( $current_mode_ eq "DATELIST" ) {
          $DATELIST_FILE = $this_arg_;
        }
        elsif ( $current_mode_ eq "COMPUTEINDIVIDUAL" ) {
          $COMPUTEINDIVIDUAL = $this_arg_;
        }
        elsif ( $current_mode_ eq "STIR" ) {
          $STIR = $this_arg_;
        }
        elsif ( $current_mode_ eq "EMAIL" ) {
          $mail_address_ = $this_arg_;
        }
        elsif ( $current_mode_ eq "QMODE" ) {
          $queue_mode_ = $this_arg_;
        }
        elsif ( $current_mode_ eq "SE" ) {
          $CUSTOM_EXEC = $this_arg_;
        }
        else {
          print "Invalid Argument: $this_arg_\n";
          print "USAGE: $USAGE\n";
          exit( 1 );
        }
        $current_mode_ = "";
      }
    }
  }
}

sub Exceptions {
  if ( $USER ne "dvctrader" && $USER ne "diwakar") {
    $LIVE_BIN_DIR = $BIN_DIR;
  }

  my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
  my $GLOBALRESULTSTRADESDIR = "/NAS1/ec2_globalresults_trades"; # since this will be synced to S3 anyways
      my $hostname_ = `hostname`;
  if ( index ( $hostname_ , "ip-10-0" ) >= 0 ) {
    $GLOBALRESULTSTRADESDIR = "/NAS1/ec2_globalresults_trades"; # since this will be synced to S3 anyways
  }
}

sub CheckIsDistributedSharedResultsFolder {
  if ( ! $is_db_ && $USE_DISTRIBUTED ) {
    if( index($GLOBALRESULTSDBDIR, "/media/shared/ephemeral") != 0 ) {
      my $folder_id = `date +%N`;
      chomp($folder_id);
      $GLOBALRESULTSDBDIR = "/media/shared/ephemeral16/run_sim_logs/".$folder_id."/";
      print "Using ".$GLOBALRESULTSDBDIR." instead. Please use shared folder with distributed version \n";
    }
    `mkdir -p $GLOBALRESULTSDBDIR; chmod a+rw $GLOBALRESULTSDBDIR`;
  }
}

sub UseCustomExec {
  if ( ! defined $CUSTOM_EXEC || $CUSTOM_EXEC eq "" ) {
    if ( defined $ENV{'SIM_STRATEGY'} ) {
      $CUSTOM_EXEC = $ENV{'SIM_STRATEGY'};
      if (! -e $CUSTOM_EXEC ) {
        print "SIM_STRATEGY env variable set and $CUSTOM_EXEC not present!. Did you provide full path? Exiting \n";
        exit(1);
      }
      if ( $is_db_ ) {
        print "Folder option required when using custom exec. Exiting \n";
        exit(1);
      }
    }
  }
}

sub CheckValidShortcode {
  my $shc = shift;
  if ( $STIR == 1 ) {
  	return 1;
  }
  if ( $shc ~~ @all_valid_shortcodes ) {
    return 1;
  }
  return 0;
}

sub CheckValidExchange {
  my $exch = shift;
  if ( $exch ~~ @all_valid_exchanges ) {
    return 1;
  }
  return 0;
}

sub GetValidShortcodesAndExchanges {
  my $load_nse_ = shift;
  @all_valid_exchanges = `$GET_CONTRACT_SPECS ALL $trading_end_yyyymmdd_ EXCHANGE $load_nse_ | cut -d' ' -f2 | sort | uniq`;
  chomp( @all_valid_exchanges );

  @all_valid_shortcodes = `$GET_CONTRACT_SPECS ALL $trading_end_yyyymmdd_ EXCHANGE $load_nse_ | cut -d' ' -f1 | sort | uniq`;
  push( @all_valid_shortcodes , "NSE_FO" );
  chomp( @all_valid_shortcodes );
}

sub CheckAndRemoveResults() {
  if (( $REMOVE_RESULTS == 1 )) {
    if ( $is_db_ ) {
      `/home/dvctrader/basetrade/scripts/remove_strat_results.pl $shortcode_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ $base_dir`;
    }
    elsif ( -d "$GLOBALRESULTSDBDIR/$shortcode_" ) {
#To avoid removing of imporatant folders by mistake
      print "Cannot remove directory $GLOBALRESULTSDBDIR. Use rm -r $GLOBALRESULTSDBDIR/$shortcode_ instead. \n"
    }
  }
}

sub SendMail
{
  if ( ( $mail_address_ ) &&
      ( $mail_body_ ) )
  {
    open(MAIL, "|/usr/sbin/sendmail -t");

## Mail Header
	my $hostname_ = `hostname`;
    print MAIL "To: $mail_address_\n";
    print MAIL "From: $mail_address_\n";
    print MAIL "Subject: run_simulations ( $shortcode_ ) $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ $GLOBALRESULTSDBDIR $hostname_\n\n";
## Mail Body
    print MAIL $mail_body_ ;

    close(MAIL);
  }
}
