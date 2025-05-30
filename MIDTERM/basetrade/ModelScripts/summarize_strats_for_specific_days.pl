#!/usr/bin/perl

# \file ModelScripts/summarize_strats_for_specific_days.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/wf_strats"; # this directory is used to store the chosen strategy files
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $GLOBALRESULTSDBDIR="DB"; # Changed for DVCTrader ... so that it 


my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
my $GET_AVG_VOLUME_SCRIPT = $SCRIPTS_DIR."/get_avg_volume_in_timeperiod_2.pl";
my $GET_UTC_HHMM_STR_EXEC = $BIN_DIR."/get_utc_hhmm_str";
if ( -e "$LIVE_BIN_DIR/get_utc_hhmm_str" )
{
    $GET_UTC_HHMM_STR_EXEC = $LIVE_BIN_DIR."/get_utc_hhmm_str";
}

require "$GENPERLLIB_DIR/sqrt_sign.pl"; # SqrtSign
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; # GetBasepxStratFirstModel
require "$GENPERLLIB_DIR/get_choose_strats_config.pl"; # GetChooseStratsConfig
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/sample_data_utils.pl";

my $USAGE = "$0 SHORTCODE TIMEPERIOD NUM_PREV_DAYS [SORT_ALGO = kCNAPnlAverage] [FEATURES_FRAC_DAYS = \"VOL 0.3 HIGH [INNDEP]\"] [FEATURES_MEDIAN_FACTOR = \"VOL 1.5 HIGH [INDEP]\"] [ECO_EVENT = \"USD 10-Year-Auction\"] [MAX_LOSS_PER_UTS = 0]";

if ( $#ARGV < 2 )
{
    print $USAGE."\n";
    exit ( 0 );
}

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $num_prev_days_ = $ARGV [ 2 ];

my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-1" );
my $trading_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_end_yyyymmdd_ , $num_prev_days_ );

my $trading_start_hhmm_ = "";
my $trading_end_hhmm_ = "";

my $sort_algo_ = "kCNAPnlAverage";

my @features_frac_days_filters_ = ( );
my @features_median_factor_filters_ = ( );

my $eco_event_ = "";
my $given_eco_event_ = 0;

my $max_loss_per_uts_ = 0;

ReadArgs ( );

my @list_of_tradingdates_to_summarize_over_ = ( );
my $list_of_tradingdates_to_summarize_over_filename_ = "";

BuildListOfDates ( );

SummarizeStratsAndPresentResults ( );

`rm -f $list_of_tradingdates_to_summarize_over_filename_`;

exit ( 0 );

sub ReadArgs
{
  { # split timeperiod into start & end hhmm
    my @timeperiod_words_ = split ( '-' , $timeperiod_ ); chomp ( @timeperiod_words_ );
    if ( $#timeperiod_words_ < 1 )
    {
      print "For this script , please specify timeperiod as EST_800-CET_1900 i.e. START_HHMM-END_HHMM\n";
      exit ( 0 );
    }

    my $st_ = $timeperiod_words_ [ 0 ];
    my $et_ = $timeperiod_words_ [ 1 ];

    $trading_start_hhmm_ = `$GET_UTC_HHMM_STR_EXEC $st_`; chomp ( $trading_start_hhmm_ );
    $trading_end_hhmm_ = `$GET_UTC_HHMM_STR_EXEC $et_`; chomp ( $trading_end_hhmm_ );
  }

  my $current_mode_ = "";

  for ( my $argc_ = 3 ; $argc_ <= $#ARGV ; $argc_ ++ )
  {
    given ( $ARGV [ $argc_ ] )
    {
      when ( "=" )
      { # do nothing
      }

      when ( "FEATURES_FRAC_DAYS" )
      {
        $current_mode_ = "FEATURES_FRAC_DAYS";
      }

      when ( "FEATURES_MEDIAN_FACTOR" )
      {
        $current_mode_ = "FEATURES_MEDIAN_FACTOR";
      }

      when ( "SORT_ALGO" )
      {
        $current_mode_ = "SORT_ALGO";
      }

      when ( "ECO_EVENT" )
      {
        $current_mode_ = "ECO_EVENT";
      }

      when ( "MAX_LOSS_PER_UTS" )
      {
        $current_mode_ = "MAX_LOSS_PER_UTS";
      }

      default
      {
        if ( $current_mode_ eq "FEATURES_FRAC_DAYS" )
        {
          push ( @features_frac_days_filters_, $ARGV [ $argc_ ] );
        }
        elsif ( $current_mode_ eq "FEATURES_MEDIAN_FACTOR" )
        {
          push ( @features_median_factor_filters_, $ARGV [ $argc_ ] );
        }
        elsif ( $current_mode_ eq "SORT_ALGO" )
        {
          $sort_algo_ = $ARGV [ $argc_ ];
        }
        elsif ( $current_mode_ eq "ECO_EVENT" )
        {
          $eco_event_ = $ARGV [ $argc_ ];
          $given_eco_event_ = $ARGV [ $argc_ ];
        }
        elsif ( $current_mode_ eq "MAX_LOSS_PER_UTS" )
        {
          $max_loss_per_uts_ = $ARGV [ $argc_ ];
        }
      }
    }
  }

  return;
}

sub BuildListOfDates
{
    my @list_of_tradingdates_in_range_ = ( );

    { # first get all trading dates in range.
	my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $trading_end_yyyymmdd_ $num_prev_days_";
	print $exec_cmd_."\n";

	my $exec_cmd_output_ = `$exec_cmd_`; chomp ( $exec_cmd_output_ );

	my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ );
	foreach my $tradingdate_ ( @exec_cmd_output_words_ )
	{
	    push ( @list_of_tradingdates_in_range_ , $tradingdate_ );
	}
    }

    my %tradingdate_to_volume_information_ = ( );
    my @list_of_volumes_ = ( );

    if ( $#features_frac_days_filters_ >= 0 )
    {
      foreach my $features_frac_days_filter_ ( @features_frac_days_filters_ ) 
      {
        my @filter_parts_ = split( " AND ", $features_frac_days_filter_ );

        my @filtered_days_vec_ = @list_of_tradingdates_in_range_;

        foreach my $t_filter_ ( @filter_parts_ ) {
          my @t_filtered_days_vec_ = ( );

          FilterFeaturesFracDays ( $t_filter_, \@list_of_tradingdates_in_range_, \@t_filtered_days_vec_ );

          @filtered_days_vec_ = grep { FindItemFromVec( $_, @filtered_days_vec_ ) } @t_filtered_days_vec_;
        }

        push ( @list_of_tradingdates_to_summarize_over_ , @filtered_days_vec_ );
      }
    }

    elsif ( $#features_median_factor_filters_ >= 0)
    {
      foreach my $features_median_filter_ ( @features_median_factor_filters_ )
      {
        my @filter_parts_ = split( " AND ", $features_median_filter_ );

        my @filtered_days_vec_ = @list_of_tradingdates_in_range_;

        foreach my $t_filter_ ( @filter_parts_ ) {
          my @t_filtered_days_vec_ = ( );

          FilterFeaturesMedianDays ( $t_filter_, \@list_of_tradingdates_in_range_, \@t_filtered_days_vec_ );

          @filtered_days_vec_ = grep { FindItemFromVec( $_, @filtered_days_vec_ ) } @t_filtered_days_vec_;
        }

        push ( @list_of_tradingdates_to_summarize_over_ , @filtered_days_vec_ );
      }
    }

    elsif ( $given_eco_event_ )
    {
      my $exec_cmd_ = "grep -h \"$eco_event_\" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_*_processed.txt";
      print $exec_cmd_."\n";

      my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

      foreach my $eco_line_ ( @exec_cmd_output_ )
      {
        my @eco_line_words_ = split ( ' ' , $eco_line_ ); chomp ( @eco_line_words_ );

        if ( $#eco_line_words_ >= 4 )
        {
          my $date_time_word_ = $eco_line_words_ [ $#eco_line_words_ ];

          my @date_time_words_ = split ( '_' , $date_time_word_ ); chomp ( @date_time_words_ );
          if ( $#date_time_words_ >= 0 )
          {
            my $tradingdate_ = $date_time_words_ [ 0 ];
            if ( FindItemFromVec ( $tradingdate_ , @list_of_tradingdates_in_range_ ) &&
                ! FindItemFromVec ( $tradingdate_ , @list_of_tradingdates_to_summarize_over_ ) )
            {
              push ( @list_of_tradingdates_to_summarize_over_ , $tradingdate_ );
            }
          }
        }
      }
    }

    else
    { # no specific criteria specified , summarize over entire set
      @list_of_tradingdates_to_summarize_over_ = @list_of_tradingdates_in_range_;
    }

    print "using the following list of tradingdates to summarize over \n".join ( ' ' , @list_of_tradingdates_to_summarize_over_ )."\n";

    { # write this list of specific dates to a file
      my $cstempfile_ = GetCSTempFileName ( $HOME_DIR."/cstemp" );
      $list_of_tradingdates_to_summarize_over_filename_ = $cstempfile_;

      open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
      foreach my $tradingdate_ ( @list_of_tradingdates_to_summarize_over_ )
      {
        print CSTF "$tradingdate_\n";
      }
      close CSTF;
    }

    return;
}

sub FilterFeaturesFracDays
{
  my ( $day_filter_, $day_unfiltered_ref_, $day_vec_ref_ ) = @_;

  @$day_vec_ref_ = ( );

  $day_filter_ =~ s/^\s+|\s+$//g;
  my @day_filter_words_ = split(/\s+/, $day_filter_);

  my $filter_tag_ = shift @day_filter_words_ || "";

# format: [bd/vbd/pbd/hv/lv/VOL/STDEV/...] [end-date] [num-days] [frac-days] [HIGH/LOW]
  {
    my @tag_aux_ = ();
    if ( $filter_tag_ eq "CORR" ) {
      my $corr_indep_ = shift @day_filter_words_;
      push ( @tag_aux_, $corr_indep_ );
    }

    my $percentile_ = shift @day_filter_words_ || 0.3;
    my $highlow_ = shift @day_filter_words_ || "HIGH";
    my $tshortcode_ = shift @day_filter_words_ || $shortcode_;

    GetFilteredDays ( $tshortcode_, $day_unfiltered_ref_, $percentile_, $highlow_, $filter_tag_, \@tag_aux_, $day_vec_ref_, $trading_start_hhmm_, $trading_end_hhmm_ );
  }
}

sub FilterFeaturesMedianDays
{
  my ( $day_filter_, $day_unfiltered_ref_, $day_vec_ref_ ) = @_;

  @$day_vec_ref_ = ( );

  $day_filter_ =~ s/^\s+|\s+$//g;
  my @day_filter_words_ = split(/\s+/, $day_filter_);

  my $filter_tag_ = shift @day_filter_words_ || "";

# format: [bd/vbd/pbd/hv/lv/VOL/STDEV/...] [end-date] [num-days] [frac-days] [HIGH/LOW]
  {
    my @tag_aux_ = ();
    if ( $filter_tag_ eq "CORR" ) {
      my $corr_indep_ = shift @day_filter_words_;
      push ( @tag_aux_, $corr_indep_ );
    }

    my $percentile_ = shift @day_filter_words_ || 0.3;
    my $highlow_ = shift @day_filter_words_ || "HIGH";
    my $tshortcode_ = shift @day_filter_words_ || $shortcode_;

    GetFilteredMedianDays ( $tshortcode_, $day_unfiltered_ref_, $percentile_, $highlow_, $filter_tag_, \@tag_aux_, $day_vec_ref_, $trading_start_hhmm_, $trading_end_hhmm_ );
  }
}

sub GetFilteredMedianDays
{
  my $this_shc_ = shift;
  my $dates_vec_ref_ = shift;
  my $medianratio_ = shift;
  my $high_low_ = shift;
  my $factor_ = shift;
  my $factor_aux_ = shift;
  my $filtered_dates_ref_ = shift;

  my $start_hhmm_ = shift || "0000";
  my $end_hhmm_ = shift || "2400";

  my %feature_avg_map_ = ();
  foreach my $tdate_ ( @$dates_vec_ref_ ) {
    my ($t_avg_, $is_valid_) = GetFeatureAverage ( $this_shc_, $tdate_, $factor_, $factor_aux_, $start_hhmm_, $end_hhmm_ );
    if ( $is_valid_ ) {
      $feature_avg_map_{ $tdate_ } = $t_avg_;
    }
  }

  my @vals_vec_ = values %feature_avg_map_;
  my $median_value_ = GetMedianAndSort ( \@vals_vec_ );

  if ( $high_low_ eq "LOW" ) {
    @$filtered_dates_ref_ = grep { $feature_avg_map_{ $_ } < $median_value_ } keys %feature_avg_map_;
  }
  elsif ( $high_low_ eq "HIGH" ) {
    @$filtered_dates_ref_ = grep { $feature_avg_map_{ $_ } > $median_value_ } keys %feature_avg_map_;
  }
  else {
    print "No HIGH or LOW selected";
    @$filtered_dates_ref_ = ( );
  }
}

sub SummarizeStratsAndPresentResults
{
    # compute the startdate by taking enddate and skipping to previous $num_days_past_ number of working days 
    my $trading_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_end_yyyymmdd_, $num_prev_days_ ) ;
    my $outsample_barrier_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_end_yyyymmdd_, ($num_prev_days_ * 2 / 3) ) ;

    my @all_strats_in_dir_ = MakeStratVecFromDir ( "$MODELING_STRATS_DIR/$shortcode_/$timeperiod_" );

    my @outsample_strats_ = ();
    my @outsample_strat_basenames_ = ();

    foreach my $full_strat_filename_ ( @all_strats_in_dir_ )
    {
        # for each file 
        # get basename($file)
        # find the last date of insample
        # if file is outsample in the entire period
        # add the file to the list, and basename to basename list
        # if you detect a duplicate entry then do not add the next one and print

	my $strat_basename_ = basename ( $full_strat_filename_ );
	my $last_insample_date_ = GetInsampleDate ( $strat_basename_ );
	if ( ! ( FindItemFromVec ( $strat_basename_, @outsample_strat_basenames_ ) ) )
	{ # not a duplicate entry
	    push ( @outsample_strats_, $full_strat_filename_ );
	    push ( @outsample_strat_basenames_, $strat_basename_ );
	}
    }
    
    {
	my $cstempfile_ = GetCSTempFileName ( $HOME_DIR."/cstemp" );
	open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
	foreach my $t_eligible_strat_file_ ( @outsample_strat_basenames_ )
	{
	    print CSTF "$t_eligible_strat_file_\n";
	}
	close CSTF;

        # ~/basetrade_install/bin/summarize_strategy_results BR_DOL_0 ~/modelling/strats/BR_DOL_0/ ~/ec2_globalresults/ 20140318 20140320 INVALIDFILE kCNAPnlAdjAverage 800
	my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $cstempfile_ $GLOBALRESULTSDBDIR $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ INVALIDFILE $sort_algo_ $max_loss_per_uts_ $list_of_tradingdates_to_summarize_over_filename_ 0";
	print $exec_cmd."\n";
	my @ssr_output_ = `$exec_cmd`;
	chomp ( @ssr_output_ );

	printf ( "STRATEGYFILEBASE  _fn_  pnl  pnl_stdev  volume  pnl_shrp  pnl_cons  pnl_median  ttc  pnl_zs  avg_min_max_pnl  PPT  S B A I MAXDD\n" );
	print join ( "\n", @ssr_output_ )."\n\n";
	`rm -f $cstempfile_`;
    }

    return;
}
