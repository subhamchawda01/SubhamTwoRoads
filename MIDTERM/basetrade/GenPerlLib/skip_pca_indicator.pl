# \file GenPerlLib/skip_pca_indicator.pl
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

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

sub SkipPCAIndicator
{
    my ($dep_shortcode_, $combo_shortcode_, $indicator_filename_) = @_;
    my $retval_ = 0;
    if ( $indicator_filename_ =~ m/PCA/ )
    {
        $retval_ = 1; # by default skip
	my $PCA_BASKETS_DEFAULT_FILE_ = "/spare/local/tradeinfo/PCAInfo/portfolio_inputs_DEFAULT";
	if ( -e $PCA_BASKETS_DEFAULT_FILE_ )
	{
	    my $is_open_ = 1;
	    open PBD_FILEHANDLE, "< $PCA_BASKETS_DEFAULT_FILE_ " ;
	    while ( my $thisline_ = <PBD_FILEHANDLE> ) 
	    {
		chomp ( $thisline_ );
		my @this_words_ = split ( ' ', $thisline_ );
		if ( $#this_words_ >= 1 )
		{
		    if ( ( $this_words_[0] eq "PLINE" ) &&
			 ( $this_words_[1] eq $combo_shortcode_ ) )
		    {
			for ( my $i = 2 ; $i <= $#this_words_ ; $i ++ )
			{
			    if ( $this_words_ [ $i ] eq $dep_shortcode_ )
			    {
				$retval_ = 0;
				next;
			    }
			}
		    }
		}
	    }
	    close PBD_FILEHANDLE ;
	}
    }
    $retval_;
}

sub SkipVolPCAIndicator
{
    my ($dep_shortcode_, $combo_shortcode_, $indicator_filename_) = @_;
    my $retval_ = 0;
    if ( $indicator_filename_ =~ m/VolumePCA/ )
    {
        my $curr_date_ = GetIsoDateFromStrMin1 ( "TODAY-5" );
        my $PCA_BASKETS_DEFAULT_FILE_ = "/spare/local/tradeinfo/PCAInfo/portfolio_inputs_DEFAULT";
        if ( -e $PCA_BASKETS_DEFAULT_FILE_ )
        {
            my $is_open_ = 1;
            open PBD_FILEHANDLE, "< $PCA_BASKETS_DEFAULT_FILE_ " ;
            while ( my $thisline_ = <PBD_FILEHANDLE> )
            {
                chomp ( $thisline_ );
                my @this_words_ = split ( ' ', $thisline_ );
                if ( $#this_words_ >= 1 )
                {
                    if ( ( $this_words_[0] eq "PLINE" ) &&
                         ( $this_words_[1] eq $combo_shortcode_ ) )
                    {
                        for ( my $i = 2 ; $i <= $#this_words_ ; $i ++ )
                        {
                            my $cmd_ = "$BINDIR/get_dv01_for_shortcode $this_words_[$i] $curr_date_";
                            my $dv01_ = `$cmd_`; chomp($dv01_) ;
                            if ( $dv01_ < 1 )
                            {
                                $retval_ = 1;
                                last;
                            }
                        }
                    }
                }
            }
            close PBD_FILEHANDLE ;
        }
    }
    $retval_;
}


1;
