#!/usr/bin/perl

# \file scripts/get_px_diff_matrix.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# output_model_filename_  indicator_list_filename_  regression_output_filename_

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_numeric_value_from_line.pl"; # GetNumericValueFromLine ;
require "$GENPERLLIB_DIR/exists_with_size.pl";

# start 
my $USAGE="$0 shortcode buysell start_date num_days_lookback";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $buysell_ = $ARGV[1];
my $start_date_ = $ARGV[2];
my $num_days_lookback_ = $ARGV[3];

my $retail_trades_dir_ = "/NAS1/data/RetailLabelledTrades/";

my $exec_cmd_ = "~/basetrade/scripts/get_list_of_dates_for_shortcode.pl $shortcode_ $start_date_ $num_days_lookback_";

my $list_of_days_string_ =`$exec_cmd_`;

chomp($list_of_days_string_);

my @list_of_days_ = split ( ' ', $list_of_days_string_ ); 

my @trades_timestamps_ = ( );

foreach my $t_date_ ( @list_of_days_ )
{
  my $t_filename_ = $retail_trades_dir_.$t_date_;
  if ( ExistsWithSize ( $t_filename_ ) )
  {
    $exec_cmd_ = "cat $t_filename_ | grep \"$buysell_ $shortcode_\" | grep T | awk '{print \$2}'";
    my @trades_file_out_ = `$exec_cmd_`;    
    foreach my $trade_timestamp_ ( @trades_file_out_ )
    {
      chomp ( $trade_timestamp_);
      push ( @trades_timestamps_, $trade_timestamp_ );
    }
  }
}

my @list_of_shortcodes_ = ( "DI1F16", "DI1J16", "DI1N16", "DI1F17", "DI1J17", "DI1N17", "DI1F18", "DI1F19", "DI1F21", "DI1F23", "DI1F25" );
my $durations_ = " 2  5 10 20"; # used as an argument to another, hence script

print "Number of instances ".$#trades_timestamps_."\n";

print "SHC       2mins    5mins    10mins    20mins  ";


foreach my $t_shortcode_ ( @list_of_shortcodes_ )
{
  print "\n";
  print $t_shortcode_." ";
  my @sum_px_changes_ = ( );
  my @sum_sq_px_changes_ = ( );  
  my $count_ = 0;
  foreach my $timestamp_ ( @trades_timestamps_ )
  {     
    $exec_cmd_ = "~/basetrade/scripts/get_px_diff.pl $t_shortcode_ $timestamp_ $durations_";
    my @px_change_lines_ = `$exec_cmd_`;    
    for ( my $i =0; $i <= $#px_change_lines_; $i ++ )
    {
      my $t_px_change_ = $px_change_lines_[$i];
      chomp ( $t_px_change_);
      if ( $#sum_px_changes_ < $i ) 
      {
        push(@sum_px_changes_, 0 );
        push(@sum_sq_px_changes_, 0 );
      }
      
      $sum_px_changes_[$i] = $sum_px_changes_[$i] + $t_px_change_;
      $sum_sq_px_changes_[$i] = $sum_sq_px_changes_[$i] + $t_px_change_ * $t_px_change_;      
    }
    $count_ = $count_ + 1;
  }   
  for ( my $i = 0; $i <= $#sum_px_changes_; $i++ )
  {
    my $mean_px_change_ = $sum_px_changes_[$i] / $count_;
    my $mean_px_change_sq_ = $sum_sq_px_changes_[$i] / $count_;
    my $stdev_px_change_ = sqrt ( $mean_px_change_sq_ - $mean_px_change_ * $mean_px_change_ );
    my $sharpe_px_change_ = $mean_px_change_ / $stdev_px_change_;
    printf '%.3f ', $mean_px_change_;
    printf '%.3f , ', $sharpe_px_change_;
  } 
}
