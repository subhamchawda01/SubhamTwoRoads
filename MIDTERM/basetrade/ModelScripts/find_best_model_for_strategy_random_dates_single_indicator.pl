#!/usr/bin/perl

# \file scripts/find_best_model_for_strategy.pl
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
use List::Util 'shuffle'; # for shuffle
use File::Basename; # basename
use Term::ANSIColor; 
use Math::Complex ; # sqrt

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
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
require "$GENPERLLIB_DIR/array_ops.pl"; # GetSum GetIndexOfMaxValue
require "$GENPERLLIB_DIR/get_weighted_sum.pl"; # GetWeightedSum
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; #GetUniqueSimIdFromCatFile

my $DEBUG = 0;

#sub declarations
sub ReadStratFile;
sub GetTrainingDates;
sub ReadModelFile;
sub GenerateWeightVecs;
sub CalcScores;
sub GetMyScore;
sub WriteOutModel ;
sub HalveChangeFractionVec ;

my $USAGE="$0 input_strat_file output_model last_trading_date num_prev_days change_fraction [batch=10] [D] [sort_algo_] [use_fake_faster_data_]";
if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }

my $strat_file_ = $ARGV [ 0 ];
my $output_model_filename_ = $ARGV[1];
my $last_trading_date_ = GetIsoDateFromStrMin1 ( $ARGV[2] ) ;
my $num_days_ = max ( 1, int($ARGV [ 3 ]) );
my $change_ = $ARGV [ 4 ];
my $batch_size = 10;

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

my $num_perturbations_ = 2;
my $MAXSTEPSCALE = 3;


my $work_dir_ = "/spare/local/$USER/PerturbedModelTests/"; 

if ( $#ARGV >= 5 )
{
    $batch_size = int($ARGV[5]);
    if ( $#ARGV >= 6 )
    {
	if ( index ( $ARGV [6] , "D" ) != -1 ) { $DEBUG = 1; }
    }
}

my $sort_algo_ = "kCNAPnlAverage";
if ( $#ARGV >= 7 )
{
    $sort_algo_ = $ARGV[7];
}

my $use_fake_faster_data_ = rand(1) > 0.5 ? 1 : 0;  #using delays randomly, to set usage use param USE_FAKE_FASTER_DATA
if ( $#ARGV >= 8 )
{
    $use_fake_faster_data_ = $ARGV[8];
}

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my @indicator_list_ = ( );

ReadStratFile( );

$work_dir_ = "/spare/local/$USER/PerturbedModelTests/"; 
$work_dir_ = $work_dir_."/".$base_shortcode_."/".$unique_gsm_id_;

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
#	print STDERR "Surprising but this dir exists\n";
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

my @best_indicator_weight_vec_ = ( );
my $model_type_ = "LINEAR";
my @siglr_alpha_vec_ = ( );

ReadModelFile( );

my @change_fraction_vec_ = (); # this allows us to have a different change fraction for each indicator and not perturb indicators with change_fraction_vec_ [ i ] < K
for ( my $i = 0; $i <= $#indicator_list_ ; $i ++ )
{
    push ( @change_fraction_vec_, $change_ ) ;
}

my @orig_indicator_weight_vec_ = @best_indicator_weight_vec_ ;

my @training_date_vec_ = ();
GetTrainingDates ( \@training_date_vec_, $last_trading_date_, $num_days_ );
my $prob_day_selected_ = min ( 1.0, $batch_size / $num_days_ ) ;

printf ( "OutputModel : %s\n", $output_model_filename_ );

$strat_pre_text_ = "STRATEGYLINE ".$base_shortcode_." ".$base_pbat_dat_." ";
$strat_post_text_ = " ".$base_param_file_." ".$base_start_time_." ".$base_end_time_." ".$base_prog_id_;

my $improvement_made_ = 1;
my $iter_count_ = 1;
my $max_number_of_iterations_ = 10 * max (1, ($num_days_/$batch_size) ); # since each iteration now is just a part of the data
my @weight_vectors_to_try = ();
my $num_halves = 0;
my $best_index_ = 0;

push ( @weight_vectors_to_try, \@best_indicator_weight_vec_ ) ; # always index=0 is the best choice so far
push ( @weight_vectors_to_try, \@orig_indicator_weight_vec_ ) ; # always index=1 is the original weight-vec
my @score_vec_ = ( );
my @volume_vec_ = ( );
my @ttc_vec_ = ( );


while ( ( $iter_count_ < $max_number_of_iterations_ ) &&
	( $improvement_made_ == 1 ) )
{
    $main_log_file_handle_->printf ( "Starting iteration %d\n", $iter_count_ ) ;
    @score_vec_ = ( );
    @volume_vec_ = ( );
    @ttc_vec_ = ( );


    # add perturbations
    my @t_weight_vectors_to_try = @weight_vectors_to_try;
    push ( @weight_vectors_to_try, GenerateWeightVecs ( $weight_vectors_to_try[0], $iter_count_ )  ) ;

    @training_date_vec_ = shuffle ( @training_date_vec_ ) ; # adding shuffle so that we can add randomization to CalcScores
    CalcScores ( \@weight_vectors_to_try, \@score_vec_, \@volume_vec_ , \@ttc_vec_, $iter_count_ );

    if ( $DEBUG ) 
    {
	$main_log_file_handle_->printf ( "SCORE_VOLUME_TTC_VEC\n" );
	for ( my $i = 0 ; $i <= $#score_vec_ ; $i ++ )
	{
	    $main_log_file_handle_->printf ( "%d %d %d %d\n", $i, $score_vec_[$i], $volume_vec_[$i], $ttc_vec_[$i] ) ;
	}
    }

    $best_index_ = GetIndexOfMaxValue ( \@score_vec_ );
    $main_log_file_handle_->printf ( "BestIndexAT Iter:%d %d\n", $iter_count_, $best_index_ ) ;

    if ( $best_index_ == 0 ) 
    {
        HalveChangeFractionVec ( );
        @weight_vectors_to_try = @t_weight_vectors_to_try;
        $iter_count_ ++;
        next;
    }

    if ( $best_index_ > 0 )
    {  # if best index isn't 0
	$improvement_made_ = 1;
	$main_log_file_handle_->printf ( "oldbest %d %d %d newbest %d %d %d \n", int($score_vec_[0]), int($volume_vec_[0]), int($ttc_vec_[0]), int($score_vec_[$best_index_]), int($volume_vec_[$best_index_]), int($ttc_vec_[$best_index_]) ) ;

	my @prev_weight_vectors_to_try = @weight_vectors_to_try ;
	@weight_vectors_to_try = CalcNextWeights ( \@prev_weight_vectors_to_try, \@score_vec_ ) ;

	$iter_count_ ++;
    }
    else
    {
	$improvement_made_ = 0; # break

	# Print orig weight and final weight
    $main_log_file_handle_->("no improvement\n");
	$main_log_file_handle_-> ( "Orig : %d %d\n", $score_vec_[1], $volume_vec_[1], $ttc_vec_[1]);
	$main_log_file_handle_-> ( "Best : %d %d\n", $score_vec_[0], $volume_vec_[0], $ttc_vec_[0] );
    }
}

# Print orig weight and final weight
$main_log_file_handle_->printf ( "Orig : %d %d\n", int($score_vec_[1]), int($volume_vec_[1]), int($ttc_vec_[1]) );
$main_log_file_handle_->printf ( "Best : %d %d\n", int($score_vec_[0]), int($volume_vec_[0]), int($ttc_vec_[0]) );

WriteOutModel ( $weight_vectors_to_try[0] );

my $end_time_ =`date +%s`; chomp ( $end_time_ );
$main_log_file_handle_->print ( "Wrote optimized $output_model_filename_ at end time $end_time_\n" );
$main_log_file_handle_->close;
exit (0);

### --- ###

# Read the strategy file to get the model file, param file and other parameters
sub ReadStratFile
{
    if ( $DEBUG ) 
    { 
	printf ( "ReadStratFile\n" ) ; 
    }
    print "StratFile : $strat_file_\n";

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

        if ($model_words_[0] eq "MODELMATH")
        {
            $model_type_ = $model_words_[1];
        }

	if($model_words_[0] eq "INDICATOR")
	{
	    my $t_indicator_name_ = RemoveFirstTwoWordsFromString ( \@model_words_ ) ; 
	    push ( @indicator_list_, $t_indicator_name_ );
	    
	    if ( $model_type_ eq "SIGLR" )
	    {
		my @t_siglr_words_ = split ':', $model_words_[1];
		push( @siglr_alpha_vec_, $t_siglr_words_[0] );
		push( @best_indicator_weight_vec_, $t_siglr_words_[1] );
	    }
	    else
	    {
	        push( @best_indicator_weight_vec_, $model_words_[1] + 0 );		
	    }
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
	    if ( $model_type_ eq "SIGLR" )
	    {
		$model_words_[1] = join ( ':', $siglr_alpha_vec_[$indicator_index_], $model_words_[1] );
	    }
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

sub GetTrainingDates
{
    @_ == 3 or die "GetTrainingDates called with !=3 args\n";
    my ( $training_date_vec_ref_, $t_last_trading_date_, $t_num_days_ ) = @_;
    if ( $DEBUG )
    {
	$main_log_file_handle_->print ( "GetTrainingDates $t_last_trading_date_ $t_num_days_\n" );
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

    my @perturbed_weights_vec_ = ();
    
    my @this_indicator_weight_vec_ = @$indicator_to_weight_base_vec_ref_ ;
    if ( $DEBUG ) { $main_log_file_handle_->printf ( "GenerateWeightVecs from \n%s\n", join(' ',@this_indicator_weight_vec_ ) ) ; }

#perturb only the last indicator
    for (my $j = $#indicator_list_; $j <= $#indicator_list_; $j++)     
    {
      if ( abs ( $this_indicator_weight_vec_ [$j] ) > 0.08 * abs ( $orig_indicator_weight_vec_ [$j] ) )
	{ # current best weoght is still at least 1% of magnitude of original weight
	{ # 1-2alpha
	    my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
	    $t_indicator_weight_vec_[$j] = ( 1 - (2*$change_fraction_vec_[$j]) ) * $this_indicator_weight_vec_ [$j] ;
	    push ( @perturbed_weights_vec_, \@t_indicator_weight_vec_ );
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	}
	{ # 1-alpha
	    my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
	    $t_indicator_weight_vec_[$j] = ( 1 - $change_fraction_vec_[$j] ) * $this_indicator_weight_vec_ [$j] ;
	    push ( @perturbed_weights_vec_, \@t_indicator_weight_vec_ );
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	}
	if ( ! ( ( ( $change_fraction_vec_[$j] > 0.49 ) && ( $change_fraction_vec_[$j] < 0.51 ) ) ||
		 ( ( $change_fraction_vec_[$j] > 0.99 ) && ( $change_fraction_vec_[$j] < 1.01 ) ) ) )
	{ # 0
	    my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
	    $t_indicator_weight_vec_[$j] = 0;
	    push ( @perturbed_weights_vec_, \@t_indicator_weight_vec_ );
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	}
	{ #1+alpha
	    my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
	    $t_indicator_weight_vec_[$j] = ( 1 + $change_fraction_vec_[$j] ) * $this_indicator_weight_vec_ [$j] ;
	    push ( @perturbed_weights_vec_, \@t_indicator_weight_vec_ );
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	}
	{ #1+2alpha
	    my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
	    $t_indicator_weight_vec_[$j] = ( 1 + (2*$change_fraction_vec_[$j]) ) * $this_indicator_weight_vec_ [$j] ;
	    push ( @perturbed_weights_vec_, \@t_indicator_weight_vec_ );
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	}
      }
    }

    return @perturbed_weights_vec_ ;
}

sub CalcScores
{
    if ( $DEBUG )
    {
	$main_log_file_handle_->print ( "CalcPnls\n" );
    }
    my ( $indicator_weight_vec_vec_ref_, $stats_vec_ref_, $volume_vec_ref_, $ttc_vec_ref_, $iter_count_ ) = @_;
    my @date_vec_= ();
    my %models_list_map_ = ( );
    my $t_strat_filename_ = $work_dir_."/"."tmp_strat_".$iter_count_;
    my $t_models_list_ = $work_dir_."/"."tmp_models_list_".$iter_count_;
    my $local_results_base_dir = $work_dir_."/local_results_base_dir".$iter_count_;
    open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
    open OUTMODELLIST, "> $t_models_list_" or PrintStacktraceAndDie ( "Could not open output_models_list_filename_ $t_models_list_ for writing\n" );
    for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
    {
	my $t_model_filename_ = $work_dir_."/"."tmp_model_".$iter_count_."_".$i;
	my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."\n"; # TODO not changing progid ?
	my $t_output_ = $base_model_start_text_;
	my $tref_ = $$indicator_weight_vec_vec_ref_[$i];

	for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
	{
            if ( $model_type_ eq "SIGLR" )
            {
                $t_output_ = $t_output_."INDICATOR ".$siglr_alpha_vec_[$j].":".$$tref_[$j]." ".$indicator_list_[$j]."\n";
            }
            else
            {
                $t_output_ = $t_output_."INDICATOR ".$$tref_[$j]." ".$indicator_list_[$j]."\n";
            }
	}

	$t_output_ = $t_output_."INDICATOREND\n";

	open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
	print OUTMODEL $t_output_;
	close OUTMODEL;
	$t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_.$i."\n";
	print OUTSTRAT $t_strat_text_;
	print OUTMODELLIST "tmp_model_".$iter_count_."_".$i."\n";
	$models_list_map_{"tmp_model_".$iter_count_."_".$i}=$i;
    }
    close OUTSTRAT;
    close OUTMODELLIST;
    
    my @nonzero_tradingdate_vec_ = ( );
    my $start_date_ = "";
    my $end_date_ = "";
    my $num_days_selected = 0;
    
    for ( my $t_day_index_ = 0; $t_day_index_ <= $#training_date_vec_ ; $t_day_index_ ++ )
    {
	my $tradingdate_ = $training_date_vec_[$t_day_index_];
    
    if ( $prob_day_selected_ * $t_day_index_ < $num_days_selected )
	{ # selection
	    next;
	}
	$num_days_selected ++;

	my $sim_strat_cerr_file_ = $t_strat_filename_."_cerr";
	my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
	my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ $unique_gsm_id_ $tradingdate_ $market_model_index_ 0 0.0 $use_fake_faster_data_ ADD_DBG_CODE -1 2>$sim_strat_cerr_file_"; # using hyper optimistic market_model_index, added nologs argument
	my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$iter_count_."_".$tradingdate_;
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
	my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_ ;
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

    }  
    my $statistics_result_file_ = $work_dir_."/"."stats_res_file_".$iter_count_;
    my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $sort_algo_ > $statistics_result_file_";
    #print $main_log_file_handle_ "$exec_cmd\n";
    my $results_ = `$exec_cmd`;
    for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
    {
	push ( @$stats_vec_ref_ , 0 );
	push ( @$volume_vec_ref_ , 0 );
	push ( @$ttc_vec_ref_ , 0 );
    }
    
    #if ( $DEBUG ) { $main_log_file_handle_->printf ( "Num days %s results = %d\n%s\n", $var_,( 1 + $#nonzero_tradingdate_vec_ ), join ( ' ', @nonzero_tradingdate_vec_ ) ) ; }
    if ( $#nonzero_tradingdate_vec_ >= 0 )
    { # at least 1 day
	if ( ExistsWithSize ($statistics_result_file_) )
	{
	    #if ( $DEBUG ) { $main_log_file_handle_->printf ( "reading result file %s\n", $statistics_result_file_ ) ; }
	    open(FILE, $statistics_result_file_) or die ("Unable to open statistics result file $statistics_result_file_");
	    my @t_stats_output_lines = <FILE>;
	    close(FILE);
	    for (my $j = 0; $j <= $#t_stats_output_lines ; $j ++ )
	    {
		my @t_stat_rwords_ = split ( ' ', $t_stats_output_lines[$j]);
		my $index = $models_list_map_{$t_stat_rwords_[1]};
		$$stats_vec_ref_[$index]=$t_stat_rwords_[-1];
		$$volume_vec_ref_[$index]=$t_stat_rwords_[4];
		$$ttc_vec_ref_[$index]=$t_stat_rwords_[9];
	    }
	}
    }
}


sub HalveChangeFractionVec
{
# scour through all non 0 index options and 
# for each indicator that had no option where
# the value was different and pnl was higher
# reduce change_fraction_vec_

    for ( my $i = 0; $i <= $#change_fraction_vec_ ; $i ++ )
    {
	$change_fraction_vec_[$i] /= 2.0 ; 
    }

}

sub CalcNextWeights
{
    my @next_weight_vectors_to_try_ = (); # list of refs of indicator weights to try
    # index=0 is best
    # index=1 is orig
    # then the other contenders to best
    # index=3 w[j] = w[0,j] + ( Sum over +ve perturbations : delta(i) * w[i,j] - w[0,j] )/( Sum delta(i) )
    # index=4 w[j] is max for that indicator
    # index=5 w[j] uses bothe +ve and -ve 


    my ( $prev_weight_vectors_to_try_ref_, $score_vec_ref_ ) = @_;
    my $t_step_size = max ( $prob_day_selected_, ( $batch_size / 30 ) ) ; # in other words this is related to the training set size as a fraction of batch size by total num_days_

    my @delta_vec = ();
    for ( my $i = 0 ; $i <= $#$score_vec_ref_ ; $i ++ ) {
      push ( @delta_vec, ( $$score_vec_ref_[$i] - $$score_vec_ref_[0] ) ) ;
    }
    my $t_best_index = GetIndexOfMaxValue ( $score_vec_ref_ );
    if ( $t_best_index <= $#$prev_weight_vectors_to_try_ref_ ) {
      my $tref_ = $$prev_weight_vectors_to_try_ref_[$t_best_index];
      my @vref = @$tref_ ;
      GetWeightedSum ( \@vref, $tref_, $$prev_weight_vectors_to_try_ref_[0], $t_step_size ) ;
      push ( @next_weight_vectors_to_try_, \@vref ); #index=0 move towards best
    }

    if ( $#$prev_weight_vectors_to_try_ref_ >= 1 ) {
      push ( @next_weight_vectors_to_try_, $$prev_weight_vectors_to_try_ref_[1] ); #index=1 set to orig
    }

    if ( $t_best_index <= $#$prev_weight_vectors_to_try_ref_ ) { # valid
      my $tref_ = $$prev_weight_vectors_to_try_ref_[0]; # base model
      my @vref = @$tref_ ;

      for ( my $indicator_idx = 0 ; $indicator_idx <= $#orig_indicator_weight_vec_ ; $indicator_idx ++ ) {
	# setting weight for this indicator
	my $sum_weight_diff = 0;
	my $sum_weight = 0;
	for ( my $i = 1 ; $i <= $#$prev_weight_vectors_to_try_ref_ ; $i ++ ) 
	  { 
	    my $base_model = $$prev_weight_vectors_to_try_ref_[0];
	    my $this_model = $$prev_weight_vectors_to_try_ref_[$i];
	    if ( ( $delta_vec[$i] > 0 ) && ( $$this_model[$indicator_idx] != $$base_model[$indicator_idx] ) ) {
	      # if this score was better and the weight of this indicator was different
	      $sum_weight_diff += $delta_vec[$i] * ( $$this_model[$indicator_idx] - $$base_model[$indicator_idx] ) ;
	      $sum_weight += $delta_vec[$i] ;
	    }
	  }
	if ( $sum_weight > 0 ) {
	  $vref[$indicator_idx] += $t_step_size * ( $sum_weight_diff / $sum_weight ) ;
	}
      }
      push ( @next_weight_vectors_to_try_, \@vref ); #index=2 optimize each indicator on runs in which it's change leads to higher score
    }

    if ( $t_best_index <= $#$prev_weight_vectors_to_try_ref_ ) { # valid
      my $tref_ = $$prev_weight_vectors_to_try_ref_[0]; # base model
      my @vref = @$tref_ ;

      for ( my $indicator_idx = 0 ; $indicator_idx <= $#orig_indicator_weight_vec_ ; $indicator_idx ++ ) {
	# setting weight for this indicator
	my $indicator_weight_diff = 0;
	my $delta_max = 0;
	for ( my $i = 1 ; $i <= $#$prev_weight_vectors_to_try_ref_ ; $i ++ ) 
	  { 
	    my $base_model = $$prev_weight_vectors_to_try_ref_[0];
	    my $this_model = $$prev_weight_vectors_to_try_ref_[$i];
	    if ( ( $delta_vec[$i] > $delta_max ) && ( $$this_model[$indicator_idx] != $$base_model[$indicator_idx] ) ) {
	      # if this score was best so far and the weight of this indicator was different than base
	      $indicator_weight_diff = ( $$this_model[$indicator_idx] - $$base_model[$indicator_idx] ) ;
	      $delta_max = $delta_vec[$i] ;
	    }
	  }
	if ( $delta_max > 0 ) {
	  $vref[$indicator_idx] += $t_step_size * $indicator_weight_diff ;
	}
      }
      push ( @next_weight_vectors_to_try_, \@vref ); #index=2 optimize each indicator on runs in which it's change leads to higher score
    }
    
    if ( $t_best_index <= $#$prev_weight_vectors_to_try_ref_ ) { # valid
      my $tref_ = $$prev_weight_vectors_to_try_ref_[0]; # base model
      my @vref = @$tref_ ;

      for ( my $indicator_idx = 0 ; $indicator_idx <= $#orig_indicator_weight_vec_ ; $indicator_idx ++ ) {
	# setting weight for this indicator
	my $sum_weight_diff = 0;
	my $sum_weight = 0;
	for ( my $i = 1 ; $i <= $#$prev_weight_vectors_to_try_ref_ ; $i ++ ) 
	  { 
	    my $base_model = $$prev_weight_vectors_to_try_ref_[0];
	    my $this_model = $$prev_weight_vectors_to_try_ref_[$i];
	    if ( $$this_model[$indicator_idx] != $$base_model[$indicator_idx] ) {
	      # if the weight of this indicator was different but score need not be better
	      $sum_weight_diff += $delta_vec[$i] * ( $$this_model[$indicator_idx] - $$base_model[$indicator_idx] ) ;
	      $sum_weight += $delta_vec[$i] ;
	    }
	  }
	if ( $sum_weight > 0 ) {
	  $vref[$indicator_idx] += $t_step_size * ( $sum_weight_diff / $sum_weight ) ;
	}
      }
      push ( @next_weight_vectors_to_try_, \@vref ); #index=2 optimize each indicator on runs in which it's change leads to higher score
    }

    return @next_weight_vectors_to_try_ ;
}
