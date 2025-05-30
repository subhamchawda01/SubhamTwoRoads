#!/usr/bin/perl

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/sync_to_all_machines.pl"; # for SyncToAllMachines

if ( $#ARGV < 0 )
{
    print "USAGE: full_dirname\n";
    exit (0);
}

my $this_dirname_ = $ARGV[0];

# If there is a trailing '/', remove it. rsync behaves differently in that case
if ( $this_dirname_ =~ /\/$/ )
{
    $this_dirname_ = substr ( $this_dirname_, 0, length ( $this_dirname_ ) - 1 );
}

my $USER=$ENV{'USER'}; 
my $HOSTNAMEIP=`hostname -i`; chomp($HOSTNAMEIP);

my @all_machines_ = GetAllMachinesVec();

my $base_dirname_ = dirname ($this_dirname_);

foreach my $remote_machine_ ( @all_machines_ )
{
    if ( $remote_machine_ ne $HOSTNAMEIP )
    {
        print "ssh $remote_machine_ mkdir -p $this_dirname_\n";
        `ssh $remote_machine_ mkdir -p $this_dirname_`;
        print "rsync -Lravz $this_dirname_ $USER\@$remote_machine_:$base_dirname_\n";
        `rsync -Lravz $this_dirname_ $USER\@$remote_machine_:$base_dirname_`;
    }
}
