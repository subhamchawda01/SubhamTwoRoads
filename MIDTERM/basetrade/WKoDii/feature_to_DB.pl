#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; #CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; #GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_date.pl"; #CalcPrevDate
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromStartDate
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl"; #FetchSingleStory InsertStory
require "$GENPERLLIB_DIR/pool_utils.pl";

my $USAGE="<script> shc feature_file(INVALIDFILE/path_to_file) CONFIG/POOL/ALL single-config/timeperiod update_existing=0/1\n";

if ($#ARGV < 2)	{ print $USAGE."\n"; exit(1); }
my $shc_ = shift;
my $feature_file_ = shift;
my $config_or_pool_ = shift;
my $config_ = shift;
my $update_existing_ = shift || 0;

if ( $config_or_pool_ ne "CONFIG" and $config_or_pool_ ne "POOL" and $config_or_pool_ ne "ALL" ) {
  print $USAGE."\n"; exit(1);
}

my $date_ = GetIsoDateFromStrMin1 ( "TODAY" );
my $num_days_ = 400;
my $address_lookahead_ = "F";
my $add_to_db_ = "T";

my @strat_files_ = ( );
if ( $config_or_pool_ eq "CONFIG" ) {
  push ( @strat_files_, basename( $config_ ) );
}
elsif ( $config_or_pool_ eq "ALL" ) {
  my @pools_ = GetPoolsForShortcode ( $shc_ );
  foreach my $pool_ ( @pools_ ) {
    my @t_configs_ = GetConfigsForPool ($shc_, $pool_);
    push (@strat_files_, @t_configs_);
  }
}
else {
  @strat_files_ = GetConfigsForPool ($shc_, $config_);
}

my %strat_stories_ = ();
FetchAllStories($shc_, \%strat_stories_);

my @recent_cnames_ = ();
FetchRecentStoriesCnames($shc_, \@recent_cnames_);
my %recent_configs_map_ = map {$_ => 1} @recent_cnames_;

my $exchange_ = `$BINDIR/get_contract_specs $shc_ $date_ EXCHANGE | cut -d' ' -f2`;
if ( defined $exchange_ && $exchange_ =~ /MICEX/ ) {
  my @dates_vec_ = GetDatesFromStartDate ( $shc_, "20160601", $date_ );
  $num_days_ = scalar @dates_vec_;
}

if ( $feature_file_ eq "INVALIDFILE" || $feature_file_ eq "IF" ) {
  my $t_feature_file_ = "/spare/local/tradeinfo/day_features/stratstory_configs/".$shc_."_config.txt";
  $feature_file_ = $t_feature_file_ if -f $t_feature_file_;
}

foreach my $sname_ ( @strat_files_ ) {
  if ( $update_existing_ == 1 || (! defined $strat_stories_{$sname_}{ "SHARPE" } || ! defined $recent_configs_map_{$sname_}) ) {
    my $command_ = $HOME_DIR."/".$REPO."/WKoDii/features_story_pnl.R " .  $shc_ . " " . $feature_file_ . " " . $sname_. " " . $date_ . " " . $num_days_ . " " . $address_lookahead_ . " " .  $add_to_db_;
    print $command_."\n";
    my @dbargs_ = `$command_`; chomp ( @dbargs_ );

    if ( $#dbargs_ >= 0 ) {
      InsertStory( $shc_, $sname_, $dbargs_[0], $dbargs_[1] );
    }
  }
}
