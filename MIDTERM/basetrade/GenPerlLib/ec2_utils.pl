# \file GenPerlLib/is_date_holiday.pl
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

sub IsEc2Machine
{
  my $hostname_ = `hostname` ; chomp($hostname_);
  if ( index ( $hostname_, "ip-10-0-") == 0 ) { return 1; }
  else { return 0; }
}

sub IsEc2Worker
{
  my $hostname_ = `hostname` ; chomp($hostname_);
  if ( index ( $hostname_, "ip-10-0-1-") == 0 ) { return 1; }
  else { return 0; }
}

sub IsEc2Controller
{
  my $hostname_ = `hostname` ; chomp($hostname_);
  if ( index ( $hostname_, "ip-10-0-0-") == 0 ) { return 1; }
  else { return 0; }
}

sub GetWorkerIPs
{
  my @workers_ips_ = `cat /mnt/sdf/JOBS/all_instances.txt | grep nil | awk '{print \$4}'`; chomp(@workers_ips_);
  return @workers_ips_;
}

1
