# \file GenPerlLib/get_strat_info.pl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetStratInfo {
    my $given_strategy_filename_ = shift;

  my $shortcode_ = "";
  my $strategyname_ = "";
  my $modelfilename_ = "";
  my $paramfilename_ = "";
  my $trading_start_hhmm_ = "";
  my $trading_end_hhmm_ = "";
  my $strategy_progid_ = 0;

  open STRATFILEHANDLE, "< $given_strategy_filename_ " or PrintStacktraceAndDie ( "Could not open $given_strategy_filename_\n" );
  while ( my $thisline_ = <STRATFILEHANDLE> ) 
    {
      my @this_words_ = split ( ' ', $thisline_ );
      if ( ( $#this_words_ >= 7 ) &&
	   ( $this_words_[0] =~ /STRATEGYLINE/ ) )
	{
	  $shortcode_ = $this_words_[1];
	  $strategyname_ = $this_words_[2];
	  $modelfilename_ = $this_words_[3];
	  $paramfilename_ = $this_words_[4];
	  $trading_start_hhmm_ = $this_words_[5];
	  $trading_end_hhmm_ = $this_words_[6];
	  $strategy_progid_ = $this_words_[7];
	  last;
	}
    }
  close ( STRATFILEHANDLE );
  $shortcode_, $strategyname_, $modelfilename_, $paramfilename_, $trading_start_hhmm_, $trading_end_hhmm_, $strategy_progid_;
}

1;
