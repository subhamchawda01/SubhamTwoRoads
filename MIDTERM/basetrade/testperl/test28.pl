#!/usr/bin/perl

# \file testperl/test28.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use strict;
use warnings;
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/get_dep_volume_normalizing_factor.pl"; # GetNonCachedDepVolumeNormalizingFactor

my $USAGE="$0 SHC DATE";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV[0];
my $this_date_ = $ARGV[1];

my $volume_normalizing_factor_ = GetNonCachedDepVolumeNormalizingFactor ( $shortcode_, $this_date_ );
print "$volume_normalizing_factor_\n";
