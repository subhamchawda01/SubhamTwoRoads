# \file GenPerlLib/get_file_vec_excluding_comments.pl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetFileVecExcludingComments {
    my $t_file_name_ = shift;
    my @lines_ = ();

    open STRATFILENAME, "< $t_file_name_ " or PrintStacktraceAndDie ( "Could not open $t_file_name_\n" );
    while ( my $thisline_ = <STRATFILENAME> ) 
    {
	my $this_uncommented_line_ = "";
	my $add_space_ = 0;
	my @t_words_ = split ( ' ', $thisline_ );
	for ( my $word_idx_ = 0 ; $word_idx_ <= $#t_words_ ; $word_idx_ ++ )
	{
	    if ( substr ( $t_words_[ $word_idx_ ], 0, 1) ne '#' )
	    {
		if ( $add_space_ == 1 )
		{
		    $this_uncommented_line_ = $this_uncommented_line_." ";
		}
		$this_uncommented_line_ = $this_uncommented_line_.$t_words_[ $word_idx_ ] ;
		$add_space_ = 1;
	    }
	    else
	    {
		last;
	    }
	}
	if ( $this_uncommented_line_ ) { push ( @lines_, $this_uncommented_line_ ); }
    }
    close STRATFILENAME ;
    @lines_;
}

1
