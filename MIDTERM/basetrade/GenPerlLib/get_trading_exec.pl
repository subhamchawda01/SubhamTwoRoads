# \file GenPerlLib/get_trading_exec.pl
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

sub GetTradingExec {
    my $t_full_strategy_name_ = shift;
    
    my $t_shc_ = "" ;
    my $t_full_exec_name_ = "";

    open STRATFILENAME, "< $t_full_strategy_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_strategy_name_\n" );

FOO: {
    while ( my $thisline_ = <STRATFILENAME> ) 
    {
	my @t_words_ = split ( ' ', $thisline_ );
	if ( $#t_words_ >= 4 )
	{ 
	    if ( $t_words_[1] eq "OptionsTrading" ) {
		$t_full_exec_name_ = $t_words_[1];
		my $t_full_model_name_ = $t_words_[2];
		open MODELFILENAME, "< $t_full_model_name_ " or PrintStacktraceAndDie ( "Could not open $t_full_model_name_\n" );
		my $found_ = 0;
		while ( my $thismline_ = <MODELFILENAME> ) {
		    {
			chomp($thismline_);
			if ( $thismline_ eq "PARAMSHCPAIR" ) {
			    $found_ = 1;
			    next;
			} elsif ( $found_ == 1 ) {
			    my @m_words_ = split ( ' ', $thismline_ );
			    $t_shc_ = $m_words_[0];
			    last FOO;
			} else {
 			    next;
			}
		    }
		}
	    } else {
		$t_shc_ = $t_words_[1];
		$t_full_exec_name_ = $t_words_[2];
	    }
	    last;
	}
    }
    }
    close STRATFILENAME ;
    $t_shc_ , $t_full_exec_name_;
}

1;
