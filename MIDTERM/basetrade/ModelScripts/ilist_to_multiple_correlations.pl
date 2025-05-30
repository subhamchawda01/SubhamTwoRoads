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
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
  $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo


# start 
my $SCRIPTNAME="$0";

my $USAGE="$0 ilist-file YYYYMMDD start_hhmm end_hhmm dmsecs dl1events dnumtrades use_eco min_price_increment norm-algo filters(csv) pred-duration(csv) [if fv filter, then daily_volume_file_path should be provided] [fsudm_level] [tail_indc_corr(csv)]";


if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }
my $arg_indx = 0;
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
my @datagen_output_lines_ = `$exec_cmd`;

foreach my $datagen_line_ ( @datagen_output_lines_ )
{
  if ( index ( $datagen_line_ , "Less Space for Datagen" ) >= 0 )
  {
    PrintStacktraceAndDie ( "Exiting because datagen exited with output : Less Space for Datagen" );
  }
}

if ( ! ExistsWithSize ( $time_data_file ) ) {
  PrintStacktraceAndDie ( "Exiting because datagen could not make non-zero sized $time_data_file" );
}

#scale @pred_duration for the desired algo
my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( "" , $pred_duration[0], $norm_algo, $time_data_file);
my @pred_duration_scaled_arr = map { int ( $_ * $this_pred_counters_ / $pred_duration[0] ) } @pred_duration;

#check if model file exists
#check if timed_data file exists

$exec_cmd = $LIVE_BIN_DIR."/timed_data_to_corr_record $ilist_file $time_data_file $min_px_incr $norm_algo ".
            (1+$#filters)." ".join(" ", @filters)." ".(1+$#pred_duration)." ".join(" ", @pred_duration_scaled_arr)." $trd_vol_file $fsudm_level_" ;

if ( $#tails_corr >= 0 ) {
  $exec_cmd .= " ".(1+$#tails_corr)." ".join(" ", @tails_corr);
}

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

#assert ( $num_pred_dur * $num_filters == $#corr_output +1);

my @ind_list=`grep -w "^INDICATOR" $ilist_file`;
for (my $i=0; $i <= $#corr_output; $i++)
{
  my @corr_output_words = split ( /\s+/, $corr_output[$i]);

  my $tail_str_ = "";
  if ($#ind_list == $#corr_output_words - 3 && $corr_output_words[0] =~ /^tail/) {
    $tail_str_ = shift @corr_output_words;
    $tail_str_ .= " ";
  }

  next if ($#ind_list != $#corr_output_words - 2);

#the following logic is based on the output generated by the cpp exec timed_data_to_corr_record
  my $filter_indx = ($i % $num_filters) ;
  my $pred_dur_indx = int (($i / $num_filters) % $num_pred_dur) ;

  my $pred_dur = $pred_duration[$pred_dur_indx]; #$corr_output_words[0];
  my $filter = $filters[$filter_indx];#corr_output_words[1];

  my $str="";
  for (my $k =0; $k<=$#ind_list; $k++){
    my @ind_words = split ( /\s+/, $ind_list[$k] );
    $ind_words[1] = $corr_output_words[$k + 2];
    $ind_words[0] = $yyyymmdd." ".$ind_words[0];

    $str = $str.$tail_str_.$filter." ".$pred_dur." ".join(" ", @ind_words)."\n";
  }

  print $str ;
}

`rm -rf $time_data_file`;
