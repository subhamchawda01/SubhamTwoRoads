# \file GenPerlLib/clear_temporary_files.pl
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

sub ClearTemporaryFiles {
    my @temporary_files_ = @_ ;
    foreach my $t_file_name_ (@temporary_files_)
    {
	`rm -f $t_file_name_`;
    }
}

1;
