#!/usr/bin/perl
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
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $DATA_DIR="/NAS1/SampleData/";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/sample_data_fetch.pl"; # getSampleSlotFromHHMM
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # FetchPnlSamples
require "$GENPERLLIB_DIR/config_utils.pl"; # IsValidConfig, AreValidConfigs

sub __fetchPnlSamplesForDay__
{
  my $shortcode_ = shift;
  my $strats_base_ref_ = shift;
  my $date_ = shift;
  my $sample_pnls_strats_vec_ref_ = shift;
  my $s_slot_ = shift;
  my $e_slot_ = shift;
  my $fetch_from_wf_db_;
  if ( $#_ >= 0 ) {
    $fetch_from_wf_db_ = shift;
  } else {
    $fetch_from_wf_db_ = AreValidConfigs ( $strats_base_ref_ );
  }

  if ( ! defined $strats_base_ref_ || $#$strats_base_ref_ < 0 ) { return; }

  if ( defined $s_slot_ ) {
    $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $s_slot_ $date_`; chomp($s_slot_);
    $s_slot_ = getSampleSlotFromHHMM($s_slot_);
  }
  if ( defined $e_slot_ ) { 
    $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $e_slot_ $date_`; chomp($e_slot_);
    $e_slot_ = getSampleSlotFromHHMM($e_slot_);
  }
  if ( defined $s_slot_ && defined $e_slot_ && $e_slot_ < $s_slot_ ) { $e_slot_ += 96; }

  my @pnl_samples_vec_vec_ = ();
  my %strat_volume_map_ = ();

  if ( $#$strats_base_ref_ == 0 ) {
    my @pnl_samples_vec_ = ();
    my $sname_ = $$strats_base_ref_[0];
    $strat_volume_map_{ $sname_ } = 0;
    FetchPnlSamplesSingleStratQuery ( $shortcode_ , $date_, $sname_, \@pnl_samples_vec_, \$strat_volume_map_{ $sname_ }, $fetch_from_wf_db_);
    @pnl_samples_vec_ = ($sname_, @pnl_samples_vec_);
    push ( @pnl_samples_vec_vec_, \@pnl_samples_vec_ );
  }
  else {
    FetchPnlSamples ( $shortcode_ , $date_, \@pnl_samples_vec_vec_, \%strat_volume_map_, $fetch_from_wf_db_);
  }

  foreach my $pnlwords_ ( @pnl_samples_vec_vec_ )
  {
    my $t_strat_ = $$pnlwords_[0];
    next if ( ! FindItemFromVec ( $t_strat_, @$strats_base_ref_ ) ) ;
# next if vol = 0
    next if ( ! defined $strat_volume_map_{ $t_strat_ } || $strat_volume_map_{ $t_strat_ } == 0 ); 
    splice ( @$pnlwords_, 0, 1);

    my %slot_pnls_map_ = ( );
    my $pnl_indx_ = 0;
    my $last_slot_ = 0;
    my $max_slot_ = 0;
    while ( $pnl_indx_ < $#$pnlwords_ )
    {
      my $tslot_ = getSampleSlotFromMfm ( $$pnlwords_[$pnl_indx_] );

      if ( ( ! defined $s_slot_ || $tslot_ > $s_slot_ )
          && ( ! defined $e_slot_ || $tslot_ <= $e_slot_ ) ) {

        if ( $tslot_ ne $last_slot_ ) {
          $slot_pnls_map_{ $tslot_ } = 0;
          $last_slot_ = $tslot_;
        }

        if ( $pnl_indx_ > 0 ) {
          $slot_pnls_map_{ $tslot_ } += $$pnlwords_ [ $pnl_indx_ + 1 ] - $$pnlwords_ [ $pnl_indx_ - 1 ];
        }
        else {
          $slot_pnls_map_{ $tslot_ } += $$pnlwords_ [ $pnl_indx_ + 1 ];
        }
      }

      if ( $max_slot_ < $tslot_ ) { $max_slot_ = $tslot_; }
      $pnl_indx_+=2;
    }

    if ( $max_slot_ > 96 ) {
      foreach my $k_ ( keys %slot_pnls_map_ ) {
        $$sample_pnls_strats_vec_ref_{ $t_strat_ } { $k_ - 96 } = $slot_pnls_map_{ $k_ };
      }
    } else {
      foreach my $k_ ( keys %slot_pnls_map_ ) {
        $$sample_pnls_strats_vec_ref_{ $t_strat_ } { $k_ } = $slot_pnls_map_{ $k_ };
      }
    }
  }
}

sub FetchPnlSamplesStrats
{
  my $shortcode_ = shift;
  my $strats_base_ref_ = shift;
  my $dates_vec_ref_ = shift;
  my $sample_pnls_strats_vec_ref_ = shift;
  my $s_slot_ = shift;
  my $e_slot_= shift;
  my $fetch_from_wf_db_;
  if ( $#_ >= 0 ) {
    $fetch_from_wf_db_ = shift;
  } else {
    $fetch_from_wf_db_ = AreValidConfigs ( $strats_base_ref_ );
  }

  for my $this_date_ ( @$dates_vec_ref_ ) 
  {
    my %t_sample_pnls_strats_ = ( );

    __fetchPnlSamplesForDay__ ($shortcode_, $strats_base_ref_, $this_date_, \%t_sample_pnls_strats_, $s_slot_, $e_slot_, $fetch_from_wf_db_);

    foreach my $t_strat_ ( keys %t_sample_pnls_strats_ ) {
      foreach my $slot_ ( keys %{ $t_sample_pnls_strats_{ $t_strat_ } } ) {
        my $slot_key_ = $this_date_."_".$slot_;
        $$sample_pnls_strats_vec_ref_{ $t_strat_ }{ $slot_key_ } = $t_sample_pnls_strats_{ $t_strat_ }{ $slot_ };
      }
    }
  }
}

sub FetchPnlDaysStrats
{
  my $shortcode_ = shift;
  my $strats_base_ref_ = shift;
  my $dates_vec_ref_ = shift;
  my $sample_pnls_strats_vec_ref_ = shift;
  my $s_slot_ = shift;
  my $e_slot_= shift;
  my $fetch_from_wf_db_;
  if ( $#_ >= 0 ) {
    $fetch_from_wf_db_ = shift;
  } else {
    $fetch_from_wf_db_ = AreValidConfigs ( $strats_base_ref_ );
  }

  for my $this_date_ ( @$dates_vec_ref_ ) 
  {
    my %t_sample_pnls_strats_ = ( );

    __fetchPnlSamplesForDay__ ($shortcode_, $strats_base_ref_, $this_date_, \%t_sample_pnls_strats_, $s_slot_, $e_slot_, $fetch_from_wf_db_);

    foreach my $t_strat_ ( keys %t_sample_pnls_strats_ ) {
      $$sample_pnls_strats_vec_ref_{ $t_strat_ }{ $this_date_ } = GetSum ( [values %{ $t_sample_pnls_strats_{ $t_strat_ } }] );
    } 
  }
}

sub CombinePnlSamples 
{
  my $strat0_pnl_vec_ref_ = shift;
  my $strat1_pnl_vec_ref_ = shift;
  my $combined_pnl_vec_ref_ = shift;

  $$combined_pnl_vec_ref_ { $_ } = $$strat0_pnl_vec_ref_{ $_ } foreach keys %{$strat0_pnl_vec_ref_};

  foreach my $t_sample_ ( keys %{$strat1_pnl_vec_ref_} ) {
    if ( ! defined $$combined_pnl_vec_ref_ { $t_sample_ } ) {
      $$combined_pnl_vec_ref_ { $t_sample_ } = 0;
    }
    $$combined_pnl_vec_ref_ { $t_sample_ } += $$strat1_pnl_vec_ref_{ $t_sample_};
  }
}

sub PnlSamplesGetStatsLong
{
  my $pnlsamples_series_ref_ = shift;
  my $stats_map_ref_ = shift;
  my $dates_vec_ref_;
  if ( @_ > 0 ) { $dates_vec_ref_ = shift; }

  my @samples_ = keys %$pnlsamples_series_ref_;

  my %dates2map_ = ( );
  my %dates2series_ = ( );

  foreach my $t_sample_ ( @samples_ ) {
    my ( $t_date_, $t_slot_ ) = split ( "_", $t_sample_ );
    if ( !defined $dates_vec_ref_ || FindItemFromVec ( $t_date_, @$dates_vec_ref_ ) ) {
      $dates2map_{$t_date_}{$t_slot_} = $$pnlsamples_series_ref_{$t_sample_};
    }
  }

  foreach my $t_date_ ( keys %dates2map_ ) {
    foreach my $t_slot_ ( sort { $a <=> $b } keys %{$dates2map_{$t_date_}} ) {
      push ( @{$dates2series_{$t_date_}}, $dates2map_{$t_date_}{$t_slot_} );
    }
  }

  my @dd_vec_ = ( );
  my @maxpnl_vec_ = ( );
  my @minpnl_vec_ = ( );
  my @pnl_vec_ = ( );
  my @sd_vec_ = ( );
  my @dates_vec_ = sort keys %dates2series_;

  foreach my $t_date_ ( @dates_vec_ ) {
    my $max_drawdown_ = 0;
    my $max_pnl_ = 0;
    my $current_pnl_ = 0;
    my $min_pnl_ = 0;
    foreach ( @{$dates2series_{$t_date_}} ) {
      $current_pnl_ += $_;
      $max_pnl_ = max ( $max_pnl_, $current_pnl_ );
      $min_pnl_ = min ( $min_pnl_, $current_pnl_ );
      my $current_drawdown_ = $max_pnl_ - $current_pnl_;
      $max_drawdown_ = max ( $max_drawdown_, $current_drawdown_ );
    }
    push ( @dd_vec_, $max_drawdown_ );
    push ( @maxpnl_vec_, $max_pnl_ );
    push ( @minpnl_vec_, $min_pnl_ );
    push ( @pnl_vec_, $current_pnl_ );
    push ( @sd_vec_, GetStdev ( \@{$dates2series_{$t_date_}} ) );
  }
  $$stats_map_ref_ { "DD" } = \@dd_vec_;
  $$stats_map_ref_ { "MAXPNL" } = \@maxpnl_vec_;
  $$stats_map_ref_ { "MINPNL" } = \@minpnl_vec_;
  $$stats_map_ref_ { "PNL" } = \@pnl_vec_;
  $$stats_map_ref_ { "SD" } = \@sd_vec_;
  $$stats_map_ref_ { "DATES" } = \@dates_vec_;
}

sub PnlSamplesGetStats
{
  my $pnlsamples_series_ref_ = shift;
  my $stats_map_ref_ = shift;
  my $dates_vec_ref_;
  if ( @_ > 0 ) { $dates_vec_ref_ = shift; }

  my %stats_map_long_ = ( );
  if ( defined $dates_vec_ref_ ) {
    PnlSamplesGetStatsLong ( $pnlsamples_series_ref_, \%stats_map_long_, $dates_vec_ref_ );
  } else {
    PnlSamplesGetStatsLong ( $pnlsamples_series_ref_, \%stats_map_long_ );
  }

  my $num_days_ = $#{ $stats_map_long_{ "DD" } } +1;
  my $pnl_avg_ = GetAverage ( \@{ $stats_map_long_{ "PNL" } } );
  my $pnl_daily_sd_ = 0.01 + GetStdev ( \@{ $stats_map_long_{ "PNL" } } );
  my $pnl_sd_avg_ = 0.01 + GetAverage ( \@{ $stats_map_long_{ "SD" } } );
  my $dd_highavg_ =  GetMeanHighestQuartile ( \@{ $stats_map_long_{ "DD" } } );
  if ( $pnl_sd_avg_ == 0 ) { $pnl_sd_avg_ += 0.01; }

  my @min_pnls_sorted_ = sort { $b <=> $a } @{ $stats_map_long_{ "MINPNL" } };
  my $percentile95_ = max(0, (0.95 * ($#min_pnls_sorted_+1)) - 1 );
  my $min_pnl_ = 0;
  if ( $#min_pnls_sorted_ >= 0 ) { 
    $min_pnl_ = $min_pnls_sorted_[ $percentile95_ ];
  }

  $$stats_map_ref_ { "SHARPE" } = $pnl_avg_ / $pnl_sd_avg_;
  $$stats_map_ref_ { "DAILY_SHARPE" } = $pnl_avg_ / $pnl_daily_sd_;
  $$stats_map_ref_ { "PNL_SHARPE_AVG" } = $pnl_avg_ * min ( 3.0, abs ( $pnl_avg_ / $pnl_sd_avg_ ) );
  $$stats_map_ref_ { "GAIN_PAIN" } = ( 1 + $pnl_avg_*$num_days_ ) / ( 1 + -1 * GetSum ( \@{ $stats_map_long_{ "MINPNL" } } ) );
  $$stats_map_ref_ { "PNL_DD" } = $pnl_avg_ / (1 + $dd_highavg_);
  $$stats_map_ref_ { "DD_HIGHAVG" } = $dd_highavg_;
  $$stats_map_ref_ { "PNL_SQRT_DD" } = $pnl_avg_ / sqrt (1 + $dd_highavg_);
  $$stats_map_ref_ { "PNL" } = $pnl_avg_;
  $$stats_map_ref_ { "MINPNL" } = $min_pnl_;
}

