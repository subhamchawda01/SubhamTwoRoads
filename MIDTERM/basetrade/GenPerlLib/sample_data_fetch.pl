#!/usr/bin/perl
use feature "switch"; # for given, when
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $DATA_DIR="/NAS1/SampleData/";
if(defined $ENV{"LIVE_TRADING_SAMPLEDATA"}){
  $DATA_DIR="/spare/local/logs/datalogs/"
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate

sub __loadSampleFile__
{
  my ($fname_, $s_slot_mins_, $e_slot_mins_, $start_check_, $end_check_, $factor_samples_vec_ref_, $verbose_) = @_;

  if ( ! ( -s $fname_ ) ) {
    if ( $verbose_ == 1 ) { print STDERR "WARNING: Sample Data file: ".$fname_." does not exist.\n"; }
    return;
  }
  my $dat_col_ = 1;

  open FHANDLE, "< $fname_" or PrintStacktraceAndDie ( "Could not open the Sample Data File: ".$fname_."\n" );
  while ( my $thisline_ = <FHANDLE> )
  {
    chomp ( $thisline_ );
    my @this_words_ = split ( ' ', $thisline_ );
    if ( $#this_words_ < $dat_col_ ) {
      next;
    }
    my $time_slot_ = $this_words_[0] ;
    my $time_slot_mins_ = ( int( $time_slot_ / 100 ) * 60 ) + ( $time_slot_ % 100 ) ;

## sample-end-time lies in between the ( start time + 5 ) and ( end time + 10 )
    if ( ( ! $start_check_ || ! defined $s_slot_mins_ || $time_slot_mins_ >= $s_slot_mins_ )
        && ( ! $end_check_ || ! defined $e_slot_mins_ || $time_slot_mins_ < ( $e_slot_mins_ + 15 ) ) ){
      $$factor_samples_vec_ref_{ $time_slot_ } = $this_words_[ $dat_col_ ];
    }
  }
}

sub __getFeatureToBasename__
{
  my ($factor_, $factor_aux_) = @_;
  
  my $fbase_ = "";
  given ( $factor_ )
  {
    when ("VOL")    { $fbase_ = "RollingSumVolume300.txt"; }
    when ("STDEV")  { $fbase_ = "RollingStdev300.txt"; }
    when ("L1SZ")   { $fbase_ = "RollingAvgL1Size300.txt"; }
    when ("L1EVPerSec")   { $fbase_ = "L1EventsPerSecond.txt"; }
    when ("TREND")  { $fbase_ = "SimpleTrend300.txt"; }
    when ("TrendStdev")  { $fbase_ = "RollingTrendStdev300.txt"; }
    when ("ORDSZ")  { $fbase_ = "RollingAvgOrdSize300.txt"; }
    when ("TRADES")  { $fbase_ = "RollingSumTrades300.txt"; }
    when ("SSTREND")  { $fbase_ = "StableScaledTrend300.txt"; }
    when ("TOR") { $fbase_ = "TurnOverRate300.txt"; }
    when ("BidAskSpread")  { $fbase_ = "MovingAvgBidAskSpread300.txt"; }
    when ("AvgPrice") { $fbase_ = "AvgPrice300.txt"; }
    when ("AvgPriceImpliedVol") { $fbase_ = "AvgPriceImpliedVol300.txt"; }
    when ("CORR") { $fbase_ = "RollingCorrelation300_".$$factor_aux_[0].".txt"; }
    when ("DELTA") { $fbase_ = "OptionsGreek300_1.txt"; }
    when ("GAMMA") { $fbase_ = "OptionsGreek300_2.txt"; }
    when ("VEGA") { $fbase_ = "OptionsGreek300_3.txt"; }
    when ("THETA") { $fbase_ = "OptionsGreek300_4.txt"; }
    default { $fbase_ = $factor_.".txt"; }
  }
  if ( $fbase_ eq "" ) {
    if ( $verbose_ == 1 ) { print STDERR "WARNING: factor ".$factor_." Invalid.. Exiting..\n"; }
    return;
  }
  return $fbase_;
}

sub getSampleSlotFromHHMM
{
  my $hhmm_ = shift;
  my $sample_mins_ = shift || 15;

  my $curr_mins_ = ( int( $hhmm_ / 100 ) * 60 ) + ( $hhmm_ % 100 ) ;
  my $slot_no_ = int ( $curr_mins_ / $sample_mins_ + 0.5);
  return $slot_no_;
}

sub getMfmFromHHMM
{
  my $hhmm_ = shift;

  my $curr_mins_ = ( int( $hhmm_ / 100 ) * 60 ) + ( $hhmm_ % 100 ) ;
  my $curr_msecs_ = $curr_mins_ * 60000;
  return $curr_msecs_;
}

sub getSampleSlotFromMfm
{
  my $mfm_ = shift;
  my $sample_msecs_ = 900000;

  my $slot_no_ = int ( $mfm_ / $sample_msecs_ + 0.5 ) ;
  return $slot_no_;
}

sub GetDaySlotKey
{
  my $date_ = shift;
  my $slot_ = shift;
  my $sample_mins_ = shift || 15;
  my $num_samples_ = 1440 / $sample_mins_;

  if ( $slot_ >= 0 ) { return $date_."_".$slot_; }
  else {
    $slot_ +=  $num_samples_;
    $date_ = CalcPrevDate( $date_ );
    return $date_."_".$slot_;
  }
}

sub ConvertKeyToFormat1
{
  my $day_slot_ = shift;

  my ($date_, $slot_) = split("_", $day_slot_);
  return GetDaySlotKey($date_, $slot_);
}

sub GetFeatureMap
{
  my ($this_shc_, $this_date_, $factor_, $s_slot_, $e_slot_, $factor_samples_vec_ref_, $factor_aux_, $verbose_, $sampling_dur_) = @_;

  if ( ! defined $verbose_ ) { $verbose_ = 0; }
  if ( ! defined $sampling_dur_ ) { $sampling_dur_ = 15; }

  my $is_intraday_ = 1;
  if ( defined $s_slot_ && defined $e_slot_ && $e_slot_ < $s_slot_ ) { $is_intraday_ = 0; }

  my $fbase_ = __getFeatureToBasename__($factor_, $factor_aux_);
  my $fname_ = $DATA_DIR."/".$this_shc_."/".$this_date_."/".$fbase_;

  my ($s_slot_mins_, $e_slot_mins_);
  if ( defined $s_slot_ ) { $s_slot_mins_ = ( int( $s_slot_ / 100 ) * 60 ) + ( $s_slot_ % 100 ); }
  if ( defined $e_slot_ ) { $e_slot_mins_ = ( int( $e_slot_ / 100 ) * 60 ) + ( $e_slot_ % 100 ); }

  my %factor_samples_map_ = ( );
  if ( $is_intraday_ ) {
    __loadSampleFile__( $fname_, $s_slot_mins_, $e_slot_mins_, 1, 1, \%factor_samples_map_, $verbose_ );
    foreach my $hhmm_ ( keys %factor_samples_map_ ) {
      $$factor_samples_vec_ref_{ getSampleSlotFromHHMM($hhmm_, $sampling_dur_) } = $factor_samples_map_{ $hhmm_ };
    }
  } else {
    __loadSampleFile__( $fname_, $s_slot_mins_, $e_slot_mins_, 0, 1, \%factor_samples_map_, $verbose_ );
    foreach my $hhmm_ ( keys %factor_samples_map_ ) {
      $$factor_samples_vec_ref_{ getSampleSlotFromHHMM($hhmm_, $sampling_dur_) } = $factor_samples_map_{ $hhmm_ };
    }

    my $day_numsamples_ = int(1440/$sampling_dur_);

    %factor_samples_map_ = ( );
    my $prev_date_ = CalcPrevDate($this_date_);
    $fname_ = $DATA_DIR."/".$this_shc_."/".$prev_date_."/".$fbase_;
    __loadSampleFile__( $fname_, $s_slot_mins_, $e_slot_mins_, 1, 0, \%factor_samples_map_, $verbose_ );
    foreach my $hhmm_ ( keys %factor_samples_map_ ) {
      $$factor_samples_vec_ref_{ getSampleSlotFromHHMM($hhmm_, $sampling_dur_) - $day_numsamples_ } = $factor_samples_map_{ $hhmm_ };
    }
  }
}

