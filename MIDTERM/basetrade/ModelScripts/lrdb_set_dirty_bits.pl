#!/usr/bin/perl

use strict;
use warnings;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$HOME_DIR/hrishav/lrdb_db_access_manager.pl";
require "$GENPERLLIB_DIR/array_ops.pl";

my $USAGE = "$0 ALL";

if ( $#ARGV < 0 )  {
  print $USAGE."\n"; exit(0);
}

my @all_pairs_ = ( );
FetchAllLRDBPairs(\@all_pairs_);

foreach my $pairref_ ( @all_pairs_ ) {
  my @values_ = ();
  FetchAllLRDBForPair($$pairref_[0], $$pairref_[1], $$pairref_[2], \@values_);
  CheckSetDirtyBits($$pairref_[0], $$pairref_[1], $$pairref_[2], \@values_);
  print "\n";
}


sub CheckSetDirtyBits
{
  my ($dep, $indep, $sessionid, $values_ref_) = @_;
  my $lookback_ = 3;

  my %dirty_bits_ = map { $$_[2] => 0 } @$values_ref_;
  my %orig_dirty_bits_ = ( );
  my $count_dirty_bits_ = 0;

  if ($#$values_ref_ >= $lookback_) {
    foreach my $idx ( $lookback_ .. $#$values_ref_ ) {
        #print join(" ", @{$$values_ref_[ $idx ]})."\n";
      my $beta = $$values_ref_[ $idx ][0];
      my $correlation = $$values_ref_[ $idx ][1];
      my $date = $$values_ref_[ $idx ][2];
      $orig_dirty_bits_{ $date } = $$values_ref_[ $idx ][3];

      my @betavec_ = map { $$_[0] } @$values_ref_[ ($idx-$lookback_)..($idx-1) ];
      my @corrvec_ = map { $$_[1] } @$values_ref_[ ($idx-$lookback_)..($idx-1) ];
      my $prev_beta = GetAverage ( \@betavec_ );
      my $prev_correlation = GetAverage ( \@corrvec_ );

      my $corr_ch_ = abs($correlation-$prev_correlation);
      my $corr_ch_fr_ = abs(($correlation-$prev_correlation)/$prev_correlation);
      my $beta_ch_fr_ = abs(($beta-$prev_beta)/$prev_beta);

      my $large_corr_change_ = ( $corr_ch_ > 0.3 ) && ( $corr_ch_fr_ > 0.3 );
      my $large_beta_change_ = ( $beta_ch_fr_ > 0.3 + $corr_ch_fr_ );
      #approximation for just considering large changes in stdev_ratio because corr change is already considered
      if ( $large_corr_change_ || $large_beta_change_ ) {
        $dirty_bits_{ $date } = 1;
        $count_dirty_bits_++;
      }

      #print "$dep $indep $sessionid $date: ".$dirty_bits_{ $date }." $prev_beta $beta $prev_correlation $correlation\n";
    }
  }

  my @betavec_ = map { $$_[0] } @$values_ref_;
  my $beta_mean_ = GetAverage ( \@betavec_ );
  my $beta_sd_ = GetStdev ( \@betavec_ );
  if ( $beta_mean_ > 0 && $beta_sd_ / $beta_mean_ > 0.5 ) {
    print "High STDEV for $dep $indep $sessionid: mean: $beta_mean_ sd: $beta_sd_\n";
  }
  if ( $#$values_ref_ >= $lookback_ && $count_dirty_bits_ / ($#$values_ref_+1-$lookback_) > 0.3 ) {
    print "Many dirty bits for $dep $indep $sessionid: $count_dirty_bits_ out of ".($#$values_ref_+1)."\n";
  }

  foreach my $date ( sort keys %dirty_bits_ ) {
    if ( $dirty_bits_{ $date } != $orig_dirty_bits_{ $date } ) {
        print "CHANGE $dep $indep $sessionid $date: ".$orig_dirty_bits_{$date}." ".$dirty_bits_{$date}."\n";
        SetDirtyBit ($dep, $indep, $sessionid, $date, $dirty_bits_{$date});
    }
  }
}

