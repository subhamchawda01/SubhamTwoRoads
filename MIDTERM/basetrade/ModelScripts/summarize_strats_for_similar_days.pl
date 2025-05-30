#!/usr/bin/perl
use strict;
use warnings;
use List::Util qw /min max/;
use Fcntl qw (:flock);

use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $TEMP_DIR="/spare/local/temp";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

my $USAGE = "<script> shc strats_dir/strats_list_file result_dir trading_start_yyyymmdd [trading_target_yyyymmdd=TODAY] [similar_days_percentile=0.1] [sortalgo=kCNAPnlAdjAverage] [Features_Config=DEFAULT] [Arima_Config=ARIMA_DEF] [skip_file=INVALIFILE]\n";

if ( $#ARGV < 3 )
{
  print $USAGE;
  exit (0);
}

my $shc_ = shift; 
my $strats_dir_ = shift;
my $result_dir_ = shift;
my $start_date_ = GetIsoDateFromStrMin1(shift);
my $target_date_ = `date  +%Y%m%d`; chomp($target_date_);
my $similarity_factor_ = 0.1;
my $sort_fn_ = "kCNAPnlAdjAverage";
#my $features_config_orig_ = "/spare/local/tradeinfo/day_features/dayfeatures_config.txt";
#my $arima_config_orig_ = "/spare/local/tradeinfo/day_features/dayfeatures_arima_param";
my $features_config_ = "";
my $arima_config_ = "ARIMA_DEF";
my $skip_file_ = "INVALIFILE";
if ( $#ARGV >= 0  ) { $target_date_ = GetIsoDateFromStrMin1(shift); }
if ( $#ARGV >= 0  ) { $similarity_factor_ = shift; }
if ( $#ARGV >= 0  ) { $sort_fn_ = shift; }
if ( $#ARGV >= 0 && $ARGV[0] ne "DEFAULT" ) { $features_config_ = shift; }
#if ( $features_config_ eq $features_config_orig_ ) { $arima_config_ = $arima_config_orig_; } 
if ( $#ARGV >= 0  ) { $arima_config_ = shift; } 
if ( $#ARGV >= 0  ) { $skip_file_ = shift; }

my $end_date_ = CalcPrevWorkingDateMult ( $target_date_, 1 );
my $cmd_ = $HOME_DIR."/".$REPO."/scripts/get_difference_between_dates.sh $start_date_ $end_date_";
print $cmd_."\n";
my $lookback_days_ = `$cmd_`; chomp ( $lookback_days_ );

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my $features_file_ = $TEMP_DIR."/wkodii_featuresdata_".$unique_gsm_id_;
my $generate_features_exec_ = $HOME_DIR."/".$REPO."/WKoDii/get_day_features.pl";
my $generate_features_cmd_ = "$generate_features_exec_ $shc_ $end_date_ $lookback_days_ $features_config_ > $features_file_";
`$generate_features_cmd_ 2>/dev/null`;
print $generate_features_cmd_."\n";

my $similarity_exec_ = "python ".$HOME_DIR."/".$REPO."/WKoDii/obtain_weights_on_days.py";
my $similarity_exec_cmd_ = "$similarity_exec_ $target_date_ -1 1 $features_file_ $arima_config_ Mahalanobis "; 
print $similarity_exec_cmd_."\n";
my @similarity_strings_ = `$similarity_exec_cmd_ 2>/dev/null`; chomp( @similarity_strings_ );

my %date2similarity_ = ( );
foreach my $sim_string_ ( @similarity_strings_ )
{
  my ( $t_date_, $t_similarity_weight_ ) = split( ' ', $sim_string_ );
  if ( $t_date_ >= $start_date_ && $t_date_ <= $end_date_ )
  {
    $date2similarity_{ $t_date_ } = $t_similarity_weight_;
  }
}

my @valid_dates_ = grep { $date2similarity_{ $_ } > 0 } keys %date2similarity_;
my @valid_dates_sorted_ = sort { $date2similarity_{ $a } <=> $date2similarity_{ $b } } keys %date2similarity_;

my $cutoff_idx_ = max( 0, min( (1 - $similarity_factor_) * @valid_dates_sorted_, $#valid_dates_sorted_ ) );

my @filtered_dates_ = @valid_dates_sorted_[ $cutoff_idx_..$#valid_dates_sorted_ ];
print "Cutoff of the Similarity Weights: ".$date2similarity_{ $filtered_dates_[0] }."\n";
print "Filtered Dates: ".join(' ', @filtered_dates_)."\n";

my $cstempfile_ = GetCSTempFileName ( $HOME_DIR."/cstemp" );
open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
print CSTF join("\n", @filtered_dates_)."\n";
close CSTF;

my $summ_exec_ = $HOME_DIR."/basetrade_install/bin/summarize_strategy_results";
my $summ_exec_cmd = "$summ_exec_ $shc_ $strats_dir_ $result_dir_ $start_date_ $end_date_ $skip_file_ $sort_fn_ 0 $cstempfile_ 0 2>/dev/null";
my @ssr_output_ = `$summ_exec_cmd`; chomp(@ssr_output_);

print join ( "\n", @ssr_output_ )."\n\n";
`rm -f $cstempfile_`;
`rm -f $features_file_`;
