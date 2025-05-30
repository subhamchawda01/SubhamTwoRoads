#!/usr/bin/perl
##Script that invokes a number of ADDTRADINGSYMBOL calls to ORS via the 
##underlying ors_control.pl script
use strict;
use warnings;
use FileHandle;

my $USAGE="$0 EXCH PROFILE";
if ( $#ARGV != 1 ) { print $USAGE."\n"; exit( 0 ); }
my $exch_ = $ARGV[ 0 ];
my $profile_ = $ARGV[ 1 ];

my $HOME_DIR=$ENV{'HOME'};
chdir $HOME_DIR ;

my $margin_file_ = $HOME_DIR."/infracore_install/files/Margin/config.txt";
my $ors_control_script_ = $HOME_DIR."/LiveExec/OrderRoutingServer/ors_control.pl";

open MFILEHANDLE, "< $margin_file_ " or die "Could not open(r) margin_file $margin_file_\n";
my $in;
my @tokens;
while( $in = <MFILEHANDLE> )
{
    chomp ( $in );
    @tokens = split( ' ', $in );
    if ( $#tokens == 5 && ( $tokens[ 4 ] eq $exch_ ) && ( $tokens[ 5 ] eq $profile_ ) )
    {
	my $max_worst_pos = $tokens[ 1 ] * 2;
	my $exec_cmd = "$ors_control_script_ $exch_ $profile_ ADDTRADINGSYMBOL $tokens[ 0 ] $tokens[ 1 ] $tokens[ 2 ] $max_worst_pos $tokens[ 3 ]";
        system( "$exec_cmd" );
	sleep( 1 );
    }
}
