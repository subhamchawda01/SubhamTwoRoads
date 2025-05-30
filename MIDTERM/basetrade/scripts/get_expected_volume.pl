#!/usr/bin/perl

# \file scripts/get_bad_periods.pl
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
use List::Util qw[min max]; # max , min

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
require "$GENPERLLIB_DIR/sample_data_utils.pl";

my $USAGE = "$0 shortcode date num_days start_time end_time";


if ( $#ARGV < 4 )
{
    printf "$USAGE\n";
    exit ( 0 );
}

my $shc_ = $ARGV[0];
my $date_ = $ARGV[1];
my $no_of_days_ = $ARGV[2];
my $start_hhmm_ = $ARGV[3];
my $end_hhmm_ = $ARGV[4];

my $s_slot_ = `$BIN_DIR/get_utc_hhmm_str $start_hhmm_ $date_`;
my $e_slot_ = `$BIN_DIR/get_utc_hhmm_str $end_hhmm_ $date_`;

my $l1_avg_ = GetFeatureAverageDays($shc_, $date_, $no_of_days_, "L1SZ", [], $s_slot_, $e_slot_, 0, 15 );
#print $l1_avg_."\n";

my $vol_avg_ = GetFeatureAverageDays($shc_, $date_, $no_of_days_, "VOL", [], $s_slot_, $e_slot_, 0, 15 );
#print $vol_avg_."\n";

my $nslots_ = ( ( int ( $e_slot_/100 ) * 60 + ( $e_slot_ % 100 ) ) - ( int ( $s_slot_/100 ) * 60 + ( $s_slot_ % 100 ) ) ) / 5 ;

my $vol_per_uts_ = ($vol_avg_*$nslots_)/(2*$l1_avg_);
my $max_uts_ = $l1_avg_*0.2;

print "MaxUTS: ".$max_uts_." VolumePerUTS: ".$vol_per_uts_."\n";


