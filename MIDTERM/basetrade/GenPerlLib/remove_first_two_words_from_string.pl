# \file GenPerlLib/remove_first_two_words_from_string.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;
use feature "switch";

sub RemoveFirstTwoWordsFromString 
{
    my $model_words_ref_ = shift;
    my @i_words_ = @$model_words_ref_ ;

    shift ( @i_words_ ); 
    shift ( @i_words_ );

    my $retval = join (' ',@i_words_ );

    $retval;
}

1
