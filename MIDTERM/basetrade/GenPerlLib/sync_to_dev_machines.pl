# \file GenPerlLib/sync_to_dev_machines.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_all_machines_vec.pl"; # GetAllMachinesVec

my $HOSTNAMEIP=`hostname -i`; chomp($HOSTNAMEIP);

sub SyncToDevMachines {
    my @dev_machines_ = GetDevMachinesVec();

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

sub SyncDirToDevMachines {
  my @dev_machines_ = GetDevMachinesVec();

  my $input_dirname_ = shift;

# If there is a trailing '/', remove it. rsync behaves differently in that case
  if ( $input_dirname_ =~ /\/$/ )
  {
    $input_dirname_ = substr ( $input_dirname_, 0, length ( $input_dirname_ ) - 1 );
  }


  my $base_dirname_ = dirname($input_dirname_);

  my $debug = 0;

  foreach my $remote_machine_ ( @dev_machines_ )
  {
    if ( $remote_machine_ ne $HOSTNAMEIP )
    {
      if ( $debug == 1 )
      {
        print "ssh $remote_machine_ mkdir -p $input_dirname_"."\n";
        print "rsync -ravz $input_dirname_ $USER\@$remote_machine_:$base_dirname_"."\n";
      }

      `ssh $remote_machine_ mkdir -p $input_dirname_`;
      `rsync -ravz $input_dirname_ $USER\@$remote_machine_:$base_dirname_`;
    }
  }
}
1;
