#!/usr/bin/perl

# \file scripts/call_run_sim_overnight_perdir.pl
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

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
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
require "$GENPERLLIB_DIR/date_utils.pl"; # GetUTCTime, GetEndTimeFromTP

my $USAGE="$0 shortcode [num_past_days=5] [time-period]";
my $is_lock_created_ = 0;

my $num_working_days_ = 5;
my $time_period_ = "";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[ 0 ];
if ( $#ARGV >= 1 ) { $num_working_days_ = $ARGV[ 1 ]; }
if ( $#ARGV >= 2 ) { $time_period_ = $ARGV [ 2 ] ; }
my $LOCKFILE=$HOME_DIR."/locks/call_run_sim_overnight_".$shortcode_."_".$num_working_days_."_".$time_period_.".lock";

my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{ # AWS
    $LOCKFILE="/mnt/sdf/locks/call_run_sim_overnight_".$shortcode_."_".$num_working_days_."_".$time_period_.".lock";
}

if (! defined $ENV{'NOLOCK'}) {
	if ( -e $LOCKFILE )
	{
		print "Lock Exists: $LOCKFILE \n";
	    exit 0;
	    #PrintStacktraceAndDie ( "$LOCKFILE already present\n" );
	}
	open LOCKFILEHANDLE, "> $LOCKFILE" or PrintStacktraceAndDie ( "$SCRIPTNAME could not open $LOCKFILE\n" );
	flock ( LOCKFILEHANDLE, LOCK_EX ); # write lock
	$is_lock_created_ = 1;
}

if (! defined $ENV{'QUICKRUN'}) {
    my $sanitize_cmd_ = "$SCRIPTS_DIR/sanitize_strat_type_FS_to_DB.pl $shortcode_ 1";
    `$sanitize_cmd_ 2>/dev/null`;
}

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $debug_ = 1;

my $last_date_with_data_ = `date +%Y%m%d`; chomp ( $last_date_with_data_ );

my $hh_ = ` date +%H%M`; chomp ( $hh_ );
# TODO ... improve to actually checking if the data directories exist
if ($time_period_  &&  $hh_ > GetUTCTime( GetEndTimeFromTP($time_period_)) ) {
}
elsif (
    ( ( $shortcode_ eq "NK_0" ) ||
      ( $shortcode_ eq "NKM_0" ) ||
      ( $shortcode_ eq "NKMF_0" ) ||
      ( $shortcode_ eq "JGBL_0" ) ||
      ( $shortcode_ eq "TOPIX_0" ) ||
      ( $shortcode_ eq "JP400_0" ) ||
      ( $shortcode_ eq "MCH_0" ) ||
      ( $shortcode_ eq "MHI_0" ) ||
      ( $shortcode_ eq "HHI_0" ) ||
      ( $shortcode_ eq "HSI_0" ) )
    && ( ( ( $hh_ >= 0745  ) &&
    ( $time_period_  && ( GetUTCTime( GetEndTimeFromTP($time_period_) ) <= 730 ) ) ) || #CME AS, pool with end time <= utc_600
      ( $hh_ >= 1600  && ( defined $ENV{'QUICKRUN'}) ) ) #>16 just ensures that we can run for all sessions
    )
{
}
elsif (
    ( ( $shortcode_ eq "ZN_0" ) ||
      ( $shortcode_ eq "ZF_0" ) ||
      ( $shortcode_ eq "ZB_0" ) ||
      ( $shortcode_ eq "NKD_0" ) ||
      ( $shortcode_ eq "NIY_0" ) ||
      ( $shortcode_ eq "ES_0" ) ||
      ( $shortcode_ eq "UB_0" ) ) &&
    ( $hh_ >= 700 ) &&
    ( $time_period_  && ( GetUTCTime( GetEndTimeFromTP($time_period_) ) <= 600 ) ) #CME AS, pool with end time <= utc_600
    )
{
}
elsif (
    ( ( $shortcode_ eq "XT_0" ) ||
      ( $shortcode_ eq "YT_0" ) ||
      ( $shortcode_ eq "AP_0" ) ||
      ( index( $shortcode_ , "IR_" ) == 0 ) ) &&
    ( $hh_ >= 800 ) &&
    ( $time_period_  && ( GetUTCTime( GetEndTimeFromTP($time_period_) ) <= 700 ) ) #ASX AS, pool with end time <= AST_1630
    )
{
}
elsif (
    ( index( $shortcode_ , "NSE_" ) == 0 ) &&
    ( $hh_ >= 1900 ) #NSE datacopy should finish by 1700
    )
{
}
elsif (
    ( index( $shortcode_ , "SGX_" ) == 0 ) &&
    ( $hh_ >= 1700 ) #NSE datacopy should finish by 1700
    )
{
}
elsif (
    ( defined $ENV{'QUICKRUN'} ) &&
    ( $hh_ >= 2100 ) #>16 just ensures that we can run for all sessions
    ) #This was called by datacopy trigger, so start today's result generation
{
}
else
{
    $last_date_with_data_ = CalcPrevWorkingDateMult ( $last_date_with_data_, 1 );
}
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
            if ( $time_period_ ) 
            { 
		@t_list_ = ();
		push ( @t_list_, $time_period_ ) ;
	    }
    	    my @sorted_item_list_ = sort @t_list_ ;

	    for my $dir_item_ ( @sorted_item_list_ ) # $dir_item is a time_period folder 
    	    {
    		# Unix file system considerations.
    		next if $dir_item_ eq '.' || $dir_item_ eq '..';
		
    		if ( -d "$top_directory_/$dir_item_" )
    		{
    		    my $exec_cmd = "time $MODELSCRIPTS_DIR/run_simulations.pl $shortcode_ $top_directory_/$dir_item_ $t_start_date_ $t_end_date_ DB -dt 1";
    		    if ( $debug_ == 1 ) { print STDERR "$exec_cmd\n"; }
    		    my @sim_strategy_output_lines_ = `{ $exec_cmd; } 2>&1`;
    		    if ( $debug_ == 1 ) {
			print STDERR "OutputLines: ".join( "", @sim_strategy_output_lines_ ) . "\n";
		    }
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
