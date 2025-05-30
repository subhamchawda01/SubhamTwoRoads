# \file GenPerlLib/filter_combinable_strategies_for_date.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 353, Evoma, #14, Bhattarhalli,
#        Old Madras Road, Near Garden City College,
#        KR Puram, Bangalore 560049, India
#        +91 80 4190 3551
#

use strict;
use warnings;
use POSIX;
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";
my $MODELLING_BASE_DIR=$HOME_DIR."/modelling";
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELLING_WF_STRATS_DIR=$MODELLING_BASE_DIR."/wf_strats";
my $WF_SCRIPTS_DIR=$HOME_DIR."/".$REPO."/walkforward";


require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; #GetBasepxStratFirstModel GetOmixFileStratFirstModel
require "$GENPERLLIB_DIR/is_strat_valid.pl"; #IsStratValid

sub GetCombinableKeyForStratContent
{
  my $strat_ = shift;
  my $strategy_content_ = shift;
  my $use_starttime_ = shift;
  $use_starttime_ = 1 if ! defined $use_starttime_;
  my $t_key_;

  my @strategy_words_ = split(' ', $strategy_content_);
  if ( $strategy_words_[0] eq "STRUCTURED_STRATEGYLINE" ) {
    $t_key_ = basename($strat_);
  }
  elsif ( $#strategy_words_ > 6 ) {
    my $execname_ = $strategy_words_[2];
    my $modelname_ = $strategy_words_[3];
    my $start_time_ = $strategy_words_[5];
    my $evtoken_ = $strategy_words_[8];
    $evtoken_ = "INVALID" if ! defined $evtoken_;

    if ($execname_ =~ /MeanRevertingTrading/) {
      $t_key_ = GetShcListKeyForMRTModel($modelname_);
    }
    else { 
      my $basepx_ = GetBasepxModel($modelname_);
      my $omix_file_ = GetOmixFileModel($modelname_);
      my $online_mix_file_ = GetOnlineMixFileModel($modelname_);
      $t_key_ = $omix_file_.$online_mix_file_;
    }

    $t_key_ = $t_key_.$evtoken_;
    $t_key_ = $start_time_.$t_key_ if $use_starttime_ eq 1;
  }

  return $t_key_;
}

sub GetCombinableKeyForStrat
{
  my $strat_ = shift;
  my $use_starttime_ = shift;
  $use_starttime_ = 1 if ! defined $use_starttime_;
    
  my $strategy_content_ = `cat $strat_ | grep -v "^#" | head -1`; chomp ( $strategy_content_);
  return GetCombinableKeyForStratContent($strat_, $strategy_content_, $use_starttime_);
}

sub FilterCombinableConfigsForDate
{
  my $stratvec_ref_ = shift;
  my $tradingdate_ = shift;
  my $use_starttime_ = shift;
  $use_starttime_ = 1 if ! defined $use_starttime_;
    
  my %empty_hash_ ;
  my $combine_key_to_files_ref_ = \%empty_hash_;

  foreach my $strat_ ( @$stratvec_ref_) {
    my $sbase_ = basename($strat_);
    my $exec_cmd_ = $WF_SCRIPTS_DIR."/print_strat_from_config.py -c $sbase_ -d $tradingdate_";
    my $strategy_content_ = `$exec_cmd_`; chomp ( $strategy_content_);
    if ($strategy_content_) {
      my $t_key_ = GetCombinableKeyForStratContent($strat_, $strategy_content_, $use_starttime_);
      push ( @{$$combine_key_to_files_ref_{$t_key_}}, $strat_);
    } else {
      print "Invalid Config for date $tradingdate_ $strat_\n";
    }
  }
  $combine_key_to_files_ref_;
}

sub FilterCombinableStrategies
{
  my $stratvec_ref_ = shift;
  my $tradingdate_ = shift;
  my $use_starttime_ = shift;
  $use_starttime_ = 1 if ! defined $use_starttime_;
    
  my %empty_hash_ ;
  my $combine_key_to_files_ref_ = \%empty_hash_;

  foreach my $strat_ ( @$stratvec_ref_) {
    my $strategy_content_ = `cat $strat_ | grep -v "^#" | head -1`; chomp ( $strategy_content_);
    if ($strategy_content_) {
      my $t_key_ = GetCombinableKeyForStratContent($strat_, $strategy_content_, $use_starttime_);
      push ( @{$$combine_key_to_files_ref_{$t_key_}}, $strat_);
    } else {
      print "Invalid Strategy $strat_\n";
    }
  }
  $combine_key_to_files_ref_;
}
