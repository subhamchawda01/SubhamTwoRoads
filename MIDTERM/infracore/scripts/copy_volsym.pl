#!/usr/bin/perl

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
require "$HOME_DIR/infracore_install/GenPerlLib/sync_to_all_machines.pl";

if ( $#ARGV < 0 ) { exit ( 0 ); }
SyncToAllMachines ( $ARGV[0] );
