# \file GenPerlLib/exists_and_same.pl
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

sub ExistsAndSame {
    my $t_a_filename_ = shift;
    my $t_b_filename_ = shift;

    my $res_str_ = `diff -Bwq $t_a_filename_ $t_b_filename_ | wc -l`; chomp ( $res_str_ );
    my $retval_ = 0;
    if ( int($res_str_) == 0 )
    {
	$retval_ = 1;
    }
    $retval_;
}

1;
