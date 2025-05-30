# \file GenPerlLib/get_num_from_numsecname.pl
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

my $HOME_DIR=$ENV{'HOME'}; 

sub GetNumFromNumSecname
{
    my ( $this_numbered_secname_ ) = @_;
    my $this_number_ = 0;
    my @swords_ = split ( '\.', $this_numbered_secname_ );
    if ( $#swords_ >= 1 )
    { # expect string like A.B
	$this_number_ = $swords_[$#swords_];
    }
    $this_number_;
}

sub GetSecnameFromNumSecname
{
    my ( $this_numbered_secname_ ) = @_;
    my $this_secname_ = 0;
    my @swords_ = split ( '\.', $this_numbered_secname_ );
    if ( $#swords_ >= 1 )
    { # expect string like A.B
	$this_secname_ = $swords_[0];
    }
    $this_secname_;
}

1;
