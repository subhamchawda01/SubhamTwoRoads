#!/usr/bin/perl

# \file ModelScripts/call_get_pred_counters_for_pred_algo.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite 217, Level 2, Prestige Omega,
#	 No 104, EPIP Zone, Whitefield,
#	 Bangalore - 560066, India
#	 +91 80 4060 0717
#
# This script takes :
# 

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # GetPredCountersForThisPredAlgo

# start 
my $USAGE="$0  shortcode  pred_duration_seconds  predalgo ";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $this_pred_duration_seconds_ = $ARGV[1];
my $this_predalgo_ = $ARGV[2]; 
my $this_day_timed_data_filename_ = "INVALIDFILE";

my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( $shortcode_, $this_pred_duration_seconds_, $this_predalgo_, $this_day_timed_data_filename_ );
printf ( "%.1f\n", $this_pred_counters_ );
