# \file GenPerlLib/create_directory.pl
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
use FileHandle; 
use File::Basename; # for basename and dirname

sub CreateEnclosingDirectory
{
    my $file_name_ = shift;
    my $filedir_name_ = dirname ( $file_name_ );
#    print STDOUT "mkdir -p $filedir_name_\n";
    `mkdir -p $filedir_name_`;
}

1;
