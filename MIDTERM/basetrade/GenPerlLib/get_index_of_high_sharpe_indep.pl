# \file GenPerlLib/get_index_of_high_sharpe_indep.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;

sub GetIndexOfHighSharpeIndep 
{
    my ( $indicator_list_filename_, $avoid_high_sharpe_indep_check_index_filename_ ) = @_;

    my @high_sharpe_indep_index_ = ();
    
    my $ilist_line_no = -1 ;
    if ( -e $indicator_list_filename_ )
    {
	open ILIST_FILEHANDLE, "< $indicator_list_filename_";
	while ( my $ilist_line_ = <ILIST_FILEHANDLE> )
	{
	    chomp ( $ilist_line_ );
	    my @rwords_ = split ' ', $ilist_line_;
	    if ( $#rwords_ >= 0 )
	    {
		if ( $rwords_[0] eq "INDICATOR" )
		{
		    $ilist_line_no = $ilist_line_no + 1 ;
		    if ( ( $rwords_[2] eq "ImpliedPrice" ) || ( $rwords_[2] eq "ImpliedPriceAverage" ) || ( $rwords_[2] eq "ImpliedPriceMPS" ) )
		    {
		    	push ( @high_sharpe_indep_index_, $ilist_line_no );
		    }
		}
	    }
	}
	close ILIST_FILEHANDLE;
    }
    # dump this array to a file
    open HSINDEX_FILEHANDLE, "> $avoid_high_sharpe_indep_check_index_filename_";
    print HSINDEX_FILEHANDLE @high_sharpe_indep_index_ ;
    close HSINDEX_FILEHANDLE ;
    @high_sharpe_indep_index_ ;
}

1
