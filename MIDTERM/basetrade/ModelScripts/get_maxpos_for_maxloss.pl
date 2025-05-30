#!/usr/bin/perl

# \file ModelScripts/get_maxpos_for_maxloss.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# product

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
#if ( $USER eq "rkumar" ) 
#{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
#}
if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

# start 
my $USAGE="$0 product maxloss";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $maxloss_ = $ARGV[1];

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );

my $stdev_numbers_ = `$LIVE_BIN_DIR/get_hist_stdev $shortcode_` ;
my $n2d_ = `$LIVE_BIN_DIR/get_numbers_to_dollars $shortcode_ $yyyymmdd_` ; 

my $stdevs_to_ride_ = 12;
my $maxpos_ = $maxloss_ / ( $stdev_numbers_ * $n2d_ ) / $stdevs_to_ride_ ;
printf "$maxpos_\n";

