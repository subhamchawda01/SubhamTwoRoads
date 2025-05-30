# \file GenPerlLib/get_strat_start_end_hhmm.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# This script takes inputs :
# STRATFILE
# and returns the start and end hhmm

use strict;
use warnings;

sub GetStratStartEndHHMM
{
    my $start_hhmm_ = "EST_800";
    my $end_hhmm_ = "EST_1600";
    my ($stratfilename_) = @_;

    if ( -e $stratfilename_ )
    {

	open STRATFILEHANDLE, "< $stratfilename_ " or die "$0 Could not open $stratfilename_\n" ;
	my @slines_ = <STRATFILEHANDLE>;
	close STRATFILEHANDLE;

	foreach my $t_sline_ ( @slines_ )
	{
	    chomp ( $t_sline_ );
	    my @swords_ = split ( ' ', $t_sline_ );
	    if ( $#swords_ >= 6 )
	    {
		$start_hhmm_ = $swords_[5];
		$end_hhmm_ = $swords_[6];
		last;
	    }
	}
    }
    $start_hhmm_, $end_hhmm_;
}

1
