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
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub GetRunningQueries;
sub GetUnitTradeSizesForStrats;
sub GetGlobalResultsForRunningStrats;
sub ChooseMaxLoss;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BASETRADE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $WF_DB_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward/wf_db_utils";
my $WF_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
	$LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $GLOBAL_RESULTS_DIR = $HOME_DIR."/ec2_globalresults";
my $GLOBAL_STAGEDRESULTS_DIR = $HOME_DIR."/ec2_staged_globalresults";
my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
if ( ! ( $USER eq "dvctrader" && ( ( $hostname_s_ eq "sdv-ny4-srv11" ) || ( $hostname_s_ eq "sdv-crt-srv11" ) ) ) )
{
  $GLOBAL_RESULTS_DIR = "/NAS1/ec2_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
  $GLOBAL_STAGEDRESULTS_DIR = "/NAS1/ec2_staged_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/strat_utils.pl"; # CheckIfRegimeParam
require "$GENPERLLIB_DIR/option_strat_utils.pl"; # IsOptionStrat
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats, PnlSamplesGetStatsLong
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl"; 

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD DAYS_TO_LOOK_BEHIND RATIO_OF_NUM_OF_MAX_LOSS_HITS NUM_TOP_LOSSES STRAT_LIST_FILE [ENDDATE=TODAY] [SKIP_DATES_FILE=INVALIDFILE]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $days_to_look_behind_ = $ARGV [ 2 ];
my $ratio_ = $ARGV [ 3 ];
my $num_top_losses_ = $ARGV [ 4 ];
$num_top_losses_ = int ( max(1, min ( $num_top_losses_, $ratio_*$days_to_look_behind_ ) ) ); 

my $strat_listname_ = $ARGV [ 5 ];
my $end_date_ = $today_yyyymmdd_;
my $skip_dates_file_ = "INVALIDFILE";

if ( $#ARGV > 5 ) { $end_date_ = $ARGV [ 6 ]; }
if ( $#ARGV > 6 ) { $skip_dates_file_ = $ARGV [ 7 ]; }


my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";

$end_date_ = CalcPrevWorkingDateMult ( $end_date_ , 1 );
my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $end_date_ $days_to_look_behind_ $skip_dates_file_";
my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
my @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );

my $use_notional_ = 0;
if (( $shortcode_ =~ /^(NSE_|BSE_)/ )) {
    $use_notional_ = 1;
}

my @running_queries_ = ( );
my %query_to_size_ = ( );
my %query_to_maxloss_ = ( );

my %query_to_shc_ = ( );
my %query_to_is_option_strat_ = ( );
my %query_to_is_config_ = ( );

GetRunningQueries ( );

my %strat_to_unit_trade_size_ = ( );
GetUnitTradeSizesForStrats ( );

my %combined_pnl_series_ = ( );
GetCombinedPnlSamples ( );

ChooseMaxLoss ( );

exit ( 0 );

sub GetRunningQueries
{
  open STTLIST, "< $strat_listname_" or PrintStacktraceAndDie ( "Could not open $strat_listname_ for writing\n" );
  my @running_queries_uts_ = <STTLIST>; chomp ( @running_queries_uts_ );

  foreach my $query_uts_line_ ( @running_queries_uts_ ) {

    my @qwords_ = split(" ", $query_uts_line_);
    if ( $#qwords_ < 0 ) { next; }
    push ( @running_queries_, $qwords_[0] );

    $query_to_is_option_strat_{$qwords_[0]} = 0;
   
    if ( $#qwords_ > 0 ) {
      $query_to_size_{ $qwords_[0] } = $qwords_[1];
    } else {
      $query_to_size_{ $qwords_[0] } = 1;
    }

    if ( $#qwords_ > 1 ) {
      $query_to_maxloss_{ $qwords_[0] } = -1 * abs($qwords_[2]);
    }

    if ( $#qwords_ > 2 ) {
      $query_to_shc_{ $qwords_[0] } = $qwords_[3];
    } else {
      $query_to_shc_{ $qwords_[0] } = $shortcode_;
    }

    if((IsOptionStrat($qwords_[0])) || (IsOptionStrat($qwords_[0],$query_to_shc_{ $qwords_[0] })))   
    {
      $use_notional_ = 0;
      $query_to_is_option_strat_{$qwords_[0]} = 1;
    }
   
   if (IsValidConfig($qwords_[0])){
	$GLOBAL_RESULTS_DIR = "DB";
	$query_to_is_config_{$qwords_[0]} = 1;
   }else{
   	$query_to_is_config_{$qwords_[0]} = 0;
   }

  }
}

sub GetUnitTradeSizesForStrats
{ 
  foreach my $running_query_ ( @running_queries_ )
  {
    my $shc_ = $query_to_shc_{ $running_query_ };
    my $unit_trade_size_ = 1;
    if ( IsStructuredQuery ( $running_query_) )
    {
      foreach my $date_ ( @dates_vec_ ) {
        my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shc_." ".$running_query_." ".$date_." 1 ".$use_notional_;
        $unit_trade_size_ = `$exec_cmd_`; chomp ( $unit_trade_size_ );
        $strat_to_unit_trade_size_ { $running_query_ }{ $date_ } = $unit_trade_size_;
      }
    }
    elsif($query_to_is_config_{$running_query_})
    {
      my $paramname;

      foreach my $date_ ( @dates_vec_) {
        my $cmd = $WF_SCRIPTS."/print_strat_from_config.py -c $running_query_ -d $date_";
        my $strategy_line = `$cmd`; chomp($cmd);
        if ( $strategy_line ) {
          my @strategy_line_words = split(' ', $strategy_line);
          if ($#strategy_line_words >= 4){
            $paramname = $strategy_line_words[4];
            my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$paramname." ".$date_." 2 ".$use_notional_;
            $unit_trade_size_ = `$exec_cmd_`; chomp($unit_trade_size_);
            last;
          }
        }
      }

      foreach my $date_ ( @dates_vec_) {
        $strat_to_unit_trade_size_ {$running_query_}{$date_} = $unit_trade_size_;
      }
    }
    elsif ( !$query_to_is_option_strat_{$running_query_} )
    {
      foreach my $date_ ( @dates_vec_ ) {
        my $exec_cmd_ = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$shc_." ".$running_query_." ".$date_." 0 ".$use_notional_;
        $unit_trade_size_ = `$exec_cmd_`; chomp ( $unit_trade_size_ );
        $strat_to_unit_trade_size_ { $running_query_ }{ $date_ } = $unit_trade_size_;
      }
    }
    else 
    {
      foreach my $date_ ( @dates_vec_ ) {
        $strat_to_unit_trade_size_ { $running_query_ }{ $date_ } = 1;
      }
    }
  }
  return;
}

sub GetCombinedPnlSamples
{
  my %sample_pnls_strats_vec_ = ( );

  %combined_pnl_series_ = ( );
  foreach my $strat_ ( @running_queries_ ) {
    my %sample_pnls_strats_vec_ = ( );
    my $shc_  = $query_to_shc_{$strat_};
    my @running_query_ = ($strat_);
 
    FetchPnlSamplesStrats ( $shc_, \@running_query_, \@dates_vec_, \%sample_pnls_strats_vec_, 0000, 2345, $query_to_is_config_{$strat_});
    my %t_pnl_series_ = ( );
    foreach my $t_sample_ ( keys %{ $sample_pnls_strats_vec_ { $strat_ } } ) {
      my ( $t_date_, $t_slot_ ) = split ( "_", $t_sample_ );
      $t_pnl_series_{ $t_date_ }{ $t_slot_ } = $sample_pnls_strats_vec_ { $strat_ }{ $t_sample_ } / $strat_to_unit_trade_size_ { $strat_ }{ $t_date_ };
      $t_pnl_series_{ $t_date_ }{ $t_slot_ } *= $query_to_size_{ $strat_ };
    }

    my %t_pnl_series2_ = ( );
    foreach my $t_date_ ( keys %t_pnl_series_ ) {
      my $current_pnl_ = 0;
      my $hit_maxloss_ = 0;
      foreach my $t_slot_ ( sort { $a <=> $b } keys %{$t_pnl_series_{ $t_date_ } } ) {
        if ( $hit_maxloss_ == 0 ) {
          $t_pnl_series2_{ $t_date_."_".$t_slot_ } = $t_pnl_series_{ $t_date_ }{ $t_slot_ };
          $current_pnl_ += $t_pnl_series_{ $t_date_ }{ $t_slot_ };
          if ( defined $query_to_maxloss_{ $strat_ } && $current_pnl_ <= $query_to_maxloss_{ $strat_ } ) {
            $hit_maxloss_ = 1;
          }
        } else {
          $t_pnl_series2_{ $t_date_."_".$t_slot_ } = 0;
        }
      }
    }

    if ( !%combined_pnl_series_ ) {
      %combined_pnl_series_ = %t_pnl_series2_;
    } else {
      CombinePnlSamples ( \%combined_pnl_series_, \%t_pnl_series2_, \%combined_pnl_series_ );
    }
  }
#  print join(" ", values %combined_pnl_series_)."\n";
}

sub IsStructuredQuery
{
  my $STRAT_FILE_PATH = $HOME_DIR."/modelling/stir_strats/".$shortcode_."/".$_ [ 0 ];
  if (-e $STRAT_FILE_PATH) { return $STRAT_FILE_PATH; }
  else
  {
    my $nm = $_[0];
    $STRAT_FILE_PATH = `ls $HOME_DIR"/modelling/stir_strats/*/*/"$nm 2>/dev/null`; chomp ( $STRAT_FILE_PATH ) ;
    if ( -e $STRAT_FILE_PATH ) { return $STRAT_FILE_PATH; }
    else { return ""; }
  }
}

sub ChooseMaxLoss
{
  my %pnl_stats_ = ( );
  PnlSamplesGetStatsLong ( \%combined_pnl_series_, \%pnl_stats_ );

  my @uts_vec_ = values %query_to_size_;
  my $uts_ = GetSum ( \@uts_vec_ );

  my $dates_vec_ref_ = $pnl_stats_{ "DATES" };
  my @min_pnl_list_ = map { $_ / $uts_ } @{$pnl_stats_{ "MINPNL" }};
  my @final_pnl_list_ = map { $_ / $uts_ } @{$pnl_stats_{ "PNL" }};

  foreach my $idx_ ( 0..$#min_pnl_list_ ) {
    if ( $final_pnl_list_[ $idx_ ] < $min_pnl_list_[ $idx_ ] ) {
      $final_pnl_list_[ $idx_ ] = $min_pnl_list_[ $idx_ ];
    }
  }

  my @max_losses_to_try_ = @min_pnl_list_;
  @max_losses_to_try_ = sort { $a <=> $b } ( @max_losses_to_try_ );

  my %max_loss_to_avg_pnl_ = ( );
  my %max_loss_to_num_times_max_loss_hit_ =  ( );
# Compute results for each value of max loss.
  foreach my $max_loss_ ( @max_losses_to_try_ )
  {
#    print "Trying for max_loss: ".$max_loss_."\n";

    $max_loss_to_avg_pnl_ { $max_loss_ } = 0;
    $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } = 1;

    for ( my $i = 0 ; $i <= $#min_pnl_list_ ; $i ++ )
    {
      my $t_min_pnl_ = $min_pnl_list_ [ $i ];
      my $t_final_pnl_ = $final_pnl_list_ [ $i ];

      if ( $t_min_pnl_ < $max_loss_ )
      { 
# With this value of max loss , this query would hit max loss and stop
        $max_loss_to_avg_pnl_ { $max_loss_ } += $max_loss_;
        $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } ++;
      }
      else
      { 
# Query would continue to trade till final pnl
        $max_loss_to_avg_pnl_ { $max_loss_ } += $t_final_pnl_;
      }
    }
    if ( @min_pnl_list_ ) { $max_loss_to_avg_pnl_ { $max_loss_ } /= ($#min_pnl_list_ + 1); }
  }

  printf ( "%10s %10s %s\n" , "MAX_LOSS" , "AVG_PNL" , "NUM_MAX_LOSS_HITS" );
  my $already_printed_ = 0;
  foreach my $max_loss_ ( sort { $max_loss_to_avg_pnl_ { $b } <=> $max_loss_to_avg_pnl_ { $a } } keys %max_loss_to_avg_pnl_ )
  {
    next if ( max ( $ratio_ * $days_to_look_behind_ , 1 ) < $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } ) ;	    

    if ( ! $use_notional_ ) {
      printf ( "%10.2f %10.2lf %3d\n" , abs ( $max_loss_ ) , $max_loss_to_avg_pnl_ { $max_loss_ } , $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } );
    } else {
      printf ( "%10.2e %10.2le %3d\n" , abs ( $max_loss_ ) , $max_loss_to_avg_pnl_ { $max_loss_ } , $max_loss_to_num_times_max_loss_hit_ { $max_loss_ } );
    }
    $already_printed_++;
    last if ( $already_printed_ >= $num_top_losses_ ); 
  }	
  return;
}
