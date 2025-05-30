# \file GenPerlLib/create_local_strat_file.pl
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

use strict ;
use warnings ;
use File::Basename ;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub CreateLocalStratFile
  {
    my ( $real_query_trades_dir_, $sim_strat_file_name_, $t_temp_progid_, $strategy_info_words_ref_ ) = @_ ;

    open OUTSTRATFILEHANDLE, "> $sim_strat_file_name_ " or PrintStacktraceAndDie ( "$0 Could not open $sim_strat_file_name_ for writing\n" );
    my @swords_ = @$strategy_info_words_ref_ ;

    if ( $#swords_ >= 6 )
      { # switching model and param locations from Livemodels to QueryLogs
	$swords_[3] = $real_query_trades_dir_."/".basename ( $swords_[3] ) ;
	$swords_[4] = $real_query_trades_dir_."/".basename ( $swords_[4] ) ;
	$swords_[7] = $t_temp_progid_ ; 
	print OUTSTRATFILEHANDLE join ( " ", @swords_ )."\n" ;
      }
    close ( OUTSTRATFILEHANDLE );
  }


1
