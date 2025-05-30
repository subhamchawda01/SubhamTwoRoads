#!/usr/bin/perl

# \file scripts/get_list_of_dates_for_shortcode.pl
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
use POSIX;
use List::Util qw[min max]; # max , min

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";


require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl";

if($#ARGV < 2){
    printf "$0 shortcode startdate/lookback_days enddate\n";
    exit(0);
}

my $shortcode_ = $ARGV[0];
my $start_date_ = $ARGV[2];
my $lookback_ = 0;

if(length($ARGV[1])==8) {
    $start_date_ = $ARGV[1];
} else {
    $lookback_ = $ARGV[1];
}

my $end_date_ = $ARGV[2];

if($lookback_ == 0 ) {
    my @dates_ = GetDatesFromStartDate($shortcode_, $start_date_, $end_date_);
    print join(" ", @dates_)."\n";
} else {
    my @dates_ = GetDatesFromNumDays($shortcode_, $end_date_, $lookback_, "INVALIDFILE");
    print join(" ", @dates_)."\n";
}

