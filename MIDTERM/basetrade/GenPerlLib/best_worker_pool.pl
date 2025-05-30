# \file GenPerlLib/best_worker_pool.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 354, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# Takes shortcode as argument and returns the best worker pool to run the process to
# TODO: improve the localization logic
use strict;
use warnings;

my @worker_pools_ = ("autoscalegroupmanual", "autoscalegroup", "autoscalegroup2");

#TODO: improve the allocation logic
sub GetBestWorkerPool
{
  my ( $base_shortcode_ ) = @_;
#my @chars = map substr( $base_shortcode_, $_, 1), 0 .. length($base_shortcode_) -1;
#return $worker_pools_[ord($chars[0]) % 2];
#Using only one autoscaling group for now, to avoid non-uniform load distribution
  return $worker_pools_[0];
}

1
