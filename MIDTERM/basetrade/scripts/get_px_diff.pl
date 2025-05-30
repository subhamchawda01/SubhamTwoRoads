#!/usr/bin/perl

# \file scripts/get_px_diff.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# output_model_filename_  indicator_list_filename_  regression_output_filename_

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_numeric_value_from_line.pl"; # GetNumericValueFromLine ;

# start 
my $USAGE="$0  shortcode unix_timestamp lookahead1(in minutes) [lookahead2 lookahead3 ... ]";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $unix_timestamp_ = $ARGV[1];
my @lookaheads_ = ( );
my $lookahead_ = int ( $ARGV[2] );

push ( @lookaheads_, $lookahead_ );

for ( my $i=3; $i <= $#ARGV; $i++ )
{
  $lookahead_ = $ARGV[$i];
  push ( @lookaheads_, $lookahead_ );
}

my $exec_cmd_ = "date -d \@$unix_timestamp_ +%Y%m%d";
my $tradingdate_ = `$exec_cmd_`;

chomp($tradingdate_);

my $filename_ = "/home/ankit/sampled_data_".$shortcode_."_".$tradingdate_;

$exec_cmd_ = "head -n1 $filename_ | awk '{print \$1}'";

$exec_cmd_ = "cat $filename_ | awk '{ if ( \$1 > $unix_timestamp_ && ( \$1 - 1.1 <  $unix_timestamp_ ) ) { print \$2 } }' | head -n1";
my $base_px_ = `$exec_cmd_`;
chomp($base_px_);

if ( $base_px_ eq "" )
{
  foreach my $t_lookahead_ (@lookaheads_ )
  {
    print "0\n";
  }
  exit(0);
}

my $next_unix_timestamp_ = 0;
my $next_px_ = 0;
my $px_diff_ = 0;

$exec_cmd_ = "~/basetrade_install/bin/get_min_price_increment $shortcode_ $tradingdate_";
my $min_price_increment_ = `$exec_cmd_`;
chomp($min_price_increment_);

foreach my $t_lookahead_ (@lookaheads_)
{
  $next_unix_timestamp_ = $unix_timestamp_ + $t_lookahead_ * 60;
  $exec_cmd_ = "cat $filename_ | awk '{if ( \$1 > $next_unix_timestamp_ && ( \$1 - 1.1 < $next_unix_timestamp_ ) ) { print \$2 } }' | head -n1";
  $next_px_ = `$exec_cmd_`;   
  chomp($next_px_);
  if ( $next_px_ eq "" )
  {
    $next_px_ = $base_px_;
  }
  $px_diff_ = ($next_px_ - $base_px_)/$min_price_increment_;
  print $px_diff_."\n"; 
}
