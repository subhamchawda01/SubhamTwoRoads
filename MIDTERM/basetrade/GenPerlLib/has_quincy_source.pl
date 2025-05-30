# \file GenPerlLib/has_liffe_source.pl
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
use feature "switch";
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_port_constituents.pl"; # GetPortConstituents

sub HasQUINCYSource
{
    my $shortcode_ = shift;
    my $retval = 0;

    given ( $shortcode_ )
    {
	when ( "6A_0" ) { return 1; }
	when ( "6B_0" ) { return 1; }
	when ( "6C_0" ) { return 1; }
	when ( "6E_0" ) { return 1; }
	when ( "6J_0" ) { return 1; }
	when ( "6M_0" ) { return 1; }
	when ( "6N_0" ) { return 1; }
	when ( "6S_0" ) { return 1; }
	when ( "ES_0" ) { return 1; }
	when ( "NQ_0" ) { return 1; }
	when ( "YM_0" ) { return 1; }
    }
    my @port_constituents = GetPortConstituents($shortcode_);
    foreach my $port_constituent ( @port_constituents)
    {
        if(HasQUINCYSource($port_constituent) == 1)
        {
          return 1;
        }
    }
    return 0;
}

1
