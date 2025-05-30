#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/sync_to_dev_machines.pl"; # for SyncToDevMachines

if ( $#ARGV < 0 )
{
    print "USAGE: full_filename\n";
    exit (0);
}

my $this_filename_ = $ARGV[0];
if ( ExistsWithSize ( $this_filename_ ) )
{
    SyncToDevMachines ( $this_filename_ ) ;
}

