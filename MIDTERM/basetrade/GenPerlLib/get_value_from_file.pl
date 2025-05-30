#!/usr/bin/perl

# \file GenPerlLib/get_value_from_file.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use FileHandle;

sub GetValueFromFile
{
    my $filename_ = shift;
    my $search_string_ = shift;

    my $return_string_ = -1;

    if ( -e $filename_ )
    {
	open INPUT_FILEHANDLE, "< $filename_ " ;
	
	while ( my $thisline_ = <INPUT_FILEHANDLE> ) 
	{
	    chomp ( $thisline_ ); # remove newline character at end
    
	    my @this_words_ = split ( ' ', $thisline_ );
	    if ( $#this_words_ >= 1 ) 
	    { # empty line ... ignore
		my $key_ = $this_words_[0];
		my $value_ = $this_words_[1];
		if ( $key_ eq $search_string_ )
		{ 
		    $return_string_ = $value_; 
		    last;
		}
	    }
	}
	close ( INPUT_FILEHANDLE );
    }

    $return_string_ ;
}

1;
