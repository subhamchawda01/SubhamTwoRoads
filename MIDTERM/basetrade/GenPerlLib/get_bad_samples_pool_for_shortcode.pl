# \file GenPerlLib/get_bad_days_for_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use feature "switch";

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $STORE="/spare/local/tradeinfo";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
if ( $USER eq "ankit" || $USER eq "kputta" || $USER eq "rkumar" || $USER eq "diwakar" )
{
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec_ref.pl"; #FindItemFromVecRef
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/array_ops.pl";
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats, PnlSamplesGetStats

sub GetBadSamplesPoolForShortcode
{
  my $shortcode_ = shift;
  my $timeperiod_ = shift;
  my $dates_vec_ref_ = shift;
  my ($array_ref) = shift;
  my $samples_percentage_ = shift || 0.2;
  my $strats_cutoff_frac_ = 0.8;
#  my $start_date_ = CalcNextWorkingDateMult ( $yyyymmdd_, $num_days_ );

#  print "GetBadSamplesPoolForShortcode ".$shortcode_." $timeperiod_ $start_date_ $num_days_\n";

  my @all_strats_in_dir_ = MakeStratVecFromDirAndTT($MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_);
  my @all_strats_basenames_ = map { basename ( $_ ) } @all_strats_in_dir_;

  my %sample_pnls_strats_vec_;
  FetchPnlSamplesStrats ( $shortcode_, \@all_strats_basenames_, $dates_vec_ref_, \%sample_pnls_strats_vec_ );

   my %strat2sharpe_ = ();
   my %strat2avgDD_ = ();
   my %samples_vec_ = ();
  
  foreach my $t_strat_name_ ( @all_strats_basenames_ ) {
    my %t_stats_map_;
    PnlSamplesGetStats ( \%{$sample_pnls_strats_vec_{$t_strat_name_}}, \%t_stats_map_, $dates_vec_ref_ );
    $strat2sharpe_{ $t_strat_name_ } = $t_stats_map_ { "SHARPE" };
    $strat2avgDD_{ $t_strat_name_ } = $t_stats_map_ { "DD_HIGHAVG" };
    
    foreach my $t_sample_ ( keys %{$sample_pnls_strats_vec_{$t_strat_name_}} ) {
      if ( ! exists $samples_vec_{ $t_sample_ } ) {
        $samples_vec_{ $t_sample_ } = 1;
      }
    }
  }

   my @strats_to_consider_ = grep { $strat2sharpe_{ $_ } > 0 } keys %strat2sharpe_;
   
   my %samples2scores_ = ();
   foreach my $t_sample_ ( keys %samples_vec_ ) {
     my %strat2pnl_dd_ = ();
     foreach my $strat_name_ ( @strats_to_consider_ ) {
       if ( exists $sample_pnls_strats_vec_{ $strat_name_ }{ $t_sample_ } ) {
         $strat2pnl_dd_{ $strat_name_ } = $sample_pnls_strats_vec_{ $strat_name_ }{ $t_sample_ } / ( 1 + $strat2avgDD_{ $strat_name_  } );
       }
     }
     my @strats_sorted_ = sort { $strat2pnl_dd_{ $a } <=> $strat2pnl_dd_{ $b } } keys %strat2pnl_dd_;

     my @sharpe_array_ = @strat2sharpe_{ @strats_sorted_ };
     my $sharpe_sum_ = GetSum ( \@sharpe_array_ );
     my $cutoff_sharpe_pos_ = $strats_cutoff_frac_  * $sharpe_sum_; 

     my $t_pos_ = 0;
     foreach my $t_strat_ ( @strats_sorted_ ) { 
       $t_pos_ += $strat2sharpe_{ $t_strat_ }; 
       if ( $t_pos_ > $cutoff_sharpe_pos_ ) { 
         $samples2scores_{ $t_sample_ } = $strat2pnl_dd_ { $t_strat_ };
         last;
       }
     }
#     print "score: $t_sample_ ".$samples2scores_{ $t_sample_ }."\n";
   }

   my @samples_sorted_ = sort { $samples2scores_{ $a } <=> $samples2scores_{ $b } } keys %samples_vec_;
#   print join(" ", @samples_sorted_)."\n";
   my $num_bad_samples_ = ceil ( @samples_sorted_ * $samples_percentage_ );
   @$array_ref = @samples_sorted_ [ 0..($num_bad_samples_-1) ];
}

1
