# \file GenPerlLib/get_shortcode_from_stratfile.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;

sub GetShortcodeFromStratFile
{

    my ( $strategy_filename_ ) = @_;

    my $retval_ = ""; # return value
    if ( -e $strategy_filename_ )
    {
	open STRAT_FILEHANDLE, "< $strategy_filename_";
	while ( my $strat_line_ = <STRAT_FILEHANDLE> )
	{
	    chomp ( $strat_line_ );
	    my @rwords_ = split ' ', $strat_line_;
	    if ( $#rwords_ >= 1 )
	    {
		if ( $rwords_[0] eq "STRATEGYLINE" )
		{
		    $retval_ = $rwords_[1];
		}
	    }
	}
	close STRAT_FILEHANDLE;
    }
    $retval_ ;
}

1
