# \file GenPerlLib/gen_ind_utils.pl
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
use feature "switch";

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; #GetExchFromSHC
require "$GENPERLLIB_DIR/get_port_constituents.pl"; # GetPortConstituents
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_bad_days_for_shortcode.pl"; # GetBadDaysForShortcode
require "$GENPERLLIB_DIR/get_very_bad_days_for_shortcode.pl"; # GetVeryBadDaysForShortcode
require "$GENPERLLIB_DIR/get_high_volume_days_for_shortcode.pl"; # GetHighVolumeDaysForShortcode
require "$GENPERLLIB_DIR/get_low_volume_days_for_shortcode.pl"; # GetLowVolumeDaysForShortcode
require "$GENPERLLIB_DIR/get_high_stdev_days_for_shortcode.pl"; # GetHighStdevDaysForShortcode
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; #GetFilteredDays
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl"; # GetPnlSamplesCorrelation

sub GenerateDaysVec ; # Generate the list of datagen/trading/validation days

sub GetDatesUsingSampleData
{
  my ($shortcode_, $dates_vec_ref_, $gd_string_, $datagen_start_hhmm_, $datagen_end_hhmm_, $datagen_day_vec_ref_) = @_;
  my ($gd_start_date_, $gd_ndays_, $gd_factor_, $gd_factor_aux_) = split(/\s+/, $gd_string_);
  $gd_start_date_ = GetIsoDateFromStrMin1 ( $gd_start_date_ );

  my @gd_fset_dates_ = ();
  my @gd_sset_dates_ = ();
  my @gd_tset_dates_ = ();

  my $gd_lbound_ = 0;
  my $gd_ubound_ = 0.3;
  GetFilteredDaysInPercRange ( $shortcode_, $dates_vec_ref_, $gd_lbound_, $gd_ubound_, $gd_factor_, $gd_factor_aux_, \@gd_fset_dates_, $datagen_start_hhmm_, $datagen_end_hhmm_);

  $gd_lbound_ = 0.3;
  $gd_ubound_ = 0.7;
  GetFilteredDaysInPercRange ( $shortcode_, $dates_vec_ref_, $gd_lbound_, $gd_ubound_, $gd_factor_, $gd_factor_aux_, \@gd_sset_dates_, $datagen_start_hhmm_, $datagen_end_hhmm_);

  $gd_lbound_ = 0.7;
  $gd_ubound_ = 1;
  GetFilteredDaysInPercRange ( $shortcode_, $dates_vec_ref_, $gd_lbound_, $gd_ubound_, $gd_factor_, $gd_factor_aux_, \@gd_tset_dates_, $datagen_start_hhmm_, $datagen_end_hhmm_);
  
  push ( @$datagen_day_vec_ref_, @gd_fset_dates_[0 .. int ( 0.5*( $#gd_fset_dates_ + 1 ) ) ]
      , @gd_sset_dates_[0 .. int ( 0.5*( $#gd_sset_dates_ + 1 ) ) ]
      , @gd_tset_dates_[0 .. int ( 0.5*( $#gd_tset_dates_ + 1 ) ) ] );
}

sub GenerateDaysVec
{
  my ($shortcode_, $start_yyyymmdd_, $end_yyyymmdd_, $start_hhmm_, $end_hhmm_, $day_vec_ref_, $day_filter_choices_ref_, $filter_start_yyyymmdd_, $filter_max_days_, $exclude_days_ref_, $datagen_or_trading_, $main_log_file_handle_, $gd_string_) = @_;
#print "gd_string: $gd_string_, $start_yyyymmdd_, $end_yyyymmdd_, $start_hhmm_, $end_hhmm_, $filter_start_yyyymmdd_, $filter_max_days_\n";
  my $filter_name_ = "";

  my @day_filter_choices_ = @$day_filter_choices_ref_;
  my $day_filter_ = "";
  if ( $#day_filter_choices_ >= 0 )
  { ### ONLY SINGLE DATAGEN_DAY_FILTER
    my @uniq_day_filter_choices_ = do { my %seen; grep { !$seen{$_}++ } @day_filter_choices_ };
    print $main_log_file_handle_ "Total datagen day filter choices: ",join( ' ', @uniq_day_filter_choices_ ),"\n";
    my $t_random_day_filter_index_ = int( rand ( $#uniq_day_filter_choices_ + 1) );
    $day_filter_ = $uniq_day_filter_choices_ [ $t_random_day_filter_index_ ];
    $day_filter_ = "" if $day_filter_ eq "normal";
#print "Filter chosen: $day_filter_\n";

# special days are less and if somebody increase the search range just for special days    
    $start_yyyymmdd_ = $filter_start_yyyymmdd_ if ( $day_filter_ ne "" && $filter_start_yyyymmdd_ ne "" );
  }

  my $max_days_at_a_time_ = 600;
 
  my @t_day_vec_ = @$day_vec_ref_;
  @$day_vec_ref_ = ( );

  if ( $#t_day_vec_ < 0 ) { 
    my @dates_vec_ = GetDatesFromStartDate( $shortcode_, $start_yyyymmdd_, $end_yyyymmdd_, "INVALIDFILE", $max_days_at_a_time_ );
    
    if ( defined $gd_string_ && $gd_string_ ne "" ) {
      GetDatesUsingSampleData ( $shortcode_, \@dates_vec_, $gd_string_, $start_hhmm_, $end_hhmm_, \@t_day_vec_ );
    }
    elsif ( $day_filter_ ne "" ) {
      print $main_log_file_handle_ "Applying $datagen_or_trading_ day filter: ".$day_filter_."\n";

      my @day_filter_parts_ = split( " AND ", $day_filter_ );
      my @day_filtered_vec_ = @dates_vec_; 
      foreach my $t_day_filter_ ( @day_filter_parts_ ) {
        my @tt_day_vec_ = ( );
        my $name_to_append_ = FilterDatagenDays ( $shortcode_, $start_hhmm_, $end_hhmm_, $t_day_filter_, \@dates_vec_, \@tt_day_vec_, $main_log_file_handle_ );
        @day_filtered_vec_ = grep { FindItemFromVec( $_, @day_filtered_vec_ ) } @tt_day_vec_;
        print $main_log_file_handle_ $t_day_filter_.": ".join(" ", @tt_day_vec_)."\n";
        
        $name_to_append_ = lc($name_to_append_);
        if ( defined $name_to_append_ && !( $filter_name_ =~ /^$name_to_append_/ ) ) {
          $filter_name_ = $name_to_append_."_".$filter_name_;
        }
      }
      @t_day_vec_ = sort ( @day_filtered_vec_ );

      if ( $#t_day_vec_ >= $filter_max_days_ )
      {
        printf $main_log_file_handle_ "retaining only last $filter_max_days_ from $datagen_or_trading_ day_vec_ among $#t_day_vec_ days\n";
        splice ( @t_day_vec_, 0, -$filter_max_days_ ); 
      }
    }
    else {
      @t_day_vec_ = @dates_vec_;
    }
  } else {
    @t_day_vec_ = grep { ValidDate($_) && ! NoDataDateForShortcode($_, $shortcode_ ) && ! IsProductHoliday($_, $shortcode_ ) } @t_day_vec_;
  }
  @t_day_vec_ = sort @t_day_vec_;

  @$day_vec_ref_ = grep { ! FindItemFromVec ( $_, @$exclude_days_ref_ ) } @t_day_vec_;
  return $filter_name_;
}

sub FilterDatagenDays
{
  my ( $shortcode_, $start_hhmm_, $end_hhmm_, $datagen_day_filter_, $datagen_day_unfiltered_ref_, $datagen_day_vec_ref_, $main_log_file_handle_ ) = @_;

  @$datagen_day_vec_ref_ = ( );

  $datagen_day_filter_ =~ s/^\s+|\s+$//g;
  my @day_filter_words_ = split(/\s+/, $datagen_day_filter_);

  my $filter_tag_ = shift @day_filter_words_ || "";

# day_filter_samplefeature_map_ maintained for backward compatibility
  my %day_filter_samplefeature_map_ = ( );
  $day_filter_samplefeature_map_{ "hv" } = "VOL HIGH";
  $day_filter_samplefeature_map_{ "lv" } = "VOL LOW";

  my @samplefeature_tags_ = qw(VOL STDEV L1SZ L1EVPerSec TREND ORDSZ TRADES SSTREND BidAskSpread CORR AvgPrice);

  my $name_to_append_="";

# format: [bd/vbd/pbd/hv/lv/VOL/STDEV/...] [end-date] [num-days] [frac-days] [HIGH/LOW]
  {
    if ( FindItemFromVec( $filter_tag_, @samplefeature_tags_ ) 
        || exists $day_filter_samplefeature_map_{ $filter_tag_ } )
    {
      my @tag_aux_ = ();
      if ( $filter_tag_ eq "CORR" ) {
        my $corr_indep_ = shift @day_filter_words_;
        push ( @tag_aux_, $corr_indep_ );
      }

      my $percentile_ = shift @day_filter_words_ || 0.3;
      my $highlow_ = shift @day_filter_words_ || "HIGH";

      if ( exists $day_filter_samplefeature_map_{ $filter_tag_ } )
      {
        my @tag_words_ = split(/\s+/, $day_filter_samplefeature_map_{ $filter_tag_ });
        $filter_tag_ = $tag_words_[0];
        $highlow_ = $tag_words_[1];
      }
      
      if ( $highlow_ eq "HIGH" || $highlow_ eq "LOW" ) {
	  GetFilteredDays ( $shortcode_, $datagen_day_unfiltered_ref_, $percentile_, $highlow_, $filter_tag_, \@tag_aux_, $datagen_day_vec_ref_, $start_hhmm_, $end_hhmm_ );
	  $name_to_append_ = (($highlow_ eq "LOW")?"l":"h").$filter_tag_;
      } else {
	  GetFilteredDaysOnSampleBounds ( $shortcode_, $datagen_day_unfiltered_ref_, $percentile_, $highlow_, $filter_tag_, \@tag_aux_, $datagen_day_vec_ref_, $start_hhmm_, $end_hhmm_ );
	  $name_to_append_ = "px_".$percentile_."_".$highlow_;
      }
    }

    elsif ( $filter_tag_ eq "bd" )
    {
      print $main_log_file_handle_ "filter: BAD_DAYS\n";
      GetBadDaysForShortcode ( $shortcode_, $datagen_day_vec_ref_ );
      $name_to_append_ = $filter_tag_;
    }

    elsif ( $filter_tag_ eq "vbd" )
    {
      print $main_log_file_handle_ "filter: VERY_BAD_DAYS\n";
      print $main_log_file_handle_ "On reading DATAGEN_VERY_BAD_DAYS datagen_day_vec reset to empty\n";
      GetVeryBadDaysForShortcode ( $shortcode_, $datagen_day_vec_ref_ );
      $name_to_append_ = $filter_tag_;
    }

    elsif ( $filter_tag_ eq "pbd" )
    {
      my $percentile_ = shift @day_filter_words_ || 0.3;
      my $timeperiod_ = "$start_hhmm_-$end_hhmm_";

      my $t_start_date_ = min ( @$datagen_day_unfiltered_ref_ );
      my $t_end_date_ = max ( @$datagen_day_unfiltered_ref_ );

      print $main_log_file_handle_ "filter: POOL_BAD_DAYS\n";
      GetBadDaysPoolForShortcode ( $shortcode_, $timeperiod_, $t_start_date_, $t_end_date_, $datagen_day_vec_ref_, $percentile_ );
      $name_to_append_ = $filter_tag_;
    }

    elsif ( $filter_tag_ eq "hsd" )
    {
      my $high_stdev_period_indep_ = shift @day_filter_words_;
      my $high_stdev_period_timeperiod_ = shift @day_filter_words_;
      my $high_stdev_period_lookahead_ = shift @day_filter_words_;
      my $t_start_date_ = min ( @$datagen_day_unfiltered_ref_ );

      print $main_log_file_handle_ "filter: HIGH_STDEV_DAYS\n";
      print $main_log_file_handle_ "HIGH_STDEV_DAYS parameters:", join( ' ', ( $high_stdev_period_indep_, $high_stdev_period_timeperiod_, $high_stdev_period_lookahead_ ) ) , "\n";
      GetHighStdevDaysForShortcode ( $high_stdev_period_indep_, $high_stdev_period_timeperiod_, $high_stdev_period_lookahead_, $datagen_day_vec_ref_, $t_start_date_ );
      $name_to_append_ = $filter_tag_."_".$high_stdev_period_indep_;
    }
  }
  return $name_to_append_;
}

1
