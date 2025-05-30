#!/usr/bin/perl
	
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/sync_running_exec_to_prod_machine.pl"; # for SyncRunningExecToProdMachineUser

if ( $#ARGV < 0 )
{
    print "USAGE: source_filename \n";
    exit (0);
}
# Using YYYYMMDD_PID as the backup exec name
my $YYYYMMDD = `date +"%Y%m%d"` ; chomp ($YYYYMMDD);

my $prod_user_ = "dvctrader";
my $source_filename_ = $ARGV[0]; chomp ( $source_filename_ ) ;
my $BASE_EXEC=`basename $source_filename_`; chomp ($BASE_EXEC);
my $dest_filename_ = "LiveExec/bin/".$BASE_EXEC; chomp ($dest_filename_);
my $this_backupname_ = "$YYYYMMDD"."_"."$$"; chomp ($this_backupname_);
my $changes_description_ = $BASE_EXEC."-Update-".$YYYYMMDD ; chomp($changes_description_);

if ( ExistsWithSize ( $source_filename_ ) )
{
    my @all_machines_ = ("10.23.82.53","10.23.82.54","10.23.82.55","10.23.82.56");	#all chi servers

    foreach my $remote_machine_ ( @all_machines_ )
    {
    	SyncRunningExecToProdMachineUser ( $prod_user_, $remote_machine_, $source_filename_ , $dest_filename_ , $this_backupname_, $changes_description_ ) ;
    }
}
