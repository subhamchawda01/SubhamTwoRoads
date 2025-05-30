#!/usr/bin/perl

# \file GenPerlLib/compute_bad_sample_correlation.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use feature "switch";
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $STORE="/spare/local/tradeinfo";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
if ( $USER eq "ankit" || $USER eq "kputta" || $USER eq "rkumar" || $USER eq "diwakar" )
{
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $GLOBALRESULTSDBDIR = "DB";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec_ref.pl"; #FindItemFromVecRef
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats
require "$GENPERLLIB_DIR/array_ops.pl";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );

my $USAGE = "$0 CONFIG-FILE [NUMDAYS=350] [BAD_SAMPLES_PERCENTILE=0.2] [NSTRATS=5]";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $config_file_ = shift;
my $numdays_ = shift || 350;
my $bad_samples_percentage_ = shift || 0.2;
my $top_nstrat_ = shift || 5;
my $sort_algo_ = "kCNAPnlSharpeAverage";

my @shortcode_vec_ = ( );
my @timeperiod_vec_ = ( );

open CHANDLE, "< $config_file_" or PrintStacktraceAndDie ( "Could not open #config_file_ for reading" );
my @clines_ = <CHANDLE>; chomp ( @clines_ );
close CHANDLE;

@shortcode_vec_ = map { (split(' ', $_))[0] } @clines_;
@timeperiod_vec_ = map { (split(' ', $_))[1] } @clines_;

my %shc_to_top_strats_ = ( );
my %shc_to_pnl_samples_map_ = ( );
my %shc_to_filtered_samples_map_ = ( );

foreach my $shc_indx_ ( 0..$#shortcode_vec_ ) {
  my $shortcode_ = $shortcode_vec_[ $shc_indx_ ];
  my $timeperiod_ = $timeperiod_vec_[ $shc_indx_ ];

  my @top_strats_ = ( );
  GetTopStrats ( $shortcode_, $timeperiod_, $numdays_, $sort_algo_, $top_nstrat_, \@top_strats_ );
  $shc_to_top_strats_{ $shortcode_ } = \@top_strats_;

  print "Top strats for $shortcode_/$timeperiod_:\n".join("\n", @top_strats_ )."\n";

  my %pnl_samples_map_ = ( );
  my @filtered_samples_vec_ = ( );

  GetBadSamplesPoolForShortcode ( $shortcode_, $timeperiod_, \@top_strats_, $yyyymmdd_, $numdays_, \%pnl_samples_map_, \@filtered_samples_vec_, $bad_samples_percentage_ );
  $shc_to_pnl_samples_map_{ $shortcode_ } = \%pnl_samples_map_;
  $shc_to_filtered_samples_map_{ $shortcode_ } = \@filtered_samples_vec_;
}

foreach my $shc1_indx_ ( 0..($#shortcode_vec_-1) ) {
  foreach my $shc2_indx_ ( ($shc1_indx_+1)..$#shortcode_vec_ ) {
    my $shortcode1_ = $shortcode_vec_[ $shc1_indx_ ];
    my $timeperiod1_ = $timeperiod_vec_[ $shc1_indx_ ];

    my $shortcode2_ = $shortcode_vec_[ $shc2_indx_ ];
    my $timeperiod2_ = $timeperiod_vec_[ $shc2_indx_ ];

    my %pnl_samples_map1_ = %{ $shc_to_pnl_samples_map_{ $shortcode1_ } };
    my @filtered_samples_vec1_ = @{ $shc_to_filtered_samples_map_{ $shortcode1_ } };

    my %pnl_samples_map2_ = %{ $shc_to_pnl_samples_map_{ $shortcode2_ } };
    my @filtered_samples_vec2_ = @{ $shc_to_filtered_samples_map_{ $shortcode2_ } };

    PrintSamples ( \%pnl_samples_map1_, \%pnl_samples_map2_, "samples_".$shortcode1_."_".$timeperiod1_."_".$shortcode2_."_".$timeperiod2_.".dat" );

    my $corr_ = GetCorrelationSamples ( \%pnl_samples_map1_, \%pnl_samples_map2_ );
    print "Samples Similarity (taking all samples) b/w $shortcode1_/$timeperiod1_ and $shortcode2_/$timeperiod2_ is: ".$corr_."\n";

    $corr_ = GetCorrelationSamples ( \%pnl_samples_map1_, \%pnl_samples_map2_, \@filtered_samples_vec1_, \@filtered_samples_vec2_ );
    print "Samples Similarity b/w $shortcode1_/$timeperiod1_ and $shortcode2_/$timeperiod2_ is: ".$corr_."\n";

    $corr_ = GetCorrelationDays ( \%pnl_samples_map1_, \%pnl_samples_map2_, 1.0 );
    print "Days Similarity (taking all days) b/w $shortcode1_/$timeperiod1_ and $shortcode2_/$timeperiod2_ is: ".$corr_."\n";

    $corr_ = GetCorrelationDays ( \%pnl_samples_map1_, \%pnl_samples_map2_, $bad_samples_percentage_ );
    print "Days Similarity b/w $shortcode1_/$timeperiod1_ and $shortcode2_/$timeperiod2_ is: ".$corr_."\n";
  }
}

sub GetBadSamplesPoolForShortcode
{
  my $shortcode_ = shift;
  my $timeperiod_ = shift;
  my $strats_vec_ref_ = shift;
  my $yyyymmdd_ = shift;
  my $num_days_ = shift;
  my $samples_map_ref_ = shift;
  my $filtered_samples_vec_ref_ = shift;
  my $samples_percentage_ = shift || 0.2;

  my @all_strats_in_dir_ = MakeStratVecFromDirAndTT($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_);
  my @all_strats_basenames_ = map { basename ( $_ ) } @all_strats_in_dir_;

  my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
  my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $yyyymmdd_ $num_days_";
  my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
  my @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );

  my %sample_pnls_strats_vec_;
  FetchPnlSamplesStrats ( $shortcode_, \@all_strats_basenames_, \@dates_vec_, \%sample_pnls_strats_vec_ );

  %$samples_map_ref_ = ( );
  foreach my $tstrat_ ( @$strats_vec_ref_ ) {
    if ( ! %$samples_map_ref_ ) {
      %$samples_map_ref_ = %{ $sample_pnls_strats_vec_{ $tstrat_ } };
    } else {
      CombinePnlSamples ( $samples_map_ref_, \%{ $sample_pnls_strats_vec_{ $tstrat_ } }, $samples_map_ref_ );
    }
  }

  foreach my $k_ ( keys %$samples_map_ref_ ) {
    if ( $k_ ne ConvertKeyToFormat1($k_) ) {
      $$samples_map_ref_{ ConvertKeyToFormat1($k_) } = delete $$samples_map_ref_{ $k_ };
    }
  }
  my @samples_sorted_ = sort { $$samples_map_ref_{ $a } <=> $$samples_map_ref_{ $b } } keys %$samples_map_ref_;

  my $samples_filtered_idx_ = max( 0, int ( $samples_percentage_ * ($#samples_sorted_ + 1) )-1 );

  @$filtered_samples_vec_ref_ = @samples_sorted_[ 0..$samples_filtered_idx_ ];
}

sub PrintSamples
{
  my $samples1_map_ref_ = shift;
  my $samples2_map_ref_ = shift;
  my $fname_ = shift;

  my @samples_common_ = grep { exists $$samples2_map_ref_{ $_ } } keys %$samples1_map_ref_;
  @samples_common_ = sort @samples_common_;

  open FHANDLE, "> $fname_" or PrintStacktraceAndDie(" Could not open $fname_ for writing.." );

  foreach my $tsample_( @samples_common_ ) {
    print FHANDLE $tsample_." ".$$samples1_map_ref_{ $tsample_ }." ".$$samples2_map_ref_{ $tsample_ }."\n";
  }
  close FHANDLE;
}

sub GetCorrelation
{
  my $pnl_series1_ref_ = shift;
  my $pnl_series2_ref_ = shift;

  my $min1_ = min @$pnl_series1_ref_;
  my $max1_ = max @$pnl_series1_ref_;

  my $min2_ = min @$pnl_series2_ref_;
  my $max2_ = max @$pnl_series2_ref_;

  my @pnl_series1_ = map { ($_ - $min1_) / ($max1_ - $min1_) } @$pnl_series1_ref_;
  my @pnl_series2_ = map { ($_ - $min2_) / ($max2_ - $min2_) } @$pnl_series2_ref_;

# Compute correlation coefficient.
  my $sumx = GetSum( \@pnl_series1_ );
  my $sumy = GetSum( \@pnl_series2_ );
  my $sumxx = 0;
  my $sumyy = 0;
  my $sumxy = 0;
  my $mn_ = $#pnl_series1_ + 1;
  print "Length of the union bad-samples set: ".$mn_."\n";

  for ( my $i = 0; $i < $mn_; $i++ ) {
    $sumxx += $pnl_series1_ [ $i ] * $pnl_series1_ [ $i ];
    $sumyy += $pnl_series2_ [ $i ] * $pnl_series2_ [ $i ];
    $sumxy += $pnl_series1_ [ $i ] * $pnl_series2_ [ $i ];
  }

  my $num_ = ( $mn_ * $sumxy ) - ( $sumx * $sumy );
  my $den_ = sqrt ( ( ( $mn_ * $sumxx ) - ( $sumx * $sumx ) ) * ( ( $mn_ * $sumyy ) - ( $sumy * $sumy ) ) );
  if ( $den_ == 0 ) { return 1; }
  my $corr_ = $num_ / $den_;

  return $corr_;
}

sub GetCorrelationSamples
{
  my $samples1_map_ref_ = shift;
  my $samples2_map_ref_ = shift;
  my $filtered_samples1_ref_ = shift;
  my $filtered_samples2_ref_ = shift;

  my @samples_union_vec_ = keys %$samples1_map_ref_;
  if ( defined $filtered_samples1_ref_ && defined $filtered_samples2_ref_ ) {
    my %samples_union_map_ = map { $_ => 1 } @$filtered_samples1_ref_;
    @samples_union_map_{ @$filtered_samples2_ref_ } = map { 1 } @$filtered_samples2_ref_;

    @samples_union_vec_ = keys %samples_union_map_;
  }

  @samples_union_vec_ = grep { exists $$samples1_map_ref_{ $_ } } @samples_union_vec_;
  @samples_union_vec_ = grep { exists $$samples2_map_ref_{ $_ } } @samples_union_vec_;
#  print join(" ", @samples_union_vec_)."\n";

  my @pnl_series1_ = @$samples1_map_ref_{ @samples_union_vec_ };
  my @pnl_series2_ = @$samples2_map_ref_{ @samples_union_vec_ };

  print "Pnl_series1 length: ".($#pnl_series1_+1)."\n";
  print "Pnl_series2 length: ".($#pnl_series2_+1)."\n";

#  print "PnlSeries1: ".join(" ", @pnl_series1_)."\n";
#  print "PnlSeries2: ".join(" ", @pnl_series2_)."\n";

  return GetCorrelation( \@pnl_series1_, \@pnl_series2_ );
}

sub GetCorrelationDays
{
  my $samples1_map_ref_ = shift;
  my $samples2_map_ref_ = shift;
  my $samples_percentage_ = shift || 0.2;

  my @samples_common_ = grep { exists $$samples2_map_ref_{ $_ } } keys %$samples1_map_ref_;
  @samples_common_ = sort @samples_common_;

  my %days1_map_ = ( );
  my %days2_map_ = ( );

  foreach my $tsample_ ( @samples_common_ ) {
    my $day_ = (split("_", $tsample_))[0];

    if ( exists $days1_map_{ $day_ } ) {
     $days1_map_{ $day_ } = 0;
    } 
    if ( exists $days2_map_{ $day_ } ) {
     $days2_map_{ $day_ } = 0;
    }
    $days1_map_{ $day_ } += $$samples1_map_ref_{ $tsample_ };
    $days2_map_{ $day_ } += $$samples2_map_ref_{ $tsample_ };
  }

  my @days_vec_ = keys %days1_map_;
  my $days_filtered_idx_ = max(0, int ( $samples_percentage_ * ($#days_vec_ + 1) )-1 );

  my @days1_vec_ = sort { $days1_map_{ $a } <=> $days1_map_{ $b } } @days_vec_;
  my @days2_vec_ = sort { $days2_map_{ $a } <=> $days2_map_{ $b } } @days_vec_;

  my @days1_vec_filtered_ = @days1_vec_[ 0..$days_filtered_idx_ ];
  my @days2_vec_filtered_ = @days2_vec_[ 0..$days_filtered_idx_ ];

  my %days_union_map_ = map { $_ => 1 } @days1_vec_filtered_;
  @days_union_map_ { @days2_vec_filtered_ } = map { 1 } @days2_vec_filtered_;

  my @days_union_vec_ = keys %days_union_map_;

  my @pnl_series1_ = @days1_map_{ @days_union_vec_ };
  my @pnl_series2_ = @days2_map_{ @days_union_vec_ };

  print "Pnl_series1 length: ".($#pnl_series1_+1)."\n";
  print "Pnl_series2 length: ".($#pnl_series2_+1)."\n";

  return GetCorrelation( \@pnl_series1_, \@pnl_series2_ );

}

sub GetTopStrats
{
  my $shortcode_ = shift;
  my $timeperiod_ = shift;
  my $numdays_ = shift;
  my $sort_algo_ = shift;
  my $topn_ = shift || 5;
  my $top_strats_ref_ = shift;

  my $start_date_ = CalcPrevWorkingDateMult ( $yyyymmdd_, $numdays_ );
  my $strats_dir_ = "$MODELING_STRATS_DIR/$shortcode_/$timeperiod_";

  my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $strats_dir_ $GLOBALRESULTSDBDIR $start_date_ $yyyymmdd_ INVALIDFILE $sort_algo_ 0 IF 1";

  print $exec_cmd_."\n";
  my @ssr_output_ = `$exec_cmd_`;
  chomp( @ssr_output_ );

  if ( $#ssr_output_ < 0 ) { return; }

  my @strats_sorted_ = map { (split(' ', $_))[1] } @ssr_output_;
  my $nstrats_to_pick_ = min( $topn_, $#strats_sorted_+1 );

  @$top_strats_ref_ = @strats_sorted_[ 0..$nstrats_to_pick_ ];
}

