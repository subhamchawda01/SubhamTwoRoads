# \file GenPerlLib/get_kth_word_sim.pl
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
