#!/usr/bin/perl

# \file ModelScripts/run_next_gen_strat.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes as input :

use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);
use File::Path qw(mkpath);

my $USER = $ENV { 'USER' };
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $LIVE_MODELSCRIPTS_DIR=$HOME_DIR."/LiveExec/ModelScripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_server_out_of_disk_space.pl"; # IsServerOutOfDiskSpace

my $SCRIPTNAME="$0";

my $QUEUEBASEDIR=$HOME_DIR."/gsq/" ;
my $HOST=`hostname -s` ; chomp ( $HOST ) ;
my $QUEUEFILE=$QUEUEBASEDIR.$HOST ;
my $RUNNINGFILE=$QUEUEBASEDIR."running_now_".$HOST ;
my $PROCESSEDFILE=$QUEUEBASEDIR."processed_".$HOST ;

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

my $config_torun_ = "";

my $today_weekday_=`date +%w`; chomp ( $today_weekday_ );

my $max_running_on_server_ = 1; # reduced for low disk space

if ( $USER ne "dvctrader" )
{ # 
    if ( index ( $HOST , "srv12" ) >= 0 || index ( $HOST , "srv11" ) >= 0 )
    {
	$max_running_on_server_ = 1;
    }
    else
    {
	$max_running_on_server_ = 1;
    }

    if ( IsServerOutOfDiskSpace ( ) )
    {
	PrintStacktraceAndDie ( "$HOST out of disk space. Exiting." );
	exit ( 0 );
    }
}
else
{
    $max_running_on_server_ = 6;
    if ( $HOST =~ m/sdv-crt-srv11/ ) 
    {
	if ( $today_weekday_ == 6 || $today_weekday_ == 7 )
	{
	    $max_running_on_server_ = 3;
	}
	else
	{
	    $max_running_on_server_ = 1;
	}
    }
    if ( IsServerOutOfDiskSpace ( ) )
    {
	PrintStacktraceAndDie ( "$HOST out of disk space. Exiting." );
	exit ( 0 );
    }
}

if ( NumRunningNow ( ) < $max_running_on_server_ )
{ # make this different per server

    if ( -e $QUEUEFILE ) 
    {
	my @txt_lines_ = ();
	open QUEUEFILEHANDLE, "+< $QUEUEFILE" or PrintStacktraceAndDie ( "$SCRIPTNAME could not open $QUEUEFILE\n" );
	flock ( QUEUEFILEHANDLE, LOCK_EX ); # write lock
	@txt_lines_ = <QUEUEFILEHANDLE>;
	chomp ( @txt_lines_ );
	for ( my $ext_loop_iter_ = 0 ; $ext_loop_iter_ < 2 ; $ext_loop_iter_ ++ )
	{
	    if ( ( $ext_loop_iter_ == 1 ) &&
		 ( $config_torun_ ) )
	    {
		# found in first loop
		last;
	    }
		 
	for ( my $tlidx_ = 0; $tlidx_ <= $#txt_lines_; $tlidx_ ++ )
	{
	    my $txt_line_ = $txt_lines_[$tlidx_];
	    my @txt_words_ = split ( ' ', $txt_line_ );
	    if ( ( $#txt_words_ >= 1 ) &&
		 ( $txt_words_[0] eq "CONFIGFILE" ) )
	    { # "CONFIGFILE" <filename> [WEEKDAY1 ... ] # day of week (0..6); 0 is Sunday OR -1 signaling only run once
		my $this_config_filename_ = $txt_words_[1];
		if ( ( $ext_loop_iter_ == 0 ) &&
		     ( ! ( $this_config_filename_ =~ /_R/ ) ) )
		{ # in first loop only _R
		    next;
		}
		if ( ExistsWithSize ( $this_config_filename_ ) )
		  {
		    my @days_allowed_ = ();
		    if ( $#txt_words_ >= 2 )
		      {
			for ( my $twidx_ = 2; $twidx_ <= $#txt_words_ ; $twidx_ ++ )
			  {
			    push ( @days_allowed_, $txt_words_[$twidx_] ) ;
			  }
		      }
		    
		    if ( ! ( FoundInProcessedFile ( $this_config_filename_ ) ||
			     FoundInRunningFile ( $this_config_filename_ ) ) )
		      {
			my $allowed_today_ = 1;
			my $only_once_ = 0;
			
			if ( $#days_allowed_ >= 0 )
			  { # process running day information
			    if ( ThisWeekDayAllowed ( \@days_allowed_ ) == 0 )
			      {
				$allowed_today_ = 0;
			      }
			    if ( ThisWeekDayAllowed ( \@days_allowed_ ) == 2 )
			      {
				$only_once_ = 1;
			      }
			  }
			
			if ( $allowed_today_ == 1 )
			  {
			    $config_torun_ = $this_config_filename_ ;
			    
			    splice ( @txt_lines_, $tlidx_, 1 ) ; # remove this
			    if ( $only_once_ == 1 )
			      {
				push ( @txt_lines_, "# ".$txt_line_ ); # comment out for future
			      }
			    else
			      {
				push ( @txt_lines_, $txt_line_ ); # add at the back
			      }
			    last;
			  }
		      }
		  }
	    } 
	    else 
	    {
		splice ( @txt_lines_, $tlidx_, 1 ) ; # remove this
		push ( @txt_lines_, $txt_line_ ); # add at the back
	    }
	}
	} # end of outer loop

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
#	print "AddRunning $config_torun_\n";
	AddToRunningFile ( $config_torun_ );
#	print "Running $config_torun_\n";

	my $TEMP_WORK_DIR="$HOME_DIR/pnl_temp";
	mkpath $TEMP_WORK_DIR;
	chdir $TEMP_WORK_DIR;
	`$LIVE_MODELSCRIPTS_DIR/generate_strategies.pl $config_torun_` ;
#	print "RemoveRunning $config_torun_\n";
	RemoveFromRunningFile ( $config_torun_ );
    }
}
#else
#{
#    print "Already >= $max_running_on_server_ processes running\n";
#}
exit ( 0 );

sub ThisWeekDayAllowed
{
    my ($array_ref) = @_;
    foreach (@$array_ref) 
    { 
	if ( $_ == $today_weekday_ )
	{
	    return 1;
	}
	if ( $_ == -1 )
	{
	    return 2;
	}
    }
    return 0;
}

sub FoundInProcessedFile 
{
    my $t_config_torun_ = shift;
    my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
    
    open PROCESSEDFILEHANDLE, "< $PROCESSEDFILE" or return 0;
    flock ( PROCESSEDFILEHANDLE, LOCK_SH ); # read lock
    my @lines_ = <PROCESSEDFILEHANDLE>; chomp (@lines_ );
    close ( PROCESSEDFILEHANDLE );
    for ( my $i = 0 ; $i <= $#lines_ ; $i++ )
    {
	if ( $lines_[$i] =~ /CONFIGFILE $t_config_torun_ $yyyymmdd_/ )
	{
	    return 1;
	}
    }
    return 0;
}

sub FoundInRunningFile
{
    my $t_config_torun_ = shift;

    open RUNNINGFILEHANDLE, "< $RUNNINGFILE" or return 0;
    flock ( RUNNINGFILEHANDLE, LOCK_SH ); # read lock
    my @lines_ = <RUNNINGFILEHANDLE>; chomp (@lines_ );
    close ( RUNNINGFILEHANDLE );
    for ( my $i = 0 ; $i <= $#lines_ ; $i++ )
    {
#	print "checking $lines_[$i]\n";
	if ( $lines_[$i] =~ /CONFIGFILE $t_config_torun_ / )
	{
#	    print "found $t_config_torun_\n";
	    return 1;
	}
    }
    return 0;
}

sub AddToRunningFile 
{
    my $t_config_torun_ = shift;
    my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
    my $unixtime_ = `date +%s`; chomp ( $unixtime_ );

    `mkdir -p $QUEUEBASEDIR`;
    open RUNNINGFILEHANDLE, ">> $RUNNINGFILE" or PrintStacktraceAndDie ( "$SCRIPTNAME could not open $RUNNINGFILE\n" );
    flock ( RUNNINGFILEHANDLE, LOCK_EX ); # write lock

    printf RUNNINGFILEHANDLE "CONFIGFILE %s %s %s\n", $t_config_torun_, $yyyymmdd_, $unixtime_ ;
    close ( RUNNINGFILEHANDLE );
}

sub RemoveFromRunningFile
{
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
	if ( $lines_[$i] =~ /CONFIGFILE $t_config_torun_ / )
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
    my $running_count_ = 0 ;
    open RUNNINGFILEHANDLE, "< $RUNNINGFILE" or return $running_count_;
    flock ( RUNNINGFILEHANDLE, LOCK_SH ); # read lock
    my @lines_ = <RUNNINGFILEHANDLE>; chomp (@lines_ );
    close ( RUNNINGFILEHANDLE );
    for ( my $i = 0 ; $i <= $#lines_ ; $i++ )
    {
	if ( $lines_[$i] =~ /CONFIGFILE / )
	{
	    $running_count_ ++;
	}
    }
    return $running_count_;
}
