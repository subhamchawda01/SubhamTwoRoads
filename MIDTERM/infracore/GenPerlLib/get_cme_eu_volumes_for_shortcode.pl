#
#   file GenPerlLib/get_cme_eu_volumes_for_shortcode.pl 
#
#   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#   Address:
#   Suite No 162, Evoma, #14, Bhattarhalli,
#   Old Madras Road, Near Garden City College,
#   KR Puram, Bangalore 560049, India
#   +91 80 4190 3551

use strict;
use warnings;
use feature "switch";
use FileHandle;
use File::Basename;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

sub GetCMEEUVolumes
{

    my $shortcode_ = shift;
    my $trade_date_ = shift || 20120101 ;

    my $volumes_ = 0;

    if( $trade_date_ == 20120101 ){

      $trade_date_ = `date +%Y%m%d`; chomp ( $trade_date_ );

    }

    my $cme_eu_volumes_info_file_ = $HOME_DIR."/trades/cme_eu_vol_info_file_".$trade_date_ ;  
    
# if $cme_eu_volumes_info_file_ doesn't exists..
    if ( ! -f $cme_eu_volumes_info_file_ ) {
      return 0;
    }  

    my $zn_eu_vol_ = 0;
    my $zf_eu_vol_ = 0;
    my $zb_eu_vol_ = 0;
    my $ub_eu_vol_ = 0;

    open CME_EU_VOL_FILE_HANDLE, "< $cme_eu_volumes_info_file_" ;
    my @cme_vol_lines_ = <CME_EU_VOL_FILE_HANDLE> ;
    close CME_EU_VOL_FILE_HANDLE ;

    for ( my $itr = 0 ; $itr <= $#cme_vol_lines_; $itr ++ )
    {
	my @cme_vol_words_ = split ( ' ', $cme_vol_lines_[$itr] );
	if ( $#cme_vol_words_ >= 1 )
	{
	    my $currency_ = $cme_vol_words_[0] ;
	    if ( index ( $currency_, "ZN" ) == 0 ){
		$zn_eu_vol_ = $cme_vol_words_[1] ;
	    }

	    if( index ( $currency_, "ZF" ) == 0 ){
		$zf_eu_vol_ = $cme_vol_words_[1] ;
	    }

	    if( index ( $currency_, "UB" ) == 0 ){
		$ub_eu_vol_ = $cme_vol_words_[1] ;
	    }

	    if( index ( $currency_, "ZB" ) == 0 ){
		$zb_eu_vol_ = $cme_vol_words_[1] ;
	    }

	}
    }

    given ( $shortcode_ )
    {
        when ( "ZN" )
        {
            $volumes_ = $zn_eu_vol_ ;
        }
        when ( "ZF" )
        {
            $volumes_ = $zf_eu_vol_ ;
        }
        when ( "ZB" )
        {
            $volumes_ = $zb_eu_vol_ ;
        }
        when ( "UB" )
        {
            $volumes_ = $ub_eu_vol_ ;
        }
	default
	{
            $volumes_ = 0;
	}
    }

    $volumes_ ;
}

1
