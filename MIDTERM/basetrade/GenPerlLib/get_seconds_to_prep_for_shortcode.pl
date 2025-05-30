# \file GenPerlLib/get_seconds_to_prep_for_shortcode.pl
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

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";


require "$GENPERLLIB_DIR/date_utils.pl"; #GetTPFromTime , GetUTCTime
require "$GENPERLLIB_DIR/strat_utils.pl"; #GetMinStratStartTime

sub GetSecondsToPrepareForShortcode
{
    my $shortcode_ = shift;
    my $seconds_to_prepare_ = 0 ;

    given ( $shortcode_ )
    {
	when ( "SXF_0" )
	{ 
	    $seconds_to_prepare_ = 900;
	}
        when ( "NK_0" )
	{ 
	    $seconds_to_prepare_ = 600;
	}
       	default
	{
	    $seconds_to_prepare_ = 1800;
	}
    }
    $seconds_to_prepare_ = 0 ;

    $seconds_to_prepare_;
}

sub GetQueryStartTimeForShortcodeAndTP
{
    my $shortcode_ = shift;
    my $start_time_ = shift ;

    my $tp_ = GetTPFromTime($start_time_);
    my $pick_strats_config_ = "$HOME_DIR/modelling/pick_strats_config/$tp_/$shortcode_.$tp_.txt" ;

    given ( $shortcode_ )
    {
        #can specify specific pick_strats config based on start_time or can hardcode specific query_start_time
        when ( "HHI_0" )
	{ 
          if ( GetTimestampFromTZ_HHMM_DATE($start_time_) <= GetTimestampFromTZ_HHMM_DATE("HKT_1000") )
          {
            $pick_strats_config_ = "$HOME_DIR/modelling/pick_strats_config/AS/HHI_0.ASM.txt" ;
          }
          elsif ( GetTimestampFromTZ_HHMM_DATE($start_time_) <= GetTimestampFromTZ_HHMM_DATE("HKT_1600") )
          {
            $pick_strats_config_ = "$HOME_DIR/modelling/pick_strats_config/AS/HHI_0.ASD.txt" ;
          } 
	}
        when ( "TOPIX_0" )
        {
          if ( $tp_ eq "AS" )
          {
            $pick_strats_config_ = "$HOME_DIR/modelling/pick_strats_config/AS/TOPIX_0.FS.txt" ;
          }
          elsif ( $tp_ eq "EU" )
          {
            $pick_strats_config_ = "$HOME_DIR/modelling/pick_strats_config/AS/TOPIX_0.SS.txt" ;
          }
          else
          {
            $pick_strats_config_ = "$HOME_DIR/modelling/pick_strats_config/AS/TOPIX_0.TS.txt" ;
          }
        }
       	default
	{
	}
    }

    if ( -s $pick_strats_config_ )
    { 
      my $query_start_time_ = `grep -v "^#" $pick_strats_config_ | grep EXEC_START_HHMM -A1 | tail -n1`; chomp($query_start_time_);
      if ( $query_start_time_ )
      {
        return $query_start_time_;
      }
    }

    return -1;
}

sub GetDataStartTimeForStrat
{
  my $stratfile_ = shift;
  my $date_ = shift;

  my $data_start_time_ = 0.0;
  my $min_start_time_ = GetMinStratStartTime ( $stratfile_ , $date_ );                          
  my $shortcode_ = `awk '{if(\$1=="STRATEGYLINE"){print \$2}}' $stratfile_ | head -n1`; chomp($shortcode_);
  if ( $min_start_time_ ne "-1" && $shortcode_)                        
  {                        
    my $query_start_time_ = GetQueryStartTimeForShortcodeAndTP ( $shortcode_, $min_start_time_ );                        
  
    if ( $query_start_time_ ne "-1" )                        
    {
      if ( GetTimestampFromTZ_HHMM_DATE($query_start_time_, $date_) < GetTimestampFromTZ_HHMM_DATE($min_start_time_, $date_) )      
      {
        #query_start_time is sane, i.e. before strat start time
        $data_start_time_ = GetTimestampFromTZ_HHMM_DATE ( $query_start_time_, $date_ );
      }
      elsif ( GetTimestampFromTZ_HHMM_DATE($query_start_time_) >= GetTimestampFromTZ_HHMM_DATE("2000") && 
          GetTimestampFromTZ_HHMM_DATE($min_start_time_) <= GetTimestampFromTZ_HHMM_DATE("0100") )        
      {
        #for cases query start a day before and trading starts next day, early AS strats
        $data_start_time_ = GetTimestampFromTZ_HHMM_DATE ( $query_start_time_, CalcPrevDate ( $date_ ) );
      }
    }                        
  }
  return $data_start_time_;
}


if ( $0 eq __FILE__ ) {
    print GetSecondsToPrepareForShortcode ( $ARGV[0] );
}

1
