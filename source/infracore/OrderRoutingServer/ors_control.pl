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

my $ors_control_exec_ = $HOME_DIR."/LiveExec/bin/ors_control_exec";
my $ors_control_port_ = -1 ; # Def CME port needs to be changed

my $configdir_ = $HOME_DIR."/infracore_install/Configs/"; 
my $configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";

my $currDir = `pwd`; chomp $currDir;
chdir $HOME_DIR ;

#given ( $exch_ )
{
#    when ("cme"||"CME")
    if ( ( $exch_ eq "cme" ) || 
	    ( $exch_ eq "CME" ) )
    {
	$configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";
	$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging
    }
#    when ("eurex"||"EUREX")
    elsif ( ( $exch_ eq "eurex" ) || 
	    ( $exch_ eq "EUREX" ) )
    {
	$configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";
	$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging
    }
#    when ("tmx"||"TMX")
    elsif ( ( $exch_ eq "tmx" ) || 
	    ( $exch_ eq "TMX" ) )
    {
	$configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";
	$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging
    }
#    when ("bmf"||"BMF")
    elsif ( ( $exch_ eq "bmf" ) || 
	    ( $exch_ eq "BMF" ) )
    {
	$configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";
	$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging
    }

#    when ("bmfep"||"BMFEP")
    elsif ( ( $exch_ eq "bmfep" ) || 
	    ( $exch_ eq "BMFEP" ) )
    {
	$configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";
	$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging
    }
    elsif ( ( $exch_ eq "tmxlopr" ) || 
	    ( $exch_ eq "TMXLOPR" ) )
    {
	$configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";
	$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging
    }
    elsif ( ( $exch_ eq "liffe" ) || 
	    ( $exch_ eq "LIFFE" ) )
    {
	$configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";
	$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging
    }
    elsif ( ( $exch_ eq "hkex" ) || 
	    ( $exch_ eq "HKEX" ) )
    {
	$configfile_ = $configdir_."/OrderRoutingServer/cfg/".$profile_."/ors.cfg";
	$ors_control_port_ = GetControlPortFromConfig ( $configfile_ ); # TODO add error logging
    }
 
#    default
    else
    {    
	die ("ERROR: As EXCH only CME, EUREX, TMX, BMFEP, BMF or HKEX supported");
    }
}

if ( $ors_control_port_ > 0 )
{ 
    my $exec_cmd = "export NEW_GCC_LIB=/usr/local/lib ; export NEW_GCC_LIB64=/usr/local/lib64 ; export LD_LIBRARY_PATH=\$NEW_GCC_LIB64:\$NEW_GCC_LIB:\$LD_LIBRARY_PATH ;".
	" "." $ors_control_exec_ $ors_control_port_ ". join ( ' ', @ARGV[2 .. $#ARGV ] );
# print ("ONLY_PRINT $exec_cmd\n");
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
