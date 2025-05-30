#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $locks_dir_ = "$HOME_DIR/locks";
use File::Basename;

sub SendReportMail;
my $debug_ = 0;
# get all the locks # 
# $HOME_DIR."/locks/call_run_sim_overnight_".$shortcode_.".lock" 
# $HOME_DIR."/locks/call_run_sim_overnight_longer_".$shortcode_."_".$skip_recent_days_."_".$num_working_days_.".lock"

# check for process 
# ps -ef | grep call_run_sim_overnight_perdir.pl | grep " LFZ_0"
# ps -ef | grep call_run_sim_overnight_perdir_longer.pl | grep " FGBM_0 "  | grep " 4 " | grep " 15"

my %lock_file_to_pid_ = ();
{ 
    my $lock_file_;
    my @pids_;
    foreach $lock_file_ ( <$locks_dir_/*> ) 
    {
	chomp ( $lock_file_);
	$lock_file_ = basename ( $lock_file_ ) ;
	if ( $debug_ == 1 ) { print $lock_file_."\n" ; }
	my @tkns_ = split ( '\.' , $lock_file_ ) ;
	if ( $debug_ == 1 ) { print scalar ( @tkns_ )."\n" ; }
	if ( scalar ( @tkns_ ) != 2  || $tkns_[1] ne "lock" )
	{
	    if ( $debug_ == 1 ) { print $tkns_[1]."\n" ; }
	    next ;
	}
	
	my @sub_tkns_ = split ( '\_', $tkns_[0] );

	my $shc_ = "NA";
	my $skip_recent_days_ = "NA";
	my $num_working_days_ = "NA";
	if ( $debug_ == 1 ) { print scalar ( @sub_tkns_)."\n" ; }

	if ( ( scalar ( @sub_tkns_ ) == 8 || scalar ( @sub_tkns_ ) == 9 || scalar ( @sub_tkns_ ) == 5 || scalar ( @sub_tkns_ ) == 4 ) && 
	     $sub_tkns_[0] eq "call" && $sub_tkns_[1] eq "run" &&
	     $sub_tkns_[2] eq "sim" && $sub_tkns_[3] eq "overnight" )
	{
	    if ( scalar ( @sub_tkns_ ) == 8 || scalar ( @sub_tkns_ ) == 9 )
	    {
		if ( $debug_ == 1 ) { print $sub_tkns_[4]."\n" ; }
		if ( $sub_tkns_[4] ne "longer" )
		{
		    next ;
		}
		if ( scalar ( @sub_tkns_ ) == 8 )
		{
		    $shc_ = $sub_tkns_[5];
		    $skip_recent_days_ = $sub_tkns_[6];
		    $num_working_days_ = $sub_tkns_[7];
		}
		if ( scalar ( @sub_tkns_ ) == 9 )
		{
		    $shc_ = $sub_tkns_[5]."_".$sub_tkns_[6];
		    $skip_recent_days_ = $sub_tkns_[7];
		    $num_working_days_ = $sub_tkns_[8];
		}

		@pids_ = `ps -ef | grep -v grep | grep \"call_run_sim_overnight_perdir_longer.pl $shc_ $skip_recent_days_ $num_working_days_\" | awk '{ print \$2}' `;
		if ( $debug_ == 1 ) { print "ps -ef | grep \"call_run_sim_overnight_perdir_longer.pl $shc_ $skip_recent_days_ $num_working_days_\" | awk '{ print \$2}'\n"};
	    }
	    else
	    {
		if ( scalar ( @sub_tkns_ ) == 5 )
		{
		    $shc_ = $sub_tkns_[4];
		}
		if ( scalar ( @sub_tkns_ ) == 4 )
		{
		    $shc_ = $sub_tkns_[4];
		}
		@pids_ = `ps -ef | grep -v grep | grep \"call_run_sim_overnight_perdir.pl\" | grep \" $shc_\" | awk '{ print \$2}' `;
		if ( $debug_ == 1 ) { print "ps -ef | grep -v grep | grep \"call_run_sim_overnight_perdir.pl\" | grep \" $shc_\" | awk '{ print \$2}'"};
	    }
	    if ( scalar ( @pids_ ) == 1 )
	    {
		chomp($pids_[0]);
		$lock_file_to_pid_{ $lock_file_ } = $pids_[0];
	    }
	    else
	    {
		$lock_file_to_pid_{ $lock_file_ } = -1;
	    }
	}
    }
}

my $msg_ = "";
for ( keys %lock_file_to_pid_ )
{
    if ( $lock_file_to_pid_{$_} == -1 )
    {
	$msg_=$msg_.$_."\n";
    }
    else
    {
	print $_." process_id_ ".$lock_file_to_pid_{$_}."\n";
    }
}

my $hostname_ = `hostname`;
chomp ( $hostname_ );
my $to_address_ = "nseall@tworoads.co.in";
my $from_address_ = $USER."\@$hostname_.com";

if ( $debug_ == 1 ) { print $to_address_." ".$from_address_."\n"; }
SendReportMail ( );

sub SendReportMail ( )
{
    print STDOUT $msg_;

    if ( $to_address_ eq "NA" )
    {
	return;
    }
    else
    {
	if ( $to_address_ && $msg_ )
	{
	    open ( MAIL , "|/usr/sbin/sendmail -t" );
	    
	    print MAIL "To: $to_address_\n";
	    print MAIL "From: $from_address_ \n";
	    print MAIL "Subject: global results locks with missing processes \n\n";
	    print MAIL $msg_ ;
	    
	    close(MAIL);
	}
    }
}
