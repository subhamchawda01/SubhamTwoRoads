# \file GenPerlLib/exists_with_size.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

my $HOME_DIR=$ENV{'HOME'}; 
#my $REPO="basetrade";

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
