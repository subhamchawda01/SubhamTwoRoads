#!/usr/bin/perl

# \file testperl/test27.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# This script takes a prod_config
# START
# TIMEPERIODSTRING US_MORN_DAY
# PREDALGO na_e3 
# PREDDURATION 0.05 0.5 2 8 32 96 
# DATAGEN_START_HHMM EST_700 
# DATAGEN_END_HHMM EST_1200 
# DATAGEN_TIMEOUT 3000 4 0
# DATAGEN_BASE_FUT_PAIR MktSizeWPrice MktSizeWPrice
# SELF FGBM_0 
# SOURCE FGBS_0 FGBM_0 
# SOURCECOMBO UBEFC 
# END

use strict;
use warnings;
use feature "switch"; # for given, when
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/skip_pca_indicator.pl"; # SkipPCAIndicator

# start 
my $SCRIPTNAME="$0";

my $USAGE="$0 PORT";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $self_shortcode_ = "JFFCE_0";
my $this_sourcecombo_shortcode_ = $ARGV[0]; #"UEEEQ";
my $this_sourcecombo_indicator_filename_ = "/home/dvctrader/indicatorwork/indicator_list_PCADeviationPairsPort_SOURCECOMBO" ;

if ( SkipPCAIndicator ( $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_ ) )
{
    print "SKIP PCA $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_\n";
}
else
{
    print "GA PCA $self_shortcode_, $this_sourcecombo_shortcode_, $this_sourcecombo_indicator_filename_\n";
}
