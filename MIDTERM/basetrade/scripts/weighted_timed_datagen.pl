#!/usr/bin/perl

# \file ModelScripts/generate_strategies.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use FileHandle;

my $HOME_DIR       = $ENV{'HOME'};
my $USER           = $ENV{'USER'};
my $SPARE_HOME     = "/spare/local/" . $USER . "/";
my $DATAGEN_LOGDIR = "/spare/local/logs/datalogs/";

my $REPO = "basetrade";

my $SCRIPTS_DIR      = $HOME_DIR . "/" . $REPO . "_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR . "/" . $REPO . "_install/ModelScripts";
my $GENPERLLIB_DIR   = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";
my $BIN_DIR          = $HOME_DIR . "/" . $REPO . "_install/bin";
#my $LIVE_BIN_DIR     = $HOME_DIR . "/LiveExec/bin";
my $LIVE_BIN_DIR = $HOME_DIR . "/" . $REPO . "_install/bin";


require "$GENPERLLIB_DIR/print_stacktrace.pl"
  ;    # PrintStacktrace , PrintStacktraceAndDie

if ($#ARGV != 9 ) {
	print "usage: datagen INDICATORLISTFILENAME TRADINGDATE UTC_STARTHHMM UTC_ENDHHMM PROGID OUTPUTFILENAME MSECS_PRINT l1EVENTS_PRINT NUM_TRADES_PRINT ECO_MODE\n";
	exit;
}

my $indicator_list_filename_ = $ARGV[0];
my $tradingdate_ = $ARGV[1];
my $datagen_start_hhmm_ = $ARGV[2];
my $datagen_end_hhmm_ = $ARGV[3];
my $unique_gsm_id_ = $ARGV[4];
my $this_day_timed_data_filename_ =  $ARGV[5];
my $datagen_msecs_timeout_ = $ARGV[6];
my $datagen_l1events_timeout_ = $ARGV[7];
my $datagen_num_trades_timeout_ = $ARGV[8];
my $to_print_on_economic_times_ = $ARGV[9];


my $this_day_timed_data_filename_weighted_ = $this_day_timed_data_filename_."_weighted";
my $this_day_timed_data_filename_weighted_ticks_ = $this_day_timed_data_filename_."_weighted_ticks";


my $exec_cmd="$BIN_DIR/datagen $indicator_list_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
`$exec_cmd`;


#my $timed_data_reader = FileHandle->new;
#my $weighted_timed_data_reader = FileHandle->new;
#my $ticks_weighted_timed_data_reader = FileHandle->new;

#$timed_data_reader->open ( "< $this_day_timed_data_filename_ " ) or PrintStacktraceAndDie ( "Could not open $this_day_timed_data_filename_ for reading\n" );
#$weighted_timed_data_reader->open ( "> $this_day_timed_data_filename_weighted_ " ) or PrintStacktraceAndDie ( "Could not open $this_day_timed_data_filename_weighted_ for writing\n" );
#$ticks_weighted_timed_data_reader->open ( "> $this_day_timed_data_filename_weighted_ticks_ " ) or PrintStacktraceAndDie ( "Could not open $this_day_timed_data_filename_weighted_ticks_ for writing\n" );

my @weights = ();
my $tick_size = 1;
my $sec_name = "";
my $line = -1;
open( INFILE_MODEL, "< $indicator_list_filename_" )
  or PrintStacktraceAndDie(
	" Could not open $indicator_list_filename_ for reading!\n");
my $count = -1;
while ( my $inline_ = <INFILE_MODEL> ) {
	my @words_ = split( ' ', $inline_ );

	if ( $line < 0 ){
		$line++;
		$sec_name = $words_[2];
	}
	
	if ($words_[0] eq "INDICATOR"){
		$count++;
		$weights[$count] = $words_[1]; 
	}
}
close(INFILE_MODEL);


my $tick_size_file = $HOME_DIR . "/" . $REPO . "/dvccode/CDefCode/security_definitions.cpp" ;
open( INFILE_TICK, "< $tick_size_file" )     
  or PrintStacktraceAndDie(
	" Could not open $tick_size_file for reading!\n");
while ( my $inline_ = <INFILE_TICK> ) {
	my @words_ = split( '"', $inline_ );
	if ($words_[1]){
	if ($words_[1] eq $sec_name){
		my @words_more = split(' ', $words_[2]);
		@words_more = split(',', $words_more[4]);
		$tick_size = $words_more[0];
		last;
	}
	}
}
	
close(INFILE_TICK);



#print "@weights";


open( OUTFILE1, "> $this_day_timed_data_filename_weighted_ " )
  or PrintStacktraceAndDie(
	"Could not open file $this_day_timed_data_filename_weighted_ for writing\n");
	
open( OUTFILE2, "> $this_day_timed_data_filename_weighted_ticks_ " )
  or PrintStacktraceAndDie(
	"Could not open file $this_day_timed_data_filename_weighted_ticks_ for writing\n");

open( INFILE, "< $this_day_timed_data_filename_" )
  or PrintStacktraceAndDie(
	" Could not open $this_day_timed_data_filename_ for reading!\n");

#my $tick_size =0.1;

my $lastcolindex_ = -1;        # primarily used to check
while ( my $inline_ = <INFILE> ) {
	my @words_ = split( ' ', $inline_ );
	if ( $lastcolindex_ == -1 ) {    #first iteration
		$lastcolindex_ = $#words_;
	}

	for (my $col = 4; $col <= $lastcolindex_; $col ++ ){
		$words_[$col] = $words_[$col] * $weights[$col - 4];
	}
	#$inline_ = join(' ', @words_);
	foreach (@words_){
		printf(OUTFILE1 "%.6f", $_);
		printf(OUTFILE1 " ");
	}
    #printf OUTFILE1 $inline_;
    printf OUTFILE1 "\n";
    
	for (my $col = 2; $col <= $lastcolindex_; $col ++ ){
		$words_[$col] = $words_[$col] / $tick_size ;
	}
	#$inline_ = join(' ', @words_);
	foreach (@words_){
		printf(OUTFILE2 "%.6f", $_);
		printf(OUTFILE2 " ");
	}
    #printf OUTFILE2 $inline_;
    printf OUTFILE2 "\n";
    
}
close(INFILE);
close(OUTFILE1);
close(OUTFILE2);





#$timed_data_reader->close;
#$weighted_timed_data_reader->close;
#$ticks_weighted_timed_data_reader->close;
