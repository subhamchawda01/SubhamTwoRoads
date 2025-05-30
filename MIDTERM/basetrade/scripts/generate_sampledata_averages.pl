#!/usr/bin/perl

use strict ;
use warnings ;
use feature "switch"; # for given, when
use sigtrap qw(handler signal_handler normal-signals error-signals);
use File::Basename ;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";

require "$GENPERLLIB_DIR/sample_data_utils.pl";
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

if ( $#ARGV < 0 ) {
  print "USAGE: <script> <shortcode> <dest_dir> [<date>] [<recompute=0/1 (def:1)>] \n";
  exit(0);
}

my $shortcode_ = shift;
my $dest_dir_ = shift;
my $current_date_ = shift || `date +%Y%m%d`; chomp ( $current_date_ );
my $recompute_ = shift;
$recompute_ = 1 if ( ! defined $recompute_ );

$current_date_ = GetIsoDateFromStrMin1 ( $current_date_ );

my @numdays_vec_ = (120,180,250);

my $config_file_ = $MODELING_BASE_DIR."/samples_features_configs/".$shortcode_."_config.txt";

if ( ! -f $config_file_ ) { 
  print "Error: $config_file_ does not exist\n";
  exit(0);
}

my @feature_lines_ = `$BIN_DIR/check_samplefeatures_presence $config_file_ $current_date_ | cut -d' ' -f1`;
chomp ( @feature_lines_ );
@feature_lines_ = map { (split("/", $_))[-1] } @feature_lines_;
$_ =~ s/.txt//g foreach @feature_lines_;
push (@feature_lines_, "L1EventsPerSecond");

$dest_dir_ .= "/AvgSamples";
if ( ! -d $dest_dir_ ) { `mkdir $dest_dir_`; }

my @dates_vec_ = ( );
my %numdays_to_firstday_vec_ = ( );
my $longest_numdays_ = max ( @numdays_vec_ );

foreach my $numdays_ ( @numdays_vec_ ) {
  my $tdest_dir_ = $dest_dir_."/".$numdays_;
  if ( ! -d $tdest_dir_ ) { `mkdir $tdest_dir_`; }

  my @t_dates_vec_ = GetDatesFromNumDays( $shortcode_, $current_date_, $numdays_ );
  $numdays_to_firstday_vec_{ $numdays_ } = min ( @t_dates_vec_ );

  if ( $numdays_ == $longest_numdays_ ) {
    @dates_vec_ = @t_dates_vec_;
  }
}


foreach my $feature_ ( @feature_lines_ ) {
  my %date_feature_map_ = ( );

  if ( $recompute_ == 0 ) {
    my $all_file_exists_ = 1;
    foreach my $numdays_ ( @numdays_vec_ ) {
      my $fname_ = $dest_dir_."/".$numdays_."/".$feature_.".txt";
      if ( ! -s $fname_ ) { $all_file_exists_ = 0; last; }
    }
    next if ($all_file_exists_ == 1);
  }

  foreach my $date_ ( @dates_vec_ ) {
    %{ $date_feature_map_{ $date_ } } = ( );
    GetFeatureMap ( $shortcode_, $date_, $feature_, 0000, 2400, $date_feature_map_{ $date_ }, [] ); 
  }

  foreach my $numdays_ ( @numdays_vec_ ) {
    my $fname_ = $dest_dir_."/".$numdays_."/".$feature_.".txt";
    next if ( $recompute_ == 0 && -s $fname_ );
    my %feature_timeslots_map_ = ( );
    
    foreach my $date_ ( keys %date_feature_map_ ) {
      if ( $date_ >= $numdays_to_firstday_vec_{ $numdays_ } ) {
        push ( @{$feature_timeslots_map_{ $_ }}, $date_feature_map_{ $date_ }{ $_ } ) foreach keys %{ $date_feature_map_{ $date_ } };
      }
    }

    open OFHANDLE, "> $fname_" or PrintStacktraceAndDie ( "Could not open $fname_ for writing" );

    foreach my $t_slot_ ( sort { $a <=> $b } keys %feature_timeslots_map_ ) {
      my $tmins_ = $t_slot_ * 15;
      my $hhmm_ = sprintf("%04d", ( ( int( $tmins_ / 60 ) * 100 ) + ( $tmins_ % 60 ) ) );
      my $tavg_ = GetAverage ( \@{$feature_timeslots_map_{ $t_slot_ }} );

      print OFHANDLE $hhmm_." ".sprintf("%.4f",$tavg_)."\n"; 
    }

    close OFHANDLE;
  }
}

