# \file GenPerlLib/get_num_from_numsecname.pl
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

use feature "switch"; # for given, when

my $HOME_DIR=$ENV{'HOME'}; 

sub GetNumFromNumSecname
{
    my ( $this_numbered_secname_ ) = @_;
    my $this_number_ = 0;
    my @swords_ = split ( '\.', $this_numbered_secname_ );
    if ( $#swords_ >= 1 )
    {
	$this_number_ = $swords_[1];
    }
    $this_number_;
}

1;
