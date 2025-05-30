# \file GenPerlLib/is_model_corr_consistent.pl
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
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# Returns 1 if all indicator_weights are the same sign as their corresponding corrs.
# else returns 0.
sub IsModelCorrConsistent
{
    my $model_file_name_ = shift;

    my $is_consistent_ = 0;

    if ( ExistsWithSize ($model_file_name_) ) 
    {
	open MODEL_FILE, "< $model_file_name_" or return 0 ; # PrintStacktraceAndDie ( "Could not open $model_file_name_ for reading\n" );
	{
	    $is_consistent_ = 1;
	    my @model_file_lines_ = <MODEL_FILE>;
	    close MODEL_FILE;

	    for (my $model_line_ = 0; $model_line_ <= $#model_file_lines_; $model_line_++) 
	    {
		if ( index ($model_file_lines_[$model_line_], "INDICATOR ") >= 0 ) 
		{
		    # INDICATOR -0.00146369 ScaledTrend 6C_0 30.0 MktSizeWPrice # -0.07
		    my @indicator_words_ = split (' ', $model_file_lines_[$model_line_]);
		    my $t_weight_ = 0;
		    if ($#indicator_words_ >= 1) 
		    {
			$t_weight_ = $indicator_words_[1]; 
		    }
		    for ( my $w_idx = 2; $w_idx < $#indicator_words_ ; $w_idx ++ )
		    {
			if ( $indicator_words_[$w_idx] eq "#" )
			{
			    my $t_corr_ = $indicator_words_[$w_idx + 1]; 
			    if ( $t_weight_ * $t_corr_ < 0 )
			    {
				print "product < 0 ".$t_weight_." ".$t_corr_."\n";
				$is_consistent_ = 0;
				last;
			    }
			}
		    }
		}
	    }
	}
    } else {
	$is_consistent_ = 0;
#	print STDERR "File not exist/not readable\n";
    }
    
    $is_consistent_;
}

1
