#!/usr/bin/perl

# \file ModelScripts/find_optimal_max_loss.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use POSIX;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use Data::Dumper;

sub GetRunningQueries;
sub GetUnitTradeSizesForStrats;
sub GetGlobalResultsForRunningStrats;
sub ChooseMaxLoss;
sub GetStratNameFromResultLine;
sub IsStructuredQuery;
sub GetExpectedStdev;
sub GetStdevMap;
sub GetStdevMapL1Norm;
sub GetStdevStrats;
sub GetHighVol;
sub GetLowVol;
sub ChooseMaxLossAllDays;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $WF_DB_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward/wf_db_utils";
my $WF_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward";

my $GLOBAL_RESULTS_DIR = "DB";
my $GLOBAL_STAGEDRESULTS_DIR = "DB";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/global_results_methods.pl"; # GetPnlFromGlobalResultsLine , GetMinPnlFromGlobalResultsLine
require "$GENPERLLIB_DIR/strat_utils.pl"; # CheckIfRegimeParam
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFeatureAverage
require "$GENPERLLIB_DIR/config_utils.pl"; # IsValidConfig
require "$GENPERLLIB_DIR/option_strat_utils.pl"; # IsOptionStrat

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD DAYS_TO_LOOK_BEHIND=200 RATIO_OF_NUM_OF_MAX_LOSS_HITS=0.05 RATIO_OF_HIGH_VOL_DAYS=0.2 STRAT_NAME [ENDDATE=TODAY] [Volatile_flag] [SKIP_DATES_FILE=INVALIDFILE] [results_dir=~/ec2_globalresults]";

if ( $#ARGV < 5 ) { print $USAGE."\n"; exit ( 0 ); }

my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $days_to_look_behind_ = 200;
my $ratio_ = 0.05;
my $num_top_losses_ = 5;
my $low_vol_ratio_ = 0.03;
my $ratio_high_vol_days_ = 0.2;
my $strat_name_ = "";
my $end_date_ = $today_yyyymmdd_;
my $volatile_flag_ = 0;
my $skip_dates_file_ = "INVALIDFILE";

if ( $#ARGV > 1 ) { $days_to_look_behind_ = $ARGV [ 2 ]; }
if ( $#ARGV > 2 ) { $ratio_ = $ARGV [ 3 ]; }
if ( $#ARGV > 3 ) { $ratio_high_vol_days_ = $ARGV [ 4 ]; }

if ( $#ARGV > 4 ) { $strat_name_ = $ARGV [ 5 ]; }

if ( $strat_name_ && !IsPoolStrat($strat_name_) && IsStagedStrat($strat_name_))
{
  $GLOBAL_RESULTS_DIR = $GLOBAL_STAGEDRESULTS_DIR;
}

if ( $#ARGV > 5 ) { $end_date_ = $ARGV [ 6 ]; }
if ( $#ARGV > 6 ) { $volatile_flag_ = $ARGV [ 7 ]; }
if ( $#ARGV > 7 ) { $skip_dates_file_ = $ARGV [ 8 ]; }
if ( $#ARGV > 8 ) { $GLOBAL_RESULTS_DIR = $ARGV [ 9 ]; }

my $start_date_ = CalcPrevWorkingDateMult ( $end_date_ , $days_to_look_behind_ );
$end_date_ = CalcPrevWorkingDateMult ( $end_date_ , 1 );

if ( ! ( $skip_dates_file_ eq "INVALIDFILE" ) )
{
  open FILE, " < $skip_dates_file_" or die "can't open '$skip_dates_file_': $!";
  my @skip_dates_vector_ = <FILE>;
  close FILE;
  @skip_dates_vector_ = sort {$a <=> $b}  @skip_dates_vector_;
  for ( my $i = $#skip_dates_vector_; $i >= 0; $i = $i - 1 )
  {
    my $date_ = $skip_dates_vector_ [ $i ];
    if ( ( $date_ >= $start_date_ ) && ( $date_ <= $end_date_ ) )
    {
      $start_date_ = CalcPrevWorkingDateMult ( $start_date_ , 1 );
    }
  }
}

my $use_notional_ = 0;
if ( $shortcode_ =~ /^(NSE_|BSE_)/ ) {
  $use_notional_ = 1;
}

my %stdev_map_ = ( );
my $avg_vol_all_days_ = 0;
my %strat_to_stdev_ = ( );

my $high_vol_max_loss_;
my $low_vol_max_loss_;
my $normal_max_loss_;
my $trading_location_ = GetTradingLocationForShortcode ( $shortcode_ , $today_yyyymmdd_ );

my @unique_id_ = `date +\%s\%N`; chomp ( @unique_id_ );
my $OML_strats_file_ = "/spare/local/".$USER."/OML_strats_file_".$shortcode_.$unique_id_ [ 0 ];

# Get a list of running queries.
my $SEE_STATS_OF_RUNNING_QUERIES = $SCRIPTS_DIR."/see_stats_of_running_queries.sh";

my @running_queries_ = ( );
my %strat_to_results_ = ( );
my %strat_to_unit_trade_size_ = ( );

GetRunningQueries ( );
if ( $#running_queries_ < 0 ) {
  PrintStacktraceAndDie( "No Running queries" );
}

GetGlobalResultsForRunningStrats ( );
if ( ! %strat_to_results_ ) {
  PrintStacktraceAndDie( "Results for the query could not be fetched" );
}

GetUnitTradeSizesForStrats ( );
if ( ! %strat_to_unit_trade_size_ ) {
  PrintStacktraceAndDie( "UTS for the query could not be fetched" );
}

GetStdevMap ( );
if ( ! %stdev_map_ ) {
  PrintStacktraceAndDie( "STDEV for the query could not be fetched" );
}

GetStdevStrats();

my @highvol_days_ = ( );
my @lowvol_days_ = ( );

my $V_0 = GetLowVol ( \@lowvol_days_ );
my $V_1 = GetHighVol ( \@highvol_days_ );

my $V_e = GetExpectedStdev ( );
if ( $volatile_flag_ ) {
  $V_e = $V_1;
}

print "\nV_0 ". $V_0. " V_1 ". $V_1. " V_e $V_e"."\n";

my ($L_0, $L_1, $L_e);

if ( $V_1 > 1.5 * $V_0 ) {
  print "\nLowVol:\n";
  $low_vol_max_loss_ = ChooseMaxLoss ( \@lowvol_days_, $low_vol_ratio_ );
  print "HighVol:\n";
  $high_vol_max_loss_ = ChooseMaxLoss ( \@highvol_days_, $ratio_ );

  $L_0 = abs( $low_vol_max_loss_ );
  $L_1 = abs( $high_vol_max_loss_ );

  print "\nL_0: ". $L_0. " L_1: ". $L_1."\n";

  if ( $L_1 > $L_0 ) {
    $L_e = int ( min( $L_1, max( $L_0, ( $L_0 + ( ( ( $L_1 - $L_0 ) / ( $V_1 - $V_0 ) ) * ( $V_e - $V_0 ) ) ) ) ) );
  }
  else {
    $L_e = $L_0;
  }
}
else {
  print "Difference b/w V_0 and V_1 not significant.. Hence Computing Optimal on All days\n";
  my @all_dates_ = keys %{ $strat_to_stdev_{$running_queries_[ 0 ]} };
  $normal_max_loss_ = ChooseMaxLoss ( \@all_dates_, $ratio_ );
  $L_e = abs( $normal_max_loss_ );
}

print "Max Loss for $end_date_: $L_e \n";

exit ( 0 );

sub GetExpectedStdev
{
  my $ndays_ = 2;
  my @stdevs_ = @stdev_map_{ reverse sort keys %stdev_map_ };
  my @stdev_ndays_ = @stdevs_[ 0..($ndays_-1) ];

  my $avg_stdev_ = GetAverage( \@stdev_ndays_ );

  return $avg_stdev_;
}

sub GetRunningQueries
{
  if ( $strat_name_ )
  {
    push ( @running_queries_ , $strat_name_ );
    return;
  }
  my $SEE_STATS_OF_RUNNING_QUERIES = $SCRIPTS_DIR."/see_stats_of_running_queries.sh";
  @running_queries_ = `$SEE_STATS_OF_RUNNING_QUERIES $trading_location_ $shortcode_ $days_to_look_behind_ $timeperiod_`;
  for ( my $t_rq_ = 0 ; $t_rq_ <= $#running_queries_ ; $t_rq_++ )
  {
    $running_queries_ [ $t_rq_ ] = GetStratNameFromResultLine ( $running_queries_ [ $t_rq_ ] );
  }
  return;
}

sub GetStdevMap
{
  my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
  my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $end_date_ $days_to_look_behind_ $skip_dates_file_";
  my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
  my @dates_vec_ = split ( ' ', $exec_output_ );

  foreach my $date_ ( @dates_vec_ ) {
    my ($t_avg_, $is_valid_) = GetFeatureAverage ( $shortcode_, $date_, "STDEV", [], "0000", "2400", 0);
#    print "$date_ $t_avg_ $is_valid_\n";
    if ( $is_valid_ ) {
      $stdev_map_{ $date_ } = $t_avg_;
    }
  }
}

sub GetStdevMapL1Norm
{
  my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
  my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $end_date_ $days_to_look_behind_";
  my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
  my @dates_vec_ = split ( ' ', $exec_output_ );

  foreach my $date_ ( @dates_vec_ ) {
    my $l1_norm_file_ = "/spare/local/L1Norms/".$date_."/".$shortcode_."_l1norm_value";

    if ( ! -f $l1_norm_file_ ) {
      open L1NORMFILEHANDLE, "< $l1_norm_file_ " or die  "$0 Could not open l1_norm file : $l1_norm_file_ for reading\n" ;
      my $stdevs_ = readline( L1NORMFILEHANDLE );
      my @result_stdev_ = split( ' ', $stdevs_);
      $stdev_map_{ $date_ } = $result_stdev_ [0];
      close L1NORMFILEHANDLE;
    }
  }
}

sub GetStdevStrats
{
  my $l1_norm_file_ = "";
  foreach my $running_query_ ( @running_queries_ )
  {
#      print $running_query_."\n";
    my $num_days_ = 0;
    $avg_vol_all_days_ = 0;
    my @vol_skip_dates_ = ( );
    foreach my $result_line_ ( @ { $strat_to_results_ { $strat_name_ } } )
    {
      my @result_line_words_ = split ( ' ' , $result_line_ );
      my $date_ = $result_line_words_ [ 0 ];

      if ( ! exists $strat_to_unit_trade_size_ { $strat_name_ }{ $date_ } ) {
        next;
      }

      my $t_final_pnl_ = $result_line_words_ [ 1 ];
      my $t_min_pnl_ = $result_line_words_ [ 9 ];
      $t_min_pnl_ /= $strat_to_unit_trade_size_ { $strat_name_ }{ $date_ };
      $t_final_pnl_ /= $strat_to_unit_trade_size_ { $strat_name_ }{ $date_ };

      if ( ! exists $stdev_map_{ $date_ } ) {
        push ( @vol_skip_dates_, $date_ );
        $strat_to_stdev_ {$strat_name_}{ $date_ }{ 'Stdev' } = 0
      } 
      else {
        $strat_to_stdev_ {$strat_name_}{ $date_ }{ 'Stdev' } = $stdev_map_{ $date_ };
        $avg_vol_all_days_ += $stdev_map_{ $date_ };
        $num_days_ = $num_days_ + 1;
      }
      $strat_to_stdev_ {$strat_name_}{ $date_ }{ 'Final Pnl' } = $t_final_pnl_;
      $strat_to_stdev_ {$strat_name_}{ $date_ }{ 'Min Pnl' } = $t_min_pnl_;
#print "$date_ ".$strat_to_stdev_ {$strat_name_}{ $date_ }{ 'Stdev' }." $t_min_pnl_ $t_final_pnl_\n";
    }
    if ( $#vol_skip_dates_ >= 0 ) {
      print "Warning: Unable to find l1norm-files for dates ".join(" ", @vol_skip_dates_)."\n";
    }
    $avg_vol_all_days_ = $avg_vol_all_days_ / $num_days_ ;
#      print $avg_vol_all_days_."avg Vol \n";
  }
}

sub GetHighVol 
{
  my $highvol_days_ref_ = shift;

  my $strat_name_ = $running_queries_[ 0 ];

  my %specific_strat_details_ = %{ $strat_to_stdev_{$strat_name_} };

  my @sort_stdev_data_ = sort{ $specific_strat_details_{$b}{Stdev} <=> $specific_strat_details_{$a}{Stdev} } keys %specific_strat_details_;

  my $num_high_vol_days_ = max(0, min( $#sort_stdev_data_, int( $days_to_look_behind_ * $ratio_high_vol_days_ ) - 1 ) );
  @$highvol_days_ref_ = @sort_stdev_data_[ 0..$num_high_vol_days_];

  print "No. of days for ChooseMaxLossHighVol: ".($#$highvol_days_ref_ + 1)."\n";

  my @high_vols_ = map { $specific_strat_details_{$_}{'Stdev'} } @$highvol_days_ref_;
  return GetSum ( \@high_vols_ ) / ( $#high_vols_ + 1 );
}
 
sub GetLowVol 
{
  my $lowvol_days_ref_ = shift;

  my $strat_name_ = $running_queries_[ 0 ];

  my %specific_strat_details_ = %{ $strat_to_stdev_{$strat_name_} };

  my @sort_stdev_data_ = sort{ $specific_strat_details_{$b}{Stdev} <=> $specific_strat_details_{$a}{Stdev} } keys %specific_strat_details_;
  
  my $num_high_vol_days_ = max(0, min( $#sort_stdev_data_, int( $days_to_look_behind_ * $ratio_high_vol_days_ ) - 1 ) );
  @$lowvol_days_ref_ = @sort_stdev_data_[ $num_high_vol_days_..$#sort_stdev_data_ ];

  print "No. of days for ChooseMaxLossLowVol ".($#$lowvol_days_ref_ + 1)."\n";

  my @low_vols_ = map { $specific_strat_details_{$_}{'Stdev'} } @$lowvol_days_ref_;
  return GetSum ( \@low_vols_ ) / ( $#low_vols_ + 1 );
}

sub ChooseMaxLoss
{
  my $all_dates_ref_ = shift;
  my $ratio_ = shift;

  my @all_dates_ = @$all_dates_ref_;

  my %strat_to_max_loss_pnl_ = ( );
  my %strat_to_max_loss_hits_ = ( );

  foreach my $strat_name_ ( @running_queries_ )
  {
    my %strat_details_ = %{ $strat_to_stdev_{$strat_name_} };

    my @min_pnl_to_try_ = ();
    my @sort_min_pnl_ = sort{ $strat_details_{$a}{'Min Pnl'} <=> $strat_details_{$b}{'Min Pnl'}} @all_dates_;
    foreach my $date_ ( @sort_min_pnl_ ) {
      if( $#min_pnl_to_try_ < ( ($#all_dates_ + 1) * $ratio_ ) ) {
        push ( @min_pnl_to_try_ , $strat_details_{$date_}{ 'Min Pnl' } );
      }
    }
    
    my %max_loss_to_sum_pnl_ = ( );
    my %max_loss_to_num_times_max_loss_hit_ = ( );
    my $optimal_max_loss_;

#    printf ( "%s =>\n" , $strat_name_ );
    printf ( "%10s %10s %s\n" , "MAX_LOSS" , "AVG_PNL" , "NUM_MAX_LOSS_HITS" );

    foreach my $max_loss_ (@min_pnl_to_try_) {
      $strat_to_max_loss_pnl_{$strat_name_} = 0;
      $strat_to_max_loss_hits_{$strat_name_} = 0;
      foreach my $date_ ( @all_dates_ ) {
        if( $strat_details_{$date_}{ 'Min Pnl'} > $max_loss_ ) {
          $max_loss_to_sum_pnl_{ $max_loss_ } += $strat_details_ { $date_ }{ 'Final Pnl' };
        } 
        else{
          $max_loss_to_sum_pnl_{ $max_loss_ } += $max_loss_;
          $max_loss_to_num_times_max_loss_hit_{ $max_loss_ } += 1;
        }
      }
      $max_loss_to_sum_pnl_{ $max_loss_ } /= ($#all_dates_+1);
      if ( ! defined $optimal_max_loss_ || $max_loss_to_sum_pnl_{ $max_loss_ } > $max_loss_to_sum_pnl_{ $optimal_max_loss_ } ) {
        $optimal_max_loss_ = $max_loss_;
      }
    }

    my @max_losses_sorted_ = sort { $max_loss_to_sum_pnl_{ $b } <=> $max_loss_to_sum_pnl_{ $a } } keys %max_loss_to_sum_pnl_;
    my $lines_to_print_ = min( $#max_losses_sorted_ + 1, $num_top_losses_ );

    foreach my $max_loss_ ( @max_losses_sorted_[ 0..($lines_to_print_-1) ] ) {
      if ( ! $use_notional_ ) {
        printf ( "%10.2f %10.2lf %3d\n" , abs ( $max_loss_ ) , $max_loss_to_sum_pnl_{ $max_loss_ }, $max_loss_to_num_times_max_loss_hit_{ $max_loss_ } );
      } else {
        printf ( "%10.2e %10.2le %3d\n" , abs ( $max_loss_ ) , $max_loss_to_sum_pnl_{ $max_loss_ }, $max_loss_to_num_times_max_loss_hit_{ $max_loss_ } );
      }
    }

    $strat_to_max_loss_pnl_{ $strat_name_ } = $max_loss_to_sum_pnl_{ $optimal_max_loss_ };
    $strat_to_max_loss_hits_{ $strat_name_ } = $max_loss_to_num_times_max_loss_hit_{ $optimal_max_loss_ };
    return $optimal_max_loss_;
  }
}

sub GetUnitTradeSizesForStrats
{
  foreach my $running_query_ ( @running_queries_ )
  {
    foreach my $result_line_ ( @{ $strat_to_results_ { $running_query_ } } )
    {
      my @result_line_words_ = split ( ' ' , $result_line_ );
      my $date_ = $result_line_words_ [ 0 ];
      my $unit_trade_size_ = 1;
      if ( IsStructuredQuery ( $running_query_) )
      {
        my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$running_query_." ".$date_." 1 ".$use_notional_;
        $unit_trade_size_ = `$exec_cmd_ 2>/dev/null`;
      }
      elsif(IsValidConfig($running_query_))
      {
        my $paramname = "INVALID";
        my $cmd = $WF_SCRIPTS."/print_strat_from_config.py -c $running_query_ -d $date_";
        my $strategy_line = `$cmd`; chomp($cmd);
        if ( $strategy_line ) {
          my @strategy_line_words = split(' ', $strategy_line);
          if ($#strategy_line_words >= 4){
            $paramname = $strategy_line_words[4];
          }
        }
        #print "PARAM: $paramname $date_\n";
        my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$paramname." ".$date_." 2 ".$use_notional_;
        $unit_trade_size_ = `$exec_cmd_`;
      }
      elsif ( !IsOptionStrat ( $running_query_) )
      {
        my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$running_query_." ".$date_." 0 ".$use_notional_;
        $unit_trade_size_ = `$exec_cmd_`;
      }
      chomp ( $unit_trade_size_ );

      if ( $unit_trade_size_ eq "" ) {
        print "WARN: could not find UTS for strat: $running_query_, date: $date_\n";
        next;
      }

      if ( $use_notional_ ) {
        my @contract_specs_ = `$LIVE_BIN_DIR/get_contract_specs $shortcode_ $date_ ALL`; chomp ( @contract_specs_ );
        my ( $lotsize_, $close_px_, $contract_multiplier_ );
        foreach my $spec_line_ ( @contract_specs_ ) {
          my @swords_ = split(/[: ]+/, $spec_line_);
          given ( $swords_[0] ) {
            when ( "LOTSIZE" ) { $lotsize_ = $swords_[1]; }
            when ( "LAST_CLOSE_PRICE" ) { $close_px_ = $swords_[1]; }
            when ( "CONTRACT_MULTIPLIER" ) { $contract_multiplier_ = $swords_[1]; }
          }
        }
        my $denom_ = $lotsize_ * $close_px_ * $contract_multiplier_;

        $unit_trade_size_ = ceil( $unit_trade_size_ / $denom_ ) * $denom_;
      }
      $strat_to_unit_trade_size_ { $running_query_ }{ $date_ } = $unit_trade_size_;
    }
  }
  return;
}

sub IsStructuredQuery
{
  my $STRAT_FILE_PATH = $HOME_DIR."/modelling/stir_strats/".$shortcode_."/".$_ [ 0 ];
  if (-e $STRAT_FILE_PATH)
  {
    return $STRAT_FILE_PATH;
  }
  else
  {
    my $nm=$_[0];
    $STRAT_FILE_PATH = `ls $HOME_DIR/modelling/stir_strats/\*/\*/$nm 2>/dev/null`; chomp ( $STRAT_FILE_PATH ) ;
    if ( -e $STRAT_FILE_PATH )
    {
      return $STRAT_FILE_PATH;
    } 
    else
    {
      return "";
    }
  }
}

sub GetGlobalResultsForRunningStrats
{
  my $SUMMARIZE_STRATEGY_RESULTS_LONG = $LIVE_BIN_DIR."/summarize_strategy_results";
  my $file_handle_ = FileHandle->new;
  $file_handle_->open ( "> $OML_strats_file_ " ) or PrintStacktraceAndDie ( "Could not open $OML_strats_file_ for writing\n" );
  for ( my $i_ = 0; $i_ <= $#running_queries_; $i_++ )
  {
    print $file_handle_ "$running_queries_[ $i_ ]\n";
  }
  $file_handle_->close;
  my $exec_cmd_ = "$SUMMARIZE_STRATEGY_RESULTS_LONG $shortcode_ $OML_strats_file_ $GLOBAL_RESULTS_DIR $start_date_ $end_date_ $skip_dates_file_ kCNAPnlAdjAverage 0 INVALIDFILE 0 2>/dev/null";
#  print $exec_cmd_."\n";
  my @exec_output_ = `$exec_cmd_`;
  my $strat_name_ = "";
  foreach my $exec_output_line_ ( @exec_output_ )
  {
    if ( $exec_output_line_ eq "\n" || index ( $exec_output_line_ , "STATISTICS" ) >= 0 )
    {
      next;
    }
    if ( index ( $exec_output_line_ , "STRATEGYFILEBASE" ) >= 0 )
    {
      chomp ( $exec_output_line_ );
      my @exec_output_line_words_ = split ( ' ' , $exec_output_line_ );
      $strat_name_ = $exec_output_line_words_ [ 1 ];
      @{ $strat_to_results_ { $strat_name_ } } = ( );
    }
    else
    {
      push ( @{ $strat_to_results_ { $strat_name_ } } , $exec_output_line_ );
    }
  }
  `rm -rf $OML_strats_file_`;
  return;
}

sub GetStratNameFromResultLine
{
# 22029 16505 2.65 w_strategy_ilist_BR_DOL_0_US_Mkt_Mkt_J0.noeu_8_na_e3_20111206_20120123_EST_800_EST_1400_500_0_0_fsr.5_3_FSHLR_0.01_0_0_0.7.tt_EST_700_EST_1400.pfi_3
  my $result_line_ = shift;
  my @result_words_ = split ( ' ' , $result_line_ );
  if ( $#result_words_ >= 3 )
  {
    return $result_words_ [ $#result_words_ ];
  }
  else
  {
    return "";
  }
}
