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
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/sample_pnl_corr_utils.pl";
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats

my $USAGE = "$0 SHORTCODE [STRAT_DIR/-1 for all SHC dirs] [DATE=TODAY-1] [NUM_DAYS=150] [0/1:Update_existing_results] [RESULT_DIR] ";
if ( $#ARGV < 2 ) { print "$USAGE\n"; exit ( 0 ); }

my $shortcode_ = shift;
my $strat_dir_ = shift || -1;
my $yyyymmdd_ = shift || "TODAY-1";
$yyyymmdd_ = GetIsoDateFromStrMin1($yyyymmdd_);
my $num_days_ = shift || 150;
my $update_existing_ = shift || 1;
if ( $update_existing_ != 0 && $update_existing_ != 1 ) {
  print "Error: Update_existing_results has to be 0 or 1.. Exiting..\n";
  exit(1);
}

my $results_dir_ = "/spare/local/tradeinfo/StratsCorrelation/".$shortcode_;
my $sync2ny11_ = 1;
if ( @_ > 0 ) {
  $results_dir_ = shift;
  $sync2ny11_ = 0;
}

if ( ! -d $results_dir_ ) { `mkdir -p $results_dir_`; }

my @strats_dirs_vec_ = ( );
if ( $strat_dir_ eq "-1" ) {
  my $base_dir_ = $MODELING_STRATS_DIR."/".$shortcode_;
  if ( -d $base_dir_ ) {
    if ( opendir my $dh, $base_dir_ ) {
      while ( my $t_item_ = readdir $dh)
      {
        next if $t_item_ eq '.' || $t_item_ eq '..';
        if ( -d "$base_dir_/$t_item_" ) {
          push ( @strats_dirs_vec_, $base_dir_."/".$t_item_ );
        }
      }
      closedir $dh;
    }
  }
} else {
  push ( @strats_dirs_vec_, $strat_dir_ );
}


foreach my $this_strat_dir_ ( @strats_dirs_vec_ ) {
  print "Generating Strategy Correlations for: ".$this_strat_dir_."\n";
  my $strat_dir_basename_ = basename ( $this_strat_dir_ );
  my $outfile_ = $results_dir_."/similarity_table_".$strat_dir_basename_;
  my $strat_idx_file_ = $results_dir_."/stratlist_".$strat_dir_basename_;

  my @all_strats_in_dir_ = MakeStratVecFromDir($this_strat_dir_); 

  my %idx2strat_present_ = ( );
  my %similarity_matrix_ = ( );

  if ( $update_existing_ == 0 
      && -f $strat_idx_file_
      && -f $outfile_ ) {
    open INFILE, "< $strat_idx_file_";
    my @infile_lines_ = <INFILE>; chomp ( @infile_lines_ );
    foreach my $infile_line_ ( @infile_lines_ ) {
      my @tokens_ = split ( ' ', $infile_line_ ); chomp ( @tokens_ );
      if ( $#tokens_ >= 1 ) { 
        $idx2strat_present_ { $tokens_[0] } = $tokens_[1];
      }
    }
    close INFILE;

    open INFILE, "< $outfile_";
    @infile_lines_ = <INFILE>; chomp ( @infile_lines_ );
    foreach my $infile_line_ ( @infile_lines_ ) {
      my @tokens_ = split ( ' ', $infile_line_ ); chomp ( @tokens_ );
      if ( $#tokens_ >= 2 ) {
        if ( exists $idx2strat_present_ { $tokens_[0] } && exists $idx2strat_present_ { $tokens_[1] } ) {
          my ($strat1_, $strat2_) = sort ( $idx2strat_present_{$tokens_[0]}, $idx2strat_present_{$tokens_[1]} );
          $similarity_matrix_ { $strat1_ } { $strat2_ } = $tokens_[2];
        }
      }
    }
  }

  my @strats_vec_ = ( );
  my @strats_basenames_vec_ = ( );

  foreach my $full_strat_filename_ ( sort @all_strats_in_dir_ )
  {
    my $strat_basename_ = basename ( $full_strat_filename_ );
    if ( !( FindItemFromVec ( $strat_basename_, @strats_basenames_vec_ ) ) )
    {
      push ( @strats_vec_, $full_strat_filename_ );
      push ( @strats_basenames_vec_, $strat_basename_ );
    }
  }
#print "Strats:\n".join("\n", @all_strats_in_dir_ )."\n\n";

  my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
  my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shortcode_ $yyyymmdd_ $num_days_";
  my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
  my @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );
#print "Dates: ".join(' ' , @dates_vec_ )."\n";

  my %sample_pnls_strats_vec_;

  FetchPnlSamplesStrats ( $shortcode_, \@strats_basenames_vec_, \@dates_vec_, \%sample_pnls_strats_vec_ );

  open OUTFILE, "> $strat_idx_file_";
  foreach my $strat1_idx_ ( 0..($#strats_basenames_vec_-1) ) {
    print OUTFILE $strat1_idx_." ".$strats_basenames_vec_[$strat1_idx_]."\n";
  }
  close OUTFILE;

  open OUTFILE, "> $outfile_";
  foreach my $strat1_idx_ ( 0..($#strats_basenames_vec_-1) ) {
    foreach my $strat2_idx_ ( ($strat1_idx_+1)..$#strats_basenames_vec_ ) {
      my ($strat1_, $strat2_) = ($strats_basenames_vec_[$strat1_idx_], $strats_basenames_vec_[$strat2_idx_]);
      if ( ! exists $similarity_matrix_ { $strat1_ } { $strat2_ } ) {
        $similarity_matrix_ { $strat1_ } { $strat2_ } = GetPnlSamplesCorrelation ( $strat1_, $strat2_, \%sample_pnls_strats_vec_ );
      }
      print OUTFILE $strat1_idx_."\t".$strat2_idx_."\t".$similarity_matrix_{ $strat1_ }{ $strat2_ }."\n";
    }
  }
  close OUTFILE;

  if ( $sync2ny11_ == 1 ) {
    my $outfile_sync_cmd_ = "rsync $outfile_ dvctrader\@10.23.74.51:$outfile_";
    my $stratlist_sync_cmd_ = "rsync $strat_idx_file_ dvctrader\@10.23.74.51:$strat_idx_file_";
    print "$outfile_sync_cmd_\n$stratlist_sync_cmd_\n";
    `ssh dvctrader\@10.23.74.51 mkdir -p $results_dir_ 2>/dev/null`;
    `$outfile_sync_cmd_`;
    `$stratlist_sync_cmd_`;
  }
}
