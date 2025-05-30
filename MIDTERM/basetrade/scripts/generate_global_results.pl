#!/usr/bin/perl

# \file scripts/generate_global_results.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 162, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes as input :

## This script is the top level script to generate global results through call_run_sim_overnight_perdir_longer (using call_run_simulations_2.pl)
## Maintain queue structure for all  [prods X durations]
## Give priority to scripts running for shorter duration while running few longer duration scripts as well
## Remove call_run_sim_overnight_ [both recent longer_1 longer_2 perdir]


use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);
use File::Path qw(mkpath);

my $USER = $ENV { 'USER' };
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_server_out_of_disk_space.pl"; # IsServerOutOfDiskSpace

my $SCRIPTNAME="$0";

my $QUEUEBASEDIR=$HOME_DIR."/grq/" ;
my $HOST=`hostname -s` ; chomp ( $HOST ) ;
my $QUEUEFILE=$QUEUEBASEDIR.$HOST ;
my $RUNNINGFILE=$QUEUEBASEDIR."running_now_".$HOST ;
my $PROCESSEDFILE=$QUEUEBASEDIR."processed_".$HOST ;

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

my $config_torun_ = "";

my $today_YYYYMMDD_=`date +%Y%m%d`; chomp ( $today_YYYYMMDD_ );

#my $max_recent_running_on_server_ = 8; 
#my $max_longer_running_on_server_ = 5;
#my $max_very_long_running_on_server_ = 3; 

my $max_total_running_on_server_ = 11;
my @max_running_on_server_ = (8,5,3);
my @sim_type_vec_ = ("RECENT", "LONGER", "VERYLONG");


if ( IsServerOutOfDiskSpace ( ) )
{
    PrintStacktraceAndDie ( "$HOST out of disk space. $SCRIPTNAME exiting." );
}


for ( my $index_ = 0; $index_ <= $#sim_type_vec_; $index_ ++ )
{
    my $sim_type_ = $sim_type_vec_[$index_];
    if ( ( NumRunningNow ( $sim_type_ ) < $max_running_on_server_[$index_] ) && ( TotalRunningNow() < $max_total_running_on_server_ ) )
    { 
	if ( -e $QUEUEFILE ) 
	{
	    my @txt_lines_ = ();
	    open QUEUEFILEHANDLE, "+< $QUEUEFILE" or PrintStacktraceAndDie ( "$SCRIPTNAME could not open $QUEUEFILE\n" );
	    flock ( QUEUEFILEHANDLE, LOCK_EX ); # write lock
	    @txt_lines_ = <QUEUEFILEHANDLE>;
	    chomp ( @txt_lines_ );
	    for ( my $tlidx_ = 0; $tlidx_ <= $#txt_lines_; $tlidx_ ++ )
	    {
		my $txt_line_ = $txt_lines_[$tlidx_];
		if ( $txt_line_ =~ /#/ ) {next ;}
		my @txt_words_ = split ( ' ', $txt_line_ );
		# RECENT LFR_0 0 5
		# LONGER LFR_0 6 30 
		# RECENT/LONGER/VERYLONG shortcode skip_recent_days num_working_days
		
		if ( ( $#txt_words_ >= 3 )&& ( $txt_words_[0] eq $sim_type_ ) )
		{ # "CONFIGFILE" <filename> [WEEKDAY1 ... ] # day of week (0..6); 0 is Sunday OR -1 signaling only run once
		    my $shortcode_ = $txt_words_[1];
		    my $skip_recent_days_ = $txt_words_[2];
		    my $num_working_days_ = $txt_words_[3];
		    my $this_config_filename_ = "$shortcode_ $skip_recent_days_ $num_working_days_";
		    splice ( @txt_lines_, $tlidx_, 1 ) ; # remove this
		    push ( @txt_lines_, $txt_line_ ); # add at the back
		    
		    if ( ! ( FoundInProcessedFile ( $sim_type_, $this_config_filename_ ) || FoundInRunningFile ( $sim_type_, $this_config_filename_ ) ) )
		    {
			$config_torun_ = $this_config_filename_;
			last;
		    }
		}
	    }
	    seek ( QUEUEFILEHANDLE, 0, 0 ); 
	    truncate ( QUEUEFILEHANDLE, 0 );
	    for ( my $tlidx_ = 0 ; $tlidx_ <= $#txt_lines_ ; $tlidx_++ )
	    {
		printf QUEUEFILEHANDLE "%s\n", $txt_lines_[$tlidx_] ;
	    }
	    close QUEUEFILEHANDLE;
	}
	else
	{
	    print "Missing $QUEUEFILE\n";
	}
	
	if ( $config_torun_ )
	{
	    #print "AddRunning $config_torun_\n";
	    AddToRunningFile ( $sim_type_, $config_torun_ );
	    #print "Running $config_torun_\n";
	    `$SCRIPTS_DIR/call_run_sim_overnight_perdir_longer.pl $config_torun_` ;
	    #print "RemoveRunning $config_torun_\n";
	    RemoveFromRunningFile ( $sim_type_, $config_torun_ );
	    exit (0) ;
	}
    }
}

exit ( 0 );



sub FoundInProcessedFile 
{
    my $t_sim_type_ = shift;
    my $t_config_torun_ = shift;
    my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
    
    open PROCESSEDFILEHANDLE, "< $PROCESSEDFILE" or return 0;
    flock ( PROCESSEDFILEHANDLE, LOCK_SH ); # read lock
    my @lines_ = <PROCESSEDFILEHANDLE>; chomp (@lines_ );
    close ( PROCESSEDFILEHANDLE );
    for ( my $i = 0 ; $i <= $#lines_ ; $i++ )
    {
	if ( $lines_[$i] =~ /$t_sim_type_ $t_config_torun_ $yyyymmdd_/ )
	{
	    return 1;
	}
    }
    return 0;
}

sub FoundInRunningFile
{
    my $t_sim_type_ = shift;
    my $t_config_torun_ = shift;

    open RUNNINGFILEHANDLE, "< $RUNNINGFILE" or return 0;
    flock ( RUNNINGFILEHANDLE, LOCK_SH ); # read lock
    my @lines_ = <RUNNINGFILEHANDLE>; chomp (@lines_ );
    close ( RUNNINGFILEHANDLE );
    for ( my $i = 0 ; $i <= $#lines_ ; $i++ )
    {
	if ( $lines_[$i] =~ /$t_sim_type_ $t_config_torun_/ )
	{
	    return 1;
	}
    }
    return 0;
}

sub AddToRunningFile 
{
    my $t_sim_type_ = shift;
    my $t_config_torun_ = shift;
    my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
    my $unixtime_ = `date +%s`; chomp ( $unixtime_ );

    `mkdir -p $QUEUEBASEDIR`;
    open RUNNINGFILEHANDLE, ">> $RUNNINGFILE" or PrintStacktraceAndDie ( "$SCRIPTNAME could not open $RUNNINGFILE\n" );
    flock ( RUNNINGFILEHANDLE, LOCK_EX ); # write lock

    printf RUNNINGFILEHANDLE "%s %s %s %s\n", $t_sim_type_, $t_config_torun_, $yyyymmdd_, $unixtime_ ;
    close ( RUNNINGFILEHANDLE );
}

sub RemoveFromRunningFile
{
    my $t_sim_type_ = shift;
    my $t_config_torun_ = shift;
    
    `mkdir -p $QUEUEBASEDIR`;
    open PROCESSEDFILEHANDLE, ">> $PROCESSEDFILE" or PrintStacktraceAndDie ( "$SCRIPTNAME could not open $PROCESSEDFILE\n" );
    flock ( PROCESSEDFILEHANDLE, LOCK_EX ); # write lock

    open RUNNINGFILEHANDLE, "+< $RUNNINGFILE" or PrintStacktraceAndDie ( "$SCRIPTNAME could not open $RUNNINGFILE\n" );
    flock ( RUNNINGFILEHANDLE, LOCK_EX ); # write lock
    my @lines_ = <RUNNINGFILEHANDLE>; chomp (@lines_ );
    seek ( RUNNINGFILEHANDLE, 0, 0 ); 
    truncate ( RUNNINGFILEHANDLE, 0 );
    for ( my $i = 0 ; $i <= $#lines_ ; $i++ )
    {
	if ( $lines_[$i] =~ /$t_sim_type_ $t_config_torun_ / )
	{
	    my $unixtime_ = `date +%s`; chomp ( $unixtime_ );
	    printf PROCESSEDFILEHANDLE "%s %s\n", $lines_[$i], $unixtime_ ;
	}
	else
	{
	    printf RUNNINGFILEHANDLE "%s\n", $lines_[$i] ;
	}
    }
    close ( RUNNINGFILEHANDLE );
    close ( PROCESSEDFILEHANDLE );
}

sub NumRunningNow
{
    my $sim_type_ = shift;
    my $running_count_ = 0 ;
    if ( -e $RUNNINGFILE )
    {
    	$running_count_ = `grep -c "$sim_type_ " $RUNNINGFILE`;
    }
    return $running_count_;
}

sub TotalRunningNow
{
    my $running_count_ = 0 ;
    if ( -e $RUNNINGFILE )
    {
    	$running_count_ = `wc -l $RUNNINGFILE | awk '{print \$1}'`;
    }
    return $running_count_;
}
