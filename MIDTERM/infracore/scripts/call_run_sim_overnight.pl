#!/usr/bin/perl

# \file scripts/call_run_sim_overnight.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# This script takes as input : SHORTCODE

use strict;
use warnings;
use POSIX;
use feature "switch";
use List::Util qw/max min/;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/infracore_install/bin";
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
my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ ) ;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $debug_ = 0;

my $last_date_with_data_ = `date +%Y%m%d`; chomp ( $last_date_with_data_ );
# TODO ... improve to actually checking f the data directories exist
$last_date_with_data_ = CalcPrevDate ( $last_date_with_data_ );

my $prev_date_ = CalcPrevDateMult ( $last_date_with_data_, 50 );

# my @possible_trading_start_end_str_vec_ = ( "UTC_715-UTC_915",
# 					    "UTC_715-UTC_1300",
# 					    "UTC_715-UTC_1200",
# 					    "UTC_715-UTC_1015",
# 					    "UTC_700-UTC_1200",
# 					    "UTC_630-UTC_900",
# 					    "UTC_400-UTC_1200",
# 					    "EST_930-EST_1400",
# 					    "EST_930-1500",
# 					    "EST_915-EST_1600",
# 					    "EST_915-EST_1545",
# 					    "EST_915-EST_1400",
# 					    "EST_830-EST_1600",
# 					    "EST_830-EST_1545",
# 					    "EST_830-EST_1430",
# 					    "EST_830-EST_1330",
# 					    "EST_830-EST_1245",
# 					    "EST_830-EST_1145",
# 					    "EST_800-EST_1400",
# 					    "EST_800-EST_1300",
# 					    "EST_800-EST_1200-DT",
# 					    "EST_800-EST_1200",
# 					    "EST_230-EST_830",
# 					    "CET_800-EST_800" );


#foreach my $trading_start_end_str_ ( @possible_trading_start_end_str_vec_ )
#{
#    my $dest_strats_dir_ = $MODELING_STRATS_DIR."/".$shortcode_."/".$trading_start_end_str_ ;
    my $dest_strats_dir_ = $MODELING_STRATS_DIR."/".$shortcode_ ;
    if ( -d $dest_strats_dir_ )
    {

	my @possible_basepx_pxtype_str_ = ( "Midprice",
					    "MktSizeWPrice",
					    "MktSinusoidal",
					    "OrderWPrice",
					    "OfflineMixMMS" );

	foreach my $basepx_pxtype_ ( @possible_basepx_pxtype_str_ )
	{
	    my $exec_cmd = "$MODELSCRIPTS_DIR/run_simulations_2.pl $shortcode_ $dest_strats_dir_ $prev_date_ $last_date_with_data_ $basepx_pxtype_ $market_model_index_";
	    `$exec_cmd`;
	    if ( $debug_ == 1 ) { print STDERR "$exec_cmd\n"; }
	}
    }
#}
