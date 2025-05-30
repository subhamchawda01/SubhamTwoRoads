# \file GenPerlLib/list_file_to_vec.pl
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

sub ListFileToVec
{
    my $model_file_list_ = shift;
    my @model_filevec_ = ();

    if ( ! ExistsWithSize ( $model_file_list_ ) )
    { 
	print STDERR "$model_file_list_ no readable or no data\n";
    }
    else
    {
	open MODEL_LIST_FILEHANDLE, "< $model_file_list_ " or PrintStacktraceAndDie ( "Could not open $model_file_list_\n" );
	
	while ( my $thisline_ = <MODEL_LIST_FILEHANDLE> ) 
	{
	    my @t_words_ = split ( ' ', $thisline_ );
	    if ( ( $#t_words_ >= 0 ) &&
		 ( $t_words_[0] !~ /#/ ) )
	    { 
		push ( @model_filevec_, $t_words_[0] );
	    }
	}
	close MODEL_LIST_FILEHANDLE ;
    }
    @model_filevec_ ;
}

1
