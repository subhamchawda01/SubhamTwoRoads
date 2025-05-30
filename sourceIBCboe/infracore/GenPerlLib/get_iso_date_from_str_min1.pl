# \file GenPerlLib/get_iso_date_from_str_min1.pl
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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

sub GetIsoDateFromStrMin1
{
    my @this_words_ = @_;
    my $retval = 0;
    if ( $this_words_[0] =~ /TODAY/ )
    { # expecting something like "TODAY -30" or "TODAY-30"
	my $num_prev_ = 0;
	my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );
	if ( $#this_words_ >= 1 )
	{
	    $num_prev_ = -1 * int($this_words_[1]) ;
	    $retval = CalcPrevWorkingDateMult ( $today_yyyymmdd_, $num_prev_ ) ;
	}
	else
	{ # one word ... could be TODAY-1
	    $num_prev_ = 1;
	    my $substr = 'TODAY';
	    my $result = index($this_words_[0], $substr) + 5;
	    my $newstr = substr ( $this_words_[0], $result );
	    if ( length ( $newstr ) > 0 )
	    {
		$num_prev_ = -1 * int($newstr) ;
		if ( ( $num_prev_ > 1000 ) ||
		     ( $num_prev_ < 0 ) )
		{
		    $num_prev_ = 1;
		}
	    }
	    $retval = CalcPrevWorkingDateMult ( $today_yyyymmdd_, $num_prev_ ) ;
	}
    }
    else
    {
	$retval = $this_words_[0];
    }
    $retval;
}

1;
