#!/usr/bin/perl
#use strict;
#use warnings;
use FileHandle;

#initialise paths:
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $REPO="basetrade";
my $bt_directory_ = $HOME_DIR."/".$REPO;

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir

#user inputs
my $USAGE="$0 portfile_ enddate_ lookback_days_ start_time_ end_time_ sampling_ms_ v1_ v2_ v3_ pred_duration_ pred_algo_ pred_filter_ output_directory_";

my $portfile_  		= $ARGV [ 0 ];
my $enddate_  		= $ARGV [ 1 ];
my $lookback_days_  	= $ARGV [ 2 ];
my $start_time_  	= $ARGV [ 3 ];
my $end_time_  		= $ARGV [ 4 ];
my $sampling_ms_  	= $ARGV [ 5 ];
my $v1_  		= $ARGV [ 6 ];
my $v2_  		= $ARGV [ 7 ];
my $v3_  		= $ARGV [ 8 ];
my $pred_duration_  	= $ARGV [ 9 ];
my $pred_algo_  	= $ARGV [ 10 ];
my $pred_filter_  	= $ARGV [ 11 ];
my $output_directory_  	= $ARGV [ 12 ];
	
if ( $#ARGV < 12 ) { print $USAGE."\n"; exit ( 0 ); }


#this function creates a temp directory /spare/local/eigengen/uniqueid/
#the contents of this directory are deleted after the run
sub CreateWorkDirectory
{
  my $unique_gsm_id_ = `date +%N`;
  chomp ( $unique_gsm_id_ );
  $unique_gsm_id1_ = int($unique_gsm_id_) + 0;
  my $SPARE_LOCAL="/spare/local/";
  my $hostname_ = `hostname`;
  if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
  {
    $SPARE_LOCAL = GetSpareLocalDir();
  }
  my $work_dir_ = $SPARE_LOCAL."eigengen";
  $work_dir_ = $work_dir_."/".$unique_gsm_id1_."/";

  if ( -d $work_dir_ ) { `rm -rf $work_dir_`; }
  `mkdir -p $work_dir_ `; #make_work_dir
  return $work_dir_;
}


#this function creates an ilist with 30 sec SimpleReturns indicators for all the components of portfolio
sub CreateIlist
{
  my ($workdir_) = @_;
  my $prod_ = `cat $portfile_`;
  my @prod_array_ = split '\n', $prod_;

  my $ilist_ = $workdir_."ilist";
  open ILIST, "> $ilist_" or PrintStacktraceAndDie ( "Could not open $ilist_ for writing\n" );
  print ILIST "MODELINIT DEPBASE NSE_$prod_array_[0]_FUT0 Midprice Midprice\n";
  print ILIST "MODELMATH LINEAR CHANGE\n";
  print ILIST "INDICATORSTART\n";
  for( my $i = 0; $i<= $#prod_array_; $i++ )
  {
    print ILIST "INDICATOR 1.00 SimpleReturns NSE_$prod_array_[$i]_FUT0 30 Midprice\n"
  }
  print ILIST "INDICATOREND\n";
}


my $work_dir_ = CreateWorkDirectory(); #create the work directory first
#print "$work_dir_\n";
CreateIlist($work_dir_);  #create ilist in the work directory

my $prod_ = `cat $portfile_`;
my @prod_array_ = split '\n', $prod_;

#get the previous day for the product, we wont take the same day data
my $last_two_ = `$bt_directory_/scripts/get_list_of_dates_for_shortcode.pl NSE_$prod_array_[0]_FUT0 $enddate_ 2`;
my @last_two_days_array_ = split ' ', $last_two_;
my $last_day_ = $last_two_days_array_[1];

my $cmd_ = $bt_directory_."/scripts/get_regdata.py NSE_$prod_array_[0]_FUT0 $work_dir_/ilist $last_day_ $lookback_days_ $start_time_ $end_time_ $sampling_ms_ $v1_ $v2_ $v3_ $pred_duration_ $pred_algo_ $pred_filter_ $work_dir_/regdata/";
#print "$cmd_\n";
`$cmd_`;


if ( not( -d $output_directory_ ))
{
  `mkdir -p $output_directory_`;
}
my $cmd1_ = $bt_directory_."/scripts/generate_eigenmatrix.R $work_dir_/regdata/catted_regdata_outfile $output_directory_/$enddate_";
`$cmd1_`;


`rm -rf $work_dir_`;
