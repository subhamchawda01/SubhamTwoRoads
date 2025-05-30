#!/usr/bin/perl
#
# It has utility functions for wf_configs
# Note: For most functionalities, it calls walkforward python scripts 
#

use strict;
use warnings;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $WF_SCRIPTS_DIR=$HOME_DIR."/".$REPO."/walkforward";

require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; #CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; #GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/date_utils.pl"; # GetUTCTime
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

sub IsValidConfig
{
  my $configname = shift;
  my $configid = GetConfigId($configname);
  if ( $configid > 0 ) {
    return 1;
  }

  return 0;
} 

sub AreValidConfigs
{
  my $cnames_ref_ = shift;

  foreach my $configname (@$cnames_ref_) {
    if ( IsValidConfig ( $configname ) ) {
      return 1;
    }
  }
  return 0;
}

sub GetConfigId
{
  my $configname = shift;
  my $configid = `$WF_SCRIPTS_DIR/is_valid_config.py -c $configname`; chomp($configid);
  $configid;
}

sub GetConfigJson
{
  my $config_ = shift;
  my $config_json_ = `$WF_SCRIPTS_DIR/process_config.py -c $config_ -m VIEW 2>/dev/null | grep -m1 ^CONFIG_JSON: | cut -d' ' -f2-`;
  chomp ( $config_json_ );
  return $config_json_;
}

sub GetConfigStartEndHHMM
{
  my $config_ = shift;
  my @start_end_times_ = `$WF_SCRIPTS_DIR/process_config.py -c $config_ -m VIEW 2>/dev/null | grep -m2 '^START_TIME\\\|^END_TIME' | awk '{print \$2}'`;
  chomp ( @start_end_times_ );
  my ($start_time, $end_time) = @start_end_times_[0..1];
  return ($start_time, $end_time);
}


