# \file GenPerlLib/exists_and_same.pl
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
