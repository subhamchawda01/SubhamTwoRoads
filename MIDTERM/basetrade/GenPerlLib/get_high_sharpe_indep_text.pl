# \file GenPerlLib/get_high_sharpe_indep_text.pl
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

sub GetHighSharpeIndepText
{

    my ( $indicator_list_filename_, @high_sharpe_indep_indices_ ) = @_;

    my @high_sharpe_indep_text_ = (); # return value
    my @all_indicator_lines_ = ();
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
		    push ( @all_indicator_lines_, $ilist_line_ );
		}
	    }
	}
	close ILIST_FILEHANDLE;
    }
    for ( my $hsiv_idx_ = 0 ; $hsiv_idx_ <= $#high_sharpe_indep_indices_ ; $hsiv_idx_ ++ ) 
    {
	push ( @high_sharpe_indep_text_, $all_indicator_lines_ [ $high_sharpe_indep_indices_ [ $hsiv_idx_ ] ] );
    }
    @high_sharpe_indep_text_ ;
}

1
