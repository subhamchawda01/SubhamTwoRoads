# \file GenPerlLib/create_specific_length_model_file.pl
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

sub CreateSpecificLengthModelFile
{

    my ( $input_indicator_list_filename_, $output_indicator_list_filename_, $this_indicator_count_ ) = @_;
    my $added_indicator_line_count_ = 0;
    if ( -e $input_indicator_list_filename_ )
    {
	open ILIST_FILEHANDLE, "< $input_indicator_list_filename_";
	open OILIST_FILEHANDLE, "> $output_indicator_list_filename_";
	while ( my $ilist_line_ = <ILIST_FILEHANDLE> )
	{
	    chomp ( $ilist_line_ );
	    my @rwords_ = split ' ', $ilist_line_;
	    if ( ( $#rwords_ >= 0 ) && ( $rwords_[0] eq "INDICATOR" ) )
	    {
		if ( $added_indicator_line_count_ < $this_indicator_count_ )
		{
		    print OILIST_FILEHANDLE $ilist_line_."\n";
		    $added_indicator_line_count_ ++;
		}
	    }
	    else
	    {
		print OILIST_FILEHANDLE $ilist_line_."\n";
	    }
	}
	close OILIST_FILEHANDLE;
	close ILIST_FILEHANDLE;
    }
}

1
