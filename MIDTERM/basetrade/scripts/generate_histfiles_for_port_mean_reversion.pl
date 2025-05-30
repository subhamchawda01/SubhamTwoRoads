#!/usr/bin/perl
use FileHandle;
use POSIX qw/floor/;
use strict;
use warnings;

#takes as input 
#1. file with one stockname per line with first row being index/very liquid 
#2. duration ( number of days ) for stock selection and fitting regression relation 
#3. date for which regression betas are needed
#4. number of predictors per stock for portfolio construction 
#4. temporary work file
#5. output file
#6. correlation threshold - only outputs ports for stocks where in-sample regression meets specified threshold
#7. mode - LM for lm and NNLS for nnls regression
# outputs file in format which can be fed to dump_mean_reversion_port_stats.pl
#

if($#ARGV < 5)
{
  print "Usage: get_ports_for_port_mean_reversion.pl <PortName> <stockname_file> <numdays> <date> <duration_regression> <error_length> [<hist_file_dir>] [<shclist_to_print>]\n";
  exit(0);
} 

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/array_ops.pl";

my $type_ =  0;
my $port_ = $ARGV[0];
my $ticker_file_ = $ARGV[1]; #first line must have index or very liquid stock
my $numdays_ = $ARGV[2]; # looback days for computing the portfolio weights
my $date_ = $ARGV[3];

my $duration_regression_ = $ARGV[4];
my $error_length_ = $ARGV[5];

my $hist_file_dir_ = "/spare/local/".$USER."/".$port_."_HFiles/";
my $histerrors_file_dir_ = "/spare/local/".$USER."/".$port_."_HErrors/";
if ( $#ARGV > 5 ) {
  $hist_file_dir_ = $ARGV[6];
}

my $shcfile_to_print_ = "INVALID";
my @dep_shclist_ = ();
if ( $#ARGV > 6 ) {
  $shcfile_to_print_ = $ARGV[7];
  @dep_shclist_ = `cat $shcfile_to_print_ 2>/dev/null`; chomp @dep_shclist_;
}

`mkdir --parents $hist_file_dir_`;
`mkdir --parents $histerrors_file_dir_`;
my $hist_file_ = $hist_file_dir_."hist_file_".$type_."_".$duration_regression_."_".$error_length_."_".$numdays_."_".$date_;
my $analysis_file = $hist_file_dir_."analysis_file_".$type_."_".$duration_regression_."_".$error_length_."_".$numdays_."_".$date_;
my $errors_file = $histerrors_file_dir_."errors_file_".$type_."_".$duration_regression_."_".$error_length_."_".$numdays_."_".$date_;

my $start_date_ = CalcPrevWorkingDateMult($date_, $numdays_);
my $last_date_ = CalcPrevWorkingDateMult($date_, 1);

my @dates_list_ = GetDatesFromNumDays("VALE5", $last_date_, $numdays_, "INVALIDFILE");
@dates_list_ = sort {$a <=> $b} @dates_list_;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 3;

my @tickers_ = `cat $ticker_file_ 2>/dev/null`; chomp ( @tickers_ );

#TODO - needs to be generalized if file paths are different
my $FILE_PATH_PREFIX = "/media/shared/ephemeral16/BarData/$port_";

my $work_dir_ = "/spare/local/".$USER."/".$port_."/".$unique_gsm_id_."/";
`mkdir --parents $work_dir_`;


print "$hist_file_\n";
my @timestamps_ = ( );
my @timestamp_price_vec_ = ( );
GetBarData (\@dates_list_, \@timestamps_, \@timestamp_price_vec_);

my $generate_analysis_ = 0;
$generate_analysis_ = 1 if -d "$FILE_PATH_PREFIX/$date_";

my @to_timestamps_ = ();
my @to_timestamp_price_vec_ = ();
if ( $generate_analysis_ == 1 ) {
  print "$analysis_file\n";
  GetBarData ([$date_], \@to_timestamps_, \@to_timestamp_price_vec_);
}

my $min_price_increment_ = 1;
my %dep_2_last_prices_ = ();
my %dep_2_corr_ = ();

# we are generting a new file per run
open HIST_FH, "> $hist_file_ " or PrintStacktraceAndDie ( "Could not open $hist_file_ for writting \n" );
open LOG_FH, "> $analysis_file " or PrintStacktraceAndDie ( "Could not open $analysis_file for writting \n" );

foreach my $ctr_ ( 0 .. $#tickers_ ) {
  my $dep_ = $tickers_[$ctr_];
  my $to_print_ = 1;
  $to_print_ = 0 if ( $#dep_shclist_ >= 0 and ! FindItemFromVec($dep_, @dep_shclist_) );

  $min_price_increment_ = `~/cvquant_install/basetrade/bin/get_min_price_increment $dep_ $date_`;
  chomp $min_price_increment_;

  my $data_file_past_ = $work_dir_."data_past";
  my @sources_vec_ = ( );
  GenerateData ($ctr_, \@timestamps_, \@timestamp_price_vec_, $data_file_past_, \@sources_vec_);

# we are appending to output_file !!
  my @weight_ref_ = ();
  PrintPortWghts ($tickers_[$ctr_], \@sources_vec_, $data_file_past_, \@weight_ref_, $to_print_);

  my %hist_details_ = ();
  $dep_2_last_prices_{$dep_} = 
      PrintHistPricesAndErrorLength (\@weight_ref_, $data_file_past_, \%hist_details_, $to_print_);
  $dep_2_corr_{$dep_} = $hist_details_{"corr"};

  if ( $generate_analysis_ == 1 ) {
    my $data_file_today_ = $work_dir_."data_today";
    GenerateData ($ctr_, \@to_timestamps_, \@to_timestamp_price_vec_, $data_file_today_);
    PrintHistAnal($dep_, \@weight_ref_, $data_file_today_, \%hist_details_);
  }
}

print HIST_FH "LAST_PRICE ".join(" ", map { $_." ".$dep_2_last_prices_{$_} } keys %dep_2_last_prices_)."\n";
print HIST_FH "INSAMPLE_CORR ".join(" ", map { $_." ".$dep_2_corr_{$_} } keys %dep_2_corr_)."\n";
close HIST_FH;
close LOG_FH;


sub GetBarData {
  my $dates_ref_ = shift;
  my $timestamp_ref_ = shift;
  my $timestamp_price_ref_ = shift;

  foreach my $dt (@$dates_ref_) {
    my @ticker_to_all_lines_ = ();
    my @t_timestamps_ = ();
#for efficiency we process in two passes - with the first pass used to isolate times - this is mainly to deal 
#with the problem that bar data is only present for minutes where trade has happened - so for illiquids there 
#will be gaps which needs to be filled 

    my $all_tickers_present_ = 1;

    my $index_first_tstamp_;
    my $index_last_tstamp_;
#hacks to get around accurate computation of business days; we assume index is most liquid .. 
    foreach my $ctr ( 0 .. $#tickers_ ) {
      my $ticker_exchsymbol_ = `$BIN_DIR/get_exchange_symbol $tickers_[$ctr] $dt | tr ' ' '~'`; 
      chomp $ticker_exchsymbol_;

      my $data_file_path_ = "$FILE_PATH_PREFIX/$dt/MB_".$ticker_exchsymbol_."_".$dt;

      my @all_lines_ = `cat \'$data_file_path_\'`; chomp @all_lines_;
      if ( $#all_lines_ < 0 ) {
        $all_tickers_present_ = 0;
        last;
      }


      my @tt_timestamps_ = map { ${ [split(/\s+/, $_)] }[0] } @all_lines_;

      if ( $ctr == 0 ) {
        $index_first_tstamp_ = $tt_timestamps_[0];
        $index_last_tstamp_ = $tt_timestamps_[$#t_timestamps_];
      }
      else {
        @tt_timestamps_ = grep { $_ >= $index_first_tstamp_ && $_ <= $index_last_tstamp_ } @tt_timestamps_;
      }

      $ticker_to_all_lines_[$ctr] = \@all_lines_;
      push @t_timestamps_, @tt_timestamps_;
    }

    next if $all_tickers_present_ == 0;

#Get union of all distinct timestamps seen - in order to fill for missing bars. 
    @t_timestamps_ = sort keys { map { $_ => 1 } @t_timestamps_ };
    push @$timestamp_ref_, @t_timestamps_;

#all unique timestamps have been populated in timestamp vector.
#now we go ahead and populate price data - 2nd pass
    foreach my $ctr_ ( 0 .. $#tickers_ ) {
      my $tstamp_index_ = 0;
      my $close_px_ = 0;
      my $curr_tstamp_ = 0;

      foreach my $data_line_ ( @{ $ticker_to_all_lines_[$ctr_] } ) {
        my @tokens_ = split(' ', $data_line_);

        if( $#tokens_ > 10 && $tokens_[0] >= $$timestamp_ref_[0] && $tokens_[0] <= $$timestamp_ref_[$#$timestamp_ref_] ) 
        {
          $curr_tstamp_ = $tokens_[0];
          $close_px_ = ($tokens_[8] + $tokens_[9])/2;
          while( $tstamp_index_ <= $#$timestamp_ref_ && $curr_tstamp_ >= $$timestamp_ref_[$tstamp_index_] )
          {
            push @{$$timestamp_price_ref_[$tstamp_index_]}, $close_px_;
            $tstamp_index_ = $tstamp_index_ + 1;
          }      
        }

        if( $#tokens_ > 8 &&  $tokens_[0] >= $$timestamp_ref_[$#$timestamp_ref_] )
        {
          while( $tstamp_index_ <= $#$timestamp_ref_)
          {
            push @{$$timestamp_price_ref_[$tstamp_index_]}, $close_px_;
            $tstamp_index_ = $tstamp_index_ + 1;
          }
          last;
        }
      }
#Corner case - if date specified is the last date for which data is in  the file then it can exit loop without tstamp_index < $#timestamps
      while( $tstamp_index_ <= $#$timestamp_ref_ )
      {
        push @{$$timestamp_price_ref_[$tstamp_index_]}, $close_px_;
        $tstamp_index_ = $tstamp_index_ + 1;
      }
    }
  }
}


sub GenerateData
{
  my $ctr = shift;
  my $timestamp_ref_ = shift;
  my $timestamp_price_ref_ = shift;
  my $data_file_ = shift;
  my $sources_ref_ = shift;

  open FILE,">$data_file_" or die "Could not open $data_file_ for writing\n";  
  foreach my $ctr_2 ( 0 .. $#$timestamp_ref_ ) {
    print FILE "$$timestamp_ref_[$ctr_2] 0 $$timestamp_price_ref_[$ctr_2][$ctr]";
    foreach my $ctr_3 ( 0 .. $#tickers_ ) {
      next if $ctr == $ctr_3;
      print FILE " $$timestamp_price_ref_[$ctr_2][$ctr_3]";
    }
    print FILE "\n";
  }
  close FILE;
   
  if ( defined $sources_ref_ ) { 
    foreach my $ctr_3 ( 0 .. $#tickers_ ) {
      next if $ctr == $ctr_3;
      push ( @$sources_ref_, $tickers_[$ctr_3] );
    }
  }
}


sub PrintPortWghts
{
  my ($t_dep_, $sources_ref_, $t_data_file_, $weight_ref_, $to_print_) = @_;
  
  my $t_wghts_file_ = $work_dir_."wghts_file";
  my $cmd_ = $MODELSCRIPTS_DIR."/call_nnls.R ".$t_data_file_." ".$t_wghts_file_;
  print "$cmd_\n";
  `$cmd_`;

  open OFHandle, "< $t_wghts_file_ " or PrintStacktraceAndDie("could not open $t_wghts_file_ for reading\n");

  while ( my $file_line_ = <OFHandle> ) {
    @$weight_ref_ = split(' ', $file_line_);

    if ( $to_print_ ) {
#PWEIGHTS PORT_GGBR4 0.0846506200495 0.244172320723 0.0 0.203760474075
      print HIST_FH "Port: ".$t_dep_." ".($#$sources_ref_+1);
      print HIST_FH " ".$$sources_ref_[$_]." ".$$weight_ref_[$_] foreach (0 .. $#$weight_ref_);
      print HIST_FH "\n";
    }
  }
  close OFHandle;
}


sub PrintHistPricesAndErrorLength
{
  my ($weight_ref_, $t_data_file_, $hist_details_ref_, $to_print_) = @_;

# our first line correponds to most recent data
# we are interested in most recent errors and prices
# we are going to print last error_length errors vec and y ( dep prices ) of regression_length
# we start reading from ( regression_length + error_length )

# start iterator = no_of_rows - (regression_length + error_length)
# end_iterator = no_of_rows

  my $cmd_ = "cat $t_data_file_ | tail -n".($duration_regression_ + $error_length_)." | cut -d' ' -f3-";
  my @recent_data_ = `$cmd_`; chomp (@recent_data_);

  $cmd_ = "cat $t_data_file_ | cut -d' ' -f3-";
  my @all_data_ = `$cmd_`; chomp (@all_data_);

  my @y = ();
  my @x = ();
  my @errors_ = ();

# no of indeps == no of weights
  OnlineFit (\@recent_data_, $weight_ref_, \@x, \@y, \@errors_);
  my $corr_ = FindCorr(\@all_data_, $weight_ref_);

  if ($duration_regression_ > scalar(@y)) {
    print LOG_FH "there is no enough data for hist prices\n";
    exit(-1);
  }
  if ($error_length_ > scalar(@errors_)) {
    print LOG_FH "there is no enough data for error history\n";
    exit(-1);
  }

  if ( $to_print_ ) {
    printf HIST_FH "HIST_PRICES ";
    printf HIST_FH "%f %f ", $y[$_], $x[$_] foreach (0 .. $#y);
    printf HIST_FH "\n";
    printf HIST_FH "HIST_ERROR ";
    printf HIST_FH "%f ", $_ foreach @errors_;
    printf HIST_FH "\n";
  }

  $$hist_details_ref_{"y"} = \@y;
  $$hist_details_ref_{"x"} = \@x;
  $$hist_details_ref_{"errors"} = \@errors_;
  $$hist_details_ref_{"corr"} = $corr_;
  return $y[$#y];
}

sub PrintHistAnal
{
  my ($dep_, $weight_ref_, $t_data_file_, $hist_details_ref_) = @_;

# for analysis, we are reading the entire data
# we'll compute the errors and then print the stdev,mean, #zerocrossings and other anal 
# on the same
  my $cmd_ = "cat $t_data_file_ | cut -d' ' -f3-";
  my @data_ = `$cmd_`; chomp (@data_);

  $cmd_ = "cat $t_data_file_ | cut -d' ' -f1";
  my @ts_vec_ = `$cmd_`; chomp (@ts_vec_);

  my @y = @{ $$hist_details_ref_{"y"} };
  my @x = @{ $$hist_details_ref_{"x"} };
  my @errors_ = ( );

# no of indeps == no of weights
  OnlineFit (\@data_, $weight_ref_, \@x, \@y, \@errors_);
  my $corr_ = FindCorr(\@data_, $weight_ref_);

  Histogram ($dep_, $$hist_details_ref_{"errors"}, \@errors_, \@ts_vec_, $$hist_details_ref_{"corr"}, $corr_);
}

sub OnlineFit
{
  my ($data_ref_, $wghts_ref_, $x_ref_, $y_ref_, $errors_ref_) = @_;

# we compute the beta first and then roll update it till the end
# once the init beta is computed we then start collect the errors using y - yhat
# yhat = r_beta * port
# port = wi*xi

  my @x = @$x_ref_;
  my @y = @$y_ref_;
  my $sum_x_y = 0;
  my $sum_xsqr = 0;
  $sum_x_y += $x[$_]*$y[$_] foreach (0 .. $#x);
  $sum_xsqr += $x[$_]*$x[$_] foreach (0 .. $#x);
  
  for(my $i = 0; $i < scalar(@$data_ref_); $i++) {
    my @tokens_ = split(" ", $$data_ref_[$i]);
    
    if ($#$wghts_ref_ != ($#tokens_ - 1)) {
      print LOG_FH "weights length is not same as indeps length\n";
      print LOG_FH scalar(@$wghts_ref_);
      print LOG_FH scalar(@tokens_);
      exit(-1);	    
    }

    my $this_y = $tokens_[0];
    my $this_x = 0;
    $this_x += $tokens_[$_] * $$wghts_ref_[($_-1)] foreach (1 .. $#tokens_);

    push @y, $this_y;
    push @x, $this_x;
    $sum_x_y += $this_x*$this_y;
    $sum_xsqr += $this_x * $this_x;
    if( $#y >= $duration_regression_ )
    {
      push @$errors_ref_, ($this_y - $this_x*$sum_x_y/$sum_xsqr);
      my $del_x = shift @x;
      my $del_y = shift @y;
      $sum_x_y -= $del_x*$del_y;
      $sum_xsqr -= $del_x*$del_x;
    }
  }
  push @$x_ref_, @x;
  push @$y_ref_, @y;
}

sub FindCorr
{
  my ($data_ref_, $wghts_ref_) = @_;

  my $sum_x_y = 0;
  my $sum_xsqr = 0;
  my $sum_ysqr = 0;
  my $sum_x = 0;
  my $sum_y = 0;
  my $count_ = 0;
  for(my $i = 0; $i < scalar(@$data_ref_); $i++) {
    my @tokens_ = split(" ", $$data_ref_[$i]);

    if ($#$wghts_ref_ != ($#tokens_ - 1)) {
      print LOG_FH "weights length is not same as indeps length\n";
      print LOG_FH scalar(@$wghts_ref_);
      print LOG_FH scalar(@tokens_);
      exit(-1);	    
    }

    my $this_y = $tokens_[0];
    my $this_x = 0;
    $this_x += $tokens_[$_] * $$wghts_ref_[($_-1)] foreach (1 .. $#tokens_);

    $sum_x_y += $this_x * $this_y;
    $sum_xsqr += $this_x * $this_x;
    $sum_ysqr += $this_y * $this_y;
    $sum_x += $this_x;
    $sum_y += $this_y;
    $count_++;
  }
  my $cov = $sum_x_y - ($sum_x * $sum_y) / $count_;
  my $varx = $sum_xsqr - ($sum_x * $sum_x) / $count_;
  my $vary = $sum_ysqr - ($sum_y * $sum_y) / $count_;

  return ($cov / (sqrt($varx) * sqrt($vary)));
}

sub sign
{
  my $val_ = shift;
  my $thresh_ = shift || 0;

  return 1 if $val_ > $thresh_;
  return -1 if $val_ < -1*$thresh_;
  return 0;
}

sub Histogram
{
  my $dep_ = shift;
  my $prev_errors_ref_ = shift;
  my $errors_ref_ = shift;
  my $ts_ref_ = shift;
  my $insample_corr_ = shift;
  my $outsample_corr_ = shift;

# remove values > 10 * tick_value
  my @refined_data_ = grep { sign($_) * min(abs($_), 15 * $min_price_increment_) } @$errors_ref_;
  my @outliers_ = grep { abs($_) > 15 * $min_price_increment_ } @$errors_ref_;

  my $mean = GetAverage ( \@refined_data_ ) / $min_price_increment_;
  my $stdev = GetStdev ( \@refined_data_ ) / $min_price_increment_;
  my $median = GetMedianConst ( \@refined_data_ ) / $min_price_increment_;
  my $min = min ( @refined_data_ ) / $min_price_increment_;
  my $max = max ( @refined_data_ ) / $min_price_increment_;
  my $no_stdev = ( ( $max - $min ) / $stdev );

  print LOG_FH sprintf("$dep_ mean: %.2f stdev: %.2f no_of_stdev: %.2f percentage_of_outliers: %.2f\n", $mean, $stdev, $no_stdev, 100*scalar(@outliers_)/scalar(@refined_data_));
  print LOG_FH sprintf("$dep_ corr_for_offline_fit: Insample: %.2f OutofSample: %.2f\n", $insample_corr_, $outsample_corr_);

  my @sd_err_vec_ = ();
  my @t_errors_vec_ = @$prev_errors_ref_;

  foreach my $err_ (@refined_data_) {
    my $sd_err_ = GetStdev ( \@t_errors_vec_ );
    push @sd_err_vec_, $sd_err_;
    push @t_errors_vec_, $err_;
    shift @t_errors_vec_;
  }

  my @sd_fact_vec_ = (0,0.3,0.5,0.7);
  my %sdfact_to_opentime_ = ();
  my %sdfact_to_zc_counts_ = ();
  my %sdfact_to_signal_frac_ = ();
  my $total_time_ = 0;
  
  for my $sd_fact_ ( @sd_fact_vec_ ) {
    my $sign_ = 0;
    my $last_ts_ = 0;
    my @openpos_times_ = ();
    my $zc_count_ = 0;
    my $day_count_ = 1;

    foreach my $ctr ( 0 .. $#refined_data_ ) {
      my $thresh_ = $sd_fact_ * $sd_err_vec_[$ctr];
      my $new_sign_ = sign($refined_data_[$ctr], $thresh_);

      if ($sign_ != $new_sign_) {
        if ($sign_ != 0) {
          $zc_count_++;
          my $ts_intv_ = $$ts_ref_[$ctr] - $last_ts_;
# time_intv of >15000 implies day change.. ignoring those
          if ( $ts_intv_ < 15000 ) {
            push @openpos_times_, $ts_intv_;
          }
        }
        if ( $new_sign_ != 0) {
          $last_ts_ = $$ts_ref_[$ctr];
        }
        $sign_ = $new_sign_;
      }
    }
    my $opentime_avg_ = GetAverage(\@openpos_times_);
    $sdfact_to_opentime_{$sd_fact_} = RoundOff($opentime_avg_, 2);
    $sdfact_to_zc_counts_{$sd_fact_} = $zc_count_;
    $sdfact_to_signal_frac_{$sd_fact_} = 0;

    $total_time_ = GetSum(\@openpos_times_) if $sd_fact_ == 0;
    $sdfact_to_signal_frac_{$sd_fact_} = GetSum(\@openpos_times_) / $total_time_ if $total_time_ > 0;
  }
  print LOG_FH "$dep_ #ZeroCrossings ".join(" ", map { $_.":".$sdfact_to_zc_counts_{$_} } sort keys %sdfact_to_zc_counts_)."\n";
  print LOG_FH "$dep_ SignalTotalTime_frac: ".join(" ", map { $_.":".sprintf("%.2f",$sdfact_to_signal_frac_{$_}) } sort keys %sdfact_to_signal_frac_)."\n";
  print LOG_FH "$dep_ SignalAvgTime: ".join(" ", map { $_.":".$sdfact_to_opentime_{$_} } sort keys %sdfact_to_opentime_)."\n";
  print LOG_FH "\n";
}

sub RoundOff
{
  my ( $number , $accuracy ) = @_;
  return ( int ( $number * ( 10 ** $accuracy ) ) / ( 10 ** $accuracy ) )
}
