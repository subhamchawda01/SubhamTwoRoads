#!/usr/bin/perl

# \file scripts/call_run_sim_specific_days.pl
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
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/;
use FileHandle;

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

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $SCRIPTNAME="$0";

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

my $USAGE="$0 shortcode strat_dir/strat_file num_past_days start_time end_time work_dir \"event_name in quotes\"";

if ( $#ARGV < 6 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[ 0 ];
my $strat_directory_ = $ARGV[ 1 ];
my $num_working_days_ = $ARGV[ 2 ];
my $start_time_ = $ARGV[ 3 ];
my $end_time_ = $ARGV[ 4 ];
my $work_dir_ = $ARGV[ 5 ];
my $eco_event_ = $ARGV[ 6 ];

my $exec_cmd_ = "grep \"$eco_event_\" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2014_processed.txt  ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2013_processed.txt ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2012_processed.txt | awk '{print \$5}' | awk -F '_' '{print \$1}'";

my @output_lines_ = `$exec_cmd_`;

my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ ) ;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $debug_ = 0;

my $last_date_with_data_ = `date +%Y%m%d`; chomp ( $last_date_with_data_ );
# TODO ... improve to actually checking if the data directories exist
$last_date_with_data_ = CalcPrevWorkingDateMult ( $last_date_with_data_, 1 ); # one more day such that this can be run along with overnight version

my $prev_date_ = CalcPrevWorkingDateMult ( $last_date_with_data_, $num_working_days_ );


my @possible_basepx_pxtype_str_ = ( "ALL" );

my $t_start_date_ = $prev_date_ ;
my $t_end_date_ = $last_date_with_data_ ;

foreach my $basepx_pxtype_ ( @possible_basepx_pxtype_str_ )
{
    foreach my $date_line_ ( @output_lines_ )
    {      
       my $t_date_ = $date_line_;
       chomp ($t_date_);
       my $exec_cmd = "sed -i '/$t_date_/d' ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2014_processed.txt";
       `$exec_cmd`;	       

       $exec_cmd = "sed -i '/$t_date_/d' ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2013_processed.txt";
       `$exec_cmd`;			

       #my $t_date_ = chomp($date_line_ ) ;
       if ( $t_date_  < $t_start_date_ || $t_date_ > $t_end_date_ )
       {
	  next;
       }      	
       chomp ($t_date_ );
       $exec_cmd = "$MODELSCRIPTS_DIR/run_simulations_specific_days.pl $shortcode_ $strat_directory_ $t_date_ $t_date_ $basepx_pxtype_ $start_time_ $end_time_ $work_dir_ $market_model_index_";
       `$exec_cmd`;		  
       if ( $debug_ == 1 ) { print STDERR "$exec_cmd\n"; }
    }
}

my $exec_cmd = "cp ~/infracore/SysInfo/BloombergEcoReports/merged_eco_2014_processed.txt ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2014_processed.txt";
`$exec_cmd_`;

$exec_cmd = "cp ~/infracore/SysInfo/BloombergEcoReports/merged_eco_2013_processed.txt ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_2013_processed.txt";
`$exec_cmd_`;
