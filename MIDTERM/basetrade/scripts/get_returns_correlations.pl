#!/usr/bin/perl

# \file scripts/compute_prod_stdev.pl
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
use POSIX;
use List::Util qw[min max]; # max , min
use Scalar::Util qw(looks_like_number);

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $RETURNS_CORR_DIR="/spare/local/tradeinfo/datageninfo/returns_corr";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE =  "$0 prod_filename indep_shc_ date start_hhmm_utc end_hhmm_utc "; 
if ( $#ARGV < 4 ) { print "$USAGE\n"; exit ( 0 ); }

my $shc_list_file_ = $ARGV[0];
my $indep_shc_ = $ARGV[1] ;
my $date_ = $ARGV[2];
my $start_hhmm_ =  $ARGV[3] ;
my $end_hhmm_ = $ARGV[4]  ;

my $start_hhmm_str_ = $start_hhmm_ ;
my $end_hhmm_str_ = $end_hhmm_ ;

$start_hhmm_ = `$BIN_DIR/get_utc_hhmm_str $start_hhmm_ `; chomp ( $start_hhmm_ ) ;
$end_hhmm_ = `$BIN_DIR/get_utc_hhmm_str $end_hhmm_` ; chomp ( $end_hhmm_ ) ;
my $start_mins_ = substr ( $start_hhmm_, 0, 2 ) * 60 + substr ( $start_hhmm_, 2, 2 ) ;
my $end_mins_ = substr ( $end_hhmm_, 0, 2 ) * 60 + substr ( $end_hhmm_, 2, 2 ) ;

#print "Start-End: ".$start_mins_." ".$end_mins_."\n";


my $temporary_loc_ = "/tmp/";
my $server_ = `hostname`; chomp ( $server_);
if ( index ( $server_, "ip-10-0") >= 0 )
{ 
  $temporary_loc_ = "/media/shared/ephemeral16/"; 
} 
my $tmp_dgen_filename_ = $temporary_loc_."/tmp_dgen";
my $tmp_reg_data_filename_ = $temporary_loc_."/tmp_regdata";
my $tmp_indicators_regdata_filename_ = $temporary_loc_."/tmp_ind_regdata";

my $uniq_id_ = `date +%N`; chomp($uniq_id_);
`mkdir -p $RETURNS_CORR_DIR`;
open PRODUCT_LIST ,  "< $shc_list_file_ " or PrintStacktraceAndDie ( "Could not open shc list file $shc_list_file_ for reading \n" ) ;
my @product_list_ = <PRODUCT_LIST>; chomp ( @product_list_ ) ;
my $indicator_list_ = MakeIndicatorList ( @product_list_ );
my $exec_cmd_ = $LIVE_BIN_DIR."/datagen $indicator_list_ $date_ $start_hhmm_ $end_hhmm_ $uniq_id_ $tmp_dgen_filename_ 500 2 0 0" ;
`$exec_cmd_` ;
if ( not ( -s $tmp_dgen_filename_ ) )
{
  print "Datagen generation error: $exec_cmd_\n" ; 
  exit ( 0 );
}
$exec_cmd_ = $LIVE_BIN_DIR."/timed_data_to_reg_data $indicator_list_ $tmp_dgen_filename_ 32 na_e3 $tmp_reg_data_filename_" ;
`$exec_cmd_` ;

if ( not ( -s $tmp_reg_data_filename_ ) ) 
{
  print "TimedTDataToRegData generation error: $exec_cmd_\n" ;
  exit ( 0 ) ;
}
$exec_cmd_ = "$BIN_DIR/get_correlation_matrix $tmp_reg_data_filename_ ";
my @result_line_ = `$exec_cmd_`; chomp ( @result_line_ ) ;
if ( $#result_line_ != $#product_list_ + 2 ) 
{
 print "Correlations matrix num rows do not match with number of products $#result_line_   $#product_list_ \n";
 exit ( 0 ) ; 
}
my $result_file_ = "$RETURNS_CORR_DIR/returns_corr_$indep_shc_"."_$date_.txt";
my @existing_results_ = () ;
if  ( -s $result_file_ ) 
{
  open RESULT_FILE , "< $result_file_" or PrintStacktraceAndDie ( "Could not open file for $result_file_ reading \n" ) ;
  @existing_results_ = <RESULT_FILE>; chomp ( @existing_results_ ) ;
  close RESULT_FILE ;
}
my %prod_to_res_map_ = () ;
foreach my $line_ ( @existing_results_ ) 
{
  my @line_words_ = split ( /\s+/, $line_ ) ;
  if ( $#line_words_ >= 2 ) 
  {
    my $t_key_ = "$line_words_[0] $line_words_[1]";
    $prod_to_res_map_ {$t_key_} = $line_words_[2] ;
  }
}
for ( my $index_ = 2 ; $index_ <= $#result_line_; $index_++ )
{
  my @res_line_words_ = split ( /\s+/, $result_line_[$index_] ) ;
  if ( $#res_line_words_ >= 0  )
  {
    my $t_key_ = "$product_list_[$index_ -2 ] $start_hhmm_str_-$end_hhmm_str_" ;
    $prod_to_res_map_{$t_key_} = $res_line_words_[1]; 
  }
}
open RESULT_WRITE_FILE, "> $result_file_ " or PrintStacktraceAndDie ( "Could not open file $result_file_ for writing \n " ) ;
foreach my $key_ ( keys %prod_to_res_map_ ) 
{
  print RESULT_WRITE_FILE "$key_ $prod_to_res_map_{$key_}\n";
}
close ( RESULT_WRITE_FILE ) ;
`rm -rf $tmp_dgen_filename_ `; 
`rm -rf $tmp_reg_data_filename_`;
`rm -rf $tmp_indicators_regdata_filename_ `;
`rm -rf $indicator_list_`;
exit ( 0 ) ;

sub MakeIndicatorList 
{
  my $ilist_name_ = "/tmp/returns_correlations_ilist.txt" ;
  open ILIST_NAME, "> $ilist_name_ " or PrintStacktraceAndDie ( " cCOuld not opent he $ilist_name_ for writing \n" ) ;
  print ILIST_NAME "MODELINIT DEPBASE $indep_shc_ OfflineMixMMS OfflineMixMMS\n";
  print ILIST_NAME "MODELMATH LINEAR CHANGE\n";
  print ILIST_NAME "INDICATORSTART\n";    
  print ILIST_NAME "INDICATOR 1.00 SimpleReturns $indep_shc_ 300 MktSizeWPrice \n";
  foreach my $prod_ ( @product_list_ ) 
  {
    print ILIST_NAME "INDICATOR 1.00 SimpleReturns $prod_ 300 MktSizeWPrice \n";
  }
  print ILIST_NAME "INDICATOREND\n";
  close ILIST_NAME ;
  return $ilist_name_ ;
}
