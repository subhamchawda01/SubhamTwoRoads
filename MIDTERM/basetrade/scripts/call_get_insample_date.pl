#!/usr/bin/perl

# \file scripts/call_get_insample_date.pl
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

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_insample_date"; # GetInsampleDate

my $USAGE="$0 date";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
print "$ARGV[0] - ".GetInsampleDate ( $ARGV[0] )."\n";
