#!/usr/bin/perl

# \file scripts/call_run_sim_overnight.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input : SHORTCODE

use strict;
use warnings;
use POSIX;
use feature "switch";
use List::Util qw/max min/;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $SPARE_HOME="/spare/local/".$USER."/";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $MODELING_MODELS_DIR=$MODELING_BASE_DIR."/models"; # this directory is used to store the chosen model files
my $MODELING_PARAMS_DIR=$MODELING_BASE_DIR."/params"; # this directory is used to store the chosen param files

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult

my $USAGE="$0 shortcode";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[ 0 ];

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $debug_ = 0;

my $last_date_with_data_ = `date +%Y%m%d`; chomp ( $last_date_with_data_ );
# TODO ... improve to actually checking f the data directories exist
$last_date_with_data_ = CalcPrevDate ( $last_date_with_data_ );

my $prev_date_ = CalcPrevDateMult ( $last_date_with_data_, 20 );

my $dest_strats_dir_ = $MODELING_STRATS_DIR."/".$shortcode_ ;
if ( -d $dest_strats_dir_ )
{    
	my $exec_cmd = "$MODELSCRIPTS_DIR/run_simulations.pl $shortcode_ $dest_strats_dir_ $prev_date_ $last_date_with_data_ DB -d 0";
	`$exec_cmd`;
	if ( $debug_ == 1 ) { print STDERR "$exec_cmd\n"; }
}
