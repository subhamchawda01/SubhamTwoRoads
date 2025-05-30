# \file GenPerlLib/get_market_model.pl
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

sub GetMarketModel{
    my $t_full_strategy_name_ = shift;

    my $t_market_model_ = 0;
    my $t_use_accurate_ = 1;

    open STRATFILENAME, "< $t_full_strategy_name_ " or PrintStacktraceAndDie("Count not open $t_full_strategy_name_\n");
    while(my $thisline_ = <STRATFILENAME>) {
	my @t_words_ = split ( ' ', $thisline_ );
	if ( $#t_words_ >= 9 ) { 
	    if ( $t_words_[1] eq "OptionsTrading" ) {
		$t_market_model_ = $t_words_[7];
		$t_use_accurate_ = $t_words_[8];
	    }
	}
    }
    close STRATFILENAME;
    $t_market_model_, $t_use_accurate_;
}

1;
