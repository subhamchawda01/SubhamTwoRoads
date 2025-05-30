# \file GenPerlLib/clear_temporary_files.pl
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

sub ClearTemporaryFiles {
    my @temporary_files_ = @_ ;
    foreach my $t_file_name_ (@temporary_files_)
    {
	`rm -f $t_file_name_`;
    }
}

1;
