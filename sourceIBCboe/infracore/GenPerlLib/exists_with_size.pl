# \file GenPerlLib/exists_with_size.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

my $HOME_DIR=$ENV{'HOME'}; 
#my $REPO="infracore";

#my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";

sub ExistsWithSize {
    my $input_filename_ = shift;
    if ( ( -e $input_filename_ ) &&
	 ( -s $input_filename_ > 0 ) )
    {
	1;
    }
    else
    {
	0;
    }
}

1;
