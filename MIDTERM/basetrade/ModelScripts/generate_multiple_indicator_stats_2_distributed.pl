#!/usr/bin/perl

# \file ModelScripts/generate_indicator_stats_2.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes a prod_config
# START
# TIMEPERIODSTRING US_MORN_DAY
# PREDALGO na_e3 
# PREDDURATION 0.05 0.5 2 8 32 96 
# DATAGEN_START_HHMM EST_700 
# DATAGEN_END_HHMM EST_1200 
# DATAGEN_TIMEOUT 3000 4 0
# DATAGEN_BASE_FUT_PAIR MktSizeWPrice MktSizeWPrice
# SELF FGBM_0 
# SOURCE FGBS_0 FGBM_0 
# SOURCECOMBO UBEFC 
# END

use strict;
use warnings;
use feature "switch"; # for given, when
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use sigtrap qw(handler signal_handler normal-signals error-signals);

sub LoadInstructionFile ;
sub HasCurveTemplate ;
sub ReadTemplateFiles ;
sub GenerateIndicatorBodyVec ;
sub GetIbToCalc ;
sub DataGenAndICorrMap ;
sub OutputResults ;
sub CleanFiles ;
sub PrintStacktraceAndSendMailDie ;
sub CreateLockFileOrExit ;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 

my $REPO="basetrade";
my $SHARED_LOG_LOC="/media/shared/ephemeral16/indicatorLogs/";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$BIN_DIR;
my $hostname_ = `hostname`;

require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir
my $SPARE_LOCAL="/spare/local/";
$SPARE_LOCAL = GetSpareLocalDir() if ( index ( $hostname_ , "ip-10-0" ) >= 0 );
my $SPARE_HOME=$SPARE_LOCAL.$USER."/";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

my $SPARE_DATA_DIR=$SPARE_HOME."DataGenOut";
if (!( -d $SPARE_DATA_DIR ))
{
  `mkdir -p $SPARE_DATA_DIR`;
}
my $IWORK_DIR=$HOME_DIR."/indicatorwork";
my $ITEMPLATES_DIR=$HOME_DIR."/modelling/indicatorwork";

my $CALENDAR_DAYS_FOR_DATAGEN=252;
my $MAX_INDICATORS_IN_FILE = 300;

my $is_lock_created_by_this_run_ = 0;
local $| = 1; # sets autoflush in STDOUT

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/has_liffe_source.pl"; # HasLIFFESource
require "$GENPERLLIB_DIR/has_tse_source.pl"; # HasTSESource
require "$GENPERLLIB_DIR/has_espeed_source.pl"; # HasESPEEDSource
require "$GENPERLLIB_DIR/has_quincy_source.pl"; # HasQuincySource
require "$GENPERLLIB_DIR/has_rtsmicex_source.pl"; # HasRTSMICEXSource
require "$GENPERLLIB_DIR/skip_pca_indicator.pl"; # SkipPCAIndicator
require "$GENPERLLIB_DIR/skip_mixed_combo_indicators.pl"; # SkipMixedComboIndicator
require "$GENPERLLIB_DIR/skip_combo_book_indicator.pl"; # SkipComboBookIndicator
require "$GENPERLLIB_DIR/has_tmx_source.pl"; # HasTMXSource
require "$GENPERLLIB_DIR/has_hkfe_source.pl"; # HasHKFESource
require "$GENPERLLIB_DIR/has_cfe_source.pl"; # HasCFESource
require "$GENPERLLIB_DIR/is_order_weighted_indicator.pl"; # IsOWIndicator
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; #CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; # GetDatesFromNumDays
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/check_ilist_data.pl"; # CheckIlistData
require "$GENPERLLIB_DIR/report_summary.pl"; # ReportSummary subject address/NA body
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; #GetExchFromSHC
require "$GENPERLLIB_DIR/gen_ind_utils.pl"; #IsProjectedIndicator IsProjectedPairShc ShcHasExchSource
require "$GENPERLLIB_DIR/best_worker_pool.pl"; #GetBestWorkerPool
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
require "$GENPERLLIB_DIR/indstats_db_access_manager.pl"; # FetchIndCountsForShc
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFeatureAverageDays

# start 
my $SCRIPTNAME="$0";

my $USAGE="$0 prodconfig [num_days=252/-1] [distributed?0/1]";
my $stime=`date`;
my $t_hostname_ = `hostname`;
my $e_txt_ = "start_time: ".$stime."host: ".$t_hostname_;
my $e_add_ = "NA";
my $e_sub_ = "gen_multiple_indicator_stats_2 script's short summary";
if ( $USER eq "dvctrader" || $USER eq "dvcinfra" )
{
  $e_add_ = "nseall@tworoads.co.in";
}

if ( $#ARGV < 0 ) 
{ 
  $e_txt_ = $e_txt_."missing config file ".$USAGE."\n" ;
  print STDERR $e_txt_."\n";
  exit ( 0 ); 
}

my $prodconfig_ = $ARGV[0];
if ( $#ARGV >= 1 )
{
  $CALENDAR_DAYS_FOR_DATAGEN = int ( $ARGV[1] );
  if ( $CALENDAR_DAYS_FOR_DATAGEN <= 0 )
  {
#resetting to default value in case of invalid values
    $CALENDAR_DAYS_FOR_DATAGEN = 252;
  }
}
$e_txt_ = $e_txt_."config file \t".$prodconfig_."\n";

my $distributed_ = 0;
if ( $#ARGV >= 2 && $ARGV[2] == 1)
{
  $distributed_ = 1;
}

splice ( @ARGV, 0, 2 );

#Variables for distributed (celery) version
my $MAX_CORES_TO_USE_IN_PARALLEL = 100;
if ( ! $distributed_ ) {
  $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );
}

my $DISTRIBUTED_EXECUTOR = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py";
my $celery_shared_location = "/media/shared/ephemeral17/temp_strats/";
my $SHARED_LOCATION = "/media/shared/ephemeral17/commands";
my $max_datagen_timeout = 1200;

my @independent_parallel_commands_ = ();
my @temp_corrlist_file_list_ = ();
my @intermediate_files_ = ();


my $self_shortcode_ = "-na-";
my @pred_duration_vec_ = ();
my $predalgo_ = "na_t3";
my $datagen_start_hhmm_ = "EST_815";
my $datagen_end_hhmm_ = "EST_1500";
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 10;
my $datagen_num_trades_timeout_ = 5;
my $to_print_on_economic_times_ = 0;
my $dep_base_pricetype_ = "MktSizeWPrice";
my $dep_pred_pricetype_ = "MktSizeWPrice";
my $timeperiod_string_ = "";
my $recompute = 0;
my @filter_list_ = ();
my @all_filters_ = ();
my $total_count_req_ = 0;
my $isFVPresent_ = 0;
my $fsudm_level_ = 0;
my @dates_vec_ = ();

my @source_vec_ = ();
my @noselfsource_vec_ = ();
my @sourcecombo_vec_ = ();
my @port_sourcecombo_vec_= ();
my @trade_indepsource_vec_ = ();
my @curve_indep_shc_vec_vec_ = (); #stores shc1 and sch2 for Curve indicators

my $read_all_template_files_ = 0;
my $use_core_files_ = 1;
my @custom_template_files_ = ( );
my @custom_ilist_files_ = ( );

my @indicator_body_vec_ = () ;
my %date_to_indicator_to_count_ = (); # Map from date to indicator_body to correlation
my $config_contents = "";

LoadInstructionFile ( );

if ( $self_shortcode_ eq "-na-" || $self_shortcode_ eq "" )
{
  $e_txt_ = $e_txt_."$self_shortcode_\n" ;
  print STDERR $e_txt_."\n";
  exit ( 0 );
}

my $LOCKFILE = CreateLockFileOrExit ( );

my $work_dir_ = $IWORK_DIR."/".$self_shortcode_."_DURATION_FILTER_".$predalgo_."_".$timeperiod_string_."_".$dep_base_pricetype_."_".$dep_pred_pricetype_;
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
print "WORKING_DIR : ".$work_dir_."\n";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
`rm -f $main_log_file_`; 
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndSendMailDie ( "Could not open $main_log_file_ for writing\n" ) ;
$e_txt_ = $e_txt_."logfile: $main_log_file_\n";

my $config_base_ = basename ( $prodconfig_ );
$config_base_ =~ s/\..*$//g;

$SHARED_LOG_LOC = $SHARED_LOG_LOC."/".$config_base_."/".$unique_gsm_id_;
`mkdir -p $SHARED_LOG_LOC`;
`chmod a+rw $SHARED_LOG_LOC`;

$e_txt_ = $e_txt_."shared log files: \t".$SHARED_LOG_LOC."\n";
$e_txt_ = $e_txt_."\nconfig contents: \n".$config_contents."\n";

my $current_time_ = `date`; chomp($current_time_);
$main_log_file_handle_->print( "$current_time_:generating indicator body vector \n");
HasCurveTemplate ( );
GenerateIndicatorBodyVec ( );
#print $_."\n" foreach @indicator_body_vec_;

$current_time_ = `date`; chomp($current_time_);
$main_log_file_handle_->print( "$current_time_:done generating indicator body vector \n");
$main_log_file_handle_->print( "$current_time_:loading existing corr records to map \n");
FetchIndCountsForShc ( $self_shortcode_, $timeperiod_string_, min ( @dates_vec_ ), max ( @dates_vec_ ), $predalgo_, \@pred_duration_vec_, \@all_filters_, $dep_base_pricetype_, $dep_pred_pricetype_, \%date_to_indicator_to_count_ );

$current_time_ = `date`; chomp($current_time_);
$main_log_file_handle_->print( "$current_time_:done loading corr records to map \n");
$main_log_file_handle_->print( "$current_time_:generating new corr records  \n");

eval {  DataGenAndICorrMap ( ); };
if ( $@ ) {
  $e_txt_ = $e_txt_." got a die signal inside DataGenAndICorrMap Message :: $@ \n cleared lock \n" ;
  print STDERR $e_txt_."\n";
  $main_log_file_handle_->print( "Error: ".$e_txt_."\n" );
  exit ( 0 );
}

$current_time_ = `date`; chomp($current_time_);
$main_log_file_handle_->print( "$current_time_:done generating new corr records  \n");
$main_log_file_handle_->print( "$current_time_:generating summary details on all corr records \n");

foreach my $pred_duration_ (@pred_duration_vec_) {
  foreach my $filter_ (@all_filters_) {
#`$SCRIPTS_DIR/summarize_indicator_stats.pl $self_shortcode_ $pred_duration_ $filter_ $predalgo_ $timeperiod_string_ $dep_base_pricetype_ $dep_pred_pricetype_`;
  }
}

$current_time_ = `date`; chomp($current_time_);
$main_log_file_handle_->print( "$current_time_:done generating summary details  \n");

my $etime=`date`;
$e_txt_ = $e_txt_."end_time: ".$etime;
exit ( 0 );

sub LoadInstructionFile 
{
  my $end_date_ = GetIsoDateFromStrMin1 ( "TODAY-1" );
  my $numdays_ = $CALENDAR_DAYS_FOR_DATAGEN;

  open PRODCONFIGHANDLE, "< $prodconfig_ " or PrintStacktraceAndSendMailDie ( "Could not open $prodconfig_\n" );
  while ( my $thisline_ = <PRODCONFIGHANDLE> ) 
  {
    chomp ( $thisline_ );
    $config_contents = $config_contents.$thisline_."\n";
    my $comment_idx_ = index( $thisline_, '#' );
    $thisline_ = substr($thisline_, 0, $comment_idx_) if $comment_idx_ >= 0;

    my @this_words_ = split ( /\s+/, $thisline_ );

    if ( $#this_words_ >= 1 )
    {
      given ( $this_words_[0] )
      {
        when ("TIMEPERIODSTRING") {
          $timeperiod_string_ = $this_words_[1];
        }
        when("PREDALGO")
        {
          $predalgo_ = $this_words_[1];
        }
        when("PREDDURATION")
        {
          shift(@this_words_);
          @pred_duration_vec_ = @this_words_;
        }
        when("DATAGEN_START_HHMM")
        {
          $datagen_start_hhmm_ = $this_words_[1];
        }
        when("DATAGEN_END_HHMM")
        {
          $datagen_end_hhmm_ = $this_words_[1];
        }
        when("DATAGEN_TIMEOUT")
        {
          if ( $#this_words_ < 3 )
          {
            PrintStacktraceAndSendMailDie ( "DATAGEN_TIMEOUT line should be like DATAGEN_TIMEOUT 3000 4 1" );
          }
          $datagen_msecs_timeout_ = $this_words_[1];
          $datagen_l1events_timeout_ = $this_words_[2];
          $datagen_num_trades_timeout_ = $this_words_[3];
        }
        when("DATAGEN_BASE_FUT_PAIR")
        {
          if ( $#this_words_ < 2 )
          {
            PrintStacktraceAndSendMailDie ( "DATAGEN_BASE_FUT_PAIR line should be like DATAGEN_BASE_FUT_PAIR MktSizeWPrice MktSizeWPrice" );
          }
          $dep_base_pricetype_ = $this_words_[1];
          $dep_pred_pricetype_ = $this_words_[2];
        }
        when("LAST_DAY_TO_CONSIDER")
        {
          $end_date_ = GetIsoDateFromStrMin1 ( $this_words_[1] );
        }
        when("NUM_DAYS")
        {
          $numdays_ = min( $numdays_, $this_words_[1] );
        }
        when ("USE_CORE_FILES")
        {
          $use_core_files_ = $this_words_[1];
        }
        when ("USE_ALL_TEMPLATE_FILES")
        {
          $read_all_template_files_ = $this_words_[1];
        }
        when ("TEMPLATE_FILES")
        {
          push ( @custom_template_files_, $_ ) foreach ( @this_words_[1..$#this_words_] );
        }
        when ("INDICATOR_LISTS")
        {
          push ( @custom_ilist_files_, $_ ) foreach ( @this_words_[1..$#this_words_] );
        }
        when ("SELF")
        {
          $self_shortcode_ = $this_words_[1];
          if ( ! IsValidShc($self_shortcode_ ) )
          {
            PrintStacktraceAndSendMailDie ( "SELF Not valid shortcode: ".$self_shortcode_ );
          }
          if ( IsStdevPresentForShc($self_shortcode_)==0 )
          {
            PrintStacktraceAndSendMailDie ( "STDEV for SELF shortcode ".$self_shortcode_." does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt" );
          }
        }
        when ("SOURCE")
        {
          foreach my $source_ ( @this_words_[1..$#this_words_] )
          {
            next if FindItemFromVec ( $source_, @source_vec_ ); 

            if ( ! IsValidShc($source_) )
            {
              $e_txt_ = $e_txt_."Not valid shortcode: ".$source_."\n";
              next;
            }
            if ( IsStdevPresentForShc( $source_ )==0 )
            {
              $e_txt_ = $e_txt_."Stdev for source_shortcode_: $source_ does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt\n";
              next;
            }
            push ( @source_vec_, $source_ );

            next if $source_ eq $self_shortcode_;
            if ( ! IsLRDBPairPresent ( $self_shortcode_, $source_ ) )
            {
              $e_txt_ = $e_txt_."LRDB value absent for: ".$self_shortcode_.", ".$source_."\n";
              next;
            }
            push ( @noselfsource_vec_, $source_ );
          }
        }
        when ("NOSELFSOURCE")
        {
          foreach my $source_ ( @this_words_[1..$#this_words_] )
          {
            next if FindItemFromVec ( $source_, @source_vec_ );
            next if $source_ eq $self_shortcode_;

            if ( ! IsValidShc($source_) )
            {
              $e_txt_ = $e_txt_."Not valid shortcode: ".$source_."\n";
              next;
            }
            if ( ! IsLRDBPairPresent ( $self_shortcode_, $source_ ) )
            {
              $e_txt_ = $e_txt_."LRDB value absent for: ".$self_shortcode_.", ".$source_."\n";
              next;
            }
            if ( IsStdevPresentForShc( $source_ )==0 )
            {
              $e_txt_ = $e_txt_."Stdev for source_shortcode_: $source_ does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt\n";
              next;
            }

            push ( @noselfsource_vec_, $source_ );
          }
        }
        when ("SOURCECOMBO")
        {
          foreach my $port_ ( @this_words_[1..$#this_words_] )
          {
            if ( ! IsValidPort($port_) )
            {
              $e_txt_ = $e_txt_."Not valid shortcode: ".$port_."\n";
              next;
            }
            my $lrdb_present_ = 1;

            foreach my $constituent_shc_ ( GetPortConstituents( $port_ ) ) {
              next if $constituent_shc_ eq $self_shortcode_;

              if ( ! IsLRDBPairPresent ( $self_shortcode_, $constituent_shc_ ) )
              {
                $e_txt_ = $e_txt_."LRDB value absent for: ".$self_shortcode_.", ".$constituent_shc_."\n";
                $lrdb_present_ = 0;
                last;
              }
            }
            next if ( ! $lrdb_present_ );

            if ( IsStdevPresentForShc( $port_)==0 )
            {
              $e_txt_ = $e_txt_."Stdev for sourcecombo_shortcode_: $port_ does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt\n";
              next;
            }
            push ( @sourcecombo_vec_, $port_ );
          }
        }
        when ("PORT")
        {
          foreach my $port_ ( @this_words_[1..$#this_words_] )
          {
            if ( ! IsValidPort($port_) )
            {
              $e_txt_ = $e_txt_."Not valid shortcode: ".$port_."\n";
              next;
            }
            if ( ! IsLRDBPairPresent ( $self_shortcode_, $port_ ) )
            {
              $e_txt_ = $e_txt_."LRDB value absent for: ".$self_shortcode_.", ".$port_."\n";
              next;
            }
            if ( IsStdevPresentForShc($port_)==0 ) 
            {
              $e_txt_ = $e_txt_."Stdev for port_shortcode_: $port_ does not exist.\nAdd stdev to /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt\n";
              next; 
            }
            push ( @port_sourcecombo_vec_, $port_ );
          }
        }
        when ("TRADEINDEP")
        {
          foreach my $source_ ( @this_words_[1..$#this_words_] )
          {
            if ( ! IsValidShc($source_) )
            {
              $e_txt_ = $e_txt_."Not valid shortcode: ".$source_."\n";
              next;
            }
            push ( @trade_indepsource_vec_, $source_ );
          }
        }
        when ("FILTER")
        {
          foreach my $filter_ ( @this_words_[1..$#this_words_] )
          {
            if ( $filter_ eq "fv") { $isFVPresent_ = 1; }
            elsif ( $filter_ eq "fsudm" || $filter_ eq "fsudm1" ) { $fsudm_level_ = 1; }
            elsif ( $filter_ eq "fsudm2" ) { $fsudm_level_ = 2; }
            elsif ( $filter_ eq "fsudm3" ) { $fsudm_level_ = 3; }
            else { push ( @filter_list_, $filter_ ); }
          }
        }
        when ("TEMPLATEDIR")
        {
          $ITEMPLATES_DIR = $this_words_[1];
        }
        when ("RECOMPUTE")
        {
          $recompute = int($this_words_[1]);
        }
      }
    }
  }
  close PRODCONFIGHANDLE ;

  @source_vec_ = GetUniqueList ( @source_vec_ );
  @noselfsource_vec_ = GetUniqueList ( @noselfsource_vec_ );
  @sourcecombo_vec_ = GetUniqueList ( @sourcecombo_vec_ );
  @port_sourcecombo_vec_= GetUniqueList ( @port_sourcecombo_vec_);
  @trade_indepsource_vec_ = GetUniqueList ( @trade_indepsource_vec_ );

  $timeperiod_string_ = $datagen_start_hhmm_."_".$datagen_end_hhmm_ if ( $timeperiod_string_ eq "" ); # default
  @dates_vec_ = GetDatesFromNumDays ( $self_shortcode_, $end_date_, $numdays_ );

  push(@filter_list_, "fsg1") if ( $#filter_list_ == -1 && $isFVPresent_ == 0 && $fsudm_level_ == 0 );

  @all_filters_ = @filter_list_;
  push ( @all_filters_, "fv" ) if ( $isFVPresent_ );
  push ( @all_filters_, "fsudm" ) if ( $fsudm_level_ > 0 );

  SanityCheck ();

  $total_count_req_ = scalar(@pred_duration_vec_) * scalar(@all_filters_);
}

sub SanityCheck
{
  my $MAX_PRED_DURATION_LIMIT = 4;
  my $MAX_FILTER_LIMIT = 2;

  if ( $#pred_duration_vec_ >= $MAX_PRED_DURATION_LIMIT ) { 
    PrintStacktraceAndSendMailDie ( "Number of Pred-durations exceeds Maximum allowed limit: $MAX_PRED_DURATION_LIMIT" );
  }
  if ( $#all_filters_ >= $MAX_FILTER_LIMIT ) {
    PrintStacktraceAndSendMailDie ( "Number of filters exceeds Maximum allowed limit: $MAX_FILTER_LIMIT" );
  }
  
  @pred_duration_vec_ = sort { $a <=> $b } @pred_duration_vec_;
  my @too_close_pred_dur_idx_ = grep { $pred_duration_vec_[ $_-1 ] > 0.8 * $pred_duration_vec_[ $_ ] } 1..$#pred_duration_vec_;
  if ( $#too_close_pred_dur_idx_ >= 0 ) {
    my $t_idx_ = $too_close_pred_dur_idx_[0];
    PrintStacktraceAndSendMailDie ( "Pred durations ".$pred_duration_vec_[ $t_idx_-1 ]." and ".$pred_duration_vec_[ $t_idx_ ]." are too close" );
  }
}

sub HasCurveTemplate
{
  my $curve_shc_list_ = $ITEMPLATES_DIR."/curve_indicator_shortcodes" ;

  if ( ExistsWithSize ( $curve_shc_list_ ) ) {
    open CURVEFILEHANDLE, "< $curve_shc_list_ ";
    while ( my $thisline_ = <CURVEFILEHANDLE> ) {
      chomp ( $thisline_ );
      my @this_words_ = split ( ' ', $thisline_ );
      if ( $#this_words_ == 2 && $this_words_[0] eq $self_shortcode_ ) {
#no use of a shortcode which has expired
        if ( index ( $this_words_[1], "DI1" ) != -1 ) {
          my $t_term_ = `$BIN_DIR/get_di_term $this_words_[1] $yyyymmdd_`; chomp($t_term_);
          next if ( $t_term_ <= 0 );
        }
        if ( index ( $this_words_[2], "DI1" ) != -1 ) {
          my $t_term_ = `$BIN_DIR/get_di_term $this_words_[2] $yyyymmdd_`; chomp($t_term_);
          next if ( $t_term_ <= 0 );
        }
        my @t_curve_indep_shc_vec_ = ();
        push ( @t_curve_indep_shc_vec_, $this_words_[1]);
        push ( @t_curve_indep_shc_vec_, $this_words_[2]);
        push ( @curve_indep_shc_vec_vec_, \@t_curve_indep_shc_vec_ );
      }
    }
    close CURVEFILEHANDLE ;
  }              
}

sub ReadCoreTemplateFiles
{
  my $core_files_ref_ = shift;

  my $CORE_FOLDER = "/spare/local/tradeinfo/IndicatorStats/core_lists";

  foreach my $tag_ ( qw( NOSELFSOURCE SELF SOURCE SOURCECOMBO ) ) {
    my $core_file_ = $CORE_FOLDER."/core_list_".$tag_;
    open COREFHANDLE, "< $core_file_" or PrintStacktraceAndDie ( "Could not open $core_file_ for reading" );
    my @lines_vec_ = <COREFHANDLE>; chomp( @lines_vec_ );
    @lines_vec_ = map { "indicator_list_".$_."_".$tag_ } @lines_vec_;
    push ( @$core_files_ref_, @lines_vec_ );
  }
}

sub GenerateIndicatorBodyVec
{
  my @files_vec_ = ( );

  if ( $read_all_template_files_ ) {
    @files_vec_ = `ls -1 $ITEMPLATES_DIR | grep \^indicator_list_`;
    chomp ( @files_vec_ );
  }
  else {
    ReadCoreTemplateFiles ( \@files_vec_ ) if $use_core_files_;
    push ( @files_vec_, @custom_template_files_ );
  }
  @files_vec_ = map { $ITEMPLATES_DIR."/".$_ } @files_vec_;
#print "Template Files:\n".join("\n", @files_vec_)."\n";

# SELF: Only Self based Indicators
# SOURCE: Indicators that take any Shortcode source, could be SELF
# NOSELFSOURCE: Any non-self Shortcode source 
# SOURCECOMBO: Any Portfolio Source ( Combo and Port indicators )
# CURVE: only Curve based indicators
# TR: only indep TR indicators

  my @self_files_ = grep { /indicator_list_.*_SELF$/ } @files_vec_;
  my @source_files_ = grep { /indicator_list_.*_SOURCE$/ } @files_vec_;
  my @noselfsource_files_ = grep { /indicator_list_.*_NOSELFSOURCE$/ } @files_vec_;
  my @sourcecombo_files_ = grep { /indicator_list_.*_SOURCECOMBO$/ } @files_vec_;
  my @curve_files_ = grep { /indicator_list_.*_CURVE$/ } @files_vec_;
  my @indep_trade_files_ = grep { /TR/ } @files_vec_;
  
  my @full_indicator_body_vec_ = ();

  my ($avgl1sz_, undef) = GetFeatureAverageDays ( $self_shortcode_, $yyyymmdd_, 100, "L1SZ", [], $datagen_start_hhmm_, $datagen_end_hhmm_ );
  my ($avgstdev_, undef) = GetFeatureAverageDays ( $self_shortcode_, $yyyymmdd_, 100, "STDEV", [], $datagen_start_hhmm_, $datagen_end_hhmm_ );
  my ($avgvol_, undef) = GetFeatureAverageDays ( $self_shortcode_, $yyyymmdd_, 100, "VOL", [], $datagen_start_hhmm_, $datagen_end_hhmm_ );
  my $ticksize_ = `$BIN_DIR/get_min_price_increment $self_shortcode_ $yyyymmdd_ 2>/dev/null`; chomp ( $ticksize_ );

  print "L1SZ: $avgl1sz_, STDEV: $avgstdev_, VOLUME: $avgvol_, TICKSIZE: $ticksize_\n";

  my @levels_options_ = (2,3,4,5,6,8,10,15,20,25,30,35,40,50);
  my @sizes_to_seek_options_ = (5,10,20,50,200,400,800,2000,4000);

  my @num_levels_ = ( );
  my @num_levels_init_ = map { $_ * $avgstdev_ / $ticksize_ } (1,2,3);
  foreach my $num_level_ ( @num_levels_init_ ) {
    $num_level_ = (grep $_ >= $num_level_ , @levels_options_)[0];
    if ( defined $num_level_ && ! FindItemFromVec ( $num_level_, @num_levels_ ) ) { push ( @num_levels_, $num_level_ ); }
  }

  my @sizes_to_seek_ = ( );
  my @sizes_to_seek_init_ = map { $_ * $avgl1sz_ } (1,1.5);
  foreach my $tsize_ ( @sizes_to_seek_init_ ) {
    $tsize_ = (grep $_ >= $tsize_ , @sizes_to_seek_options_)[0];
    if ( defined $tsize_ && ! FindItemFromVec ( $tsize_, @sizes_to_seek_ ) ) { push ( @sizes_to_seek_, $tsize_ ); }
  }

  print "LEVELS: ".join(" ", @num_levels_)."\n";
  print "LSIZE ".join(" ", @sizes_to_seek_)."\n";

  my @decay_factor_vec_ = (0.4,0.6,0.8);

# SELF indicators
  foreach my $this_self_indicator_filename_ ( @self_files_ ) {
    next if ( ! ExistsWithSize ( $this_self_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_self_indicator_filename_ " or PrintStacktraceAndSendMailDie ( "Could not open $this_self_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;   

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      my @ind_parts = split(/\s+/, $this_indicator_body_);
      next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($self_shortcode_) );

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;

      my @this_indicator_body_vec_ = ( $this_indicator_body_ );
      if ( $this_indicator_body_ =~ /\bLEVELS\b/ ) {
        @this_indicator_body_vec_ = ( );
        foreach my $num_level_ ( @num_levels_ ) {
           my $this_indicator_body_t_ = $this_indicator_body_;
           $this_indicator_body_t_ =~ s/\bLEVELS\b/$num_level_/g;
           push ( @this_indicator_body_vec_, $this_indicator_body_t_ );
        }
      }

      my @this_indicator_body_temp_vec_ = @this_indicator_body_vec_;
      @this_indicator_body_vec_ = ( );
      foreach my $this_indicator_body_ ( @this_indicator_body_temp_vec_ ) {
        if ( $this_indicator_body_ =~ /\bLSIZE\b/ ) {
          foreach my $tsize_ ( @sizes_to_seek_ ) {
            my $this_indicator_body_t_ = $this_indicator_body_;
            $this_indicator_body_t_ =~ s/\bLSIZE\b/$tsize_/g;
            push ( @this_indicator_body_vec_, $this_indicator_body_t_ );
          }
        }
        else {
          push ( @this_indicator_body_vec_, $this_indicator_body_ );
        }
      }

      @this_indicator_body_temp_vec_ = @this_indicator_body_vec_;
      @this_indicator_body_vec_ = ( );
      foreach my $this_indicator_body_ ( @this_indicator_body_temp_vec_ ) {
        if ( $this_indicator_body_ =~ /\bDECAY_FACTOR\b/ ) {
          foreach my $decay_factor_ ( @decay_factor_vec_ ) {
            my $this_indicator_body_t_ = $this_indicator_body_;
            $this_indicator_body_t_ =~ s/\bDECAY_FACTOR\b/$decay_factor_/g;
            push ( @this_indicator_body_vec_, $this_indicator_body_t_ );
          }
        }
        else {
          push ( @this_indicator_body_vec_, $this_indicator_body_ );
        }
      }

      push ( @full_indicator_body_vec_, @this_indicator_body_vec_ );
    }
  }

# SOURCE indicators
  foreach my $this_source_indicator_filename_ ( @source_files_ ) {
    next if ( ! ExistsWithSize ( $this_source_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_source_indicator_filename_ " or PrintStacktraceAndSendMailDie ( "Could not open $this_source_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      my @ind_parts = split(/\s+/, $this_indicator_body_);
      my $is_ow_ind = IsOWIndicator($ind_parts[0]);
      my $is_projected_indicator_ = IsProjectedIndicator($this_indicator_body_);

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;
      
      foreach my $this_source_shortcode_ ( @source_vec_ ) {
        next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($this_source_shortcode_) );
        next if ( ($is_projected_indicator_) && (!IsProjectedPairShc($self_shortcode_, $this_source_shortcode_)) );

        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bSOURCE\b/$this_source_shortcode_/g;

        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }
    }
  }

# NOSELFSOURCE indicators
  foreach my $this_source_indicator_filename_ ( @noselfsource_files_ ) {
    next if ( ! ExistsWithSize ( $this_source_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_source_indicator_filename_ " or PrintStacktraceAndSendMailDie ( "Could not open $this_source_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      my @ind_parts = split(/\s+/, $this_indicator_body_);
      my $is_ow_ind = IsOWIndicator($ind_parts[0]);
      my $is_projected_indicator_ = IsProjectedIndicator($this_indicator_body_);

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;
      
      foreach my $this_source_shortcode_ ( @noselfsource_vec_ ) {
        next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($this_source_shortcode_) );
        next if ( ($is_projected_indicator_) && (!IsProjectedPairShc($self_shortcode_, $this_source_shortcode_)) );

        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bNOSELFSOURCE\b/$this_source_shortcode_/g;

        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }
    }
  }

# SOURCECOMBO indicators
  foreach my $this_sourcecombo_indicator_filename_ ( @sourcecombo_files_ ) {
    next if ( ! ExistsWithSize ( $this_sourcecombo_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_sourcecombo_indicator_filename_ " or PrintStacktraceAndSendMailDie ( "Could not open $this_sourcecombo_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      my @ind_parts = split(/\s+/, $this_indicator_body_);
      my $is_ow_ind = IsOWIndicator($ind_parts[0]);
      my $is_projected_indicator_ = IsProjectedIndicator($this_indicator_body_);

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;
      
      foreach my $this_sourcecombo_shortcode_ ( @sourcecombo_vec_ )
      {
# Check if shc is !win && !ind && ind ~ mult.*combo then skip
# Skip PCA Indicators if dep is not in portfolio
# Skip Combo indicators if the portfolio has products with mixed correlations (such as ZN_0 and ES_0 ) except the Offline ones
          next if ( ( SkipComboBookIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_ , $this_sourcecombo_indicator_filename_ ) ) ||
                   ( SkipPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) ) ||
                   ( SkipMixedComboIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) ) ||
                   ( SkipVolPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) )
                  ) ;

        next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($this_sourcecombo_shortcode_) );
        next if ( ($is_projected_indicator_) && (!IsProjectedPairShc($self_shortcode_, $this_sourcecombo_shortcode_)) );

        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bSOURCECOMBO\b/$this_sourcecombo_shortcode_/g;

        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }

      if ( $this_indicator_body_ =~ m/Port\b/ ) {
        foreach my $this_sourcecombo_shortcode_ ( @sourcecombo_vec_ )
        {
          next if ( ( SkipPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) ) ||
              ( SkipVolPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) )
              ) ;

          next if ( IsOWIndicator($ind_parts[0]) && OrderInfoAbsent($this_sourcecombo_shortcode_) );
          next if ( ($is_projected_indicator_) && (!IsProjectedPairShc($self_shortcode_, $this_sourcecombo_shortcode_)) );

          my $t_this_indicator_body_ = $this_indicator_body_;
          $t_this_indicator_body_ =~ s/\bSOURCECOMBO\b/$this_sourcecombo_shortcode_/g;

          push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
        }
      }
    }
  }

#CURVE indicators    
  foreach my $this_curve_indicator_filename_ ( @curve_files_ ) {
    next if ( ! ExistsWithSize ( $this_curve_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_curve_indicator_filename_ " or PrintStacktraceAndSendMailDie ( "Could not open $this_curve_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      $this_indicator_body_ =~ s/\bSELF\b/$self_shortcode_/g;
      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;
      
      foreach my $indeps_ ( @curve_indep_shc_vec_vec_ ) {
        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bINDEP1\b/$$indeps_[0]/g;
        $t_this_indicator_body_ =~ s/\bINDEP2\b/$$indeps_[1]/g;
        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }
    }
  }

#INDEP_TRADE indicators
  foreach my $this_trade_indicator_filename_ ( @indep_trade_files_ ) {
    next if ( ! ExistsWithSize ( $this_trade_indicator_filename_ ) ) ;

    open THIS_ILIST_FILEHANDLE, "< $this_trade_indicator_filename_ " or PrintStacktraceAndSendMailDie ( "Could not open $this_trade_indicator_filename_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;

      $this_indicator_body_ =~ s/\bBASEPRICETYPE\b/$dep_base_pricetype_/g;
      
      foreach my $this_source_shortcode_ ( @trade_indepsource_vec_ ) {
        my $t_this_indicator_body_ = $this_indicator_body_;
        $t_this_indicator_body_ =~ s/\bSELF\b/$this_source_shortcode_/g;
        push ( @full_indicator_body_vec_, $t_this_indicator_body_ );
      }
    }
  }

# USER provided custom ilist files
  foreach my $this_ilist_file_ ( @custom_ilist_files_ ) {
    next if ( ! ExistsWithSize ( $this_ilist_file_ ) );

    open THIS_ILIST_FILEHANDLE, "< $this_ilist_file_ " or PrintStacktraceAndSendMailDie ( "Could not open $this_ilist_file_\n" );
    my @ilines_ = <THIS_ILIST_FILEHANDLE>; chomp ( @ilines_ );
    close THIS_ILIST_FILEHANDLE;

    foreach my $this_indicator_body_ ( @ilines_ ) {
      next if ( $this_indicator_body_ !~ m/INDICATOR 1.00 / );
      $this_indicator_body_ =~ s/INDICATOR 1.00\s+// ;
      push ( @full_indicator_body_vec_, $this_indicator_body_ );
    }
  }
  
  # replace multiple whitespaces in each indicator with single space
  map { s/\s+/ /g } @full_indicator_body_vec_;

  @indicator_body_vec_ = GetUniqueList ( @full_indicator_body_vec_ );
}

sub GetIbToCalc
{
  # if recompute = 2 recompute for all the indicators
  # if recompute = 1 add those indicators with undefined correlation to array zero_correlation_indicator; and will also recompute for these indicators;
  # if recompute = 0 array zero_correlation_indicator will be empty 
  my $date_ = shift;
  if ($recompute == 2){ return  @indicator_body_vec_;} 
  
  my @zero_correlation_indicator = ( );
  if ($recompute == 1)
  {
    foreach my $pred_duration (@pred_duration_vec_){
      foreach my $filter__ (@all_filters_){
        foreach my $indicator_body (@indicator_body_vec_)
        { my @indicator_vec_ = split(/\ /, $indicator_body);
          my $indicator_ = $indicator_vec_[2];
          my @corr_ = FetchIndStats($self_shortcode_, $timeperiod_string_, $date_, $pred_duration , $predalgo_,$filter__, $dep_base_pricetype_, $dep_pred_pricetype_, $indicator_);
          if ( ! defined $corr_[0] ){ push(@zero_correlation_indicator,$indicator_body);}
        }
      }
    }
    if ( $#zero_correlation_indicator >= 0 ) {
      $main_log_file_handle_->print( "Null/Undefined correlation exists for  ".scalar(@zero_correlation_indicator)." indicators for  $date_ \n" );
    }
  }

  if ( ! exists $date_to_indicator_to_count_{$date_} )
  {
    $main_log_file_handle_->print( "no correlation data exists for all indicators for  $date_ \n" );
    return @indicator_body_vec_ ;
  }
   
  my @ib_to_calc_vec_ = grep { ! exists $date_to_indicator_to_count_{$date_}{$_} # include those indicators which are not present for the date
  || $date_to_indicator_to_count_{$date_}{$_} < $total_count_req_               # include those indicators which were not computed for all pred duration and filter for the date
  || $_ ~~ @zero_correlation_indicator} @indicator_body_vec_;                   # include those indicators which have undefined correlation in case of recompute =1 

  my @ib_data_absent_ = grep { ! exists $date_to_indicator_to_count_{$date_}{$_} } @ib_to_calc_vec_;

  $main_log_file_handle_->print( "No correlation data exists for ".scalar(@ib_data_absent_)." indicators for  $date_ \n" );
  $main_log_file_handle_->print( "Insufficient correlation data exists for ".(scalar(@ib_to_calc_vec_)-scalar(@ib_data_absent_))." indicators for  $date_ \n" );
  $main_log_file_handle_->print( "Computing Correlation for ".scalar(@ib_to_calc_vec_)." indicators for  $date_ \n" );

  my @ib_to_calc_vec_sorted_ = sort @ib_to_calc_vec_ ;
  return @ib_to_calc_vec_sorted_ ;
}

sub DataGenAndICorrMap
{
  my $main_ilist_filehandle_ = FileHandle->new;
  my %indicator_shcvec_map_ = ( );

  my $ilist_header_ = "MODELINIT DEPBASE $self_shortcode_ $dep_base_pricetype_ $dep_pred_pricetype_\n";
  $ilist_header_ .= "MODELMATH LINEAR CHANGE\nINDICATORSTART";

  foreach my $curr_date_ ( @dates_vec_ ) 
  {
    $main_log_file_handle_->print( "Trading date in datagenandicorrmap $curr_date_\n" );

    my $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $self_shortcode_ $curr_date_` ; chomp ( $min_price_increment_ );

# for this trading date look up the map and see which indicator_body_ we do not have data for.
    my @ib_to_calc_vec_ = GetIbToCalc ( $curr_date_ );

    if ( $#ib_to_calc_vec_ < 0 )
    {
      $main_log_file_handle_->print( "ib_to_calc_vec_ is empty for $curr_date_, going to previous day \n" );
      $curr_date_ = `$BIN_DIR/calc_prev_week_day $curr_date_ 1` ;
      next;
    }

    $main_log_file_handle_->print( "Building ilist for $curr_date_\n");

    my @data_absent_vec = CheckIlistData($curr_date_,\@ib_to_calc_vec_, \%indicator_shcvec_map_);
    my @ib_to_calc_data_absent_idx_ = grep { $data_absent_vec[$_] } 0..$#ib_to_calc_vec_;
    $main_log_file_handle_->print( join("\n", map { "no-data-hence-skipping ib ".$ib_to_calc_vec_[ $_ ] } @ib_to_calc_data_absent_idx_)."\n" );

    my @ib_to_calc_data_present_idx_ = grep { ! $data_absent_vec[$_] } 0..$#ib_to_calc_vec_;
    @ib_to_calc_vec_ = map { $ib_to_calc_vec_[ $_ ] } @ib_to_calc_data_present_idx_;

    my @indicator_list_filename_vec_ = ();
    my $indicator_list_filename_index_ = 0;
    my $ib_idx_ = 0;

    while ( $ib_idx_ <= $#ib_to_calc_vec_ ) 
    {
      my $next_ib_idx_ = min( $ib_idx_ + $MAX_INDICATORS_IN_FILE, scalar @ib_to_calc_vec_ );

      my @indicators_vec_ = map { "INDICATOR 1.00 ".$ib_to_calc_vec_[ $_ ] } $ib_idx_..($next_ib_idx_-1);

      my $new_indicator_list_filename_ = $work_dir_."/ilist_file_".$curr_date_."_".$indicator_list_filename_index_.".txt"; 

      $main_ilist_filehandle_->open ( "> $new_indicator_list_filename_ " ) or PrintStacktraceAndSendMailDie ( "Could not open $new_indicator_list_filename_\n" );
      print $main_ilist_filehandle_ $ilist_header_."\n".join("\n", @indicators_vec_)."\nINDICATOREND\n";
      close $main_ilist_filehandle_;

      $ib_idx_ = $next_ib_idx_;
      push ( @indicator_list_filename_vec_, $new_indicator_list_filename_ );
      push ( @intermediate_files_, $new_indicator_list_filename_ ); 
      $indicator_list_filename_index_ ++;
    }
    if ( $#ib_to_calc_vec_ >= 0 )
    { 
      for ( my $j = 0 ; $j <= $#indicator_list_filename_vec_ ; $j ++ )
      {
        my $indicator_list_filename_ = $indicator_list_filename_vec_[$j];

        if ( $distributed_ ) {
          my $celery_ilist_name_ = $celery_shared_location.$indicator_list_filename_;
          my $copy_cmd_ = "mkdir -p `dirname $celery_ilist_name_`; cp $indicator_list_filename_ $celery_ilist_name_";
          `$copy_cmd_`;
          push ( @intermediate_files_, $celery_ilist_name_ );
          $indicator_list_filename_ = $celery_ilist_name_;
        }

        my $corr_filename_ = $work_dir_."/corr_list_".$curr_date_."_".$j.".txt";

        my $cmd_for_corrs_ = "$MODELSCRIPTS_DIR/ilist_to_multiple_correlations_to_DB.pl $self_shortcode_ $timeperiod_string_ $indicator_list_filename_ $curr_date_ $datagen_start_hhmm_ $datagen_end_hhmm_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ $min_price_increment_ $predalgo_ ".join(",", @filter_list_ )." ".join(",", @pred_duration_vec_)." > $SHARED_LOG_LOC/log.$curr_date_.$j";

        my $trade_per_sec_file = "";
        if ( $isFVPresent_ == 1 )
        {
#do the trade volume file generation part
          my $rnd_num_ =  int( rand(10000000) );
          $trade_per_sec_file = $DATAGEN_LOGDIR.$rnd_num_."_".$self_shortcode_."_trd_per_sec";
          $cmd_for_corrs_ ="$LIVE_BIN_DIR/daily_trade_aggregator $self_shortcode_ $curr_date_ $trade_per_sec_file ; ".$cmd_for_corrs_." ".$trade_per_sec_file." ".$fsudm_level_." 0.1,1.0 ; rm $trade_per_sec_file ";
        }
        else {
          $cmd_for_corrs_ = $cmd_for_corrs_." INVALIDFILE ".$fsudm_level_." 0.1,1.0";
        }
#add the 1.0 for tail correlation

        if( ! $distributed_ ) {
          $cmd_for_corrs_ = "bash -c \"".$cmd_for_corrs_."\" > $corr_filename_;";
        }

        $main_log_file_handle_->print ( $cmd_for_corrs_."\n" );

        push ( @independent_parallel_commands_ , $cmd_for_corrs_ );
        push ( @temp_corrlist_file_list_, $corr_filename_ );
        push ( @intermediate_files_, $corr_filename_ );
      }
    }
  }
  ExecuteCommands();

}

sub ExecuteCommands
{
  if ( $distributed_ ) {
  my $commands_file_id_  = `date +%N`;
  chomp( $commands_file_id_ );
  my $commands_file_ = $SHARED_LOCATION."/".$commands_file_id_;
  my $commands_file_handle_ = FileHandle->new;
  $commands_file_handle_->open ( "> $commands_file_ " ) or PrintStacktraceAndDie ( "Could not open $commands_file_ for writing\n" );
  print $commands_file_handle_ "$_ \n" for @independent_parallel_commands_;
  close $commands_file_handle_;
  my $dist_exec_cmd_ = "";
  $dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $commands_file_ -s 1";
  $dist_exec_cmd_ = $dist_exec_cmd_." -d indicator_stats_$self_shortcode_";
  
  print "Command executed: $dist_exec_cmd_ \n";
  my @output_lines_ = `$dist_exec_cmd_`;
  chomp( @output_lines_ );
  print "$_ \n" for @output_lines_;
  }
  else {
  foreach my $command ( @independent_parallel_commands_ ) {
    my @output_lines_ = `$command`;
    my $return_val_ = $?;
    if ( $return_val_ != 0 ) {
      print "Output: \n".join ( "", @output_lines_ )."\n";
    }
  }
  }
}

sub PrintStacktraceAndSendMailDie
{
  my $err_str_ = shift;

  $e_txt_ .= "\nERROR: $err_str_\n";
  PrintStacktraceAndDie ( $err_str_ );
}

sub signal_handler
{
  die "Caught a signal $!\n";
}

sub CreateLockFileOrExit 
{
  my $lockfile_ = $HOME_DIR."/locks/generate_multiple_indicator_stats_".$self_shortcode_."_".$timeperiod_string_."_".$dep_base_pricetype_."_".$dep_pred_pricetype_.".lock" ;

  { # check for aws specific lock
    my $hostname_ = `hostname`;
    if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
    { # AWS
      $lockfile_ = "/mnt/sdf/locks/generate_multiple_indicator_stats_".$self_shortcode_."_".$timeperiod_string_."_".$dep_base_pricetype_."_".$dep_pred_pricetype_.".lock";
    }
  }

  if ( -e $lockfile_ ) 
  {
    $e_txt_ = $e_txt_."$lockfile_ already exists .. exiting... \n" ;
    print STDERR $e_txt_."\n";
    exit ( 0 ) ;
  }
  open LOCKFILEHANDLE, "> $lockfile_" or PrintStacktraceAndSendMailDie ( "$SCRIPTNAME could not open $lockfile_\n" );
  flock ( LOCKFILEHANDLE, LOCK_EX );
  $is_lock_created_by_this_run_ = 1;

  return $lockfile_;
}

sub END
{
  if ( $is_lock_created_by_this_run_ )
  {
    $main_log_file_handle_->close;
    `rm -f $main_log_file_.gz`; `gzip $main_log_file_`;

    close ( LOCKFILEHANDLE );          
    `rm -f $LOCKFILE`;
  }
  ReportSummary ( $e_sub_, $e_add_, $e_txt_ ) ;  
}
