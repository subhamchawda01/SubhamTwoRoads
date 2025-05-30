#!/usr/bin/perl

# \file GenPerlLib/get_bad_days_for_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 353, Evoma, #14, Bhattarhalli,
#        Old Madras Road, Near Garden City College,
#        KR Puram, Bangalore 560049, India
#        +91 80 4190 3551
#
use strict;
use warnings;
use feature "switch";

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_bad_days_for_shortcode.pl";

my $USAGE="$0 SHORTCODE TIMEPERIOD START_DATE END_DATE [DAYS_FRAC=0.2]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit(0); }

my $shortcode_ = shift;
my $timeperiod_ = shift;
my $start_date_ = shift;
my $end_date_ = shift;
my $days_percentage_ = shift || 0.2;

my @bad_days_ = ( );

GetBadDaysPoolForShortcode ( $shortcode_, $timeperiod_, $start_date_, $end_date_, \@bad_days_, $days_percentage_ );

print join ( "\n", @bad_days_ )."\n";
