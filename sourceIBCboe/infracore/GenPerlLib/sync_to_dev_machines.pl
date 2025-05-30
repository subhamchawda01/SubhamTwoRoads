# \file GenPerlLib/sync_to_dev_machines.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 
# my $REPO="infracore";

# my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $HOSTNAMEIP=`hostname -i`; chomp($HOSTNAMEIP);

sub SyncToAllMachines {
    my @dev_machines_ = ();

#ny4
    push ( @dev_machines_, "10.23.199.51" );
    push ( @dev_machines_, "10.23.199.52" );
    push ( @dev_machines_, "10.23.199.53" );
    push ( @dev_machines_, "10.23.199.54" );
    push ( @dev_machines_, "10.23.199.55" );
#crt
    push ( @dev_machines_, "10.23.142.51" );

    my $input_filename_ = shift;
    my $dirname_ = dirname($input_filename_);

    my $debug = 0;
    
    foreach my $remote_machine_ ( @dev_machines_ )
    {
	if ( $remote_machine_ ne $HOSTNAMEIP )
	{
	    if ( $debug == 1 )
	    {
		print "ssh $remote_machine_ mkdir -p $dirname_"."\n";
		print "scp $input_filename_ $USER\@$remote_machine_:$input_filename_"."\n";
	    }

	    `ssh $remote_machine_ mkdir -p $dirname_`;
	    `scp $input_filename_ $USER\@$remote_machine_:$input_filename_`;
	}
    }
}

1;
