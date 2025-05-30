# \file GenPerlLib/sync_to_all_machines.pl
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

my $HOME_DIR = $ENV{'HOME'};

sub GetDevMachinesVec {
	my @dev_machines_ = ();

	my $servers_filename = '/spare/local/tradeinfo/all_servers.cfg';

	open( my $fh, '<:encoding(UTF-8)', $servers_filename )
	  or die "Could not open file '$servers_filename' $!";

	my $read = 0;

	while ( my $row = <$fh> ) {
		chomp $row;

		if ( $read == 1 && substr( $row, 0, 1 ) eq "#" ) {
			last;
		}

		if ( $read == 1 ) {
			push( @dev_machines_, $row );
		}

		if ( $row eq "#ny4" ) {
			$read = 1;
		}
	}
	return @dev_machines_;

}

sub GetTradeMachinesVec {
	my @trade_machines_ = ();

	my $servers_filename = '/spare/local/tradeinfo/all_servers.cfg';

	open( my $fh, '<:encoding(UTF-8)', $servers_filename )
	  or die "Could not open file '$servers_filename' $!";

	my $read = 0;

	while ( my $row = <$fh> ) {
		chomp $row;

		if ( $read == 0 && substr( $row, 0, 1 ) eq "#" ) {
			$read = 1;
		}

		if ( $row eq "#ny4" ) {
			$read = 0;
		}

		if ( $read == 1 && !( substr( $row, 0, 1 ) eq "#" ) ) {
			push( @trade_machines_, $row );
		}
	}
	return @trade_machines_;
}

sub GetAllMachinesVec {
	my @all_machines_ = ();
	push( @all_machines_, GetDevMachinesVec() );
	push( @all_machines_, GetTradeMachinesVec() );
	return @all_machines_;
}

1;
