#!/usr/bin/perl
#
# \file ModelScripts/find_best_params_permute.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
#
#




use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw(max); # for max
use FileHandle;

sub GetUTCHHMMfromEpoch;

package ResultLine;
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl";
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort

if ( $USER eq "diwakar" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";

my $TRADELOG_DIR="/spare/local/tradeinfo/volatilitylogs/";

my $GET_UTC_FROM_EPOCH_TIME=$SCRIPTS_DIR."/unixtime2gmtstr.pl";
my $GENERATE_PRICE_EXEC_=$BIN_DIR."/generate_price_data";
my $GENERATE_VOLUME_EXEC=$BIN_DIR."/get_periodic_volume_on_day";

my $USAGE = "$0 DATE[ TODAY ] TIMEDURATION [ 600 ]";
if ( $#ARGV < 0 )
{
  print $USAGE."\n";
  exit;
}
my $tradingdate_ = `date +%Y%m%d`; chomp ($tradingdate_);
my $generate_daily_file_ = "true";
if ( $ARGV[0] ne "TODAY" )
{
  $tradingdate_ = $ARGV[0];
  $generate_daily_file_ = "";
}
my $TIME_DURATION = "600";

if ( $#ARGV >= 1 )
{
  $TIME_DURATION = $ARGV[1];
  if ($#ARGV >= 2 )
  {
    $generate_daily_file_ = "true";
  }
}
my @shortcodes_ =  ( 'ZT_0','ZF_0','ZN_0','ZB_0','UB_0','6A_0','6B_0','6E_0','6J_0','6M_0','6N_0','6S_0','CL_0','GC_0','ES_0','YM_0','NQ_0','CGB_0','BAX_0','BAX_1','BAX_2','BAX_3','BAX_4','BAX_5','BAX_6','SXF_0','FGBS_0','FGBM_0','FGBL_0','FGBX_0','FESX_0','FDAX_0','FSMI_0','FESB_0','FBTP_0','FOAT_0','FSTB_0','FXXP_0','BR_DOL_0','BR_WDO_0','BR_IND_0','BR_WIN_0','DI1F15','DI1F16','DI1F17','DI1F18','DI1F19','DI1F20','DI1N14','DI1N15','DI1N16','DI1N17','DI1N18','DI1J15','DI1J16','DI1J17','JFFCE_0','KFFTI_0','LFR_0','LFZ_0','LFI_0','LFI_1','LFI_2','LFI_3','LFI_4','LFI_5','LFL_0','LFL_1','LFL_2','LFL_3','LFL_4','LFL_5','HHI_0','HSI_0','MCH_0','MHI_0','NKM_0','NK_0','RI_0','Si_0','USD000UTSTOM' );


my $this_log_file_ = $TRADELOG_DIR."/avg_vol_price_".$TIME_DURATION.".".$tradingdate_;
my $daily_log_file_ = $TRADELOG_DIR."/avg_vol_price_".$TIME_DURATION;
open ( LOG_FILE, ">", $this_log_file_ ) or PrintStacktraceAndDie ( " Could not open $this_log_file_ to write.\n");

print LOG_FILE "#SHORTCODE AVG_VOLUME MIN_PRICE MAX_PRICE\n";

foreach my $shortcode_  ( @shortcodes_)
{

  my $periodic_volume_cmd_ = $GENERATE_VOLUME_EXEC." ".$shortcode_." ".$tradingdate_." ".$TIME_DURATION;
  if ( $USER eq "diwakar")
  {
      print $periodic_volume_cmd_."\n"
  }
  my @periodic_volumes_ = `$periodic_volume_cmd_`; chomp(@periodic_volumes_);

  my @volume_time_ = ( );
  my @volume_value_ = ( );

  foreach my $volume_line_ (@periodic_volumes_)
  {
    my @volume_line_words_ = split (' ', $volume_line_);
    if ( $#volume_line_words_ >= 1 )
    {
      push ( @volume_time_, $volume_line_words_[0] );
      push ( @volume_value_, $volume_line_words_[1] );
    }
  }
  print " volume vec_len: ".$#volume_time_." ".$#volume_value_ ." shc ".$shortcode_."\n";
  my $average_ = GetAverage ( \@volume_value_ );

  my $start_time_ = min ( @volume_time_ )  ;
  my $end_time_ = max ( @volume_time_ );
  my $start_hhmm_ = GetUTCHHMMfromEpoch ( $start_time_ );
  my $end_hhmm_ = GetUTCHHMMfromEpoch ( $end_time_ );

  my $periodic_price_cmd_ = $GENERATE_PRICE_EXEC_." SHORTCODE ".$shortcode_." -1 PORT -1 ".$tradingdate_." Midprice ".$start_hhmm_." ".$end_hhmm_. " -1 1";
  if ( $USER eq "diwakar")
  {
    print $periodic_price_cmd_."\n";
  }

  my @periodic_price_ = `$periodic_price_cmd_`; chomp ( @periodic_price_);
  my $max_price_ = "0";
  my $min_price_ = "0";
  for ( my $i = 0; $i < $#periodic_price_ ; $i++ )
  {
    my @price_line_words_ = split (' ', $periodic_price_[$i] );         
    if ( $i == 0 )
    {
      $max_price_ = $price_line_words_[2];
      $min_price_ = $price_line_words_[2];
    }
    else
    {
      if ( $price_line_words_[2] > $max_price_ )
      {
        $max_price_ = $price_line_words_[2];
      }
      if ( $price_line_words_[2] < $min_price_ )
      {                                 
        $min_price_ = $price_line_words_[2];
      }
    }
  }
  print LOG_FILE $shortcode_." ".int ( $average_ ) ." ".$min_price_." ".$max_price_."\n";

}
close(LOG_FILE);
if ($generate_daily_file_ )
{
  `cp $this_log_file_ $daily_log_file_`;
}
`$SCRIPTS_DIR/sync_dir_to_all_machines.pl /spare/local/tradeinfo/volatilitylogs `;
exit;
sub GetUTCHHMMfromEpoch
{
  my ( $this_epoch_ ) = @_;
  my $get_utc_hh_mm_cmd_ = $GET_UTC_FROM_EPOCH_TIME." ".$this_epoch_;
  if ( $USER eq "diwakar" )
  {
    print $get_utc_hh_mm_cmd_."\n";
  }
  my $utc_hhmm_str_ = `$get_utc_hh_mm_cmd_`; chomp ($utc_hhmm_str_);
  my @utc_time_words_ = split ( ' ', $utc_hhmm_str_ );        
  my $utc_hhmmss_str_ = $utc_time_words_[3];
  my @hhmmss_words_ = split (':', $utc_hhmmss_str_);
  return $hhmmss_words_[0].$hhmmss_words_[1];
  
}

