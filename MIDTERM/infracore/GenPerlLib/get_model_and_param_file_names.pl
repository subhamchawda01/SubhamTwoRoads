# \file GenPerlLib/get_model_and_param_file_names.pl
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

sub GetModelAndParamFileNames {
    my $t_full_strategy_name_ = shift;
    
    my $t_full_model_name_ = "";
    my $t_full_param_name_ = "";

    open STRATFILENAME, "< $t_full_strategy_name_ " or die "Could not open $t_full_strategy_name_\n" ;

    while ( my $thisline_ = <STRATFILENAME> ) 
    {
	my @t_words_ = split ( ' ', $thisline_ );
	if ( $#t_words_ >= 4 )
	{ 
	    $t_full_model_name_ = $t_words_[3];
	    $t_full_param_name_ = $t_words_[4];
	    last;
	}
    }
    close STRATFILENAME ;
    $t_full_model_name_, $t_full_param_name_;
}

1;
