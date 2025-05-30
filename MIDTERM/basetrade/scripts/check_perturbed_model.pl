#!/usr/bin/perl

# \file scripts/check_perturbed_model.pl
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
use FileHandle;
use POSIX;
use List::Util qw/max min/; # for max

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

use File::Basename;
use Term::ANSIColor; 
use Data::Dumper;

my $DEBUG = 0;

#sub declarations
sub ReadStratFile;
sub ReadModelFile;
sub GenerateStrategies;
sub Selection;
sub Evaluate;
sub GetAverageSimPnlForPopulation;

my $USAGE="$0 STRAT_FILE START_DATE NUM_PREV_DAYS CHANGE [D]";
if ( $#ARGV < 3){ print $USAGE."\n"; exit ( 0 ); }

my $strat_file_ = $ARGV [ 0 ];
my $start_date_ = $ARGV [ 1 ];
my $num_days_ = $ARGV [ 2 ];
my $change_ = $ARGV [ 3 ];
my $base_model_file_ = "";
my $base_param_file_ = "";
my $base_start_time_ = "";
my $base_end_time_ = "";
my $base_prog_id_ = "";
my $base_pbat_dat_ = "";
my $base_shortcode_ = "";
my $base_model_start_text_ = "";
my $strat_pre_text_ = "";
my $strat_post_text_ = "";
my $work_dir_ = "/spare/local/$USER/PerturbedModelTests/"; 

if ( $#ARGV > 5 )
{
    if ( index ( $ARGV [ 6 ] , "D" ) != -1 ) { $DEBUG = 1; }
}

my @indicator_list_ = ( );

ReadStratFile( );

my %indicator_to_weight_base_model_ = ( );
ReadModelFile( );
$work_dir_ = $work_dir_."/".$base_shortcode_;
`mkdir -p $work_dir_ `; #make_work_dir

$strat_pre_text_ = "STRATEGYLINE ".$base_shortcode_." ".$base_pbat_dat_." ";
$strat_post_text_ = " ".$base_param_file_." ".$base_start_time_." ".$base_end_time_." ".$base_prog_id_;

my @strats_ = ( ) ;
my @strats_to_volume_score_ = ( );
my @strats_to_pnl_score_ = ( );

GenerateStrategies();
Evaluate();

my $index_ = 0;
print "Original Model ".ceil($strats_to_pnl_score_[$index_])." ".ceil($strats_to_volume_score_[$index_])."\n";
$index_ ++;

for (my $i = 0; $i <= $#indicator_list_ ; $i++ )
{
    print "$indicator_list_[$i] - ".ceil($strats_to_pnl_score_[$index_])." ".ceil($strats_to_volume_score_[$index_])."\n";
    $index_ ++;
    print "$indicator_list_[$i] + ".ceil($strats_to_pnl_score_[$index_])." ".ceil($strats_to_volume_score_[$index_])."\n";
    $index_ ++;
}

exit (0);

# Read the strategy file to get the model file, param file and other parameters
sub ReadStratFile
{
    if ( $DEBUG ) 
    { 
	print "ReadStratFile\n"; 
    }
    
    open STRAT_FILE, "< $strat_file_" or PrintStacktraceAndDie ( "Could not open strategy file $strat_file_ for reading\n" );
    my $strat_line_ = <STRAT_FILE>;
    my @strat_words_ = split ' ', $strat_line_;
    
    $base_shortcode_ = $strat_words_[1];
    $base_pbat_dat_ = $strat_words_[2];
    $base_model_file_ = $strat_words_[3];
    $base_param_file_ = $strat_words_[4];
    $base_start_time_ = $strat_words_[5];
    $base_end_time_ = $strat_words_[6];
    $base_prog_id_ = $strat_words_[7];
    return;
}

# Read the strategy file to get the model file, param file and other parameters
sub ReadModelFile
{
    #please make sure the model file has scores with '#', right now there is no check for that
    if ( $DEBUG )
    {
	print "ReadModelFile\n";
    }
    
    open MODEL_FILE, "< $base_model_file_" or PriceStacktraceAndDie ( "Could not open model file $base_model_file_ for reading\n" );
    
    my $indicator_start_reaced_ = 0;

    while ( my $model_line_ = <MODEL_FILE> )
    {
	chomp($model_line_);
		
	my @model_words_ = split ' ', $model_line_;

        if( not $indicator_start_reaced_ )
	{
	    $base_model_start_text_ = $base_model_start_text_.$model_line_."\n";
            if($model_words_[0] eq "INDICATORSTART")
            {
              $indicator_start_reaced_ = 1;
            }
	}

	my @i_words_ = @model_words_;
	shift(@i_words_); shift(@i_words_);
	pop(@i_words_);pop(@i_words_);

	if($model_words_[0] eq "INDICATOR")
	{
	    my $t_indicator_name_ = join(' ',@i_words_);
	    push( @indicator_list_, $t_indicator_name_);
	    $indicator_to_weight_base_model_{$t_indicator_name_} = $model_words_[1];
	}
    }
    return;
}

sub GenerateStrategies
{
    if ( $DEBUG )
    {
	print "GenerateInitialPopulation\n";
    }
    
    my %model_ = ();
    for (my $j = 0; $j <= $#indicator_list_; $j++)
    {
	$model_ { $indicator_list_[$j] } = $indicator_to_weight_base_model_ { $indicator_list_[$j] } ;
    }
    
    push ( @strats_, \%model_ );
    
    for (my $j = 0; $j <= $#indicator_list_; $j++)
    {
	{
	    my %t_model_ = %model_;					
	    $t_model_ { $indicator_list_[$j] } = ( 1 - $change_ ) * $t_model_ { $indicator_list_[$j] } ;
	    push (@strats_, \%t_model_ );
	}
	
	{	
	    my %t_model_ = %model_;
	    $t_model_ { $indicator_list_[$j] } = ( 1 + $change_ ) * $t_model_ { $indicator_list_[$j] } ;
	    push (@strats_, \%t_model_ );		
	}
	
    }

    return;
}


sub Evaluate
{
    if ( $DEBUG )
    {
	print "Evaluate\n";
    }
    my $t_strat_filename_ = $work_dir_."/"."tmp_strat";
    open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
    for ( my $i = 0; $i <= $#strats_ ; $i ++)
    {
	my $t_model_filename_ = $work_dir_."/"."tmp_model".$i;
	my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."\n";
	my $t_output_ = $base_model_start_text_;

	for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
	{
	    $t_output_ = $t_output_."INDICATOR ".$strats_[$i]{$indicator_list_[$j]}." ".$indicator_list_[$j]."\n";			
	}

	$t_output_ = $t_output_."INDICATOREND\n";

	open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
	print OUTMODEL $t_output_;
	close OUTMODEL;
	$t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_.$i."\n";
	print OUTSTRAT $t_strat_text_;
    }

    close OUTSTRAT;
    
    GetAverageSimPnlForPopulation($t_strat_filename_);	

    return;
}

sub GetAverageSimPnlForPopulation( )
{
    
    my ( $t_strat_filename_ ) = @_ ;
    if ( $DEBUG )
    {
	print "GetAveragePnlForPopulation\n";
    }
    my $tradingdate_ = $start_date_;
    my @t_sim_strategy_output_lines_ = ( );
    my $num_strats_running = 0;
    my @sample_dates =( );

    my $flag = 1;

    for ( my $t_day_index_ = 0; $t_day_index_ < $num_days_; $t_day_index_ ++ )
    {
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) || 
	     ( IsDateHoliday ( $tradingdate_ ) || ( ( $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
	    $t_day_index_ --;
	    next;
	}
	
	my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ 99919 $tradingdate_ ADD_DBG_CODE -1";
	my $log_=$work_dir_."sim_res_".$tradingdate_;
	
	my $t_tradingdate_ = "";
	push (@sample_dates, $tradingdate_);
	`$exec_cmd_ > $log_ `; 
	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
    }
    
    for (my $j = 0; $j <=$#strats_ ; $j++ )
    {
	push ( @strats_to_pnl_score_ , 0 );
	push ( @strats_to_volume_score_ , 0 );
    }

    foreach $tradingdate_ ( @sample_dates )
    {
	open(FILE, $work_dir_."sim_res_".$tradingdate_) or die("Unable to open file");
	# read file into an array
	@t_sim_strategy_output_lines_ = <FILE>;
	close(FILE);
	for (my $j = 0; $j <=$#t_sim_strategy_output_lines_ ; $j ++ )
	{
	    my @t_sim_rwords_ = split ( ' ', $t_sim_strategy_output_lines_[$j] );
	    $strats_to_pnl_score_[$j] += $t_sim_rwords_[1]/$num_days_;
	    $strats_to_volume_score_[$j] += $t_sim_rwords_[2]/$num_days_;
	}
    }
}

