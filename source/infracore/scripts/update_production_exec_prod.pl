#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/sync_running_exec_to_prod_machine.pl"; # for SyncRunningExecToProdMachineUser
require "$GENPERLLIB_DIR/get_all_machines_vec.pl"; # for GetAllMachinesVec

if ( $#ARGV < 4 )
{
    print "USAGE: user source_filename dest_filename backup_name changes_in_new_exec \n";
    exit (0);
}

my $prod_user_ = $ARGV[0];
my $source_filename_ = $ARGV[1];
my $dest_filename_ = $ARGV[2];
my $this_backupname_ = $ARGV[3]; 
my $changes_description_ = $ARGV[4] ;
if ( ExistsWithSize ( $source_filename_ ) )
{
    my @all_machines_ = GetAllMachinesVec();

    foreach my $remote_machine_ ( @all_machines_ )
    {
      if ( index ( $remote_machine_, "10.23.23.12" ) >= 0 ) { next; }
        SyncRunningExecToProdMachineUser ( $prod_user_, $remote_machine_, $source_filename_ , $dest_filename_ , $this_backupname_, $changes_description_ ) ;
    }
}

