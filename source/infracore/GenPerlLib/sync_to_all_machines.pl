# \file GenPerlLib/sync_to_all_machines.pl
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
use POSIX;
use File::Basename;    # for basename and dirname

my $USER     = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};

require $HOME_DIR . "/infracore/GenPerlLib/get_all_machines_vec.pl";

# my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $HOSTNAMEIP = `hostname -i`;
chomp($HOSTNAMEIP);

sub SyncToAllMachines {
	my @all_machines_ = GetAllMachinesVec();

	my $input_filename_ = shift;
	my $dirname_ = dirname($input_filename_);

	my $debug = 0;
	foreach my $remote_machine_ (@all_machines_) {
		if ( $remote_machine_ ne $HOSTNAMEIP ) {
			if ( $debug == 1 ) {
				print "ssh $remote_machine_ mkdir -p $dirname_\n";
				print "scp $input_filename_ $USER\@$remote_machine_:$input_filename_\n";
			}
			 my $enter_time = time();
			`ssh -o ConnectTimeout=30 $remote_machine_ mkdir -p $dirname_`;
			`scp -o ConnectTimeout=30 -p $input_filename_ $USER\@$remote_machine_:$input_filename_`;
 			 my $exit_time = time();
			 my $diff = $exit_time - $enter_time;
                         print "Time to sync file to machine: $remote_machine_ : $diff \n";
			 print "Exit code: $? \n";
		}
	}
}

1;
