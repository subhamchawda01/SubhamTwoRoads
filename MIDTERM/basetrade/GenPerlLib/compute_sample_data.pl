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
use Math::Complex ; # sqrt
my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $generic_tmp_dgen_filename_ = "/spare/local/".$USER."/tmp_dgen_sample";
my $generic_tmp_dgen_sample_filename_ = $generic_tmp_dgen_filename_."_short";
my $generic_tmp_reg_data_filename_ = "/spare/local/".$USER."/tmp_regdata_sample";
my $generic_indicator_list_ = "/spare/local/".$USER."/tmp_ilist";

sub GetL1szSamples {
  my $sample_map_ref_ = shift;
  my $shc_ = shift;
  my $date_ = shift;
  my $start_hhmm_ = sprintf ( "%04s", shift );
  my $end_hhmm_ = sprintf ( "%04s", shift ) ;

  my $sampling_mins_ = 15;
  if (@_ > 3 ) {
    $sampling_mins_ = shift;
  }
  my $start_mins_ = substr ( $start_hhmm_, 0, 2 ) * 60 + substr ( $start_hhmm_, 2, 2 ) ;
  my $end_mins_ = substr ( $end_hhmm_, 0, 2 ) * 60 + substr ( $end_hhmm_, 2, 2 ) ;
  my $prev_mins_ = $start_mins_;
  my $curr_mins_ = $start_mins_ + $sampling_mins_;

  while ( $curr_mins_ <= $end_mins_ ) {
    my $t_start_hhmm_ = sprintf ( "%02d%02d", int( $prev_mins_ / 60 ), int( $prev_mins_ % 60 ) );
    my $t_end_hhmm_ = sprintf ( "%02d%02d", int( $curr_mins_ / 60 ), int( $curr_mins_ % 60 ) );

    my $exec_cmd_ = $BIN_DIR."/get_avg_l1sz_on_day_using_mds_data $shc_ $date_ $t_start_hhmm_ $t_end_hhmm_ | awk '{print \$3}'";
    my $t_l1sz_ = `$exec_cmd_ 2>/dev/null`; chomp ($t_l1sz_);

    if ( $t_l1sz_ > 0 ) {
      $$sample_map_ref_ { $t_end_hhmm_ } = $t_l1sz_;
    }
    $prev_mins_ = $curr_mins_;
    $curr_mins_ = $curr_mins_ + $sampling_mins_;
  }
}

sub GetCorrSamples { 
  my $sample_map_ref_ = shift;
  my $shc_ = shift;
  my $indep_shc_ = shift;
  my $date_ = shift;
  my $start_hhmm_ = sprintf ( "%04s", shift );
  my $end_hhmm_ = sprintf ( "%04s", shift ) ;

  my $sampling_mins_ = 15;
  if (@_ > 3 ) {
    $sampling_mins_ = shift;
  }
  my $min_regd_lines_ = 10;
  my @pred_durations_ = (300); #(2,10,30,120,300);

  my @tmp_files_ = ( );

  my $start_mins_ = substr ( $start_hhmm_, 0, 2 ) * 60 + substr ( $start_hhmm_, 2, 2 ) ;
  my $end_mins_ = substr ( $end_hhmm_, 0, 2 ) * 60 + substr ( $end_hhmm_, 2, 2 ) ;

  my $tmp_dgen_filename_ = $generic_tmp_dgen_filename_."_".$shc_."_".$date_;
  my $tmp_dgen_sample_filename_ = $generic_tmp_dgen_sample_filename_."_".$shc_."_".$date_;
  my $tmp_reg_data_filename_ = $generic_tmp_reg_data_filename_."_".$shc_."_".$date_;
  my $indicator_list_ = $generic_indicator_list_."_".$shc_."_".$date_;
  {
    open IND_FILE, "> $indicator_list_" or PrintStacktraceAndDie ( "Could not open output_file_ $indicator_list_ for writing\n" );
    print IND_FILE "MODELINIT DEPBASE $shc_ OfflineMixMMS OfflineMixMMS\nMODELMATH LINEAR CHANGE\nINDICATORSTART\n";
    print IND_FILE "INDICATOR 1.00 SimpleTrend $shc_ 300 OfflineMixMMS\n";
    print IND_FILE "INDICATOR 1.00 SimpleTrend $indep_shc_ 300 OfflineMixMMS\n";
    print IND_FILE "INDICATOREND\n";
    close IND_FILE;
    push ( @tmp_files_, $indicator_list_ );
  }

  push ( @tmp_files_, $tmp_dgen_filename_ );
  push ( @tmp_files_, $tmp_dgen_sample_filename_ );
  push ( @tmp_files_, $tmp_reg_data_filename_ );
 
  my $uniq_id_ = 11111;
  my $exec_cmd_ = $LIVE_BIN_DIR."/datagen $indicator_list_ $date_ $start_hhmm_ $end_hhmm_ $uniq_id_ $tmp_dgen_filename_ 5000 0 0 0" ;
  `$exec_cmd_` ;
  if ( not ( -s $tmp_dgen_filename_ ) )
  {
    print "Datagen generation error: $exec_cmd_\n" ; 
    return;
  }

  my $prev_mins_ = $start_mins_;
  my $curr_mins_ = $start_mins_ + $sampling_mins_;
  while ( $curr_mins_ <= $end_mins_ ) {
    my $prev_msecs_ = $prev_mins_ * 60000;
    my $curr_msecs_ = $curr_mins_ * 60000;
    my $t_start_hhmm_ = sprintf ( "%02d%02d", int( $prev_mins_ / 60 ), int( $prev_mins_ % 60 ) );
    my $t_end_hhmm_ = sprintf ( "%02d%02d", int( $curr_mins_ / 60 ), int( $curr_mins_ % 60 ) );

    $exec_cmd_ = "awk '{ if ( \$1 > $prev_msecs_ && \$1 <= $curr_msecs_ ) { print \$_; } }' ".$tmp_dgen_filename_." > ".$tmp_dgen_sample_filename_;
    `$exec_cmd_`;
    my $regd_lines_ = `wc -l $tmp_dgen_sample_filename_ | awk '{print \$1}' `; chomp ( $regd_lines_ );
    if ( $regd_lines_ eq "" || $regd_lines_ < $min_regd_lines_ ) {
      $prev_mins_ = $curr_mins_; $curr_mins_ += $sampling_mins_;
      next;
    }

    my $tmp_indicators_regdata_filename_ = $tmp_dgen_sample_filename_."_inds" ;
    $exec_cmd_ = "cat $tmp_dgen_sample_filename_ | awk '{print \$(NF-1),\$NF}' > $tmp_reg_data_filename_" ;
    `$exec_cmd_` ;

    $exec_cmd_ = $LIVE_BIN_DIR."/get_dep_corr $tmp_reg_data_filename_" ;
    my $t_corr_ = `$exec_cmd_`; chomp($t_corr_);
    if ( ! looks_like_number ( $t_corr_ ) || $t_corr_ eq 'inf' || $t_corr_ eq '-inf' ) {
      $prev_mins_ = $curr_mins_; $curr_mins_ += $sampling_mins_;
      next;
    }
    $t_corr_ = $t_corr_ + 0.0;
    $$sample_map_ref_ { $t_end_hhmm_ } = $t_corr_;
    
    $prev_mins_ = $curr_mins_; $curr_mins_ += $sampling_mins_;
  }
  RemoveFiles( \@tmp_files_ );
}

sub GetStdevSamples {
  my $sample_map_ref_ = shift;
  my $shc_ = shift;
  my $date_ = shift;
  my $start_hhmm_ = sprintf ( "%04s", shift );
  my $end_hhmm_ = sprintf ( "%04s", shift ) ;

  my $sampling_mins_ = 15;
  if (@_ > 3 ) {
    $sampling_mins_ = shift;
  }
  my $min_regd_lines_ = 10;
  my @pred_durations_ = (300); #(2,10,30,120,300);

  my @tmp_files_ = ( );

  my $start_mins_ = substr ( $start_hhmm_, 0, 2 ) * 60 + substr ( $start_hhmm_, 2, 2 ) ;
  my $end_mins_ = substr ( $end_hhmm_, 0, 2 ) * 60 + substr ( $end_hhmm_, 2, 2 ) ;

  my $tmp_dgen_filename_ = $generic_tmp_dgen_filename_."_".$shc_."_".$date_;
  my $tmp_dgen_sample_filename_ = $generic_tmp_dgen_sample_filename_."_".$shc_."_".$date_;
  my $tmp_reg_data_filename_ = $generic_tmp_reg_data_filename_."_".$shc_."_".$date_;
  my $indicator_list_ = $generic_indicator_list_."_".$shc_."_".$date_;

#building empty ilist for stdev calculation of dependent
  {
    open IND_FILE, "> $indicator_list_" or PrintStacktraceAndDie ( "Could not open output_file_ $indicator_list_ for writing\n" );
    print IND_FILE "MODELINIT DEPBASE $shc_ OfflineMixMMS OfflineMixMMS\nMODELMATH LINEAR CHANGE\nINDICATORSTART\n";
    print IND_FILE "INDICATOREND\n";
    close IND_FILE;
    push ( @tmp_files_, $indicator_list_ );
  }

  my %tmp_dgen_dur_filename_ ;
  my $datagen_issues_ = 0;
  foreach my $duration_ (@pred_durations_)
  {
    $tmp_dgen_dur_filename_ { $duration_ }  = $tmp_dgen_filename_."_".$duration_;
    my $cmd_ = "$LIVE_BIN_DIR/datagen $indicator_list_ $date_ $start_hhmm_ $end_hhmm_ 11111 $tmp_dgen_dur_filename_{$duration_} $duration_ 1 0 0";
    `$cmd_`;
    if( not (-s $tmp_dgen_dur_filename_ { $duration_ }) )
    {	   
      $datagen_issues_ = 1;
      next;
    }
    else {
      push ( @tmp_files_, $tmp_dgen_dur_filename_ { $duration_ } );
    }
  }

  if ( $datagen_issues_ ) {
    print "Datagen could not be Generated.. Exiting..\n";
    RemoveFiles ( \@tmp_files_ );
    return;
  }

  push ( @tmp_files_, $tmp_dgen_sample_filename_ );
  push ( @tmp_files_, $tmp_reg_data_filename_ );

  my $prev_mins_ = $start_mins_;
  my $curr_mins_ = $prev_mins_ + $sampling_mins_;

  while ( $curr_mins_ <= $end_mins_ ) {
    my $prev_msecs_ = $prev_mins_ * 60000;
    my $curr_msecs_ = $curr_mins_ * 60000;
    my $t_end_hhmm_ = sprintf ( "%02d%02d", int( $curr_mins_ / 60 ), int( $curr_mins_ % 60 ) );

    my @stdev_row_vec_ = ( );
    my $stdev_line_ = $t_end_hhmm_." ";
    my $row_issue_ = 0;
    for my $duration_ ( keys %tmp_dgen_dur_filename_ ) {
      `rm $tmp_dgen_sample_filename_ $tmp_reg_data_filename_ 2>/dev/null`;
      my $cmd_ = "awk '{ if ( \$1 > $prev_msecs_ && \$1 <= $curr_msecs_ ) { print \$_; } }' ".$tmp_dgen_dur_filename_{$duration_}." > ".$tmp_dgen_sample_filename_;
      `$cmd_`;
      if ( not ( -s $tmp_dgen_sample_filename_ ) ) {
        $row_issue_ = 1;
        last;
      }
      my $regd_lines_ = `wc -l $tmp_dgen_sample_filename_ | awk '{print \$1}' `; chomp ( $regd_lines_ );
      if ( $regd_lines_ eq "" || $regd_lines_ < $min_regd_lines_ ) {
        $row_issue_ = 1;
        last;
      }
      $cmd_ = "$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_ $tmp_dgen_sample_filename_ $duration_ na_t1 $tmp_reg_data_filename_";
      `$cmd_`;
      if ( not ( -s $tmp_reg_data_filename_ ) ) {
        $row_issue_ = 1;
        last;
      }

      $cmd_ = "cat $tmp_reg_data_filename_ | awk '{sum1+=\$1; sum2+=(\$1 * \$1);}END{ print sqrt(sum2/NR - (sum1*sum1)/(NR*NR)) }'";
      my $t_stdev_ = `$cmd_`; $t_stdev_ += 0.0;
      push ( @stdev_row_vec_, $t_stdev_ );
    }
    if ( $row_issue_ == 0 ) {
      $$sample_map_ref_ { $t_end_hhmm_ } = join( ' ', @stdev_row_vec_ );
    }
    $prev_mins_ = $curr_mins_;
    $curr_mins_ += $sampling_mins_;
  }
  RemoveFiles ( \@tmp_files_ );
}


sub RemoveFiles
{
  my $tmp_files_ref_ = shift;
  for my $tmp_file_ ( @$tmp_files_ref_ ) {
    `rm $tmp_file_ 2>/dev/null`;
  }
}
