# \file GenPerlLib/global_results_methods.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use List::Util qw/max min/; # for max

my $HOME_DIR=$ENV{'HOME'};
my $USER=$ENV{'USER'};
my $REPO="basetrade";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetMeanHighestQuartile
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl";
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # FetchStratsWithResults , FetchResults

my $GLOBALRESULTSDBDIR="DB";


# filename date 
use constant GlobalResultStratNameOffset => 0;
use constant GlobalResultDateOffset => 1;

# pnl volume 
use constant GlobalResultPnlOffset => 2;
use constant GlobalResultVolOffset => 3;

# supporting% bestlevel% agg% 
use constant GlobalResultSupOrderOffset => 4;
use constant GlobalResultBestLevelOffset => 5;
use constant GlobalResultAggOrderOffset => 6;

# Average_Abs_Pos 
use constant GlobalResultAvgAbsPosOffset => 8;

# Median_Trade_Close_Seconds  Avg_Trade_Close_Seconds 
use constant GlobalResultMedianTTCOffset => 9;
use constant GlobalResultAvgTTCOffset => 10;

#  Median_Trade_PNL  Avg_Trade_PNL Stdev_Trade_PNL  Sharpe_Trade_PNL  Fracpos_Trade_PNL 
use constant GlobalResultMedianTradePnlOffset => 11;
use constant GlobalResultAvgTradePnlOffset => 12;
use constant GlobalResultStdTradePnlOffset => 13;
use constant GlobalResultSharpePnlOffset => 14;
use constant GlobalResultFracPnlOffset => 15;

# MIN_PNL_OF_DAY  MAX_PNL_OF_DAY DD
use constant GlobalResultMinPnlOffset => 16;
use constant GlobalResultMaxPnlOffset => 17;
use constant GlobalResultMaxDDOffset => 18;

use constant DEBUG => 0;

sub GetGlobalResultsForShortcodeDate;

sub CombineGlobalResultsLineFromVec;

sub GetStratNameFromGlobalResultsLine;
sub GetPnlFromGlobalResultsLine;
sub GetMinPnlFromGlobalResultsLine;
sub GetVolFromGlobalResultsLine;
sub GetDateFromGlobalResultsLine;
sub GetMaxDDFromGlobalResultsLine;
sub GetMedianTTCFromGlobalResultsLine;
sub GetAvgTTCFromGlobalResultsLine;
sub GetSharpePnlFromGlobalResultsLine;
sub GetMinPnlFromGlobalResultsLine;
sub GetMaxPnlFromGlobalResultsLine;
sub GetAvgAbsPosFromGlobalResultsLine;

#these are faster
sub GetStratNameFromGlobalResultVecRef;
sub GetPnlFromGlobalResultVecRef;
sub GetMinPnlFromGlobalResultVecRef;
sub GetVolFromGlobalResultVecRef;
sub GetDateFromGlobalResultVecRef;
sub GetMaxDDFromGlobalResultVecRef;
sub GetMedianTTCFromGlobalResultVecRef;
sub GetAvgTTCFromGlobalResultVecRef;
sub GetSharpePnlFromGlobalResultVecRef;
sub GetMinPnlFromGlobalResultVecRef;
sub GetMaxPnlFromGlobalResultVecRef;
sub GetAvgAbsPosFromGlobalResultVecRef;

sub InvalidGlobalResult;

sub GetGlobalResultsForShortcodeDate
{
  my $shortcode_ = shift;
  my $yyyymmdd_ = shift;
  my $results_vec_vec_ref_ = shift;
  my $local_res_dir_ = shift || $GLOBALRESULTSDBDIR;
  my $db_res_type_ = shift || "N";
  my $fetch_from_wf_db_ = 1;
  #print "GetGlobalResultsForShortcodeDate $shortcode_ $yyyymmdd_ $strats_vec_ref_ $local_res_dir_ $db_res_type_ $fetch_from_wf_db_\n";
  
  @$results_vec_vec_ref_ = ();
  
  if ( $local_res_dir_ eq "DB" )
  {
    FetchResults ( $shortcode_, $yyyymmdd_, $results_vec_vec_ref_ , $db_res_type_, $fetch_from_wf_db_);
  }
  else
  {
    my $results_file_name_ = $local_res_dir_."/".$shortcode_."/".substr ( $yyyymmdd_ , 0 , 4 )."/".substr ( $yyyymmdd_ , 4 , 2 )."/".substr ( $yyyymmdd_ , 6 , 2 )."/results_database.txt";
    if ( ExistsWithSize ( $results_file_name_ ) )
    {
      open RESULTS_FILE , "< $results_file_name_ " or PrintStacktraceAndDie ( "Could not open $results_file_name_\n" );
      my @t_results_lines_vec_ = <RESULTS_FILE>;
      close RESULTS_FILE;
      chomp(@t_results_lines_vec_);
      foreach my $t_result_line_ ( @t_results_lines_vec_ )
      {
        my @t_result_vec_ = split ( " ", $t_result_line_ );
        push ( @$results_vec_vec_ref_, \@t_result_vec_ ); 
      }
    }
  }
  return ( $#$results_vec_vec_ref_ + 1 );
}

sub GetSummarizeResultsForStrat
{
  my ($shc_, $strat_, $end_date_, $numdays_, $resultline_vec_ref_, $skip_days_file_) = @_;
  $skip_days_file_ = "INVALIDFILE" if ! defined $skip_days_file_;
  my $start_date_ = $numdays_;
  if ( ! ValidDate( $start_date_ ) ) {
    $start_date_ = CalcPrevWorkingDateMult ( $end_date_, $numdays_, $shc_ );
  }

  my $cstemp_file_ = GetCSTempFileName ( "/spare/local/temp/" );
  open FHANDLE, "> $cstemp_file_" or PrintStacktrace ( "Could not open $cstemp_file_ for writing" );
  print FHANDLE $strat_."\n";
  close FHANDLE;

  my $exec_cmd_ = $HOME_DIR."/basetrade_install/bin/summarize_strategy_results $shc_ $cstemp_file_ DB $start_date_ $end_date_ $skip_days_file_ kCNAPnlAdjAverage 0 INVALIDFILE 0";
  my @exec_output_ = `$exec_cmd_ 2>/dev/null`; chomp ( @exec_output_ );
  `rm -f $cstemp_file_`;
  
  foreach my $exec_output_line_ ( @exec_output_ )
  {
    next if ( $exec_output_line_ eq "\n" || index ( $exec_output_line_ , "STRATEGYFILEBASE" ) >= 0 );
    
    my @lwords_ = split ( ' ' , $exec_output_line_ );
    next if ( $#lwords_ < 2 );

    if ( $lwords_[0] eq "STATISTICS" ) {
      return $exec_output_line_;
    }

    elsif ( ValidDate( $lwords_[0] ) ) {
      $$resultline_vec_ref_{ $lwords_[0] } = $exec_output_line_;
    }
  }
}

sub GetSummarizeResultsForStratDir
{
  my ($shc_, $stratdir_, $end_date_, $numdays_, $strat_to_resultline_vec_ref_, $strat_to_summary_ref_, $skip_days_file_) = @_;
  $skip_days_file_ = "INVALIDFILE" if ! defined $skip_days_file_;
  my $start_date_ = CalcPrevWorkingDateMult ( $end_date_, $numdays_, $shc_ );

  my $exec_cmd_ = $HOME_DIR."/basetrade_install/bin/summarize_strategy_results $shc_ $stratdir_ DB $start_date_ $end_date_ $skip_days_file_ kCNAPnlAdjAverage 0 INVALIDFILE 0";
#print $exec_cmd_."\n";
  my @exec_output_ = `$exec_cmd_ 2>/dev/null`; chomp ( @exec_output_ );
  `rm -f $cstemp_file_`;
  
  my $strat_name_;
  foreach my $exec_output_line_ ( @exec_output_ )
  {
    my @lwords_ = split ( ' ' , $exec_output_line_ );
    next if ( $#lwords_ < 1 );

    if ( $lwords_[0] eq "STRATEGYFILEBASE" ) { 
      $strat_name_ = $lwords_[1];
      next;
    }
    
    next if ( $#lwords_ < 2 || ! defined $strat_name_ );
    if ( $lwords_[0] eq "STATISTICS" ) {
      $$strat_to_summary_ref_{ $strat_name_ } = $exec_output_line_;
    }

    elsif ( ValidDate( $lwords_[0] ) ) {
      $$strat_to_resultline_vec_ref_{ $strat_name_}{ $lwords_[0] } = $exec_output_line_;
    }
  }
}

sub GetStratsWithGlobalResultsForShortcodeDate
{
  my $shortcode_ = shift;
  my $yyyymmdd_ = shift;
  my $strats_vec_ref_ = shift;
  my $local_res_dir_ = shift || $GLOBALRESULTSDBDIR;
  my $db_res_type_ = shift || "A";
  my $fetch_from_wf_db_ = 1;
  #print "GetStratsWithGlobalResultsForShortcodeDate $shortcode_ $yyyymmdd_ $strats_vec_ref_ $local_res_dir_ $db_res_type_ $fetch_from_wf_db_\n";
  
  @$strats_vec_ref_ = ();
  
  if ( $local_res_dir_ eq "DB" )
  {
    FetchStratsWithResults ( $shortcode_, $yyyymmdd_, $strats_vec_ref_, $db_res_type_, $fetch_from_wf_db_);
  }
  else
  {
    my $results_file_name_ = $local_res_dir_."/".$shortcode_."/".substr ( $yyyymmdd_ , 0 , 4 )."/".substr ( $yyyymmdd_ , 4 , 2 )."/".substr ( $yyyymmdd_ , 6 , 2 )."/results_database.txt";
    if ( ExistsWithSize ( $results_file_name_ ) )
    {
      open RESULTS_FILE , "< $results_file_name_ " or PrintStacktraceAndDie ( "Could not open $results_file_name_\n" );
      my @t_results_lines_vec_ = <RESULTS_FILE>;
      close RESULTS_FILE;
      chomp(@t_results_lines_vec_);
      foreach my $t_result_line_ ( @t_results_lines_vec_ )
      {
        my @t_result_vec_ = split ( " ", $t_result_line_ );
        push ( @$strats_vec_ref_, $t_result_vec_[0] ); 
      }
    }
  }
  return ( $#$strats_vec_ref_ + 1 );
}


sub CombineGlobalResultsLineFromVec
{
  my ( @global_results_lines_ ) = @_;

  if ( DEBUG ) { print "\n => CombineGlobalResultsLineFromVec\n" };

  my $t_sum_pnl_ = 0;
  my $t_sum_vol_ = 0;
  my $t_max_dd_ = 0;
  my @t_dd_vec_ = ( );
  my $t_date_ = 0;
  my $t_median_ttc_ = 0;
  my @t_median_ttc_vec_ = ( );
  my $t_avg_ttc_ = 0;
  my $t_min_pnl_ = 0;
  my @t_min_pnl_vec_ = ( );
  my $t_max_pnl_ = 0;
  my @t_max_pnl_vec_ = ( );
  my $t_sharpe_pnl_ = 0;

  for ( my $i = 0 ; $i <= $#global_results_lines_ ; $i ++ )
  {
    if ( DEBUG ) { print "Combining result with : $global_results_lines_[$i]\n"; }

    $t_sum_pnl_ += GetPnlFromGlobalResultsLine ( $global_results_lines_ [ $i ] );
    $t_sum_vol_ += GetVolFromGlobalResultsLine ( $global_results_lines_ [ $i ] );

    my $t_max_dd_ = GetMaxDDFromGlobalResultsLine ( $global_results_lines_ [ $i ] );
    push ( @t_dd_vec_ , $t_max_dd_ );

    $t_date_ = GetDateFromGlobalResultsLine ( $global_results_lines_ [ $i ] );

    my $t_median_ttc_ = GetMedianTTCFromGlobalResultsLine ( $global_results_lines_ [ $i ] );
    push ( @t_median_ttc_vec_ , $t_median_ttc_ );

    $t_avg_ttc_ += GetAvgTTCFromGlobalResultsLine ( $global_results_lines_ [ $i ] );

# Assuming they hit their avg. mins and maxs together.
    my $t_min_pnl_ = GetMinPnlFromGlobalResultsLine ( $global_results_lines_ [ $i ] );
    push ( @t_min_pnl_vec_ , $t_min_pnl_ );

    my $t_max_pnl_ += GetMaxPnlFromGlobalResultsLine ( $global_results_lines_ [ $i ] );
    push ( @t_max_pnl_vec_ , $t_max_pnl_ );

    $t_sharpe_pnl_ += GetSharpePnlFromGlobalResultsLine ( $global_results_lines_ [ $i ] );
  }

# Optimistic dd , min & max estimations of this set of results.
# Assuming they won't all hit min_pnl together , in which case using 
# the sum would be a more accurate description of this set of results.
  $t_max_dd_ = GetMeanLowestQuartile ( \@t_dd_vec_ ) * ( $#global_results_lines_ + 1 );
  $t_min_pnl_ = GetMeanHighestQuartile ( \@t_min_pnl_vec_ ) * ( $#global_results_lines_ + 1 );
  $t_max_pnl_ = GetMeanLowestQuartile ( \@t_max_pnl_vec_ ) * ( $#global_results_lines_ + 1 );

  $t_median_ttc_ = GetMedianConst ( \@t_median_ttc_vec_ );
  if ( $#global_results_lines_ >= 0 )
  {
    $t_avg_ttc_ /= ( $#global_results_lines_ + 1 );
    $t_sharpe_pnl_ /= ( $#global_results_lines_ + 1 );
  }

# STRAT 20120308 140 1573 24 75 0 4.6 76 175 12 6 156 0.04 0.62 -327 1969 1648
  my $t_combo_result_ = sprintf ( "COMBO %d %0.2f %d 0 0 0 0 %0.2f %0.2f 0 0 0 %0.2f 0 %0.2f %0.2f %0.2f" ,
                                 $t_date_ , $t_sum_pnl_ , $t_sum_vol_ , $t_median_ttc_ , $t_avg_ttc_ , $t_sharpe_pnl_ , $t_min_pnl_ , $t_max_pnl_ , $t_max_dd_ );

  return $t_combo_result_;
}

sub GetStratNameFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultStratNameOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetStratNameFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetPnlFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultPnlOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetPnlFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMinPnlFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultMinPnlOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetMinPnlFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetRiskAdjPnlAndDD
{
  if ( @_ < 2 ) { return; }

  my ($shc_, $strat_, $lines_to_print_ref_, $skip_days_file_) = @_;

  my $exec_cmd_ = $HOME_DIR."/basetrade_install/bin/get_UTS_for_a_day $shc_ $strat_ 20160630 0";
  my $unit_trade_size_ = `$exec_cmd_`; chomp ( $unit_trade_size_ );

  my $date_ = `date +%Y%m%d`; chomp ( $date_ );
  my %results_line_vec_ = ( );
 
  my $exchange_ = `$BINDIR/get_contract_specs $shc_ $date_ EXCHANGE | cut -d' ' -f2`;

  my $statistics_line_;
  if ( defined $exchange_ && $exchange_ =~ /MICEX/ ) {
    $statistics_line_ = GetSummarizeResultsForStrat( $shc_, $strat_, $date_, 20160601, \%results_line_vec_, $skip_days_file_ );
  } else {
    $statistics_line_ = GetSummarizeResultsForStrat( $shc_, $strat_, $date_, 400, \%results_line_vec_, $skip_days_file_ );
  }

  my %daily_pnl_ = map { $_ => (split(/\s+/, $results_line_vec_{$_}))[1] } keys %results_line_vec_;
  my $maxloss_ = abs( (split(/\s+/, $statistics_line_))[22] );
  
  my $oml_ = $maxloss_ / $unit_trade_size_;

  my ($avg_pnl_act_, $drawdown_, $avg_pnl_metric_, $riskadj_drawdown_) = GetRiskAdjPnl ( $unit_trade_size_, $oml_, \%daily_pnl_, $lines_to_print_ref_);

  my $sharpe_5days_ = GetndaysSharpe ( \%daily_pnl_, 5 );

  return ($avg_pnl_act_, $drawdown_, $avg_pnl_metric_, $riskadj_drawdown_, $sharpe_5days_);
}

sub GetndaysSharpe
{
  if ( @_ < 2 ) { return; }

  my $pnl_series_ref_ = shift;
  my $ndays_ = shift || 5;


  return if ! defined $pnl_series_ref_;

  my @dates_sorted_ = sort { $a <=> $b } keys %$pnl_series_ref_;
# PNL series should atleast have 100 days 
  if ( $#dates_sorted_ < 40 ) { return; }

  my @pnls_ndays_vec_ = ( );
  my $pnls_ndays_ = 0;
  my $old_idx_ = 0;

  $pnls_ndays_ += $$pnl_series_ref_{ $dates_sorted_[ $_ ] } foreach 0..($ndays_-1);
  push ( @pnls_ndays_vec_, $pnls_ndays_ );

  foreach my $dt_idx_ ( $ndays_..$#dates_sorted_ ) {
    $pnls_ndays_ -= $$pnl_series_ref_{ $dates_sorted_[ $old_idx_ ] };
    $pnls_ndays_ += $$pnl_series_ref_{ $dates_sorted_[ $old_idx_ + $ndays_ ] };
    push ( @pnls_ndays_vec_, $pnls_ndays_ );

    $old_idx_++;
  }

  my $sd_ = GetStdev ( \@pnls_ndays_vec_ );
  my $t_sharpe_ = undef;
  $t_sharpe_ = GetAverage ( \@pnls_ndays_vec_ ) / $sd_ if defined $sd_ && $sd_ != 0;

  return $t_sharpe_;
}

sub GetRiskAdjPnl
{
  if ( @_ < 3 ) { return; }

  my ($uts_, $oml_, $pnl_series_ref_, $lines_to_print_ref_) = @_;

  if ( ! defined $uts_ || ! defined $oml_ || ! defined $pnl_series_ref_ ) {
    return;
  }

  my @dates_sorted_ = sort { $a <=> $b } keys %$pnl_series_ref_;
# PNL series should atleast have 100 days 
  if ( $#dates_sorted_ < 50 ) { return; }

  my $risk_intv_ = 50;
  my $beta_risk_ = 10;
  my $max_uts_ = 5 * $uts_;
  my $min_uts_ = 0.2 * $uts_;
  my %dates_to_uts_ = ( );
  my %dates_to_adj_pnl_ = ( );

  my $last_day_idx_to_consider_ = 0;
  my $sum_pnl_for_risk_ = 0;
  
  my $total_sum_pnl_ = 0;
  my $max_pnl_ = 0;
  my $drawdown_ = 0;
  my $total_sum_riskadj_pnl_ = 0;
  my $max_riskadj_pnl_ = 0;
  my $riskadj_drawdown_ = 0;

  my $days_count_ = 0;
  foreach my $date_ ( @dates_sorted_[ 0..($risk_intv_-1) ] ) {
    my $t_uts_ = ( 1 - ($last_day_idx_to_consider_ / $risk_intv_) ) * $uts_ 
      + $beta_risk_ * ( ( $sum_pnl_for_risk_ / $risk_intv_ ) / $oml_ );
    $dates_to_uts_{ $date_ } = min( $max_uts_, max( $min_uts_, $t_uts_) );
    $dates_to_adj_pnl_{ $date_ } = $dates_to_uts_{ $date_ } * ($$pnl_series_ref_{ $date_ } / $uts_);
    
    $sum_pnl_for_risk_ += $dates_to_adj_pnl_{ $date_ };
    
    $total_sum_pnl_ += $$pnl_series_ref_{ $date_ };
    $max_pnl_ = max( $max_pnl_, $total_sum_pnl_ );
    $drawdown_ = max( $drawdown_, ($max_pnl_-$total_sum_pnl_) );
    
    $total_sum_riskadj_pnl_ += $dates_to_adj_pnl_{ $date_ };
    $max_riskadj_pnl_ = max( $max_riskadj_pnl_, $total_sum_riskadj_pnl_ );
    $riskadj_drawdown_ = max( $riskadj_drawdown_, ($max_riskadj_pnl_-$total_sum_riskadj_pnl_) );

    $last_day_idx_to_consider_++;

    my $tline_ = $$pnl_series_ref_{ $date_ }." ".$dates_to_adj_pnl_{ $date_ }." ".$dates_to_uts_{ $date_ };
    push ( @$lines_to_print_ref_, $tline_ );
  }

  foreach my $date_ ( @dates_sorted_[ $risk_intv_..$#dates_sorted_ ] ) {
    my $t_risk_ = $beta_risk_ * ( $sum_pnl_for_risk_  / $risk_intv_ );
    $dates_to_uts_{ $date_ } = min( $max_uts_, max( $min_uts_, $t_risk_ / $oml_) );
    $dates_to_adj_pnl_{ $date_ } = $dates_to_uts_{ $date_ } * ($$pnl_series_ref_{ $date_ } / $uts_);

    $sum_pnl_for_risk_ -= $dates_to_adj_pnl_{ $dates_sorted_[ $last_day_idx_to_consider_ - $risk_intv_ ] };
    $sum_pnl_for_risk_ += $dates_to_adj_pnl_{ $date_ };
    
    $total_sum_pnl_ += $$pnl_series_ref_{ $date_ };
    $max_pnl_ = max( $max_pnl_, $total_sum_pnl_ );
    $drawdown_ = max( $drawdown_, ($max_pnl_-$total_sum_pnl_) );
    
    $total_sum_riskadj_pnl_ += $dates_to_adj_pnl_{ $date_ };
    $max_riskadj_pnl_ = max( $max_riskadj_pnl_, $total_sum_riskadj_pnl_ );
    $riskadj_drawdown_ = max( $riskadj_drawdown_, ($max_riskadj_pnl_-$total_sum_riskadj_pnl_) );

    $last_day_idx_to_consider_++;

    my $tline_ = $$pnl_series_ref_{ $date_ }." ".$dates_to_adj_pnl_{ $date_ }." ".$dates_to_uts_{ $date_ };
    push ( @$lines_to_print_ref_, $tline_ );
  }

  my $avg_pnl_act_ = GetAverage( [ values %$pnl_series_ref_ ] );
  my $avg_pnl_metric_ = GetAverage( [ values %dates_to_adj_pnl_ ] );
  return ($avg_pnl_act_, $drawdown_, $avg_pnl_metric_, $riskadj_drawdown_);
}

sub GetVolFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultVolOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetVolFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetDateFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultDateOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMaxDDFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultMaxDDOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetMaxDDFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMedianTTCFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultMedianTTCOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetAvgTTCFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultAvgTTCOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetSharpePnlFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultSharpePnlOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMinPnlFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultMinPnlOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMaxPnlFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultMaxPnlOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetAvgAbsPosFromGlobalResultsLine
{
  my ( $global_result_line_ ) = @_;

  my @global_result_words_ = split ( ' ' , $global_result_line_ );

  my $ret_ = "";
  my $field_offset_ = GlobalResultAvgAbsPosOffset;

  if ( $#global_result_words_ >= $field_offset_ ) { $ret_ = $global_result_words_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultsLine Malformed global_result_line_ : $global_result_line_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetStratNameFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultStratNameOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetStratNameFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetPnlFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultPnlOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetPnlFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMinPnlFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultMinPnlOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetMinPnlFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetVolFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultVolOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetVolFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetDateFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultDateOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMaxDDFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultMaxDDOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetMaxDDFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMedianTTCFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultMedianTTCOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetAvgTTCFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultAvgTTCOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetSharpePnlFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultSharpePnlOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMinPnlFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultMinPnlOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetMaxPnlFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultMaxPnlOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}

sub GetAvgAbsPosFromGlobalResultVecRef
{
  my ( $global_result_vec_ref_ ) = @_;

  my $ret_ = "";
  my $field_offset_ = GlobalResultAvgAbsPosOffset;

  if ( $#$global_result_vec_ref_ >= $field_offset_ ) { $ret_ = $$global_result_vec_ref_ [ $field_offset_ ]; }
  else
  { print "GetDateFromGlobalResultVecRef Malformed global_result_vec_ref_ : @$global_result_vec_ref_\n"; exit ( 0 ); }

  return $ret_;
}


sub InvalidGlobalResult
{
  return "INVALID 0 -100000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 100000";
}

1;
