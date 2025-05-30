#!/usr/bin/perl

# \file ModelScripts/t_model_components.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# modelfile date start_hhmm end_hhmm

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $USAGE="$0 modelfile date starthhmm endhhmm ";

if( $#ARGV < 3 ) { print $USAGE."\n"; exit( 0 ); }

my $model_filename_ = $ARGV[0];
my $trading_date_ = $ARGV[1];
my $datagen_start_hhmm_ = $ARGV[2];
my $datagen_end_hhmm_ = $ARGV[3];

my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 13;
my $datagen_num_trades_timeout_ = 2;
my $to_print_on_economic_times_ = 0;
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

my @book_coeff_cols_ = ();
my @book_coeffs_ = ();
my $sum_book_vars_ = 0;
my $sum_book_vars_sqr_= 0;

my @trade_coeff_cols_ = ();
my @trade_coeffs_ = ();
my $sum_trade_vars_ = 0;
my $sum_trade_vars_sqr_ = 0;

my @trend_coeff_cols_ = ();
my @trend_coeffs_ = ();
my $sum_trend_vars_ = 0;
my $sum_trend_vars_sqr_ = 0;

my @mean_reversion_coeff_cols_ = ();
my @reversion_coeffs_ = ();
my $sum_reversion_vars_ = 0;
my $sum_reversion_vars_sqr_ = 0;

my @online_coeff_cols_ = ();
my @online_coeffs_ = ();
my $sum_online_vars_ = 0;
my $sum_online_vars_sqr_ = 0;

my @all_vars_cols_ = ();
my @all_vars_coeffs_ = ();
my $sum_all_vars_ = 0;
my $sum_all_vars_sqr_ = 0;

my $COL_OFFSET = 4;

open MFILE, "< $model_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $model_filename_ for reading\n" );
my $indicator_count_ = 0;
while ( my $mline_ = <MFILE> )
{
    chomp ( $mline_ );
    if ( $mline_ =~ /INDICATOR / )
    {
	my @mwords_ = split ( ' ', $mline_ );
	if ( $#mwords_ > 2 && ( $mwords_[2] =~ /Mult/ || $mwords_[2] =~ /Book/ || $mwords_[2] =~ /BidAsk/ || $mwords_[2] =~ /DiffPriceL1/ || $mwords_[2] =~ /DiffPairPriceType/ || $mwords_[2] =~ /AmplifyLevelChange/ || $mwords_[2] =~ /DiffPriceType/ ) )
	{
	    push( @book_coeff_cols_, $COL_OFFSET + $indicator_count_ );
	    push( @book_coeffs_, $mwords_[1] );
	}
	elsif( $#mwords_ > 2 && ( $mwords_[2] =~ /TR/ || $mwords_[2] =~ /TD/ || $mwords_[2] =~ /SizeWBP/ || $mwords_[2] =~ /DiffED/ ) )
	{
	    push( @trade_coeff_cols_, $COL_OFFSET + $indicator_count_ );
	    push( @trade_coeffs_, $mwords_[1] );	    
	}
	elsif( $#mwords_ > 2 && ( $mwords_[2] =~ /Trend/ ) )
	{
	    push( @trend_coeff_cols_, $COL_OFFSET + $indicator_count_ );
	    push( @trend_coeffs_, $mwords_[1] );	    	    
	}
	elsif( $#mwords_ > 2 && ( $mwords_[2] =~ /Offline/ || $mwords_[2] =~ /PCA/ ) )
	{
	    push( @mean_reversion_coeff_cols_, $COL_OFFSET + $indicator_count_ );
	    push( @reversion_coeffs_, $mwords_[1] );	    	    	    
	}
	elsif( $#mwords_ > 2 && ( $mwords_[2] =~ /Online/ ) )
	{
	    push( @online_coeff_cols_, $COL_OFFSET + $indicator_count_ );
	    push( @online_coeffs_, $mwords_[1] );	    	    	    	    
	}
	else
	{
	    printf "unclassified indicator %s\n", $mline_;
	}
	push ( @all_vars_cols_, $COL_OFFSET + $indicator_count_ );
	push ( @all_vars_coeffs_, $mwords_[1] );
	$indicator_count_ = $indicator_count_ + 1;
    }
}
close MFILE;

my $this_day_timed_data_filename_ = $SPARE_HOME."timed_data".$trading_date_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$unique_gsm_id_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_;
my $exec_cmd="$LIVE_BIN_DIR/datagen $model_filename_ $trading_date_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
#	print $main_log_file_handle_ "$exec_cmd\n";
`$exec_cmd`;
my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$trading_date_.".".$unique_gsm_id_;
`rm -f $datagen_logfile_`;
my $num_lines_ = 0;

#process the datagen output file
if ( -e $this_day_timed_data_filename_ )
{
    open DFILE, "< $this_day_timed_data_filename_" or PrintStacktraceAndDie ( "Could not open this_day_timed_data_filename_ $this_day_timed_data_filename_ for reading\n" );
    while ( my $data_line_ = <DFILE> )
    {
	chomp ( $data_line_ );
	my @dwords_ = split ( ' ', $data_line_ );
	my $wt_val_ = 0;
	#compute for all
	for ( my $i = 0; $i <= $#all_vars_cols_; $i = $i+1 )
	{
	    $wt_val_ = $wt_val_ + $dwords_[ $all_vars_cols_[$i] ]*$all_vars_coeffs_[$i]
	}
	$sum_all_vars_ = $sum_all_vars_ + $wt_val_;
	$sum_all_vars_sqr_ = $sum_all_vars_sqr_ + $wt_val_*$wt_val_;	    
	
	$wt_val_ = 0;
	for ( my $i = 0; $i <= $#book_coeff_cols_; $i = $i+1 )
	{
	    $wt_val_ = $wt_val_ + $dwords_[ $book_coeff_cols_[$i] ]*$book_coeffs_[$i]
	}
	$sum_book_vars_ = $sum_book_vars_ + $wt_val_;
	$sum_book_vars_sqr_ = $sum_book_vars_sqr_ + $wt_val_*$wt_val_;	    
	
	$wt_val_ = 0;
	for ( my $i = 0; $i <= $#trade_coeff_cols_; $i = $i+1 )
	{
	    $wt_val_ = $wt_val_ + $dwords_[ $trade_coeff_cols_[$i] ]*$trade_coeffs_[$i]
	}
	$sum_trade_vars_ = $sum_trade_vars_ + $wt_val_;
	$sum_trade_vars_sqr_ = $sum_trade_vars_sqr_ + $wt_val_*$wt_val_;	    
	
	$wt_val_ = 0;
	for ( my $i = 0; $i <= $#trend_coeff_cols_; $i = $i+1 )
	{
	    $wt_val_ = $wt_val_ + $dwords_[ $trend_coeff_cols_[$i] ]*$trend_coeffs_[$i]
	}
	$sum_trend_vars_ = $sum_trend_vars_ + $wt_val_;
	$sum_trend_vars_sqr_ = $sum_trend_vars_sqr_ + $wt_val_*$wt_val_;	    
	
	$wt_val_ = 0;
	for ( my $i = 0; $i <= $#mean_reversion_coeff_cols_; $i = $i+1 )
	{
	    $wt_val_ = $wt_val_ + $dwords_[ $mean_reversion_coeff_cols_[$i] ]*$reversion_coeffs_[$i]
	}
	$sum_reversion_vars_ = $sum_reversion_vars_ + $wt_val_;
	$sum_reversion_vars_sqr_ = $sum_reversion_vars_sqr_ + $wt_val_*$wt_val_;	    
		
	$wt_val_ = 0;
	for ( my $i = 0; $i <= $#online_coeff_cols_; $i = $i+1 )
	{
	    $wt_val_ = $wt_val_ + $dwords_[ $online_coeff_cols_[$i] ]*$online_coeffs_[$i]
	}
	$sum_online_vars_ = $sum_online_vars_ + $wt_val_;
	$sum_online_vars_sqr_ = $sum_online_vars_sqr_ + $wt_val_*$wt_val_;	    
        $num_lines_ = $num_lines_ + 1;
    }
}
`rm -f $this_day_timed_data_filename_`;

my $model_stdev_ = sqrt( $sum_all_vars_sqr_/$num_lines_ - ( $sum_all_vars_/$num_lines_*$sum_all_vars_/$num_lines_ ) );
my $book_vars_stdev_ = sqrt( $sum_book_vars_sqr_/$num_lines_ - ( $sum_book_vars_/$num_lines_*$sum_book_vars_/$num_lines_ ) ) ;
my $trade_vars_stdev_ = sqrt( $sum_trade_vars_sqr_/$num_lines_ - ( $sum_trade_vars_/$num_lines_*$sum_trade_vars_/$num_lines_ ) );
my $trend_vars_stdev_ = sqrt( $sum_trend_vars_sqr_/$num_lines_ - ( $sum_trend_vars_/$num_lines_ * $sum_trend_vars_/$num_lines_ ) );
my $reversion_vars_stdev_ = sqrt( $sum_reversion_vars_sqr_/$num_lines_ - ( $sum_reversion_vars_/$num_lines_ * $sum_reversion_vars_/$num_lines_ ) );
my $online_vars_stdev_ = sqrt( $sum_online_vars_sqr_/$num_lines_ - ( $sum_online_vars_/$num_lines_*$sum_online_vars_/$num_lines_ ) );

printf "Total Stdev:\t\t %.10f\n",$model_stdev_;
printf "Book_Vars Stdev:\t %.10f \t %.02f\n", $book_vars_stdev_, $book_vars_stdev_/$model_stdev_;
printf "Trade_Vars Stdev:\t %.10f \t %.02f\n", $trade_vars_stdev_, $trade_vars_stdev_/$model_stdev_;
printf "Trend_Vars Stdev:\t %.10f \t %.02f\n", $trend_vars_stdev_, $trend_vars_stdev_/$model_stdev_;
printf "Reversion_Vars Stdev:\t %.10f \t %.02f\n", $reversion_vars_stdev_, $reversion_vars_stdev_/$model_stdev_;
printf "Online_Vars Stdev:\t %.10f \t %.02f\n", $online_vars_stdev_, $online_vars_stdev_/$model_stdev_;
