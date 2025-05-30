#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
# use switch; # for given when if supported

sub GetControlPortFromConfig ;

my $USAGE="$0 EXCH PROFILE CMD_WORDS ...";
if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $exch_ = $ARGV[0];
my $profile_ = $ARGV[1];

my $HOME_DIR=$ENV{'HOME'};

my $ors_control_exec_ = "/home/pengine/prod/live_execs/ors_control_exec";
my $ors_control_port_ = -1 ; # Def CME port needs to be changed

my $configdir_ = "/home/pengine/prod/live_configs/" ;
my $configfile_ = $configdir_."common_"."$profile_"."_ors.cfg";

my $currDir = `pwd`; chomp $currDir;
chdir $HOME_DIR ;

$configfile_ = $configdir_."common_"."$profile_"."_ors.cfg" ; 
$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging

if ( $ors_control_port_ > 0 )
{ 
    my $exec_cmd = "$ors_control_exec_ $ors_control_port_ ". join ( ' ', @ARGV[2 .. $#ARGV ] );
    system ("$exec_cmd"); # not using backslash operator to send output to screen
}

chdir $currDir ;

exit ( 0 );

sub GetControlPortFromConfig
{
    my $cme_config_file_ = shift;
    my $cme_control_port_ = -1;
    if ( -e $cme_config_file_ )
    {
	open CONFIGFILEHANDLE, "< $cme_config_file_" or die "Could not open(r) cme_config_file $cme_config_file_\n";
	my @cme_config_lines_ = <CONFIGFILEHANDLE> ;
	close CONFIGFILEHANDLE ;
	
	for ( my $i = 0 ; $i <= $#cme_config_lines_ ; $i ++ )
	{
	    chomp ( $cme_config_lines_[$i] );
	    my @cme_config_words_ = split ( ' ', $cme_config_lines_[$i] );
	    if ( ( $#cme_config_words_ >= 1 ) && 
			( $cme_config_words_[0] =~ /Control_Port/ ) )
	    {
		$cme_control_port_ = $cme_config_words_[1];
	    }
	}
    }
    return ($cme_control_port_);
}
