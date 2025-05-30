# \file GenPerlLib/get_avg_price_moves_per_sec_for_shortcode.pl
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
use feature "switch";

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $STORE="/spare/local/tradeinfo";

my $USER=$ENV{'USER'};
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetAvgPriceMovesPerSecForShortcode
{
    my $shortcode_ = shift;

    my $avg_price_moves_per_sec_ = 1000.0;

    my $avg_price_moves_file_ = $STORE."/datageninfo/avg_price_moves.txt";

    if ( -e $avg_price_moves_file_ )
    { # If file available , use those values.
	open ( AVG_PRICE_FILE , "< $avg_price_moves_file_ " ) or PrintStacktraceAndDie ( "$0 Could not open $avg_price_moves_file_\n" );

	while ( my $t_line_ = <AVG_PRICE_FILE> )
	{
	    chomp ( $t_line_ );
	    if ( index ( $t_line_ , "#" ) >= 0 ) { next; }
	    my @line_words_ = split ( ' ' , $t_line_ );

	    if ( $#line_words_ >= 1 )
	    {
		my $t_shortcode_ = $line_words_ [ 0 ]; chomp ( $t_shortcode_ );
		if ( $t_shortcode_ eq $shortcode_ )
		{
		    $avg_price_moves_per_sec_ = $line_words_ [ 1 ]; chomp ( $avg_price_moves_per_sec_ );
		    last;
		}
	    }
	}

	close ( AVG_PRICE_FILE );

	return $avg_price_moves_per_sec_;
    }

    # Default values.
    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.00299921; # 45 days
	}
	when ( "ZN_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.00335718; # 45 days
	}
	when ( "ZB_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.00414949; # 45 days
	}
	when ( "ZT_0" )
	{
	    $avg_price_moves_per_sec_ = 0.30705; # 45 days
	}
	when ( "UB_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.00804130; # 45 days
	}
	when ( "CGB_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.01028735; # 45 days
	}
	when ( "BAX_0" )
	{
	    $avg_price_moves_per_sec_ = 0.00000116861; # 45 days
	}
	when ( "BAX_1" )
	{ 
	    $avg_price_moves_per_sec_ = 0.0000196073; # 45 days
	}
	when ( "BAX_2" )
	{ 
	    $avg_price_moves_per_sec_ = 0.0000196073; # 45 days
	}
	when ( "BAX_3" )
	{ 
	    $avg_price_moves_per_sec_ = 0.0000380615; # 45 days
	}
	when ( "BAX_4" )
	{ 
	    $avg_price_moves_per_sec_ = 0.0000655064; # 45 days
	}
	when ( "BAX_5" )
	{ 
	    $avg_price_moves_per_sec_ = 0.0000726793; # 45 days
	}
	when ( "FGBS_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.00071363; # 45 days
	}
	when ( "FGBM_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.00410296; # 45 days
	}
	when ( "FGBL_0" )
	{
	    $avg_price_moves_per_sec_ = 0.01705087; # 45 days
	}
	when ( "FESX_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.00471040; # 45 days
	}
	when ( "BR_DOL_0" )
	{
	    $avg_price_moves_per_sec_ = 0.00500338; # 45 days
	}
	when ( "BR_WDO_0" )
	{ 
	}
	when ( "BR_IND_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.183542 # 45 days
	}
	when ( "BR_WIN_0" )
	{ 
	    $avg_price_moves_per_sec_ = 0.16319 # 45 days
	}
	default
	{
	    $avg_price_moves_per_sec_ = 1000.0;
	}
    }

    return $avg_price_moves_per_sec_;
}

1
