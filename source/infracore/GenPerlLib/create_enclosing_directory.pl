# \file GenPerlLib/create_directory.pl
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
