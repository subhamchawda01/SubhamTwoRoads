#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/sync_running_exec_to_all_machines.pl"; # for SyncToAllMachines

if ( $#ARGV < 1 )
{
    print "USAGE: full_filename backup_name\n";
    exit (0);
}

my $this_filename_ = $ARGV[0];
my $this_backupname_ = $ARGV[1]; 
if ( ExistsWithSize ( $this_filename_ ) )
{
    SyncRunningExecToAllMachines ( $this_filename_, $this_backupname_ ) ;
}

