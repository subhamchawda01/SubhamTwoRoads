#!/usr/bin/perl

# \file scripts/get_iso_date_from_str_min1.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch";

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

my $USAGE = "$0 STRING";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $string_ = $ARGV [ 0 ];

my $iso_date_ = GetIsoDateFromStrMin1 ( $string_ );

print $iso_date_."\n";

exit ( 0 );
