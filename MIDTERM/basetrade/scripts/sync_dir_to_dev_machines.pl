#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/sync_to_dev_machines.pl"; # for SyncToDevMachines

if ( $#ARGV < 0 )
{
    print "USAGE: dirname\n";
    exit (0);
}

my $this_dirname_ = $ARGV[0];

if ( $this_dirname_ =~ /\/$/ )
{
  $this_dirname_ = substr ( $this_dirname_, 0, length ( $this_dirname_ ) - 1 );
}

SyncDirToDevMachines ( $this_dirname_ );
