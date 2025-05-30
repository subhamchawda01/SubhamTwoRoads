#!/usr/bin/perl

# \file scripts/call_run_sim_overnight_perdir_longer.pl
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
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/;
use FileHandle;
use sigtrap qw(handler signal_handler normal-signals error-signals);

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

my $USAGE="$0 shortcode [skip_recent_days=4] [num_past_days=70]";
my $is_lock_created_ = 0;

my $skip_recent_days_ = 4;
my $num_working_days_ = 70;

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[ 0 ];
if ( $#ARGV >= 1 ) { $skip_recent_days_ = $ARGV[ 1 ]; }
if ( $#ARGV >= 2 ) { $num_working_days_ = $ARGV[ 2 ]; }

my $LOCKFILE=$HOME_DIR."/locks/call_run_sim_overnight_longer_".$shortcode_."_".$skip_recent_days_."_".$num_working_days_.".lock";

my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{ # AWS
    $LOCKFILE="/mnt/sdf/locks/call_run_sim_overnight_longer_".$shortcode_."_".$skip_recent_days_."_".$num_working_days_.".lock";
}

if ( -e $LOCKFILE ) 
{ # lockfile already exists, so exit
    exit 0;
    #PrintStacktraceAndDie ( "$SCRIPTNAME exits since $LOCKFILE exists\n" );
} 
open LOCKFILEHANDLE, "> $LOCKFILE" or PrintStacktraceAndDie ( "$SCRIPTNAME could not open $LOCKFILE\n" );
flock ( LOCKFILEHANDLE, LOCK_EX ) or PrintStacktraceAndDie ( "$SCRIPTNAME could not lock $LOCKFILE\n" ); # write lock
$is_lock_created_ = 1;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $debug_ = 0;

my $last_date_with_data_ = `date +%Y%m%d`; chomp ( $last_date_with_data_ );
# TODO ... improve to actually checking if the data directories exist
$last_date_with_data_ = CalcPrevWorkingDateMult ( $last_date_with_data_, $skip_recent_days_ ); # one more day such that this can be run along with overnight version

my $prev_date_ = CalcPrevWorkingDateMult ( $last_date_with_data_, $num_working_days_ );

my $dest_strats_dir_ = $MODELING_STRATS_DIR."/".$shortcode_ ;

my $t_start_date_ = $prev_date_ ;
my $t_end_date_ = $last_date_with_data_ ;

    my $top_directory_ = File::Spec->rel2abs ( $dest_strats_dir_ );
    if ( -d $top_directory_ )
    {
	if (opendir my $dh, $top_directory_)
	{
	    my @t_list_=();
	    while ( my $t_item_ = readdir $dh )
	    {
		push @t_list_, $t_item_;
	    }
	    closedir $dh;

	    my @sorted_item_list_ = sort @t_list_ ;

	    for my $dir_item_ ( @sorted_item_list_ )
	    {
		# Unix file system considerations.
		next if $dir_item_ eq '.' || $dir_item_ eq '..';
		
		if ( -d "$top_directory_/$dir_item_" )
		{
		    my $exec_cmd = "$MODELSCRIPTS_DIR/run_simulations.pl $shortcode_ $top_directory_/$dir_item_ $t_start_date_ $t_end_date_ DB -d 0";
		    `$exec_cmd`;
		    if ( $debug_ == 1 ) { print STDERR "$exec_cmd\n"; }
		}
	    }
	}
    }

#$t_end_date_ = CalcPrevWorkingDateMult ( $t_start_date_, 1 );
#$t_start_date_ = CalcPrevWorkingDateMult ( $t_end_date_, 30 );

close ( LOCKFILEHANDLE );
exit 0;

sub signal_handler
{
  die "Caught a signal $!\n";
}

sub END
{
  if($is_lock_created_)
  {
    `rm -f $LOCKFILE`;
  }
}
