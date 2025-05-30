# \file GenPerlLib/get_kth_word_sim.pl
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

sub GetKthWord
{
    my ( $corr_matrix_dep_line_, $indep_col_index_ ) = @_;
    
    my @corr_matrix_words_ = split ( ' ', $corr_matrix_dep_line_ );

    my $corr_value_ = 0;
    if ( $#corr_matrix_words_ >= $indep_col_index_ )
    {
	$corr_value_ = $corr_matrix_words_[$indep_col_index_];
    }

    $corr_value_ ;
}

1;
