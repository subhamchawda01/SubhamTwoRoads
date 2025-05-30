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

my $USER=$ENV{'USER'}; 
my $HOSTNAMEIP=`hostname -i`; chomp($HOSTNAMEIP);

my @all_machines_ = ();

push ( @all_machines_, "10.23.199.51" );
push ( @all_machines_, "10.23.199.52" );
push ( @all_machines_, "10.23.199.53" );
push ( @all_machines_, "10.23.199.54" );
push ( @all_machines_, "10.23.199.55" );
push ( @all_machines_, "10.23.142.51" );

my $base_dirname_ = dirname ($this_dirname_);

foreach my $remote_machine_ ( @all_machines_ )
{
    if ( $remote_machine_ ne $HOSTNAMEIP )
    {
	`ssh $remote_machine_ mkdir -p $this_dirname_`;
	`rsync -Lravz $this_dirname_ $USER\@$remote_machine_:$base_dirname_`;

    }
}
