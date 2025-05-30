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
if ( $USER eq "hagarwal" ) {
  my $MODELING_BASE_DIR = "/home/dvctrader/modelling";
}
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec 
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie 
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList 
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage
require "$GENPERLLIB_DIR/strat_utils.pl"; # IsStagedStrat
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; # FetchAllCorrelationForStrats

my $USAGE = "$0 SHORTCODE TIMEPERIOD";
if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }

my $shortcode_ = shift;
my $tperiod = shift;

my $fetch_configs_cmd_ = "$HOME_DIR/basetrade/walkforward/get_pool_configs.py -m POOL -shc $shortcode_ -tp $tperiod -type N";
my @strats_base_ = `$fetch_configs_cmd_ 2>/dev/null`; chomp ( @strats_base_ );

if ( $#strats_base_ < 0 ) {
  print "No configs in the pool $shortcode_ $tperiod\n";
  exit(0);
}
print "No. of configs: ".($#strats_base_+1)."\n";

my %similarity_map_ = ( );
FetchAllCorrelationForStrats ( \@strats_base_, \%similarity_map_ );

my @similarity_scores_ = values %similarity_map_;

my $num_scores_ = scalar @similarity_scores_;
if ( $num_scores_ <= 0 ) {
  print "No correlations present for pool $shortcode_ $tperiod\n";
  exit(0);
}
print "No. of correlation pairs: $num_scores_\n";

my $smedian_ = GetMedianAndSort ( \@similarity_scores_ );
my $smean_ = GetAverage ( \@similarity_scores_ );
print "Mean: ".$smean_.", Median_: ".$smedian_."\n";

my $quantile_str_ = "";
foreach my $t_quantile_ ( 0..10 ) {
  my $t_percent_ = $t_quantile_ * 10;
  my $t_idx_ = int( $t_quantile_ / 10.0 * $num_scores_ );
  if ( $t_idx_ != 0 ) { $t_idx_ -= 1; }
  my $t_value_ = $similarity_scores_[$t_idx_];
  $quantile_str_ = $quantile_str_.$t_percent_."\%:".$t_value_."\n";
}
print $quantile_str_;
