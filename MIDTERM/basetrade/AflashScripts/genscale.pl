#!/usr/bin/perl

use strict;
use warnings;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";
my $REPO = "basetrade";

my $USAGE = "$0 <shortcode> <event_datfile> <event_id> <mins_to_runs(2/5/10)> <maxpxch_percentile> <thresh_percentile>=0 <datum1_id> <datum2_id> ..";

if ( $#ARGV < 6 ) {
  print $USAGE."\n";
  exit(0);
}

my $shc_ = $ARGV[0];
my $datfile_ = $ARGV[1];
my $event_id_ = $ARGV[2];
my $betamins_ = $ARGV[3];
my $maxpxch_percentile_ = $ARGV[4];
my $thresh_percentile_ = $ARGV[5];
my @ev_ids_ = @ARGV[6..$#ARGV];

if ( $betamins_ !~ /^(2|5|10|15)$/ ) { 
  print $USAGE."\n";
  exit(0);
}
$betamins_ = 5 if $betamins_ == 15;

my $findScale_cmd_ = "$HOME_DIR/$REPO/AflashScripts/findScale.R $datfile_ $shc_ $shc_/pxchange.dat $thresh_percentile_,$maxpxch_percentile_";
my @scale_out_ = `$findScale_cmd_ 2>/dev/null`; chomp(@scale_out_);

my ($beta_str_) = grep { $_ =~ /^beta$betamins_/ } @scale_out_;
if ( defined $beta_str_ && $beta_str_ ne "" ) {
  my @beta_wds_ = split(/\s+/, $beta_str_);
  @beta_wds_ = @beta_wds_[1..($#beta_wds_-1)];
  if ( $#beta_wds_ < $#ev_ids_ ) {
    print "ERROR: no. of beta coeffs < no. of datum_ids\n";
    exit(0);
  }
  my $beta_outstr_ = join(",", map { $ev_ids_[$_].":".$beta_wds_[$_] } 0..$#beta_wds_ );
  print "$shc_ $event_id_ $beta_outstr_\n";
  print "AF_SCALE_BETA $beta_outstr_\n";
}

my $mins_idx_ = ($betamins_ == 2) ? 1 : ( ($betamins_ == 5) ? 2 : 3 );

my ($perc_str_) = grep { $_ =~ /^percentile$maxpxch_percentile_/ } @scale_out_;
if ( $maxpxch_percentile_ > 0 && defined $perc_str_ && $perc_str_ ne "" ) {
  my @perc_wds_ = split(/\s+/, $perc_str_);
  my $maxpxch_ = $perc_wds_[$mins_idx_];
  print "PXCH_FOR_MAXORDERSIZE $maxpxch_\n";
}

($perc_str_) = grep { $_ =~ /^percentile$thresh_percentile_/ } @scale_out_;
if ( $thresh_percentile_ > 0 && defined $perc_str_ && $perc_str_ ne "" ) {
  my @perc_wds_ = split(/\s+/, $perc_str_);
  my $maxpxch_ = $perc_wds_[$mins_idx_];
  print "AGGRESSIVE_THRESHOLD $maxpxch_\n";
}

