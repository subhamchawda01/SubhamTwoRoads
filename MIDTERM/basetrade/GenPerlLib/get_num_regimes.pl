# \file GenPerlLib/get_num_regimes.pl
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

sub GetNumRegimesFromRegimeInd {
  my $regime_indicator_ = shift;
  my $num_tokens_ = shift;
  my $regime_string_ = shift;
  my $num_regimes_ = 5;
  given ( $regime_indicator_ )
  {
    when ( "TrendVolBasedRegime" )
    {
      $num_regimes_ = 5;
    }
    when ( "TrendVolBasedRegimePort" )
    {
      $num_regimes_ = 5;
    }
    when ( "TrendingMeanReverting" )
    {
      $num_regimes_ = 2;
    }
    when ( "SimpleTrendRegime" )
    {
      $num_regimes_ = 2;
    }
    when ( "TimeBasedRegimeBinary" )
    {
      $num_regimes_ = 2;
    }
    when ( "TimeBasedRegime" )
    {
      $num_regimes_ = $num_tokens_ - 1 ;
    }
    when ( "MovingCorrelationCutOff" )
    {
      $num_regimes_ = 2 ;
    }
    when ( "RegimeDiffPxModDiffPx" )
    { 
      $num_regimes_ = 2;
    }
    when ( "RollingL1SzRegime")
    {
      $num_regimes_ = 2;
    }
    when ( "L1BookbiasRegime")
    {
      $num_regimes_ = 3;
    }
    when ( "RegimeSlowStdev" )
    { 
      $num_regimes_ = 2;
    }
    when ( "RegimeSlowStdev2" )
    { 
      $num_regimes_ = 2;
    }

    when ( "MaxMovingCorrelation" )
    {
      $num_regimes_ = $num_tokens_ - 3;
    }
    when ( "OnlineOfflineCorrDiffBaseRegime" )
    {
      $num_regimes_ = 2;
    }
    when ( "SizeBasedRegime" )
    {
      $num_regimes_ = 2 ;
    }
    when ( "RegimeOnlineBeta")
    {
      $num_regimes_ = 2;
    }
    when ( "ConvexTrendRegime")
    { 
      $num_regimes_ = 2;
    }
    when ( "TrendSwitchRegime" )
    {
      $num_regimes_ = 3;
    }
    when ( "SrcSwitchRegPort" )
    {
      $num_regimes_ = 2;
    }
    when ( "RegimeOnlineOfflineStdevRatio" )
    {
      $num_regimes_ = 2;
    }
    when ( "RegimeOnlineOfflineBeta" )
    {
      $num_regimes_ = 2;
    }
    when ( "RegimeOnlineOfflineCorr" )
    {
      $num_regimes_ = 2;
    }
    when ( "RegimeSlowStdevTrend" )
    { 
      $num_regimes_ = 2;
    }
    when ( "EventBasedRegime" )
    {
      $num_regimes_ = 2;
    }
    when ( "CurveRegime" )
    {
      $num_regimes_ = 4;
    }
    when ( "VolumeRatioRegime" ) 
    {
      $num_regimes_ = 3;
    }
    when ( "ScaledVolumeRatioRegime" ) 
    {
      $num_regimes_ = 3;
    }

    when ( "RegimeSlope" )
    {
      $num_regimes_ = 2;
    }                                
    when ( "RegimeStatsDiff" )
    {
      $num_regimes_ = 2;
    }
    when ( "TrendBasedRegime") {
      $num_regimes_ = 3;
    }
    when("StdevRatioRegime") {
      $num_regimes_ = 3;
    }      
    when("SgMgTrRegime") {
      $num_regimes_ = 3;
    }
    when("RegimeVolStdev") {
      $num_regimes_ = 2;
    }
    when("PriceRegime"){
      $num_regimes_ = 3;
    }
    when("Expression") {
      my @regime_words_ = split ' ',$regime_string_;
      my ($index_) = grep { $regime_words_[$_] eq 'PARAMS' } (0 .. @regime_words_-1);
      if ( $index_ ) {
        $num_regimes_ = $regime_words_[$index_+1];
        if ( ($#regime_words_ - $index_ -1) != $num_regimes_ ) {
          print "Wrong regime indicator $regime_string_\n";
          exit ( 1 );
        }
      } else {
        $num_regimes_ = 5;
      }
    }
    when("RegimeMarketMaking"){
      $num_regimes_ = 2;
    }
    default
    {
      $num_regimes_ = 5;
    }
  }

  $num_regimes_;

}

1;
