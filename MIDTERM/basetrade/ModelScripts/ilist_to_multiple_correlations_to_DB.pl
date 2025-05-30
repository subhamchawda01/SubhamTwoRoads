#!/usr/bin/perl

# \file ModelScripts/td_to_multiple_correlation_records.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#      Suite No 353, Evoma, #14, Bhattarhalli,
#      Old Madras Road, Near Garden City College,
#      KR Puram, Bangalore 560049, India
#      +91 80 4190 3551
#
# This script is used at the end of generate_indicator_corr_records
# It takes input an ilist file, a_timed_data_file, set of filters, set of prediction_duration, trade_volume_file



use strict;
use warnings;
use feature "switch"; # for given, when
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;


my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$BIN_DIR;

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo
require "$GENPERLLIB_DIR/indstats_db_access_manager.pl"; # for InsertMultipleIndStats

# start 
my $SCRIPTNAME="$0";

my $USAGE="$0 shortcode timeperiod ilist-file YYYYMMDD start_hhmm end_hhmm dmsecs dl1events dnumtrades use_eco min_price_increment norm-algo filters(csv) pred-duration(csv) [if fv filter, then daily_volume_file_path should be provided] [fsudm_level] [tail_indc_corr(csv)]";


if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }
my $arg_indx = 0;
my $shortcode = $ARGV[$arg_indx ++];
my $timeperiod = $ARGV[$arg_indx ++];
my $ilist_file = $ARGV[$arg_indx ++];
my $yyyymmdd = $ARGV[$arg_indx ++];
my $start_hhmm = $ARGV[$arg_indx ++];
my $end_hhmm = $ARGV[$arg_indx ++];
my $dmsecs = $ARGV[$arg_indx ++];
my $dl1events = $ARGV[$arg_indx ++];
my $dnumtrades = $ARGV[$arg_indx ++];
my $use_eco = $ARGV[$arg_indx ++];
my $min_px_incr = $ARGV[$arg_indx ++];
my $norm_algo = $ARGV[$arg_indx ++];
my $filters_csv = $ARGV[$arg_indx ++];
my $pred_duration_csv = $ARGV[$arg_indx ++];
my $save_to_db_ = 1;

my $TEMPDIR = "/spare/local/temp/";
if ( ! -d $TEMPDIR ) { `mkdir -p $TEMPDIR`; }

my $trd_vol_file="INVALIDFILE";
if ($#ARGV >= $arg_indx)
{
    $trd_vol_file = $ARGV[$arg_indx ++];    
}

my $fsudm_level_ = 0 ;
if ($#ARGV >= $arg_indx)
{
    $fsudm_level_ = $ARGV[$arg_indx ++];    
}

my $tails_corr_csv = "";
if ($#ARGV >= $arg_indx)
{
  $tails_corr_csv = $ARGV[$arg_indx ++];
}

$filters_csv =~ s/\s+//g ;
my @filters = split( ',', $filters_csv) ;
 
$pred_duration_csv=~ s/\s+//g ;
my @pred_duration = split( ',', $pred_duration_csv) ;

$tails_corr_csv=~s/\s+//g ;
my @tails_corr = split( ',', $tails_corr_csv) ;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my $time_data_file = $TEMPDIR."/timed_data_".$yyyymmdd."_".$unique_gsm_id_;

my $exec_cmd="$LIVE_BIN_DIR/datagen $ilist_file $yyyymmdd $start_hhmm $end_hhmm $unique_gsm_id_ $time_data_file $dmsecs $dl1events $dnumtrades $use_eco ADD_DBG_CODE -1 2>&1";
print $exec_cmd."\n";
my @datagen_output_lines_ = `$exec_cmd`;
my $retcode = $?;

if ( $retcode ne 0 ) {
  if ( grep { $_ =~ /Less Space for Datagen/ } @datagen_output_lines_ ) {
    print STDERR "Exiting because datagen exited with output : Less Space for Datagen\n";
  }
  PrintStacktraceAndDie ( join("\n", @datagen_output_lines_) );
}

if ( ! ExistsWithSize ( $time_data_file ) ) {
  print STDERR "Exiting because datagen could not make non-zero sized $time_data_file\n";
  PrintStacktraceAndDie ( join("\n", @datagen_output_lines_) );
}

#scale @pred_duration for the desired algo
my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( "" , $pred_duration[0], $norm_algo, $time_data_file);
my @pred_counters_ = map { int ( $_ * $this_pred_counters_ / $pred_duration[0] ) } @pred_duration;
my %pred_counters_to_durs_ = map { $pred_counters_[$_] => $pred_duration[$_] } 0..$#pred_counters_;

#check if model file exists
#check if timed_data file exists

$exec_cmd = $LIVE_BIN_DIR."/timed_data_to_corr_record $ilist_file $time_data_file $min_px_incr $norm_algo ".
            (1+$#filters)." ".join(" ", @filters)." ".(1+$#pred_duration)." ".join(" ", @pred_counters_)." $trd_vol_file $fsudm_level_" ;

if ( $#tails_corr >= 0 ) {
  $exec_cmd .= " ".(1+$#tails_corr)." ".join(" ", @tails_corr);
}
print $exec_cmd."\n";

my @corr_output=` $exec_cmd `;

my $num_filters=$#filters + 1;
if ( ! ($trd_vol_file eq "INVALIDFILE") )
{
  $num_filters ++;
  push ( @filters, "fv");
}
if ( $fsudm_level_ > 0 )
{
    $num_filters ++;
    push (@filters, "fsudm" );
}
my $num_pred_dur = $#pred_duration + 1;

my ( $basepx, $futpx );
my @ind_list = ( );

open IHANDLE, "< $ilist_file" or PrintStacktraceAndDie ( "Could not open $ilist_file for reading" );
while ( my $line = <IHANDLE> ) {
  chomp ( $line );
  my @lwords = split ( /\s+/, $line );

  if ( $lwords[0] eq "MODELINIT" && $#lwords >= 4 ) {
    $basepx = $lwords[3];
    $futpx = $lwords[4];
  } 
  elsif ( $lwords[0] eq "INDICATOR" && $#lwords >= 2 ) {
    push ( @ind_list, join(" ", @lwords[2..$#lwords]) );
  }
}
close IHANDLE;

#assert ( $num_pred_dur * $num_filters == $#corr_output +1);

my %filter_preddur_indicator_to_corr_ = ( );
my %filter_preddur_indicator_to_tailed_corr_ = ( );

for (my $i=0; $i <= $#corr_output; $i++)
{
  my @corr_output_words = split ( /\s+/, $corr_output[$i]);

  my $tail_ = 0;
  if ($#ind_list == $#corr_output_words - 3 && $corr_output_words[0] =~ /^tail/) {
    $tail_ = shift @corr_output_words;
    $tail_ =~ s/tail_//g;
  }

  next if ($#ind_list != $#corr_output_words - 2);

  my $pred_dur = $pred_counters_to_durs_{ shift @corr_output_words };
  my $filter = shift @corr_output_words;

  foreach my $k ( 0..$#ind_list ) {
    my $indc_ = $ind_list [$k];
    my $tcorr_ = $corr_output_words[$k];
#    print "tail_"."$tail_ $pred_dur $filter $indc_ $tcorr_\n";
    $tcorr_ = undef if $tcorr_ eq "nan";

    if ( $save_to_db_ ) {
      if ( $tail_ < 0.2 ) {
        $filter_preddur_indicator_to_corr_{ $filter }{ $pred_dur }{ $indc_ } = $tcorr_;
      }
      else {
        $filter_preddur_indicator_to_tailed_corr_{ $filter }{ $pred_dur }{ $indc_ } = $tcorr_;
      }
    }
    else {
      print "tail_".$tail_." ".$filter." ".$pred_dur." ".$indc_."\n";
    }
  }
}

if ( $save_to_db_ ) {
  foreach my $filter ( keys %filter_preddur_indicator_to_corr_ ) {
    foreach my $pred_dur ( keys %{ $filter_preddur_indicator_to_corr_{ $filter } } ) {
      InsertMultipleIndStats ( $shortcode, $timeperiod, $yyyymmdd, $pred_dur, $norm_algo, $filter, $basepx, $futpx, $filter_preddur_indicator_to_corr_{ $filter }{ $pred_dur }, $filter_preddur_indicator_to_tailed_corr_{ $filter }{ $pred_dur } );
    }
  }
}

`rm -rf $time_data_file`;
