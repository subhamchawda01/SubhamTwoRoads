#!/usr/bin/perl

# \file ModelScripts/summarize_strats_for_specific_days.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
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

sub GetPnlSamplesCorrelation 
{
  my $strat_name0_ = shift;
  my $strat_name1_ = shift;
  my $sample_pnls_strats_vec_ref_ = shift;
  my $min_sample_size_ = shift || 50;

  if ( ! exists $$sample_pnls_strats_vec_ref_ { $strat_name0_ } ) {
    print "No SamplePnl series for ".$strat_name0_."\n";
    return 0;
  } elsif ( ! exists $$sample_pnls_strats_vec_ref_ { $strat_name1_ } ) {
    print "No SamplePnl series for ".$strat_name1_."\n";
    return 0;
  }

  my @samples_intersect_;
  foreach my $t_sample_ ( keys %{$$sample_pnls_strats_vec_ref_ { $strat_name0_ }} ) {
    if ( exists $$sample_pnls_strats_vec_ref_ { $strat_name1_ } { $t_sample_} ) {
      push ( @samples_intersect_, $t_sample_ );
    }
  }

  my @pnl_series0_ = @{ $$sample_pnls_strats_vec_ref_ { $strat_name0_ } } { @samples_intersect_ };
  my @pnl_series1_ = @{ $$sample_pnls_strats_vec_ref_ { $strat_name1_ } } { @samples_intersect_ };
    
  my $min_ = 0;
  my $max_ = 0;
  my $mn_ = @samples_intersect_;

  if ( $mn_ < $min_sample_size_ ) {
    print "Intersection set size of pnlsamples: ".$mn_.", returning correlation as -1\n";
    return -1;
  }

  for ( my $i = 0; $i < $mn_; $i++ ) {
    $min_ = min ( $min_ , $pnl_series0_ [ $i ] );
    $min_ = min ( $min_ , $pnl_series1_ [ $i ] );
    $max_ = max ( $max_ , $pnl_series0_ [ $i ] );
    $max_ = max ( $max_ , $pnl_series1_ [ $i ] );
  }

  if ( $min_ == $max_ ) { return 0; }

  for ( my $i = 0; $i < $mn_; $i++ ) {
    $pnl_series0_ [ $i ] = ( $pnl_series0_ [ $i ] - $min_ ) / ( $max_ - $min_ );
    $pnl_series1_ [ $i ] = ( $pnl_series1_ [ $i ] - $min_ ) / ( $max_ - $min_ );		
  }

# Compute correlation coefficient.
  my $sumxy = 0;
  my $sumx = 0;
  my $sumy = 0;
  my $sumxx = 0;
  my $sumyy = 0;
  for ( my $i = 0; $i < $mn_; $i++ ) {
    $sumx += $pnl_series0_ [ $i ];
    $sumy += $pnl_series1_ [ $i ];
    $sumxx += $pnl_series0_ [ $i ] * $pnl_series0_ [ $i ];
    $sumyy += $pnl_series1_ [ $i ] * $pnl_series1_ [ $i ];
    $sumxy += $pnl_series0_ [ $i ] * $pnl_series1_ [ $i ];
  }

  my $num_ = ( $mn_ * $sumxy ) - ( $sumx * $sumy );
  my $den_ = sqrt ( ( ( $mn_ * $sumxx ) - ( $sumx * $sumx ) ) * ( ( $mn_ * $sumyy ) - ( $sumy * $sumy ) ) );
  if ( $den_ == 0 ) { return 1; }
  my $corr_ = $num_ / $den_;

  return $corr_;
}
