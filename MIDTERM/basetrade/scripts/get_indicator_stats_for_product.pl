#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use List::Util qw/max min/; # for max
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);
use File::Basename ;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $INSTALL_BIN = $HOME_DIR."/".$REPO."_install/bin";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats";
my $PYLIB = $HOME_DIR."/".$REPO."/pylib";
my $TMP_FOLDER = "/spare/local/logs/";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage, GetStdev
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # FetchPnlSamplesStrats GetPnlSamplesCorrelation
require "$GENPERLLIB_DIR/indstats_db_access_manager.pl"; #GetNameFromIdIndStats
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays
require "$GENPERLLIB_DIR/date_utils.pl"; # GetTrainingDatesFromDB

my $USAGE = "$0 SHORTCODE TIMEPERIOD DATE/DATESFILE NUMDAYS/-1 OUTPUT_FILE_PREFIX [PRINT_SUMMARY = 0 (default:1)] [FEATURES_FRAC_DAYS = \"VOL 0.3 HIGH [INDEP]\"] [FEATURES_MEDIAN_FACTOR = \"VOL 1.5 HIGH [INDEP]\"] [ECO_EVENT = \"USD 10-Year-Auction\"]";

# [PRED_ALGO/PRED_DUR/FILTER = <comma_delimited_list>] [BASEFUTPX = \"Midprice-Midprice\"]

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV[0];
my $timeperiod_ = $ARGV[1];

my $end_date_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $num_prev_days_ = $ARGV[3];
my $output_file_prefix_ = $ARGV[4];

my $print_summary_ = 1;
my $print_combined_ = 0;

my @features_frac_days_filters_ = ( );
my @features_median_factor_filters_ = ( );

my $eco_event_ = "";
my $given_eco_event_ = 0;

my @predalgo_vec_ = ( );
my @preddur_vec_ = ( );
my @filter_vec_ = ( );
my ($basepx_, $futpx_);

ReadArgs ( );

my @dates_vec_ = ( );
BuildListOfDates ( );

my %algodurfilter_to_indc_to_corr_stats_ = ( );
my %algodurfilter_to_indc_to_tailed_corr_stats_ = ( );
my %algodurfilter_to_indc_to_count_ = ( );

my ($predalgo_ref_, $preddur_ref_, $filter_ref_, $basepx_ref_, $futpx_ref_);
$predalgo_ref_ = \@predalgo_vec_ if $#predalgo_vec_ >= 0;
$preddur_ref_ = \@preddur_vec_ if $#preddur_vec_ >= 0;
$filter_ref_ = \@filter_vec_ if $#filter_vec_ >= 0;
$basepx_ref_ = [ $basepx_ ] if defined $basepx_;
$futpx_ref_ = [ $futpx_ ] if defined $futpx_;

my $tmp_id = `date +%N`;      
chomp($tmp_id);
my $output_file = $TMP_FOLDER."/".$tmp_id;

if ( $print_summary_ ) {

  my $query = "\"Select predalgo_id, pred_dur, filter_id, basepx_id, futpx_id, indicator_id, AVG(correlation), STD(correlation),
  MIN(correlation), MAX(correlation), AVG(tail_correlation), STD(tail_correlation), MIN(tail_correlation), MAX(tail_correlation),
  COUNT(*) from (Select PI.predalgo_id, PI.pred_dur, PI.filter_id, PI.basepx_id, PI.futpx_id, PI.indicator_id, I.correlation, I.tail_correlation
  from ProdIndicators PI, IndicatorStats I where PI.prod_indc_id = I.prod_indc_id and PI.shc_id = (select shc_id from Shortcodes where shortcode = \'$shortcode_\')
   and PI.tp_id = (select tp_id from Timeperiod where timeperiod = \'$timeperiod_\') and I.date in ( " .join(',', @dates_vec_) . " ) and I.correlation is not NULL)t
   group by predalgo_id, pred_dur, filter_id, basepx_id, futpx_id,indicator_id; \"";


  my $exec_cmd = "$PYLIB/indstats_db_access_manager.py -q $query -of $output_file ";

  my @output_lines = `$exec_cmd 2>&1`;
  chomp(@output_lines);
  print join("\n", @output_lines);

  open OUTPUT, "< $output_file" or PrintStacktraceAndDie ( "Error in opening file $output_file");
  @output_lines = <OUTPUT>; chomp ( @output_lines );
  close OUTPUT;
#print $_."\n" foreach @output_lines;
  get_map(\@output_lines, \%algodurfilter_to_indc_to_corr_stats_, \%algodurfilter_to_indc_to_tailed_corr_stats_, \%algodurfilter_to_indc_to_count_, 0 );
  PrintStats ( );
  PrintStatsCombined ( );
}
else {
    my $query = "\"Select predalgo_id, pred_dur, filter_id, basepx_id, futpx_id, date, indicator_id, correlation, tail_correlation from
    (Select PI.predalgo_id, PI.pred_dur, PI.filter_id, PI.basepx_id, PI.futpx_id, I.date, PI.indicator_id, I.correlation, I.tail_correlation
  from ProdIndicators PI, IndicatorStats I where PI.prod_indc_id = I.prod_indc_id and PI.shc_id = (select shc_id from Shortcodes where shortcode = \'$shortcode_\')
   and PI.tp_id = (select tp_id from Timeperiod where timeperiod = \'$timeperiod_\') and I.date in (" .join(',', @dates_vec_) . ") and I.correlation is not NULL)t \"";

  my $exec_cmd = "$PYLIB/indstats_db_access_manager.py -q $query -of $output_file ";

  #print $exec_cmd."\n";
  my @output_lines = `$exec_cmd 2>&1`;
  chomp(@output_lines);
  print join("\n", @output_lines);
  open OUTPUT, "< $output_file" or PrintStacktraceAndDie ( "Error in opening file $output_file");
  @output_lines = <OUTPUT>; chomp ( @output_lines );
  close OUTPUT;

  get_map(\@output_lines, \%algodurfilter_to_indc_to_corr_stats_, \%algodurfilter_to_indc_to_tailed_corr_stats_, undef, 1 );
  PrintDetailedStats ( );
}


sub ReadArgs
{
  my $current_mode_ = "";

  foreach my $this_arg_ ( @ARGV[ 5..$#ARGV ] ) {
    given ( $this_arg_ ) {
      when ( "=" ) { };

      when ( "FEATURES_FRAC_DAYS" ) { $current_mode_ = "FEATURES_FRAC_DAYS"; }

      when ( "FEATURES_MEDIAN_FACTOR" ) { $current_mode_ = "FEATURES_MEDIAN_FACTOR"; }

      when ( "ECO_EVENT" ) { $current_mode_ = "ECO_EVENT"; }
     
      when ( "PRED_ALGO" ) { $current_mode_ = "PRED_ALGO"; }

      when ( "PRED_DUR" ) { $current_mode_ = "PRED_DUR"; }

      when ( "FILTER" ) { $current_mode_ = "FILTER"; }

      when ( "BASEFUTPX" ) { $current_mode_ = "BASEFUTPX"; }

      when ( "PRINT_COMBINED" ) { $current_mode_ = "PRINT_COMBINED"; }

      when ( "PRINT_SUMMARY" ) { $current_mode_ = "PRINT_SUMMARY"; }

      default {
        if ( $current_mode_ eq "FEATURES_FRAC_DAYS" ) {
          push ( @features_frac_days_filters_, $this_arg_ );
        }
        elsif ( $current_mode_ eq "FEATURES_MEDIAN_FACTOR" ) {
          push ( @features_median_factor_filters_, $this_arg_ );
        }
        elsif ( $current_mode_ eq "ECO_EVENT" ) {
          $eco_event_ = $this_arg_;
          $given_eco_event_ = $this_arg_;
        }
        elsif ( $current_mode_ eq "BASEFUTPX" ) {
          ($basepx_, $futpx_) = split('-', $this_arg_);
        }
        elsif ( $current_mode_ eq "PRED_ALGO" ) {
          @predalgo_vec_ = split(',', $this_arg_);
        }
        elsif ( $current_mode_ eq "PRED_DUR" ) {
          @preddur_vec_ = split(',', $this_arg_);
        }
        elsif ( $current_mode_ eq "FILTER" ) {
          @filter_vec_ = split(',', $this_arg_);
        }
        elsif ( $current_mode_ eq "PRINT_COMBINED" ) {
          $print_combined_ = $this_arg_;
        }
        elsif ( $current_mode_ eq "PRINT_SUMMARY" ) {
          $print_summary_ = $this_arg_;
        }
      }
    }
  }
}

sub BuildListOfDates
{
  my @list_of_tradingdates_in_range_ = ( );

  if ( ! ValidDate($end_date_) || $num_prev_days_ <= 0 ) {
    if ( -f $end_date_ ) {
      open DATESHANDLE, "< $end_date_" or PrintStacktraceAndDie ( "Could not open file $end_date_ for reading" );
      my @dates_from_file_ = <DATESHANDLE>;
      chomp @dates_from_file_;
      close DATESHANDLE;
      my %dates_from_file_map_ =map{$_ =>1} @dates_from_file_;
      @list_of_tradingdates_in_range_ = grep ( $dates_from_file_map_{$_}, GetTrainingDatesFromDB($dates_from_file_[0], $dates_from_file_[-1], "", "INVALIDFILE"));
    }
    else {
      print STDERR "Either End_Date is invalid or num_days is <= 0\n";
      exit(0);
    }
  }
  else {
    @list_of_tradingdates_in_range_ = GetTrainingDatesFromDB("", $end_date_, $num_prev_days_, "INVALIDFILE");
  }


  my %tradingdate_to_volume_information_ = ( );
  my @list_of_volumes_ = ( );

  if ( $#features_frac_days_filters_ >= 0 ) {
    foreach my $features_frac_days_filter_ ( @features_frac_days_filters_ ) {
      my @filter_parts_ = split( " AND ", $features_frac_days_filter_ );
      my @filtered_days_vec_ = @list_of_tradingdates_in_range_;

      foreach my $t_filter_ ( @filter_parts_ ) {
        my @t_filtered_days_vec_ = ( );

        FilterFeaturesFracDays ( $t_filter_, \@list_of_tradingdates_in_range_, \@t_filtered_days_vec_ );
        @filtered_days_vec_ = grep { FindItemFromVec( $_, @filtered_days_vec_ ) } @t_filtered_days_vec_;
      }
      push ( @dates_vec_ , @filtered_days_vec_ );
    }
  }

  elsif ( $#features_median_factor_filters_ >= 0) {
    foreach my $features_median_filter_ ( @features_median_factor_filters_ ) {
      my @filter_parts_ = split( " AND ", $features_median_filter_ );
      my @filtered_days_vec_ = @list_of_tradingdates_in_range_;

      foreach my $t_filter_ ( @filter_parts_ ) {
        my @t_filtered_days_vec_ = ( );

        FilterFeaturesMedianDays ( $t_filter_, \@list_of_tradingdates_in_range_, \@t_filtered_days_vec_ );
        @filtered_days_vec_ = grep { FindItemFromVec( $_, @filtered_days_vec_ ) } @t_filtered_days_vec_;
      }
      push ( @dates_vec_ , @filtered_days_vec_ );
    }
  }

  elsif ( $given_eco_event_ ) {
    my $exec_cmd_ = "grep -h \"$eco_event_\" ~/infracore_install/SysInfo/BloombergEcoReports/merged_eco_*_processed.txt";
    print $exec_cmd_."\n";

    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    foreach my $eco_line_ ( @exec_cmd_output_ ) {
      my @eco_line_words_ = split ( ' ' , $eco_line_ ); chomp ( @eco_line_words_ );

      if ( $#eco_line_words_ >= 4 ) {
        my $date_time_word_ = $eco_line_words_ [ $#eco_line_words_ ];
        my @date_time_words_ = split ( '_' , $date_time_word_ ); chomp ( @date_time_words_ );

        if ( $#date_time_words_ >= 0 ) {
          my $tradingdate_ = $date_time_words_ [ 0 ];
          if ( FindItemFromVec ( $tradingdate_ , @list_of_tradingdates_in_range_ ) &&
              ! FindItemFromVec ( $tradingdate_ , @dates_vec_ ) ) {
            push ( @dates_vec_ , $tradingdate_ );
          }
        }
      }
    }
  }

  else { # no specific criteria specified , summarize over entire set
    @dates_vec_ = @list_of_tradingdates_in_range_;
  }
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

    GetFilteredDays ( $tshortcode_, $day_unfiltered_ref_, $percentile_, $highlow_, $filter_tag_, \@tag_aux_, $day_vec_ref_ );
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

    GetFilteredMedianDays ( $tshortcode_, $day_unfiltered_ref_, $percentile_, $highlow_, $filter_tag_, \@tag_aux_, $day_vec_ref_ );
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
  my $start_hhmm_ = shift;
  my $end_hhmm_ = shift;

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

sub PrintStats
{
  foreach my $algodurfilter_ ( keys %algodurfilter_to_indc_to_corr_stats_ ) {
    my ($predalgo_, $preddur_, $filter_, $basepx_, $futpx_) = split(' ', $algodurfilter_);
#print "Indicator Stats for $shortcode_: pred_algo: $predalgo_, pred_dur: $preddur_, filter: $filter_, pricetype: $basepx_-$futpx_ sorted by avg correlation:\n";
    my $ind_stats_ = $algodurfilter_to_indc_to_corr_stats_{ $algodurfilter_ };
    my $ind_tailed_stats_ = $algodurfilter_to_indc_to_tailed_corr_stats_{ $algodurfilter_ };
    my $ind_ndates_ = $algodurfilter_to_indc_to_count_{ $algodurfilter_ };

    my @indc_sorted_ = sort { abs($$ind_stats_{$b}{ "AVG" }) <=> abs($$ind_stats_{$a}{ "AVG" }) } keys %$ind_stats_;

    my $algodurfilter1_ = $algodurfilter_; $algodurfilter1_ =~ s/\s+/_/g;
    my $filename_ = $output_file_prefix_."_".$algodurfilter1_;
    open FHANDLE, "> $filename_" or PrintStackTraceAndDie ( "Could not open $filename_ for writing" );
    print FHANDLE "CORR_AVG CORR_SHARPE  TAIL_CORR_AVG TAIL_CORR_SHARPE  NDAYS\tINDICATOR\n";
    
    foreach my $indc_ ( @indc_sorted_ ) {
      my $corr_avg_ = $$ind_stats_{$indc_}{ "AVG" };
      my $corr_sd_ = $$ind_stats_{$indc_}{ "STD" };
      my $corr_sharpe_ = 0;
      $corr_sharpe_ = ($corr_avg_ / $corr_sd_) if $corr_sd_ > 0;
      
      my $line_to_print_ = sprintf("%8.3f",$corr_avg_)." ".sprintf("%8.3f",$corr_sharpe_);

      if ( defined $ind_tailed_stats_ && defined $$ind_tailed_stats_{$indc_} ) { 
        my $tailed_corr_avg_ = $$ind_tailed_stats_{$indc_}{ "AVG" };
        my $tailed_corr_sd_ = $$ind_tailed_stats_{$indc_}{ "STD" };
        my $tailed_corr_sharpe_ = 0;
        $tailed_corr_sharpe_ = ($tailed_corr_avg_ / $tailed_corr_sd_) if $tailed_corr_sd_ > 0;
        $line_to_print_ .= "  ".sprintf("%8.3f",$tailed_corr_avg_)." ".sprintf("%8.3f",$tailed_corr_sharpe_);
      }
      else {
        $line_to_print_ .= "  ".sprintf("%8s  %8s", "None", "None");
      }

      if ( defined $ind_ndates_ && defined $$ind_ndates_{$indc_} ) {
        $line_to_print_ .= " ".sprintf("%5d",$$ind_ndates_{$indc_});
      } else {
        $line_to_print_ .= "  None";
      }

      $line_to_print_ .= "\t$indc_";

      print FHANDLE $line_to_print_."\n";
    }
    close FHANDLE;
  }
}

sub PrintStatsCombined
{
  my %ind_to_count_ = ( );
  my %ind_to_stats_ = ( );
  my %ind_to_tailed_stats_ = ( );
  my %ind_to_algodurfilter_ = ( );

  foreach my $algodurfilter_ ( keys %algodurfilter_to_indc_to_corr_stats_ ) {
    my $ind_stats_ = $algodurfilter_to_indc_to_corr_stats_{ $algodurfilter_ };
    my $ind_tailed_stats_ = $algodurfilter_to_indc_to_tailed_corr_stats_{ $algodurfilter_ };

    foreach my $indc_ ( keys %$ind_stats_ ) {
      if ( ! defined $ind_to_stats_{ $indc_ } ) {
        $ind_to_count_{ $indc_ } = 1;
        $ind_to_algodurfilter_{ $indc_ } = $algodurfilter_;
        $ind_to_stats_{ $indc_ } = $$ind_stats_{$indc_};
        $ind_to_tailed_stats_{ $indc_ } = $$ind_tailed_stats_{$indc_};
      }
      else {
        $ind_to_count_{ $indc_ } += 1;
        if ( abs($$ind_stats_{$indc_}{ "AVG" }) > abs($ind_to_stats_{ $indc_ }{ "AVG" }) ) {
          $ind_to_algodurfilter_{ $indc_ } = $algodurfilter_;
          $ind_to_stats_{ $indc_ } = $$ind_stats_{$indc_};
          $ind_to_tailed_stats_{ $indc_ } = $$ind_tailed_stats_{$indc_};
        }
      }
    }
  }

  my @indc_sorted_ = sort { abs($ind_to_stats_{$b}{ "AVG" }) <=> abs($ind_to_stats_{$a}{ "AVG" }) } keys %ind_to_stats_;

  my $filename_ = $output_file_prefix_."_combined";
  open FHANDLE, "> $filename_" or PrintStackTraceAndDie ( "Could not open $filename_ for writing" );
  print FHANDLE "CORR_AVG CORR_SHARPE  TAIL_CORR_AVG TAIL_CORR_SHARPE\tINDICATOR (PredAlgo,PredDur,Filter,Basepx-Futpx)\n";

  foreach my $indc_ ( @indc_sorted_ ) {
    my $corr_avg_ = $ind_to_stats_{$indc_}{ "AVG" };
    my $corr_sd_ = $ind_to_stats_{$indc_}{ "STD" };
    my $corr_sharpe_ = 0;
    $corr_sharpe_ = ($corr_avg_ / $corr_sd_) if $corr_sd_ > 0;

    my $tailed_corr_avg_ = $ind_to_tailed_stats_{$indc_}{ "AVG" };
    my $tailed_corr_sd_ = $ind_to_tailed_stats_{$indc_}{ "STD" };
    my $tailed_corr_sharpe_ = 0;
    $tailed_corr_sharpe_ = ($tailed_corr_avg_ / $tailed_corr_sd_) if $tailed_corr_sd_ > 0;

    my $algodurfilter_ = $ind_to_algodurfilter_{ $indc_ } ;
    my ($predalgo_, $preddur_, $filter_, $basepx_, $futpx_) = split(' ', $algodurfilter_);

    my $line_to_print_ = sprintf("%8.3f",$corr_avg_)." ".sprintf("%8.3f",$corr_sharpe_);
    $line_to_print_ .= "  ".sprintf("%8.3f",$tailed_corr_avg_)." ".sprintf("%8.3f",$tailed_corr_sharpe_);


    $line_to_print_ .= "\t$indc_\t($predalgo_,$preddur_,$filter_,$basepx_-$futpx_)";

    print FHANDLE $line_to_print_."\n";
  }
  close FHANDLE;
}

sub PrintDetailedStats
{
  foreach my $algodurfilter_ ( keys %algodurfilter_to_indc_to_corr_stats_ ) {
    my ($predalgo_, $preddur_, $filter_, $basepx_, $futpx_) = split(' ', $algodurfilter_);
#print "Indicator Stats for $shortcode_: pred_algo: $predalgo_, pred_dur: $preddur_, filter: $filter_, pricetype: $basepx_-$futpx_ sorted by avg correlation:\n"; 
    my @dates_vec_ = sort keys %{ $algodurfilter_to_indc_to_corr_stats_{ $algodurfilter_ } };

    my $algodurfilter1_ = $algodurfilter_; $algodurfilter1_ =~ s/\s+/_/g;
    my $filename_ = $output_file_prefix_."_".$algodurfilter1_;
    open FHANDLE, "> $filename_" or PrintStackTraceAndDie ( "Could not open $filename_ for writing" );
    print FHANDLE "DATE\tCORR\tTAIL_CORR\tINDICATOR\n";

    foreach my $tdate_ ( @dates_vec_ ) {  
      my $ind_stats_ = $algodurfilter_to_indc_to_corr_stats_{ $algodurfilter_ }{ $tdate_ };
      my $ind_tailed_stats_ = $algodurfilter_to_indc_to_tailed_corr_stats_{ $algodurfilter_ }{ $tdate_ };

      my @indc_sorted_ = sort keys %$ind_stats_;

      foreach my $indc_ ( @indc_sorted_ ) {
        my $line_to_print_ = $tdate_."\t".sprintf("%8.3f",$$ind_stats_{ $indc_ })."\t";
        if ( defined $$ind_tailed_stats_{ $indc_ } ) {
          $line_to_print_ .= sprintf("%8.3f",$$ind_tailed_stats_{ $indc_ });
        } else {
          $line_to_print_ .= sprintf("%8s", "None");
        }
        $line_to_print_ .= "\t".$indc_;
        print FHANDLE $line_to_print_."\n";
      }
    }
    close FHANDLE;
  }
}

sub get_map {
  my $output_lines = shift;
  my $algodurfilter_to_indc_to_corr_ = shift;
  my $algodurfilter_to_indc_to_tailed_corr_ = shift;
  my $algodurfilter_to_count_ref_ = shift;
  my $detailed = shift;
  for ( my $i = 0; $i <= $#$output_lines; $i++ ) {
    my $line = $$output_lines[$i];
#print "$result_ref_ \n";
    my @result_ref_ = split ',', $line;
    if ( $#result_ref_ >= 7 ) {
      my $predalgo_ = $result_ref_[0];
      my $filter_ = $result_ref_[2];
      my $basepx = $result_ref_[3];
      my $futpx = $result_ref_[4];
      my $algodurfilter_key_ = $predalgo_.' '.$result_ref_[1].' '.$filter_.' '.$basepx.' '.$futpx;

      if ( $detailed eq 1) {
        my $date_ = $result_ref_[5];
        my $indc_ = $result_ref_[6];
        if ( $result_ref_[7] ne "None" ) {
          $$algodurfilter_to_indc_to_corr_{ $algodurfilter_key_ }{ $date_ }{ $indc_ } = $result_ref_[7];
        }
        if ( $result_ref_[8] ne "None" ) {
          $$algodurfilter_to_indc_to_tailed_corr_{ $algodurfilter_key_ }{ $date_ }{ $indc_ } = $result_ref_[8];
        }
      }
      else {
        my $indc_ = $result_ref_[5];
        if ( $result_ref_[6] ne "None" ) {
          $$algodurfilter_to_indc_to_corr_{ $algodurfilter_key_ }{ $indc_ }{ "AVG" } = $result_ref_[6];
          $$algodurfilter_to_indc_to_corr_{ $algodurfilter_key_ }{ $indc_ }{ "STD" } = $result_ref_[7];
          $$algodurfilter_to_indc_to_corr_{ $algodurfilter_key_ }{ $indc_ }{ "MIN" } = $result_ref_[8];
          $$algodurfilter_to_indc_to_corr_{ $algodurfilter_key_ }{ $indc_ }{ "MAX" } = $result_ref_[9];
        }

        if ( $result_ref_[10] ne "None" && $#result_ref_ >= 13 && defined $algodurfilter_to_indc_to_tailed_corr_) {
          $$algodurfilter_to_indc_to_tailed_corr_{ $algodurfilter_key_ }{ $indc_ }{ "AVG" } = $result_ref_[10];
          $$algodurfilter_to_indc_to_tailed_corr_{ $algodurfilter_key_ }{ $indc_ }{ "STD" } = $result_ref_[11];
          $$algodurfilter_to_indc_to_tailed_corr_{ $algodurfilter_key_ }{ $indc_ }{ "MIN" } = $result_ref_[12];
          $$algodurfilter_to_indc_to_tailed_corr_{ $algodurfilter_key_ }{ $indc_ }{ "MAX" } = $result_ref_[13];
        }

        if ( $#result_ref_ >= 14 && defined $algodurfilter_to_count_ref_ ) {
          $$algodurfilter_to_count_ref_{ $algodurfilter_key_ }{ $indc_ } = $result_ref_[14];
        } 
      }
    }
  }
}

sub get_full_cmd {
  my $exec_cmd = shift;
  my $dates_ref_ = shift;
  my $predalgo_ref_ = shift;
  my $preddur_ref_ = shift;
  my $filter_ref_ = shift;
  my $basepx_ref_ = shift;
  my $futpx_ref_ = shift;

  if ( $#$dates_ref_ >= 0 ) {
    $exec_cmd = $exec_cmd." -dt '".join(',', @$dates_ref_)."'";
  }
  if ( $#$predalgo_ref_ >= 0 ) {
    $exec_cmd = $exec_cmd." -pa '".join(',', @$predalgo_ref_)."'";
  }
  if ( $#$preddur_ref_ >= 0 ) {
    $exec_cmd = $exec_cmd." -pd '".join(',', @$preddur_ref_)."'";
  }
  if ( $#$filter_ref_ >= 0 ) {
    $exec_cmd = $exec_cmd." -f '".join(',', @$filter_ref_)."'";
  }
  if ( $#$basepx_ref_ >= 0 ) {
    $exec_cmd = $exec_cmd." -bp '".join(',', @$basepx_ref_)."'";
  }
  if ( $#$futpx_ref_ >= 0 ) {
    $exec_cmd = $exec_cmd." -fp '".join(',', @$futpx_ref_)."'";
  }
  return $exec_cmd;
}
