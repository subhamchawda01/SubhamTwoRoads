# \file GenPerlLib/sync_to_all_machines.pl
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

sub SyncToAllMachines {
  my @all_machines_ = GetAllMachinesVec();
  
  my $input_filename_ = shift;
  my $dirname_ = dirname($input_filename_);

  my $debug = 0;
                      
  foreach my $remote_machine_ ( @all_machines_ )
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
