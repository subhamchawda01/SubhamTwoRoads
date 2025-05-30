#!/usr/bin/perl

# \file ModelScripts/apply_dep_filter.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This scipt takes 2 arguments: an input file and a filter name
# if the filter name is valid, it creates an output filtered file and prints to console the name of the output file

use strict;
use warnings;
use feature "switch";          # for given, when
use File::Basename;            # for basename and dirname
use File::Copy;                # for copy
use List::Util qw/max min/;    # for max
use FileHandle;


my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $INSTALL_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $EOD_PNL_DIR = "/NAS1/data/MFGlobalTrades/EODPnl";

if ( !(-d $EOD_PNL_DIR) ){
	$EOD_PNL_DIR = "/apps/data/MFGlobalTrades/EODPnl/";
	if ( !(-d $EOD_PNL_DIR) ){
		print "$EOD_PNL_DIR doesn't exist\n";
	}
}

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $#ARGV < 0 ){
	print "USAGE: [<tradingdate>][<shortcode>]";
	exit(0);
}

my $tradingdate_ = $ARGV[0];

my @product_vec_ = ();

my %shortcode_to_new_order_msg_count_ = ();
my %shortcode_to_cancel_msg_count_ = ();
my %shortcode_to_modify_msg_count_ = ();
my %shortcode_to_fill_number_count_ = ();
my %shortcode_to_volume_count_ = ();
my %shortcode_to_benchmark_ratio_ = ();

my $email_string_ = "";

if ( $#ARGV >= 1 ){
	push ( @product_vec_, $ARGV[1] );
} else {
	my $exec_cmd_ = "$LIVE_BIN_DIR/get_contract_specs ALL $tradingdate_ EXCHANGE";
	my @shortcode_exchange_pair_vec_ = `$exec_cmd_`; chomp ( @shortcode_exchange_pair_vec_);
	for ( my $i_=0; $i_ <= $#shortcode_exchange_pair_vec_;$i_++ ){
		my @words_ = split(' ', $shortcode_exchange_pair_vec_[$i_]);
		if ( $#words_ >= 1 && $words_[1] eq 'CME' ){
			push ( @product_vec_, $words_[0] );
		}
	}
}


my $benchmark_file_ = "/spare/local/tradeinfo/cme_volume_ratio_thresholds.txt";
if (ExistsWithSize($benchmark_file_)){
	open BENCHMARKFILE, "< $benchmark_file_" or PrintStackTraceAndDie("Could not open benchmark file $benchmark_file_ for reading.\n");
	my @benchmark_values_ = <BENCHMARKFILE>;
	foreach my $line_ ( @benchmark_values_){
		if ( substr($line_, 0, 1) eq '#' ){
			next;
		}
		
		my @line_words_ = split ( ' ', $line_);
		if ( $#line_words_ >= 1 ){
			$shortcode_to_benchmark_ratio_{$line_words_[0]} = $line_words_[1];
		}
	}

} else {
	print "Benchmark File not available\n";
}

foreach my $product_ ( @product_vec_) {
	$shortcode_to_new_order_msg_count_{$product_} = 0;
	$shortcode_to_cancel_msg_count_{$product_} = 0;
	$shortcode_to_fill_number_count_{$product_} = 0;
	$shortcode_to_volume_count_{$product_} = 0;
	$shortcode_to_modify_msg_count_{$product_} = 0;
	
	my $obr_cmd_ = "$INSTALL_BIN_DIR/ors_binary_reader $product_ $tradingdate_ 2>/dev/null";
	my @obr_output_ =`$obr_cmd_`; chomp ( @obr_output_);
	#SYM: KEZ6 Px: 444.250000 INTPX: 1777 BS: B ST: 1471613402.143291 DT: 1471613402.143230 ORR: Seqd SAOS: 2875 CAOS: 1 CLTPOS: 0 SAMS: 1 SACI: 4980746 GBLPOS: 0 SIZE: 1 SE: 0 Seq: 140665268720000 BidLvSz: 0 AskLvSz: 0
	for ( my $id_=0; $id_ < $#obr_output_;$id_++){
		my @line_words_ = split(' ',$obr_output_[$id_]);
		if ( $#line_words_ <= 12 ) {
			next;
		}
		my $orr_ = $line_words_[13];
		if ( $orr_ eq "Seqd" ){
			$shortcode_to_new_order_msg_count_{$product_}++;
		} elsif ( $orr_ eq "CxlSeqd" ){
			$shortcode_to_cancel_msg_count_{$product_}++;
		} elsif ( $orr_ eq "Exec" ){
			$shortcode_to_fill_number_count_{$product_}++;
		} elsif ( $orr_ eq "CxRe" ){
			$shortcode_to_modify_msg_count_{$product_}++;
		}
	}
	
	my $product_vol_ = 0;
	my $exchange_symbol_ = `$INSTALL_BIN_DIR/get_exchange_symbol $product_ $tradingdate_`; chomp ( $exchange_symbol_);
	my $pnl_file_ = "$EOD_PNL_DIR/ors_pnls_$tradingdate_.txt";
	#print $pnl_file_."\n";
	if (ExistsWithSize($pnl_file_) ){
		open PNLFILE, "< $pnl_file_" or PrintStackTraceAndDie("Could not open eod-file for writing\n");
		my @pnl_lines_ = <PNLFILE>; chomp ( @pnl_lines_);
		
		foreach my $product_line_ (@pnl_lines_){
			my @product_line_words_ = split ( ' ', $product_line_); 
			if ( $#product_line_words_ >= 13 && $product_line_words_[1] eq $exchange_symbol_){
				$product_vol_ = $product_line_words_[13];
				#print $exchange_symbol_." ".$product_vol_."\n";
			}
		}
		close PNLFILE;
	}
	
	if ( $shortcode_to_cancel_msg_count_{$product_} > 0 || $shortcode_to_modify_msg_count_{$product_} > 0 ){
		my $msg_score_ = 3 * $shortcode_to_cancel_msg_count_{$product_} + $shortcode_to_modify_msg_count_{$product_};	
		my $fill_ratio_= 0;
		if ( $shortcode_to_fill_number_count_{$product_} > 0 )
		{
			$fill_ratio_= $msg_score_/$shortcode_to_fill_number_count_{$product_};
		}
		
		my $volume_ratio_ = 0;
		if ( $product_vol_ > 0 ){
			$volume_ratio_ = $msg_score_/$product_vol_;
		}
		
		my $benchmark_ratio_ = 0;
		if ( exists $shortcode_to_benchmark_ratio_{$product_}){
			$benchmark_ratio_ = $shortcode_to_benchmark_ratio_{$product_};
		}
		
		my $this_prod_string_ = sprintf("%12s %8d %5d %6d %0.3f %0.3f %0.3f", $product_, $msg_score_, $shortcode_to_fill_number_count_{$product_},$product_vol_,$fill_ratio_, $volume_ratio_, $benchmark_ratio_);
		$email_string_ = $email_string_." ".$this_prod_string_." \n";
	}
}

if ( $email_string_ ) {
	print "#shortcode #msg_score #total_msg #volume #fill_ratio #volume_ratio #benchmark\n";
	print $email_string_;
} else{
	print "No-Products Traded in CME.\n";
}