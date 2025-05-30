# \file GenPerlLib/get_bad_periods_for_shortcode_for_date.pl
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

sub GetBadPeriodsForShortcodeForDate
{
    my ( $shortcode_ , $yyyymmdd_ ) = @_;

    my @bad_periods_start_ = ( );
    my @bad_periods_end_ = ( );

    my $bad_periods_file_ = $STORE."/datageninfo/bad_periods.".$shortcode_;

    if ( -e $bad_periods_file_ )
    {
	open ( BAD_PERIOD_FILE , "< $bad_periods_file_ " ) or PrintStacktraceAndDie ( "$0 Could not open $bad_periods_file_\n" );

	while ( my $t_line_ = <BAD_PERIOD_FILE> )
	{
	    chomp ( $t_line_ );
	    my @line_words_ = split ( ' ' , $t_line_ );

	    if ( $#line_words_ >= 1 )
	    {
		if ( $line_words_ [ 0 ] == $yyyymmdd_ )
		{ # 20120312 256000 280000 290000 300000
		    for ( my $i = 1 ; $i <= $#line_words_ ; $i += 2 )
		    {
			my $t_start_mfm_ = $line_words_ [ $i ]; chomp ( $t_start_mfm_ );
			my $t_end_mfm_ = $line_words_ [ $i + 1 ]; chomp ( $t_end_mfm_ );

			push ( @bad_periods_start_ , $t_start_mfm_ );
			push ( @bad_periods_end_ , $t_end_mfm_ );
		    }

		    last;
		}
	    }
	}
    }

    return ( \@bad_periods_start_ , \@bad_periods_end_ );
}

1
