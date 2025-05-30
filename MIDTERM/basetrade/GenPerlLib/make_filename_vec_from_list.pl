# \file GenPerlLib/make_filename_vec_from_list.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 353, Evoma, #14, Bhattarhalli,
#        Old Madras Road, Near Garden City College,
#        KR Puram, Bangalore 560049, India
#        +91 80 4190 3551
#

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub MakeFilenameVecFromList
{
    my $filename_list_filename_ = shift;
    my @files_ = ();
    if ( -f $filename_list_filename_ )
    {
	open FILENAME_LIST_FILEHANDLE, "< $filename_list_filename_ " or PrintStacktraceAndDie ( "Could not open $filename_list_filename_\n" );
	while ( my $thisline_ = <FILENAME_LIST_FILEHANDLE> ) 
	{
	    chomp ( $thisline_ );
	    my @t_words_ = split ( ' ', $thisline_ );
	    if ( ( $#t_words_ >= 0 ) &&
		 ( ! ( $t_words_[0] =~ /#/ ) ) )
	    { 
		push ( @files_, $t_words_[0] );
	    }
	}
	
	close FILENAME_LIST_FILEHANDLE;
    }
    @files_ ;
}

1
