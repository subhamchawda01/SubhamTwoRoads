# \file GenPerlLib/get_shortcode_from_ilist.pl
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

sub GetShortcodeFromIList
{

    my ( $indicator_list_filename_ ) = @_;

    my $retval_ = ""; # return value
    if ( -e $indicator_list_filename_ )
    {
	open ILIST_FILEHANDLE, "< $indicator_list_filename_";
	while ( my $ilist_line_ = <ILIST_FILEHANDLE> )
	{
	    chomp ( $ilist_line_ );
	    my @rwords_ = split ' ', $ilist_line_;
	    if ( $#rwords_ >= 2 )
	    {
		if ( ( $rwords_[0] eq "MODELINIT" ) && ( $rwords_[1] eq "DEPBASE" ) )
		{
		    $retval_ = $rwords_[2];
		}
	    }
	}
	close ILIST_FILEHANDLE;
    }
    $retval_ ;
}

1
