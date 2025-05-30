# \file GenPerlLib/change_start_time_of_strategy_file.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# This script takes inputs :
# STRATFILE HHMM

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub ChangeStartTimeofStrategyFile
{
  my ( $stratfilename_, $start_hhmm_ ) = @_;

  if ( -e $stratfilename_ )
    {
      
      open STRATFILEHANDLE, "< $stratfilename_ " or PrintStacktraceAndDie ( "$0 Could not open $stratfilename_\n" );
      my @slines_ = <STRATFILEHANDLE>;
      chomp ( @slines_ );
      close STRATFILEHANDLE;
      
      open OUTSTRATFILEHANDLE, "> $stratfilename_ " or PrintStacktraceAndDie ( "$0 Could not open $stratfilename_ for writing\n" );
      foreach my $t_sline_ ( @slines_ )
	{
	  my @swords_ = split ( ' ', $t_sline_ );
	  if ( $#swords_ >= 6 )
	    {
	      $swords_[5] = $start_hhmm_;
	      print OUTSTRATFILEHANDLE join ( " ", @swords_ )."\n" ;
	    }
	  else
	    {
	      print OUTSTRATFILEHANDLE $t_sline_."\n" ;
	    }
	}
      close ( OUTSTRATFILEHANDLE );
    }
}

1
