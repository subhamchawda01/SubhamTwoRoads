#!/usr/bin/perl
use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO = "basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $BASETRADEINFODIR="/spare/local/tradeinfo/";

my $SPARE_HOME="/spare/local/".$USER."/";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

# start 
my $USAGE="$0 MODELFILENAME  TRADINGDATE  STARTHHMM  ENDHHMM  PROGID  OUTPUTFILENAME MSECSTIMEOUT L1EVENTSTIMEOUT NUMTRADESTIMEOUT <PRINT_DURING_ECO> <0/1>";

if ( $#ARGV < 8) 
{
    print "$USAGE\n";
    exit (-1);
}

#run the exec
my $model_file_name_ = $ARGV[0];
my $datagen_trading_date_ = $ARGV[1];
my $datagen_start_hhmm_ = $ARGV[2];
my $datagen_end_hhmm_ = $ARGV[3];
my $datagen_unique_gsm_id_ = $ARGV[4];
my $weighted_indicator_output_file_name_ = $ARGV[5]; # Contains the final output with extra weighted sum column.
my $datagen_msecs_timeout_ = $ARGV[6];
my $datagen_l1events_timeout_ = $ARGV[7];
my $datagen_num_trades_timeout_ = $ARGV[8];
my $to_print_on_economic_times_ = 0;
my $print_format_ = 0;
if ( $#ARGV >= 8 )
{
    $to_print_on_economic_times_ = $ARGV[9];
}
if ( $#ARGV >= 9 )
{
    $print_format_ = $ARGV[10];
}

my $datagen_output_file_name_ = $weighted_indicator_output_file_name_.".".$datagen_unique_gsm_id_.".".$datagen_trading_date_;

# Map from indicator index to indicator weight.
my %indicator_index_to_weight_map_ = (); 
my $indicator_count_ = 0;

# Read in INDICATOR weights.
open MODEL_FILE, "< $model_file_name_" or PrintStacktraceAndDie ( "Could not open $model_file_name_\n" );
while (my $model_file_line_ = <MODEL_FILE>) {
    chomp ($model_file_line_);

    my @model_file_lines_ = split (' ', $model_file_line_);

    if ($model_file_lines_[0] eq "INDICATOR") {
	$indicator_index_to_weight_map_{$indicator_count_} = $model_file_lines_[1];
	$indicator_count_++;
    }
}
close MODEL_FILE;

my $datagen_exec_cmd_ = "$LIVE_BIN_DIR/datagen $model_file_name_ $datagen_trading_date_ $datagen_start_hhmm_ $datagen_end_hhmm_ $datagen_unique_gsm_id_ $datagen_output_file_name_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";

# Run datagen
`$datagen_exec_cmd_`;

open DATAGEN_OUTPUT_FILE, "< $datagen_output_file_name_" or PrintStacktraceAndDie ( "Could not open $datagen_output_file_name_\n" );
open WEIGHTED_INDICATOR_OUTPUT_FILE, "> $weighted_indicator_output_file_name_" or PrintStacktraceAndDie ( "Could not open $weighted_indicator_output_file_name_\n" );

while (my $datagen_output_file_line_ = <DATAGEN_OUTPUT_FILE>) {
    chomp ($datagen_output_file_line_);

    my @datagen_output_file_lines_ = split (' ', $datagen_output_file_line_);

    if ($#datagen_output_file_lines_ + 1 != 4 + $indicator_count_) {
	# Malformed line in datagen?
	print "Error malformed line in datagen or model file\n";
	exit (-1);
    }

    my $indicator_weigted_sum_ = 0.0;
    # TIME L1EVENTS PRICE PRICE INDICATOR_1 INDICATOR_2 ... INDICATOR_N
    for (my $datagen_col_no_ = 4; $datagen_col_no_ <= $#datagen_output_file_lines_; $datagen_col_no_++) {
	$indicator_weigted_sum_ += ($datagen_output_file_lines_[$datagen_col_no_] * $indicator_index_to_weight_map_{$datagen_col_no_ - 4}); # Indicators were stored 0, 1, ...
    }
    
    if ( $print_format_ == 0  )
    {
	printf WEIGHTED_INDICATOR_OUTPUT_FILE "$datagen_output_file_lines_[0] $datagen_output_file_lines_[1] $datagen_output_file_lines_[2] $datagen_output_file_lines_[3] %0.6f\n", $indicator_weigted_sum_; # print TIME EVENTS_COUNTER BASEPRICE_TYPE PREDPRICE_TYPE TGT_VALUE
    }
    elsif ( $print_format_ == 1 )
    {
	printf WEIGHTED_INDICATOR_OUTPUT_FILE "$datagen_output_file_lines_[0] $datagen_output_file_lines_[1] $datagen_output_file_lines_[2] $datagen_output_file_lines_[3] %0.6f", $indicator_weigted_sum_; # print TIME EVENTS_COUNTER BASEPRICE_TYPE PREDPRICE_TYPE TGT_VALUE ALL_INDICATOR_VALUES
	for ( my $i = 4 ; $i < 4 + $indicator_count_ ; $i++ )
	{
	    printf WEIGHTED_INDICATOR_OUTPUT_FILE " $datagen_output_file_lines_[$i]";
	}
	printf WEIGHTED_INDICATOR_OUTPUT_FILE "\n";
    }
}

close WEIGHTED_INDICATOR_OUTPUT_FILE;
close DATAGEN_OUTPUT_FILE;

`rm -f $datagen_output_file_name_`;
