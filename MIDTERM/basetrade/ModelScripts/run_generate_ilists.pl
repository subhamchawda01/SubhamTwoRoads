#!/usr/bin/perl

# \file ModelScripts/run_next_gen_strat.pl
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

use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);
use File::Path qw(mkpath);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $SCRIPTNAME="$0";

my $QUEUEBASEDIR=$HOME_DIR."/gil/" ;
my $HOST=`hostname -s` ; chomp ( $HOST ) ;
my $QUEUEFILE=$QUEUEBASEDIR.$HOST ;
my $RUNNINGFILE=$QUEUEBASEDIR."running_now_".$HOST ;
my $PROCESSEDFILE=$QUEUEBASEDIR."processed_".$HOST ;

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

my $config_torun_ = "";
#my $common_string = "-im 250 -sd TODAY-40 -ed TODAY-5 -dc 0.85 -ic 0.8 -ss SHARPE -cs TODAY-5 -ce TODAY-2 -mm 600 -nm 1";
my $common_string = "-im 1000 -sd TODAY-80 -ed TODAY-5 -cc 1.5 -ic 0.8 -ss SHARPE -cs TODAY-4 -ce TODAY-2 -mm 5000 -rm 300 -nm 1";

my $today_weekday_=`date +%w`; chomp ( $today_weekday_ );

my $max_running_on_server_ = 3; # File-server perks.
if ( NumRunningNow ( ) < $max_running_on_server_ )
{ # make this different per server

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
	    my @txt_words_ = split ( ' ', $txt_line_ );
	    if ( ( $#txt_words_ >= 1 ) &&
		 ( $txt_words_[0] eq "CONFIGFILE" ) )
	    { # "CONFIGFILE" <filename> [WEEKDAY1 ... ] # day of week (0..6); 0 is Sunday OR -1 signaling only run once
		my $this_config_ = $txt_words_[1];
		
		my @days_allowed_ = ();
		if ( $#txt_words_ >= 2 )
		{
		    for ( my $twidx_ = 2; $twidx_ <= $#txt_words_ ; $twidx_ ++ )
		    {
			push ( @days_allowed_, $txt_words_[$twidx_] ) ;
		    }
		}

		if ( ! ( FoundInProcessedFile ( $this_config_ ) ||
			   FoundInRunningFile ( $this_config_ ) ) ) 
		{
		    #my $comb_config_files_exist_ = CombConfigFilesExist ( $HOME_DIR."/indicatorwork/prod_configs", $this_config_ );
		    #if ( $comb_config_files_exist_ )
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
			    $config_torun_ = $this_config_ ;
			    
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

	AddToRunningFile ( $config_torun_ );

	my $SHC = `grep SELF $config_torun_ | awk '{print \$2}'`;
	chomp ( $SHC ) ;
	my $USER = `echo -n \$USER` ;

	my $TEMP_WORK_DIR="$HOME_DIR/indicatorwork/ilists";
	mkpath $TEMP_WORK_DIR;
	chdir $TEMP_WORK_DIR;

	`perl $MODELSCRIPTS_DIR/generate_ilists.pl -un $USER -sc $SHC -uf $config_torun_ $common_string` ;

	`rm ilists_*$SHC*`;	    

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
