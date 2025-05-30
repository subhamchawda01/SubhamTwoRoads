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
use feature "switch";
use FileHandle;
use POSIX;
use List::Util qw/max min/; # for max
use File::Basename;
use Term::ANSIColor; 
#use Data::Dumper;
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
my $SAVE_TRADELOG_FILE=0;
if ( $USER ne "dvctrader" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/remove_first_two_words_from_string.pl"; # RemoveFirstTwoWordsFromString
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
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
require "$GENPERLLIB_DIR/array_ops.pl"; # GetSum
require "$GENPERLLIB_DIR/get_weighted_sum.pl"; # GetWeightedSum
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl";

my $DEBUG = 0;

#sub declarations
sub ReadStratFile;
sub FillDateVec;
sub ReadModelFile;
sub GenerateWeightVecs;
sub CalcPnls;
sub GetBestIndex ;
sub WriteOutModel ;
sub UpdateChangeFractionVec ;
sub CrossValidate;
sub CompareVectors;

my $USAGE="$0 input_strat_file output_model last_trading_date num_prev_days change_fraction max_number_iterations [algo=gradalpha3] [D] [sortalgo]";
if ( $#ARGV < 5 ) { print $USAGE."\n"; exit ( 0 ); }

my $strat_file_ = $ARGV [ 0 ];
my $output_model_filename_ = $ARGV[1];
my $last_trading_date_ = GetIsoDateFromStrMin1 ( $ARGV[2] ) ;
my $num_days_ = max ( 1, int($ARGV [ 3 ]) );
my $change_ = $ARGV [ 4 ];
my $max_number_of_iterations_ = max ( 1, int($ARGV [ 5 ]) );
my $calc_weights_algo_ = "gradalpha3"; 
if ( $#ARGV >= 6 ) { $calc_weights_algo_ = $ARGV[6]; } # BEST / GRAD

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

my $num_perturbations_ = 2;

if ( $#ARGV >= 7 )
{
    if ( index ( $ARGV [ 7 ] , "D" ) != -1 ) { $DEBUG = 1; print "DEBUG MODE\n"; }
}
my $sort_algo_ = "kCNAPnlAverage";
if ( $#ARGV >= 8 )
{
    $sort_algo_ = $ARGV[8];
}
my $use_ttc_bound_ = 0;
my $use_volume_bound_ = 0;
my $volume_min_train_ = 0;
my $volume_min_test_ = 0;
my $ttc_max_train_ = 0;
my $ttc_max_test_ = 0;
if ( $#ARGV >=9 )
{
    $use_volume_bound_=1;
    $volume_min_train_=int($ARGV[9]);
    $volume_min_test_=int($ARGV[9]);
}
if ( $#ARGV >=10 )
{
    $use_ttc_bound_=1;
    $ttc_max_train_=int($ARGV[10]);
    $ttc_max_test_=int($ARGV[10]);
}
#printf ("TTC bound %d VOLUME bound %d",$use_ttc_bound_,$use_volume_bound_);
# compute dates to train and crossvalidate on
my @training_date_vec_ = ();
my @testing_date_vec_ = ();
my @outofsample_date_vec_ = ();
my @sign_vec_ = ();
FillDateVec ( \@training_date_vec_, $last_trading_date_, 0.6 * $num_days_ );
if ( $#training_date_vec_ < 0 )
{
    print "No training dates\n";
    exit ( 0 );
}
FillDateVec ( \@testing_date_vec_, $training_date_vec_[$#training_date_vec_], 0.3 * $num_days_ );
if ( $#testing_date_vec_ < 0 )
{
    print "No testing dates\n";
    exit ( 0 );
}
FillDateVec ( \@outofsample_date_vec_, $testing_date_vec_[$#testing_date_vec_], 0.1 * $num_days_ );
if ( $#outofsample_date_vec_ < 0 )
{
    print "No outofsample dates\n";
    exit ( 0 );
}

if ( $DEBUG ) { printf ("NUmber of iterations : %d\n", $max_number_of_iterations_); }

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my @indicator_list_ = ( );

ReadStratFile( );

$work_dir_ = "/spare/local/$USER/PerturbedModelTests/"; 
$work_dir_ = $work_dir_."/".$base_shortcode_."/".$unique_gsm_id_;

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
	$work_dir_ = "/spare/local/$USER/PerturbedModelTests/"."/".$base_shortcode_."/".$unique_gsm_id_;
    }
    else
    {
	last;
    }
}

if ( -d $work_dir_ ) { `rm -rf $work_dir_`; }
`mkdir -p $work_dir_ `; #make_work_dir
if ( $DEBUG ) 
{ 
    printf ( "Working Directory: %s\n", $work_dir_ ); 
}
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);
my $start_time_ =`date +%s`; chomp ( $start_time_ );
$main_log_file_handle_->print ( "Optimizing $strat_file_ starting at $start_time_\n" );

if ( $DEBUG ) 
{
    print ( "Sort algo is $sort_algo_\n" ) ;
    print ( "Calc Weights algo is $calc_weights_algo_\n" ) ;
    $main_log_file_handle_->print ( "Sort algo is $sort_algo_\n" ) ;
    $main_log_file_handle_->print ( "Calc Weights algo is $calc_weights_algo_\n" ) ;
}

my @best_indicator_weight_vec_ = ( );
ReadModelFile( );
my @change_fraction_vec_ = ();
for ( my $i = 0; $i <= $#indicator_list_ ; $i ++ )
{
    push ( @change_fraction_vec_, $change_ ) ;
}
my @orig_indicator_weight_vec_ = @best_indicator_weight_vec_ ;

$main_log_file_handle_->printf ( " Date Vectors sizes TR : %d \t CV:%d \t OS:%d\n", $#training_date_vec_, $#testing_date_vec_, $#outofsample_date_vec_ ) ;
my $orig_pnl_ = 0;
my $orig_vol_ = 0;
my $orig_ttc_ = 0;

my $best_seen_pnl_ = 0;
my $best_seen_vol_ = 0;
my $best_seen_ttc_ = 0;
my @best_seen_indicator_weight_vec_ = (); # to store the max pnl seen so far ... due to nonlinearity

$strat_pre_text_ = "STRATEGYLINE ".$base_shortcode_." ".$base_pbat_dat_." ";
$strat_post_text_ = " ".$base_param_file_." ".$base_start_time_." ".$base_end_time_." ".$base_prog_id_;

my $improvement_made_ = 1;
my $iter_count_ = 1;
my $num_change_factions_significant_ = 1;

while ( ( $iter_count_ < $max_number_of_iterations_ ) &&
	( $improvement_made_ == 1 ) )
{
    $main_log_file_handle_->printf ( "Starting iteration %d\n", $iter_count_ ) ;
    my @indicator_weight_vec_vec_ = GenerateWeightVecs(\@best_indicator_weight_vec_, $iter_count_);

    my @stats_vec_ = ( );
    my @volume_vec_ = ( );
    my @ttc_vec_ = ( );
    my @ttc_vol_check_ = ( );
    CalcPnls (\@indicator_weight_vec_vec_, \@stats_vec_, \@volume_vec_, \@ttc_vec_, $iter_count_, \@ttc_vol_check_, "train", \@training_date_vec_);

    if ( $iter_count_ == 1 )
    {
	$orig_pnl_ = $stats_vec_[0];
	$orig_vol_ = $volume_vec_[0];
	$orig_ttc_ = $ttc_vec_[0];

	$best_seen_pnl_ = $stats_vec_[0];
	$best_seen_vol_ = $volume_vec_[0];
	$best_seen_ttc_ = $ttc_vec_[0];
	
	my $tref_ = $indicator_weight_vec_vec_[0];
	@best_seen_indicator_weight_vec_ = @$tref_ ; # set to best
    }

    if ( 1 ) # $DEBUG ) 
    {
	$main_log_file_handle_->printf ( "No\tStats\tVolume\tTTC\n" );
	for ( my $i = 0 ; $i <= $#stats_vec_ ; $i ++ )
	{
	    $main_log_file_handle_->printf ( "%d\t%.2f\t%d\t%d\n", $i, $stats_vec_[$i], $volume_vec_[$i], $ttc_vec_[$i] ) ;
	}
    }
    my $should_check_=1;

    my $best_index_ = GetBestIndex ( \@stats_vec_, \@volume_vec_, \@ttc_vol_check_, 1 );
    $main_log_file_handle_->printf ("Best index is %d\n", $best_index_);

    # check if we had taken a bad step last time
    if ( $best_seen_pnl_ > $stats_vec_[$best_index_] )
    { # last time we had seen a best that is better than this time
	$main_log_file_handle_->printf ("Reset weights. We had taken a bad step current best pnl %d < old best pnl %d\n", $stats_vec_[$best_index_], $best_seen_pnl_) ;
	my $tref_ = $indicator_weight_vec_vec_[0];
	@best_indicator_weight_vec_ = @best_seen_indicator_weight_vec_ ;

	# make all changes half # $change_ = $change_/2.0;
	for ( my $cfidx_ = 0 ; $cfidx_ <= $#change_fraction_vec_ ; $cfidx_ ++ )
	{
	    $change_fraction_vec_[$cfidx_] /= 2.0 ; 
	}
	$iter_count_++;

	if ( -e $work_dir_."/exit_file" )
	{ last; }
	next; # continue hoping our alpha was too high
    }
    #my $vec_1 = $indicator_weight_vec_vec_[$best_index_];
    #my $vec_2 = $indicator_weight_vec_vec_[0];

    if ( ( $best_index_ > 0 ) &&
	 ( $best_index_ <= $#indicator_weight_vec_vec_ ) )#&& ( CompareVectors (\@$vec_1, \@$vec_2) == 1 ) )
    {  # if best index isn't 0
	$improvement_made_ = 1;

	if ( $best_seen_pnl_ < $stats_vec_[$best_index_] )
	{
	    $best_seen_pnl_ = $stats_vec_[$best_index_];
	    $best_seen_vol_ = $volume_vec_[$best_index_];
	    $best_seen_ttc_ = $ttc_vec_[$best_index_];
	    
	    my $tref_ = $indicator_weight_vec_vec_[$best_index_];
	    @best_seen_indicator_weight_vec_ = @$tref_ ; # set to best
	}
	if ( -e $work_dir_."/exit_file" )
	{ last; }

	@best_indicator_weight_vec_ = CalcNextWeights ( \@indicator_weight_vec_vec_, \@stats_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $best_index_, $calc_weights_algo_ );

	$main_log_file_handle_->printf ( "oldpnl_volume_ttc %d %d %d maxpnl_volume_ttc %d %d %d bestpnl_volume_ttc %d %d %d\n", int($stats_vec_[0]), int($volume_vec_[0]), int($ttc_vec_[0]), int($stats_vec_[$best_index_]), int($volume_vec_[$best_index_]), int($ttc_vec_[$best_index_]), int($best_seen_pnl_), int($best_seen_vol_), int($best_seen_ttc_)) ;
	if ( $DEBUG ) { $main_log_file_handle_->printf ( "new weights: %s\n", join(' ',@best_indicator_weight_vec_)) ;  }
	$iter_count_ ++;
    }
    else
    {
	if ( -e $work_dir_."/exit_file" )
	{ last; }

	$improvement_made_ = 0; # only breaking on weight insignificance
	$main_log_file_handle_->printf ( "No improvement made at iteration %d\n", $iter_count_ ) ;
	next;
    }

    my @stats_compare_vec_ = ();
    my $temp_ = $indicator_weight_vec_vec_[0];
#=pod
    my @stats_vec_crossval_ = ( );
    my @volume_vec_crossval_ = ( );
    my @ttc_vec_crossval_ = ( );
    my @ttc_vol_check_crossval_ = ( );
    my @indicator_weight_vec_vec_crossval_ = ( );
    push (@indicator_weight_vec_vec_crossval_, \@$temp_);
    push (@indicator_weight_vec_vec_crossval_, \@best_indicator_weight_vec_);
    
    CalcPnls (\@indicator_weight_vec_vec_crossval_, \@stats_vec_crossval_, \@volume_vec_crossval_, \@ttc_vec_crossval_, $iter_count_, \@ttc_vol_check_crossval_, "test", \@testing_date_vec_);

    #CrossValidate ( \@best_indicator_weight_vec_, \@$temp_, \@testing_date_vec_, \@stats_compare_vec_, $iter_count_ );
    # if ( $DEBUG ) {
    #for ( my $t_day_index_ = 0; $t_day_index_ <= $#testing_date_vec_ ; $t_day_index_ ++ )
    #{
    #printf ("Date %d PNL change %d\n", $testing_date_vec_[$t_day_index_], $stats_compare_vec_[$t_day_index_]) ;
    #}
#}
    #my $sum_pnl_diffs_ = GetSum (\@stats_compare_vec_);
    my $index_ = GetBestIndex ( \@stats_vec_crossval_, \@volume_vec_crossval_, \@ttc_vol_check_crossval_, 1 );
    printf("Best index on crossval: %d\n",$index_);
    
    if ( $DEBUG )
    {
	printf ("PNL Volume TTC base on crossval %d %d %d\tPNL Volume TTC merged on crossval %d %d %d\n",  $stats_vec_crossval_[0], $volume_vec_crossval_[0], $ttc_vec_crossval_[0], $stats_vec_crossval_[1], $volume_vec_crossval_[1], $ttc_vec_crossval_[1]);
	$main_log_file_handle_->printf ("PNL Volume TTC base on crossval %d %d %d\tPNL Volume TTC merged on crossval %d %d %d\n",  $stats_vec_crossval_[0], $volume_vec_crossval_[0], $ttc_vec_crossval_[0], $stats_vec_crossval_[1], $volume_vec_crossval_[1], $ttc_vec_crossval_[1]);
    }
    if ($index_ == 0 )
    {
	printf ("Reset weights. We had taken a bad step current best pnl on test data < old best pnl on test data or the new model doesn't pass the vol/ttc checks \n") ;
	@best_indicator_weight_vec_ = @best_seen_indicator_weight_vec_ ; # set to previous best
	for ( my $cfidx_ = 0 ; $cfidx_ <= $#change_fraction_vec_ ; $cfidx_ ++ )
        {
            $change_fraction_vec_[$cfidx_] /= 2.0 ;
        }
        next;
    }
=pod
	if ( $sum_pnl_diffs_ < 0) 
    {
    printf ("Reset weights. We had taken a bad step current best pnl on test data < old best pnl on test data \n") ;
    $main_log_file_handle_->printf ("Reset weights. We had taken a bad step current best pnl on test data < old best pnl on test data \n") ;
    @best_indicator_weight_vec_ = @best_seen_indicator_weight_vec_ ; # set to previous best
    for ( my $cfidx_ = 0 ; $cfidx_ <= $#change_fraction_vec_ ; $cfidx_ ++ )
    {
    $change_fraction_vec_[$cfidx_] /= 2.0 ;
    }
    next;
    
    }
=cut
    my $significance_thresh_ = 0.10 ;
    $num_change_factions_significant_ = UpdateChangeFractionVec ( \@stats_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $significance_thresh_ );
    if ( $num_change_factions_significant_ <= 0 )
    { # break out of loop if nothign significant
	$main_log_file_handle_->printf ( "Breaking out of loop since num of significant (%f) change fractions = 0\n", $significance_thresh_ );
	last;
    }
#=cut
    
}

# Print orig weight and final weight
printf ( "Orig on training: %d %d %d\n", $orig_pnl_, $orig_vol_, $orig_ttc_ );
$main_log_file_handle_->printf ( "Orig : %d %d %d\n", $orig_pnl_, $orig_vol_, $orig_ttc_ );
printf ( "Best on training: %d %d %d\n", $best_seen_pnl_, $best_seen_vol_, $best_seen_ttc_ );
$main_log_file_handle_->printf ( "Best : %d %d %d\n", $best_seen_pnl_, $best_seen_vol_, $best_seen_ttc_ );
#my @stats_compare_vec_ = ();
#CrossValidate ( \@best_seen_indicator_weight_vec_, \@orig_indicator_weight_vec_, \@testing_date_vec_, \@stats_compare_vec_, "final" );
#printf ("Final Difference in PNLs %d\n", GetSum (\@stats_compare_vec_)); 
#$main_log_file_handle_->printf ("Final Difference in PNLs %d\n", GetSum (\@stats_compare_vec_)); 
WriteOutModel ( \@best_seen_indicator_weight_vec_ );
my @stats_vec_crossval_ = ( );
my @volume_vec_crossval_ = ( );
my @ttc_vec_crossval_ = ( );
my @ttc_vol_check_crossval_ = ( );
my @indicator_weight_vec_vec_crossval_ = ( );
push (@indicator_weight_vec_vec_crossval_, \@orig_indicator_weight_vec_);
push (@indicator_weight_vec_vec_crossval_, \@best_indicator_weight_vec_);
CalcPnls (\@indicator_weight_vec_vec_crossval_, \@stats_vec_crossval_, \@volume_vec_crossval_, \@ttc_vec_crossval_, $iter_count_, \@ttc_vol_check_crossval_, "outofsample", \@outofsample_date_vec_);
printf ("Orig on outofsample: %d %d %d\n",$stats_vec_crossval_[0], $volume_vec_crossval_[0], $ttc_vec_crossval_[0]);
$main_log_file_handle_->printf ("Orig on outofsample: %d %d %d\n",$stats_vec_crossval_[0], $volume_vec_crossval_[0], $ttc_vec_crossval_[0]);
printf ("Best on outofsample: %d %d %d\n",$stats_vec_crossval_[1], $volume_vec_crossval_[1], $ttc_vec_crossval_[1]);
$main_log_file_handle_->printf ("Best on outofsample: %d %d %d\n",$stats_vec_crossval_[1], $volume_vec_crossval_[1], $ttc_vec_crossval_[1]);

my $end_time_ =`date +%s`; chomp ( $end_time_ );
$main_log_file_handle_->print ( "Wrote optimized $output_model_filename_ at end time $end_time_\n" );
$main_log_file_handle_->close;
exit (0);

# Read the strategy file to get the model file, param file and other parameters
sub ReadStratFile
{
    if ( $DEBUG ) 
    { 
	printf ( "ReadStratFile\n" ) ; 
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
	$main_log_file_handle_->print ( "ReadModelFile\n" );
    }
    
    print ( "ModelFile : $base_model_file_\n" ) ;
    $main_log_file_handle_->print ( "ModelFile : $base_model_file_\n" ) ;
    
    open MODEL_FILE, "< $base_model_file_" or PrintStacktraceAndDie ( "Could not open model file $base_model_file_ for reading\n" );
    
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

	if($model_words_[0] eq "INDICATOR")
	{
	    my $t_indicator_name_ = RemoveFirstTwoWordsFromString ( \@model_words_ ) ; 

	    push( @indicator_list_, $t_indicator_name_);
	    push( @best_indicator_weight_vec_, $model_words_[1] + 0 );
	    if ( $model_words_[1] + 0 >= 0 ) { push (@sign_vec_ , 1 ); }
	    else { push (@sign_vec_, -1 ); }
	}
    }
    return;
}

sub WriteOutModel
{
    if ( $DEBUG )
    {
	$main_log_file_handle_->print ( "WriteOutModel\n" );
    }
    my ( $tref_ ) = @_;

    my $indicator_index_ = 0;
    open OUTMODEL, "> $output_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_filename_ for writing\n" );
    open MODEL_FILE, "< $base_model_file_" or PrintStacktraceAndDie ( "Could not open model file $base_model_file_ for reading\n" );
    while ( my $model_line_ = <MODEL_FILE> )
    {
	chomp($model_line_);
	my @model_words_ = split ' ', $model_line_;
	
	if ( ( $#model_words_ >= 2 ) && 
	     ( $model_words_[0] eq "INDICATOR" ) )
	{
	    $model_words_[1] = ( $$tref_[$indicator_index_] + 0 ) ;
	    printf OUTMODEL "%s\n", join ( ' ', @model_words_ ) ;
	    $indicator_index_ ++;
	}
	else
	{
	    printf OUTMODEL "%s\n", $model_line_ ;
	}
    }

    close OUTMODEL;
}

sub FillDateVec
{
    @_ == 3 or die "FillDateVec called with !=3 args\n";
    my ( $training_date_vec_ref_, $t_last_trading_date_, $t_num_days_ ) = @_;
    if ( $DEBUG )
    {
	#$main_log_file_handle_->print  ("FillDateVec %d %d\n", $t_last_trading_date_ $t_num_days_ );
    }
    my $tradingdate_ = $t_last_trading_date_;
    for ( my $t_day_index_ = 0; $t_day_index_ < $t_num_days_; $t_day_index_ ++ )
    {
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) || 
	     ( IsDateHoliday ( $tradingdate_ ) || ( ( $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
	    $t_day_index_ --;
	    next;
	}
	push ( @$training_date_vec_ref_, $tradingdate_ ); 
	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
}

sub GenerateWeightVecs
{
    my ( $indicator_to_weight_base_vec_ref_, $iter_count_ ) = @_;

    my @indicator_weight_vec_vec_ = ();
    
    my @this_indicator_weight_vec_ = @$indicator_to_weight_base_vec_ref_ ;
    push ( @indicator_weight_vec_vec_, \@this_indicator_weight_vec_ );
    $main_log_file_handle_->printf ( "GenerateWeightVecs from \n%s\n", join(' ',@this_indicator_weight_vec_ ) ) ;
    
    for (my $j = 0; $j <= $#indicator_list_; $j++)
    {
	if ( $this_indicator_weight_vec_ [$j] != 0 )
	{
	    {
		my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
		$t_indicator_weight_vec_[$j] = ( 1 - $change_fraction_vec_[$j] ) * $this_indicator_weight_vec_ [$j] ;
		if ($t_indicator_weight_vec_[$j] * $sign_vec_[$j] < 0) { $t_indicator_weight_vec_[$j] = 0; }
		push ( @indicator_weight_vec_vec_, \@t_indicator_weight_vec_ );
		if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	    }
	    {
		my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
		$t_indicator_weight_vec_[$j] = ( 1 + $change_fraction_vec_[$j] ) * $this_indicator_weight_vec_ [$j] ;
		if ($t_indicator_weight_vec_[$j] * $sign_vec_[$j] < 0) { $t_indicator_weight_vec_[$j] = 0; }
		push ( @indicator_weight_vec_vec_, \@t_indicator_weight_vec_ );
		if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	    }
	    #{
	    #   my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
	    #   $t_indicator_weight_vec_[$j] = 0 ;
	    #   push ( @indicator_weight_vec_vec_, \@t_indicator_weight_vec_ );
	    #   if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	    #}
	}
    }

    return @indicator_weight_vec_vec_ ;
}

sub CalcPnls
{
    if ( $DEBUG )
    {
	$main_log_file_handle_->print ( "CalcPnls\n" );
    }
    my ( $indicator_weight_vec_vec_ref_, $stats_vec_ref_, $volume_vec_ref_, $ttc_vec_ref_, $iter_count_, $ttc_volume_check_ref_, $var_, $date_vec_ref_ ) = @_;
    my @date_vec_= ();
    #if ($var_ == "train" ){ $date_vec_=$training_date_vec_; }
    #else if ($var_ == "test" ) { $date_vec_=$testing_date_vec_; }
    #else { $date_vec_=$outofsample_date_vec_; }
    my %models_list_map_ = ( );
    my $t_strat_filename_ = $work_dir_."/"."tmp_strat_".$iter_count_."_".$var_;
    my $t_models_list_ = $work_dir_."/"."tmp_models_list_".$iter_count_."_".$var_;
    my $local_results_base_dir = $work_dir_."/local_results_base_dir".$iter_count_."_".$var_;
    open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
    open OUTMODELLIST, "> $t_models_list_" or PrintStacktraceAndDie ( "Could not open output_models_list_filename_ $t_models_list_ for writing\n" );
    for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
    {
	my $t_model_filename_ = $work_dir_."/"."tmp_model_".$iter_count_."_".$i."_".$var_;
	my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."\n"; # TODO not changing progid ?
	my $t_output_ = $base_model_start_text_;
	my $tref_ = $$indicator_weight_vec_vec_ref_[$i];

	for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
	{
	    $t_output_ = $t_output_."INDICATOR ".$$tref_[$j]." ".$indicator_list_[$j]."\n";
	}

	$t_output_ = $t_output_."INDICATOREND\n";

	open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
	print OUTMODEL $t_output_;
	close OUTMODEL;
	$t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_.$i."\n";
	print OUTSTRAT $t_strat_text_;
	print OUTMODELLIST "tmp_model_".$iter_count_."_".$i."_".$var_."\n";
	$models_list_map_{"tmp_model_".$iter_count_."_".$i."_".$var_}=$i;
    }
    close OUTSTRAT;
    close OUTMODELLIST;
    
    my @nonzero_tradingdate_vec_ = ( );
    my $start_date_ = "";
    my $end_date_ = "";

    for ( my $t_day_index_ = 0; $t_day_index_ <= $#$date_vec_ref_ ; $t_day_index_ ++ )
    {
	my $tradingdate_ = $$date_vec_ref_[$t_day_index_];
	
	my $sim_strat_cerr_file_ = $t_strat_filename_."_cerr"."_".$var_;
	my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
	my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 2>$sim_strat_cerr_file_"; # using hyper optimistic market_model_index, added nologs argument
	my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$iter_count_."_".$tradingdate_."_".$var_;
	if ( -e $t_sim_res_filename_ ) { `rm -f $t_sim_res_filename_`; }
	#$main_log_file_handle_->print ( "$exec_cmd_ | grep SIMRESULT > $t_sim_res_filename_\n" );
	my @sim_strategy_output_lines_=();
	my %unique_id_to_pnlstats_map_ =();
	`$exec_cmd_ > $t_sim_res_filename_ `; 
	#@sim_strategy_output_lines_=`$exec_cmd`;
	if ( ExistsWithSize ( $t_sim_res_filename_ ) )
	{	
	    push (@nonzero_tradingdate_vec_, $tradingdate_);
	    @sim_strategy_output_lines_=`cat $t_sim_res_filename_`;
	}
	my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
	#print "trades file name ".$this_tradesfilename_."\n";
	if ( ExistsWithSize ( $this_tradesfilename_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";
	    # print $main_log_file_handle_ "$exec_cmd\n";
	    my @pnlstats_output_lines_ = `$exec_cmd`;
	    for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
	    {
		my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
		if( $#rwords_ >= 1 )
		{
		    my $unique_sim_id_ = $rwords_[0];
		    splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
		    $unique_id_to_pnlstats_map_{$unique_sim_id_} = join ( ' ', @rwords_ );
		}
	    }
	}
	if ( $SAVE_TRADELOG_FILE == 0 )
	{
	`rm -f $this_tradesfilename_`;
	my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_gsm_id_);
	`rm -f $this_tradeslogfilename_`;
	}

	my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_."_".$var_.".txt" ;
	open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );

	for ( my $t_sim_strategy_output_lines_index_ = 0, my $psindex_ = 0; $t_sim_strategy_output_lines_index_ <= $#sim_strategy_output_lines_; $t_sim_strategy_output_lines_index_ ++ )
	{
	    if ( $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] =~ /SIMRESULT/ )
	    { # SIMRESULT pnl volume sup% bestlevel% agg%
		my @rwords_ = split ( ' ', $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] );
		if ( $#rwords_ >= 2 )
		{
		    splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc
		    my $remaining_simresult_line_ = join ( ' ', @rwords_ );
		    if ( ( $rwords_[1] > 0 ) || # volume > 0
			 ( ( $base_shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day
		    {
			my $unique_sim_id_ = GetUniqueSimIdFromCatFile ( $t_strat_filename_, $psindex_ );
			if ( ! exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
			{
			    $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0 0 0";
#                               PrintStacktraceAndDie ( "unique_id_to_pnlstats_map_ missing $unique_sim_id_ for listfile: $temp_results_list_file_ catfile: $temp_strategy_cat_file_ rline: $remaining_simresult_line_\n" );
			}
			#printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_ ;
			printf TRLF "%s %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_;
		    }
		}
		else
		{
		    PrintStacktraceAndDie ( "ERROR: SIMRESULT line has less than 3 words\n" );
		}
		$psindex_ ++;
	    }
	}
	close TRLF;
	if ( ExistsWithSize ( $temp_results_list_file_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database_2.pl $t_strat_filename_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir/$base_shortcode_"; # TODO init $local_results_base_dir
	    #print $main_log_file_handle_ "$exec_cmd\n";
	    my $this_local_results_database_file_ = `$exec_cmd`;
	    #push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
	    if ($end_date_ eq ""){  $end_date_ = $tradingdate_; }
	    $start_date_ = $tradingdate_;
	}

	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
    }  
    my $statistics_result_file_ = $work_dir_."/"."stats_res_file_".$iter_count_."_".$var_;
    my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $sort_algo_ | awk '{print \$2, \$5, \$10, \$NF}' > $statistics_result_file_";
    #print $main_log_file_handle_ "$exec_cmd\n";
    my $results_ = `$exec_cmd`;
    my @count_instances_ = ();
    for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
    {
	push ( @$stats_vec_ref_ , 0 );
	push ( @$volume_vec_ref_ , 0 );
	push ( @$ttc_vec_ref_ , 0 );
	push ( @count_instances_, 0 );
        push ( @$ttc_volume_check_ref_, 1 );
    }
    
    if ( $DEBUG ) { $main_log_file_handle_->printf ( "Num days %s results = %d\n%s\n", $var_,( 1 + $#nonzero_tradingdate_vec_ ), join ( ' ', @nonzero_tradingdate_vec_ ) ) ; }
    if ( $#nonzero_tradingdate_vec_ >= 0 )
    { # at least 1 day
=pod
	    foreach my $tradingdate_ ( @nonzero_tradingdate_vec_ )
	{
	my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$iter_count_."_".$tradingdate_;
	if ( ExistsWithSize ( $t_sim_res_filename_ ) )
	{
	if ( $DEBUG ) { $main_log_file_handle_->printf ( "reading result file %s\n", $t_sim_res_filename_ ) ; }
	open(FILE, $t_sim_res_filename_) or die("Unable to open sim res file $t_sim_res_filename_");
	# read file into an array
	my @t_sim_strategy_output_lines_ = <FILE>;
	close(FILE);
	for (my $j = 0; $j <= $#t_sim_strategy_output_lines_ ; $j ++ )
	{
	my @t_sim_rwords_ = split ( ' ', $t_sim_strategy_output_lines_[$j] );
	if ( ( $#t_sim_rwords_ >= 2 ) &&
	    ( $t_sim_rwords_[0] eq "SIMRESULT" ) )
	{
	$$pnl_vec_ref_[$j] += $t_sim_rwords_[1];
	$$volume_vec_ref_[$j] += $t_sim_rwords_[2];
	$count_instances_[$j] += 1;
	}
	}
	}
	}
=cut
	if ( ExistsWithSize ($statistics_result_file_) )
	{
	    #if ( $DEBUG ) { $main_log_file_handle_->printf ( "reading result file %s\n", $statistics_result_file_ ) ; }
	    open(FILE, $statistics_result_file_) or die ("Unable to open statistics result file $statistics_result_file_");
	    my @t_stats_output_lines = <FILE>;
	    close(FILE);
	    for (my $j = 0; $j <= $#t_stats_output_lines ; $j ++ )
	    {
		my @t_stat_rwords_ = split ( ' ', $t_stats_output_lines[$j]);
		my $index = $models_list_map_{$t_stat_rwords_[0]};
		$$stats_vec_ref_[$index]=$t_stat_rwords_[3];
		$$volume_vec_ref_[$index]=$t_stat_rwords_[1];
		$$ttc_vec_ref_[$index]=$t_stat_rwords_[2];
	    }
	    for (my $j = 0; $j <= $#t_stats_output_lines ; $j ++ )
            {
    	        if ( $j==0 && $iter_count_<=2 )
                {
		    if ($var_ eq "train" && $iter_count_==1)
		    {
			if ( $use_ttc_bound_== 1 && $ttc_max_train_<$$ttc_vec_ref_[$j]) { $ttc_max_train_=$$ttc_vec_ref_[$j]; }
			if ( $use_volume_bound_==1 && $volume_min_train_>$$volume_vec_ref_[$j]) { $volume_min_train_=$$volume_vec_ref_[$j]; }
			printf ("Final volume and ttc bounds for %s are %d %d\n",$var_, $volume_min_train_, $ttc_max_train_);
		    }
		    if ($var_ eq "test" )
		    {
			if ( $use_ttc_bound_== 1 && $ttc_max_test_<$$ttc_vec_ref_[$j]) { $ttc_max_test_=$$ttc_vec_ref_[$j]; }
			if ( $use_volume_bound_==1 && $volume_min_test_>$$volume_vec_ref_[$j]) { $volume_min_test_=$$volume_vec_ref_[$j]; }
			printf ("Final volume and ttc bounds for %s are %d %d\n",$var_, $volume_min_test_, $ttc_max_test_);
		    }
                }
		if ( $use_ttc_bound_== 1 && $var_ eq "train" )
                {
		    if ( $$ttc_vec_ref_[$j] >= $ttc_max_train_ )
		    {
                        $$ttc_volume_check_ref_[$j] = 0;
		    }
                }
                if ( $use_volume_bound_== 1 && $$ttc_volume_check_ref_[$j]== 1 && $var_ eq "train")
                {
		    if ( $$volume_vec_ref_[$j] <= $volume_min_train_ )
		    {
                        $$ttc_volume_check_ref_[$j] = 0;
		    }
                }
		if ( $use_ttc_bound_== 1 && $var_ eq "test" )
                {
		    if ( $$ttc_vec_ref_[$j] >= $ttc_max_test_ )
		    {
                        $$ttc_volume_check_ref_[$j] = 0;
		    }
                }
                if ( $use_volume_bound_ == 1 && $$ttc_volume_check_ref_[$j]== 1 && $var_ eq "test")
                {
		    if ( $$volume_vec_ref_[$j] <= $volume_min_test_ )
		    {
                        $$ttc_volume_check_ref_[$j] = 0;
		    }
                }
	    }

	}
    }

    #compute averages
=pod
	if ( ( $#count_instances_ >= 0 ) &&
	( $count_instances_[0] > 0 ) )
    {
    for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
    {
    $$pnl_vec_ref_[$i] = $$pnl_vec_ref_[$i] / $count_instances_[$i] ;
    $$volume_vec_ref_[$i] = $$volume_vec_ref_[$i] / $count_instances_[$i] ;
    }
    }
=cut
}

sub GetBestIndex
{
    my ( $pnl_vec_ref_, $volume_vec_ref_, $ttc_volume_check_ref_, $should_check_ ) = @_;
    my $best_index_ = 0;
    my @sorting_value_vec_ = ();
    for ( my $i = 0 ; $i <= $#$pnl_vec_ref_ ; $i ++ )
    {
	push ( @sorting_value_vec_, $$pnl_vec_ref_[$i] );
	#printf( "index %d check_pass %d\n", $i, $$ttc_volume_check_ref_[$i] );
    }
    my $max_val_ = $sorting_value_vec_[0];
    for ( my $i = 1 ; $i <= $#sorting_value_vec_ ; $i ++ )
    {
	if ( ( $sorting_value_vec_[$i] > $max_val_ ) &&
	     (( $$ttc_volume_check_ref_[$i]==1 && $should_check_==1 ) || $should_check_==0) )
	{
	    $max_val_ = $sorting_value_vec_[$i];
	    $best_index_ = $i;
	}
    }
    return $best_index_;
}

sub UpdateChangeFractionVec
{
    my ( $pnl_vec_ref_, $volume_vec_ref_, $num_indicators_, $significance_thresh_ ) = @_;

    my $t_num_change_factions_significant_ = 0;

    # sanity checks
    if ( ( $#$pnl_vec_ref_ != $#$volume_vec_ref_ ) ||
	 ( $#$pnl_vec_ref_ < ( $num_perturbations_ * $num_indicators_ ) ) )
    {
	return ;
    }
    
    my $base_score_ = $$pnl_vec_ref_[0];
    for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
    {
	my $minus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $i) + 1];
	my $plus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $i) + 2];
	if ( ( $base_score_ >= ( $minus_score_ - 1 ) ) &&
	     ( $base_score_ >= ( $plus_score_ - 1 ) ) )
	{
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "Halving CF ( %d ) = %f PNLS: %d %d %d \n", (1+$i),$change_fraction_vec_[$i], $base_score_, $minus_score_, $plus_score_ ); }
	    $change_fraction_vec_[$i] /= 2.0;
	}
	if ( ( $change_fraction_vec_[$i] > $significance_thresh_ ) &&
	     ( max ( abs ( $base_score_ - $plus_score_ ), abs ( $base_score_ - $minus_score_ ) ) >= ( 0.05 * abs ( $base_score_ ) ) ) ) # at least 5 percent PNL difference
	{
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "CF ( %d ) = %f still significant \n", (1+$i), $change_fraction_vec_[$i], $base_score_, $minus_score_, $plus_score_ ); }
	    $t_num_change_factions_significant_ ++;
	}
    }
    return $t_num_change_factions_significant_;
}

sub CalcNextWeights
{
    my @t_best_indicator_weight_vec_ = ();
    my ( $indicator_weight_vec_vec_ref_, $pnl_vec_ref_, $volume_vec_ref_, $num_indicators_, $best_index_, $calc_weights_algo_ ) = @_;
    my $alpha_ = 0.5;

    if ( ( $calc_weights_algo_ eq "grad10" ) ||
	 ( $calc_weights_algo_ eq "grad5" ) ||
	 ( $calc_weights_algo_ eq "grad2" ) ||
	 ( $calc_weights_algo_ eq "grad1" ) ||
	 ( $calc_weights_algo_ eq "grad0.5" ) ||
	 ( $calc_weights_algo_ eq "grad0.25" ) ||
	 ( $calc_weights_algo_ eq "gradalpha3" ) ||
	 ( $calc_weights_algo_ eq "gradalpha10" ) )
    {
	if ( $calc_weights_algo_ eq "grad10" ) 
	{ $alpha_ = 10; }
	if ( $calc_weights_algo_ eq "grad5" ) 
	{ $alpha_ = 5; }
	if ( $calc_weights_algo_ eq "grad2" ) 
	{ $alpha_ = 2; }
	if ( $calc_weights_algo_ eq "grad1" ) 
	{ $alpha_ = 1; }
	if ( $calc_weights_algo_ eq "grad0.5" ) 
	{ $alpha_ = 0.5; }
	if ( $calc_weights_algo_ eq "grad0.25" ) 
	{ $alpha_ = 0.25; }
	if ( $calc_weights_algo_ eq "gradalpha3" ) 
	{ $alpha_ = ( ( $iter_count_ + 3 ) / ( $iter_count_ ) ) ; }
	if ( $calc_weights_algo_ eq "gradalpha10" ) 
	{ $alpha_ = ( ( $iter_count_ + 10 ) / ( $iter_count_ ) ) ; }

	if ( $DEBUG ) 
	{ 
	    $main_log_file_handle_->print ( "$calc_weights_algo_ $alpha_ $iter_count_\n" );
	}

	# sanity checks
	if ( ( $#$pnl_vec_ref_ != $#$volume_vec_ref_ )) #||
	    #( $#$pnl_vec_ref_ < ( $num_perturbations_ * $num_indicators_ ) ) )
	{
	    my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
	    @t_best_indicator_weight_vec_ = @$tref_ ; # set to best
	    return @t_best_indicator_weight_vec_ ;
	}
	my $tref_ = $$indicator_weight_vec_vec_ref_[0];
	
	my $base_score_ = $$pnl_vec_ref_[0];
	my @score_gradient_vec_ = ();
	my @sign_gradient_vec_ = ();
	for ( my $i = 0, my $j = 0 ; $i < $num_indicators_; $i ++ )
	{
	    if ( $$tref_[$j]!=0 )
	    {
		my $minus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $j) + 1];
		my $plus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $j) + 2];
		#my $zero_score_ = $$pnl_vec_ref_[($num_perturbations_ * $j) + 3];
		my @scores_vec_ = ();
		push( @scores_vec_, $minus_score_ );
		push( @scores_vec_, $plus_score_ );
		#  push( @scores_vec_, $zero_score_);
		my $should_check_ = 0;
		my $max_score_index_ = GetBestIndex(\@scores_vec_, \@scores_vec_, \@scores_vec_, 0);
		if ( $scores_vec_[$max_score_index_] > $base_score_ )
		{
		    push ( @score_gradient_vec_, $scores_vec_[$max_score_index_]-$base_score_ );
		    push ( @sign_gradient_vec_, $max_score_index_ + 1 );
		}
		else
		{
		    push ( @score_gradient_vec_, 0 );
		    push ( @sign_gradient_vec_, 0 );
		}
		$j++;
	    }
	}
	my $sum_gradients_ = GetSum ( \@score_gradient_vec_ );
	
	if ( $sum_gradients_ > 0 )
	{
	    $main_log_file_handle_->printf (" Sum of gradients is more than 0 \n");
	    my $tref_ = $$indicator_weight_vec_vec_ref_[0];
	    @t_best_indicator_weight_vec_ = @$tref_ ; # set it to the first vector and return
	    
	    for ( my $i = 0, my $j = 0 ; $i < $num_indicators_; $i ++ )
	    {
		if ( $$tref_[$j]!=0 )
		{
		    my $minus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $j) + 1];
		    my $plus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $j) + 2];
		    
		    #if ( $DEBUG ) { $main_log_file_handle_->printf ( "For indicator %d weight = %f\n", $i, $sign_gradient_vec_[$i] * ( $alpha_ * $score_gradient_vec_[$i] / $sum_gradients_ ) ) ; }
		    
		    $tref_ = $$indicator_weight_vec_vec_ref_[($num_perturbations_ * $j) + $sign_gradient_vec_[$j]];
		    $t_best_indicator_weight_vec_[$i] += ( $alpha_ * $score_gradient_vec_[$j] / $sum_gradients_ ) * ( $$tref_[$i] - $t_best_indicator_weight_vec_[$i] ) ;
		    if ( $sign_gradient_vec_[$i] == 3 ) { $t_best_indicator_weight_vec_[$i] += ( $alpha_ * $score_gradient_vec_[$j] / $sum_gradients_ ) * ( 0 - $t_best_indicator_weight_vec_[$i] ) ; }
		    $j++;
		}
	    }
	}
	else
	{ # somehow all gradients are <= 0 .. should not happen since best_index_ is not 0, but anyway
	    my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
	    @t_best_indicator_weight_vec_ = @$tref_ ; # set it to the best vector and return
	}
    }
    if ( ( $calc_weights_algo_ eq "best10" ) ||
	 ( $calc_weights_algo_ eq "best5" ) ||
	 ( $calc_weights_algo_ eq "best2" ) ||
	 ( $calc_weights_algo_ eq "best1" ) ||
	 ( $calc_weights_algo_ eq "best0.5" ) ||
	 ( $calc_weights_algo_ eq "best0.25" ) ||
	 ( $calc_weights_algo_ eq "bestalpha3" ) ||
	 ( $calc_weights_algo_ eq "bestalpha10" ) )
    {
	if ( $calc_weights_algo_ eq "best10" ) 
	{ $alpha_ = 10; }
	if ( $calc_weights_algo_ eq "best5" ) 
	{ $alpha_ = 5; }
	if ( $calc_weights_algo_ eq "best2" ) 
	{ $alpha_ = 2; }
	if ( $calc_weights_algo_ eq "best1" ) 
	{ $alpha_ = 1; }
	if ( $calc_weights_algo_ eq "best0.5" ) 
	{ $alpha_ = 0.5; }
	if ( $calc_weights_algo_ eq "best0.25" ) 
	{ $alpha_ = 0.25; }
	if ( $calc_weights_algo_ eq "bestalpha3" ) 
	{ $alpha_ = ( ( $iter_count_ + 3 ) / ( $iter_count_ ) ) ; }
	if ( $calc_weights_algo_ eq "bestalpha10" ) 
	{ $alpha_ = ( 10 / ( $iter_count_ ) ) ; }

	if ( $DEBUG ) 
	{ 
	    $main_log_file_handle_->print ( "$calc_weights_algo_ $alpha_ $iter_count_\n" ) ;
	}

	my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
	@t_best_indicator_weight_vec_ = @$tref_ ; # linear combination of first and best

	GetWeightedSum ( \@t_best_indicator_weight_vec_, $$indicator_weight_vec_vec_ref_[$best_index_], $$indicator_weight_vec_vec_ref_[0], $alpha_ ) ;

	if ( $DEBUG ) 
	{ 
	    $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
	    $main_log_file_handle_->printf ( "A weights: %s\n", join(' ',@$tref_) ) ; 
	    $tref_ = $$indicator_weight_vec_vec_ref_[0];
	    $main_log_file_handle_->printf ( "B weights: %s\n", join(' ',@$tref_) ) ; 
	    $main_log_file_handle_->printf ( "Final weights: %s\n", join(' ',@t_best_indicator_weight_vec_) ) ; 
	}

	return @t_best_indicator_weight_vec_ ;
    }


    if ( $#t_best_indicator_weight_vec_ < 0 )
    { # default = best1
	my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
	@t_best_indicator_weight_vec_ = @$tref_ ; # set it to the best vector and return
    }

    return @t_best_indicator_weight_vec_ ;
}

sub CrossValidate
{
    my ( $new_coeff_vec_ref_, $origin_coeff_vec_ref_, $testing_date_vec_, $stats_compare_vec_ref_, $iteration_num_ ) = @_;
    my $t_strat_filename_ = $work_dir_."/"."original_strat_";

    open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );

    my $t_model_filename_ = $work_dir_."/"."original_model_";
    my $t_model_filename_1 = $work_dir_."/"."final_model_";
    my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."1\n";

    {
	my $t_output_ = $base_model_start_text_;
	for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
	{
	    $t_output_ = $t_output_."INDICATOR ".$$origin_coeff_vec_ref_[$j]." ".$indicator_list_[$j]."\n";
	}
	$t_output_ = $t_output_."INDICATOREND\n";
	
	open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
	print OUTMODEL $t_output_;
	close OUTMODEL;
    }
    
    print OUTSTRAT $t_strat_text_;

    $t_strat_text_ = $strat_pre_text_.$t_model_filename_1.$strat_post_text_."2\n";

    {
	my $t_output_ = $base_model_start_text_;
	for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
	{
	    $t_output_ = $t_output_."INDICATOR ".$$new_coeff_vec_ref_[$j]." ".$indicator_list_[$j]."\n";
	}
	$t_output_ = $t_output_."INDICATOREND\n";
	
	open OUTMODEL, "> $t_model_filename_1" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_1 for writing\n" );
	print OUTMODEL $t_output_;
	close OUTMODEL;
    }

    print OUTSTRAT $t_strat_text_;
    close OUTSTRAT;
    my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
    
    for ( my $t_day_index_ = 0; $t_day_index_ <= $#$testing_date_vec_ ; $t_day_index_ ++ )
    {
	my $tradingdate_ = $$testing_date_vec_[$t_day_index_];
	my $sim_strat_cerr_file_ = $t_strat_filename_."_cerr_final";

	my $exec_cmd_1 = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 2>$sim_strat_cerr_file_"; # using hyper optimistic market_model_index, added nologs argument
	my $t_sim_res_filename_ = $work_dir_."/"."sim_res_final_".$tradingdate_."_".$iteration_num_;
	if ( -e $t_sim_res_filename_ ) { `rm -f $t_sim_res_filename_`; }
	$main_log_file_handle_->print ( "$exec_cmd_1 | grep SIMRESULT > $t_sim_res_filename_\n" );
	`$exec_cmd_1 >> $t_sim_res_filename_ `;
	if ( ExistsWithSize ( $t_sim_res_filename_ ) )
	{
	    open(FILE, $t_sim_res_filename_) or die("Unable to open sim res file $t_sim_res_filename_");
	    my @t_sim_strategy_output_lines_ = <FILE>;
	    close(FILE);
	    my @t_sim_rwords_1_ = split ( ' ', $t_sim_strategy_output_lines_[0] );
	    my @t_sim_rwords_2_ = split ( ' ', $t_sim_strategy_output_lines_[1] );
	    push ( @$stats_compare_vec_ref_, $t_sim_rwords_2_[1] - $t_sim_rwords_1_[1] ); # TODO changfe to a better score than just pnl
	}
	else { push ( @$stats_compare_vec_ref_, 0 ); }
	
    }
}

sub CompareVectors
{
    my ( $vector_1_ref_, $vector_2_ref_ ) = @_;
    if ( $#$vector_1_ref_ != $#$vector_2_ref_ ) { return 0; }
    for ( my $i =0; $i <= $#$vector_1_ref_; $i ++ ) 
    { 
	if ( $$vector_1_ref_[$i] != $$vector_2_ref_[$i] ) 
	{ return 1; } 
    }
    return 0;
}

