# \file GenPerlLib/is_server_out_of_disk_space.pl
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
use feature "switch"; # for given, when

my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";

sub IsServerOutOfDiskSpace
{
    #change to not look for disk space on ec2 servers. Typically /spare is mounted on /media/ephemeral1/ 
    #But since this is not generic, we will avoid having this check for now
    my $hostname_ = `hostname`;
    if ( substr($hostname_,0,3) ne "sdv" ) {
       return 0;
    }
    my $is_out_of_space_ = 0;

    my $home_usage_ = `df | grep -v "Use" | awk '{ if ( \$6 == "/home" ) print \$5; }' | awk -F% '{ print \$1; }'`;

    chomp ( $home_usage_ );

    if ( $home_usage_ > 94 )
    {
	$is_out_of_space_ = 1;
    }

    return $is_out_of_space_;
}

1
