# \file GenPerlLib/get_model_and_param_file_names.pl
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

sub GetModelAndParamFileNames {
    my $t_full_strategy_name_ = shift;
    
    my $t_full_model_name_ = "";
    my $t_full_param_name_ = "";

    open STRATFILENAME, "< $t_full_strategy_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_strategy_name_\n" );

    while ( my $thisline_ = <STRATFILENAME> ) 
    {
	my @t_words_ = split ( ' ', $thisline_ );
	if ( $#t_words_ >= 4 )
	{ 
	    if ( $t_words_[1] eq "OptionsTrading" ) { 
		$t_full_model_name_ = $t_words_[2];
		open MODELFILENAME, "< $t_full_model_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_model_name_\n" );
		my $found_ = 0;
		while ( my $thismline_ = <MODELFILENAME> ) {
		    {
			if ( $thismline_ eq "PARAMSHCPAIR" ) {
			    $found_ = 1;
			    next;
			} elsif ( $found_ == 1 ) {
			    my @m_words_ = split ( ' ', $thismline_ );
			    $t_full_param_name_ = $m_words_[1];
			    last;
			} else {
 			    next;
			}
		    }
		}
	    } else {
		$t_full_model_name_ = $t_words_[3];
		$t_full_param_name_ = $t_words_[4];
	    }
	    last;
	}
    }
    close STRATFILENAME ;
    $t_full_model_name_, $t_full_param_name_;
}

1;
