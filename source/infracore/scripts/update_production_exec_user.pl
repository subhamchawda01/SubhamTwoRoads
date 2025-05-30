#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/sync_running_exec_to_prod_machine.pl"; # for SyncToAllMachines

if ( $#ARGV < 5 )
{
    print "USAGE: user server-ip source_filename dest_filename backup_name changes_in_new_exec \n";
    exit (0);
}

my $prod_user_ = $ARGV[0];
my $prod_machine_ = $ARGV[1];
my $source_filename_ = $ARGV[2];
my $dest_filename_ = $ARGV[3];
my $this_backupname_ = $ARGV[4]; 
my $changes_description_ = $ARGV[5] ;
if ( ExistsWithSize ( $source_filename_ ) )
{
    SyncRunningExecToProdMachineUser ( $prod_user_, $prod_machine_, $source_filename_ , $dest_filename_ , $this_backupname_, $changes_description_ ) ;
}

