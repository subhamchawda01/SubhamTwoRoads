#!/usr/bin/perl

# \file scripts/call_calc_prev_working_date_mult.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $GENSTRATWORKDIR=$SPARE_HOME."GSW/";

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

my $USAGE="$0 currentdate pastdays";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $current_date_ = GetIsoDateFromStrMin1 ( $ARGV[0] );
my $num_past_days_ = 1;
if ( $#ARGV >= 1 ) 
{ 
    $num_past_days_ = $ARGV[1];
}

my $prev_date_ = CalcPrevWorkingDateMult ( $current_date_, $num_past_days_ );

print "$prev_date_";
