#!/usr/bin/perl

# \file scripts/call_get_iso_date_from_str_min1.pl
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

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "infracore";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

# start
my $USAGE="$0 DATE";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 0 ] );

print $yyyymmdd_."\n";

exit ( 0 );
