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

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
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

my $DEBUG = 0;

#sub declarations
sub ReadStratFile;
sub GetTrainingDates;
sub ReadModelFile;
sub GenerateWeightVecs;
sub CalcPnls;
sub GetMyScore;
sub GetBestIndex ;
sub WriteOutModel ;
sub UpdateChangeFractionVec ;

my $USAGE="$0 input_strat_file output_model last_trading_date num_prev_days change_fraction [algo=gradalpha3] [D]";
if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }

my $strat_file_ = $ARGV [ 0 ];
my $output_model_filename_ = $ARGV[1];
my $last_trading_date_ = GetIsoDateFromStrMin1 ( $ARGV[2] ) ;
my $num_days_ = max ( 1, int($ARGV [ 3 ]) );
my $change_ = $ARGV [ 4 ];
my $calc_weights_algo_ = "gradalpha3"; 
if ( $#ARGV >= 5 ) { $calc_weights_algo_ = $ARGV[5]; } # BEST / GRAD

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
my $MAXSTEPSCALE = 3;

if ( $#ARGV >= 6 )
{
    if ( index ( $ARGV [ 6 ] , "D" ) != -1 ) { $DEBUG = 1; print "DEBUG MODE\n"; }
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

my $orig_pnl_ = 0;
my $orig_vol_ = 0;

my $best_seen_pnl_ = 0;
my $best_seen_vol_ = 0;
my @best_seen_indicator_weight_vec_ = (); # to store the max pnl seen so far ... due to nonlinearity

$strat_pre_text_ = "STRATEGYLINE ".$base_shortcode_." ".$base_pbat_dat_." ";
$strat_post_text_ = " ".$base_param_file_." ".$base_start_time_." ".$base_end_time_." ".$base_prog_id_;

my $improvement_made_ = 1;
my $iter_count_ = 1;
my $num_change_factions_significant_ = 1;
my $max_number_of_iterations_ = 10;

while ( ( $iter_count_ < $max_number_of_iterations_ ) &&
	( $improvement_made_ == 1 ) )
{
    $main_log_file_handle_->printf ( "Starting iteration %d\n", $iter_count_ ) ;
    my @indicator_weight_vec_vec_ = GenerateWeightVecs(\@best_indicator_weight_vec_, $iter_count_);

    my @pnl_vec_ = ( );
    my @volume_vec_ = ( );
    CalcPnls (\@indicator_weight_vec_vec_, \@pnl_vec_, \@volume_vec_, $iter_count_);

    if ( $iter_count_ == 1 )
    {
	$orig_pnl_ = $pnl_vec_[0];
	$orig_vol_ = $volume_vec_[0];

	$best_seen_pnl_ = $pnl_vec_[0];
	$best_seen_vol_ = $volume_vec_[0];
	    
	my $tref_ = $indicator_weight_vec_vec_[0];
	@best_seen_indicator_weight_vec_ = @$tref_ ; # set to best
    }

    if ( 1 ) # $DEBUG ) 
    {
	$main_log_file_handle_->printf ( "PNL_VOLUME_VEC\n" );
	for ( my $i = 0 ; $i <= $#pnl_vec_ ; $i ++ )
	{
	    $main_log_file_handle_->printf ( "%d %d %d\n", $i, $pnl_vec_[$i], $volume_vec_[$i] ) ;
	}
    }

    my $best_index_ = GetBestIndex ( \@pnl_vec_, \@volume_vec_ );

    my $pnl_base_level = min @pnl_vec_ ;
    {
	my $volume_base_level = max @volume_vec_ ;
	$pnl_base_level = min ( 0, ( $pnl_vec_[0] - ( $volume_vec_[0] * 0.10 ) ) ) ; # hopefully this is just 0
    }
    if ( $DEBUG ) { $main_log_file_handle_->printf ( "AT Iter:%d pnl_base_level: %d\n", $iter_count_, $pnl_base_level ) ; }

    # check if we had taken a bad step last time
    if ( ( $best_seen_vol_ > 0 ) && 
	 ( GetMyScore ( $best_seen_pnl_ - $pnl_base_level, 1 ) > GetMyScore ( $pnl_vec_[$best_index_] - $pnl_base_level, ( $volume_vec_[$best_index_] / $best_seen_vol_ ) ) ) )
    { # last time we had seen a best that is better than this time
	$main_log_file_handle_->printf ( "Reset weights. We had taken a bad step current best pnl %d, score %.2f < old best pnl %d score %.2f\n", GetMyScore ( $pnl_vec_[$best_index_] - $pnl_base_level, ( $volume_vec_[$best_index_] / $best_seen_vol_ ) ), $pnl_vec_[$best_index_], $best_seen_pnl_, GetMyScore ( $best_seen_pnl_ - $pnl_base_level, 1 ) ) ;
	@best_indicator_weight_vec_ = @best_seen_indicator_weight_vec_ ;

	# make all changes half # $change_ = $change_/2.0;
	for ( my $cfidx_ = 0 ; $cfidx_ <= $#change_fraction_vec_ ; $cfidx_ ++ )
	{
	    $change_fraction_vec_[$cfidx_] /= 2.0 ; 
	}

	$iter_count_ ++;
	next; # continue hoping our alpha was too high
    }

    if ( ( $best_index_ > 0 ) &&
	 ( $best_index_ <= $#indicator_weight_vec_vec_ ) )
    {  # if best index isn't 0
	$improvement_made_ = 1;

	if ( ( $best_seen_vol_ <= 0 ) ||
	     ( GetMyScore ( $best_seen_pnl_ - $pnl_base_level, 1 ) < GetMyScore ( $pnl_vec_[$best_index_] - $pnl_base_level, ( $volume_vec_[$best_index_] / $best_seen_vol_ ) ) ) )
	{
	    $best_seen_pnl_ = $pnl_vec_[$best_index_];
	    $best_seen_vol_ = $volume_vec_[$best_index_];
	    
	    my $tref_ = $indicator_weight_vec_vec_[$best_index_];
	    @best_seen_indicator_weight_vec_ = @$tref_ ; # set to best
	}

	@best_indicator_weight_vec_ = CalcNextWeights ( \@indicator_weight_vec_vec_, \@pnl_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $best_index_, $calc_weights_algo_ );

	$main_log_file_handle_->printf ( "oldpnl_volume %d %d maxpnl_volume %d %d bestpnl_volume %d %d\n", int($pnl_vec_[0]), int($volume_vec_[0]), int($pnl_vec_[$best_index_]), int($volume_vec_[$best_index_]), int($best_seen_pnl_), int($best_seen_vol_) ) ;
	if ( $DEBUG ) { $main_log_file_handle_->printf ( "new weights: %s\n", join(' ',@best_indicator_weight_vec_) ) ; }
	$iter_count_ ++;
    }
    else
    {
	$improvement_made_ = 0; # only breaking on weight insignificance
	$iter_count_ ++;
    }

    my $significance_thresh_ = 0.10 ;
    $num_change_factions_significant_ = UpdateChangeFractionVec ( \@pnl_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $significance_thresh_ );
    if ( $num_change_factions_significant_ <= 0 )
    { # break out of loop if nothign significant
	$main_log_file_handle_->printf ( "Breaking out of loop since num of significant (%f) change fractions = 0\n", $significance_thresh_ ) ;
	last;
    }
    
}

# Print orig weight and final weight
printf ( "OutputModel : %s\n", $output_model_filename_ );
printf ( "Orig : %d %d\n", $orig_pnl_, $orig_vol_ );
printf ( "Best : %d %d\n", $best_seen_pnl_, $best_seen_vol_ );

WriteOutModel ( \@best_seen_indicator_weight_vec_ );

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
	    my @i_words_ = @model_words_;
	    shift(@i_words_); shift(@i_words_);
#	pop(@i_words_);pop(@i_words_);
	    my $t_indicator_name_ = join(' ',@i_words_);

	    push( @indicator_list_, $t_indicator_name_);
	    
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

    my @indicator_weight_vec_vec_ = ();
    
    my @this_indicator_weight_vec_ = @$indicator_to_weight_base_vec_ref_ ;
    push ( @indicator_weight_vec_vec_, \@this_indicator_weight_vec_ );
    $main_log_file_handle_->printf ( "GenerateWeightVecs from \n%s\n", join(' ',@this_indicator_weight_vec_ ) ) ;
    
    for (my $j = 0; $j <= $#indicator_list_; $j++)
    {
	{
	    my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
	    $t_indicator_weight_vec_[$j] = ( 1 - $change_fraction_vec_[$j] ) * $this_indicator_weight_vec_ [$j] ;
	    push ( @indicator_weight_vec_vec_, \@t_indicator_weight_vec_ );
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
	}
	{
	    my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
	    $t_indicator_weight_vec_[$j] = ( 1 + $change_fraction_vec_[$j] ) * $this_indicator_weight_vec_ [$j] ;
	    push ( @indicator_weight_vec_vec_, \@t_indicator_weight_vec_ );
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
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
    my ( $indicator_weight_vec_vec_ref_, $pnl_vec_ref_, $volume_vec_ref_, $iter_count_ ) = @_;

    my $t_strat_filename_ = $work_dir_."/"."tmp_strat_".$iter_count_;
    open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
    for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
    {
	my $t_model_filename_ = $work_dir_."/"."tmp_model_".$iter_count_."_".$i;
	my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."\n";
	my $t_output_ = $base_model_start_text_;
	my $tref_ = $$indicator_weight_vec_vec_ref_[$i];

	for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
	{
	    if ( $model_type_ eq  "SIGLR" )
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
    }
    close OUTSTRAT;
    
    my @nonzero_tradingdate_vec_ = ( );

    for ( my $t_day_index_ = 0; $t_day_index_ <= $#training_date_vec_ ; $t_day_index_ ++ )
    {
	my $tradingdate_ = $training_date_vec_[$t_day_index_];
	
	my $sim_strat_cerr_file_ = $t_strat_filename_."_cerr";
	my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
	my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 2>$sim_strat_cerr_file_ | grep SIMRESULT"; # using hyper optimistic market_model_index, added nologs argument
	my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$iter_count_."_".$tradingdate_;
	if ( -e $t_sim_res_filename_ ) { `rm -f $t_sim_res_filename_`; }
	$main_log_file_handle_->print ( "$exec_cmd_ > $t_sim_res_filename_\n" );
	`$exec_cmd_ > $t_sim_res_filename_ `; 
	if ( ExistsWithSize ( $t_sim_res_filename_ ) )
	{	
	    push (@nonzero_tradingdate_vec_, $tradingdate_);
	}

	my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
	`rm -f $this_tradesfilename_`;

	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
    }
    
    my @count_instances_ = ();
    for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
    {
	push ( @$pnl_vec_ref_ , 0 );
	push ( @$volume_vec_ref_ , 0 );
	push ( @count_instances_, 0 );
    }
    
    if ( $DEBUG ) { $main_log_file_handle_->printf ( "Num days results = %d\n%s\n", ( 1 + $#nonzero_tradingdate_vec_ ), join ( ' ', @nonzero_tradingdate_vec_ ) ) ; }
    if ( $#nonzero_tradingdate_vec_ >= 0 )
    { # at least 1 day
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
    }

    #compute averages
    if ( ( $#count_instances_ >= 0 ) &&
	 ( $count_instances_[0] > 0 ) )
    {
	for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
	{
	    $$pnl_vec_ref_[$i] = $$pnl_vec_ref_[$i] / $count_instances_[$i] ;
	    $$volume_vec_ref_[$i] = $$volume_vec_ref_[$i] / $count_instances_[$i] ;
	}
    }
}

sub GetMyScore
{
    my ( $t_pnl_, $t_volume_ ) = @_;
    return ( $t_pnl_ * sqrt ( $t_volume_ ) ) ; # pnl * sqrt ( volume )
}

sub GetBestIndex
{
    my ( $pnl_vec_ref_, $volume_vec_ref_ ) = @_;
    my $best_index_ = 0;
    if ( ( $#$volume_vec_ref_ > 0 ) &&
	 ( $$volume_vec_ref_[0] > 0 ) )
    {
	my @sorting_value_vec_ = ();

	my $pnl_base_level = min @$pnl_vec_ref_ ;
	{
	    my $volume_base_level = max @$volume_vec_ref_ ;
	    $pnl_base_level = min ( 0, ( $$pnl_vec_ref_[0] - ( $$volume_vec_ref_[0] * 0.10 ) ) ) ; # hopefully this is just 0
#	    $pnl_base_level = min ( 0, ( $pnl_base_level - ( $volume_base_level * 0.10 ) ) ) ; # hopefully this is just 0
	}

	for ( my $i = 0 ; $i <= $#$pnl_vec_ref_ ; $i ++ )
	{
	    push ( @sorting_value_vec_, GetMyScore ( $$pnl_vec_ref_[$i] - $pnl_base_level, $$volume_vec_ref_[$i]/$$volume_vec_ref_[0] ) ) ; 
	    
#	if ( ( $i > 0 ) && 
#	     ( log ( $$volume_vec_ref_[$i] / $$volume_vec_ref_[0] ) < -0.09 ) )
#	{ 
#	    push ( @sorting_value_vec_, $sorting_value_vec_[0] - 1 ) ; # impossible to choose this 
#	}
#	else
#	{
#	    push ( @sorting_value_vec_, ( $$pnl_vec_ref_[$i] * ( 1 + log ( $$volume_vec_ref_[$i] / $$volume_vec_ref_[0] ) ) ) ) ; # pnl[i] * ( 1 + log ( volume[i]/volume[0] ) )
#	}
	}
	
	my $max_val_ = $sorting_value_vec_[0];
	for ( my $i = 1 ; $i <= $#sorting_value_vec_ ; $i ++ )
	{
	    if ( ( $sorting_value_vec_[$i] > $max_val_ ) &&
		 ( $$volume_vec_ref_[$i] > $orig_vol_ * 0.8 ) )
	    {
		$max_val_ = $sorting_value_vec_[$i];
		$best_index_ = $i;
	    }
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

    my $pnl_base_level = min @$pnl_vec_ref_ ;
    {
	my $volume_base_level = max @$volume_vec_ref_ ;
	$pnl_base_level = min ( 0, ( $$pnl_vec_ref_[0] - ( $$volume_vec_ref_[0] * 0.10 ) ) ) ; # hopefully this is just 0
#	$pnl_base_level = min ( 0, ( $pnl_base_level - ( $volume_base_level * 0.10 ) ) ) ; # hopefully this is just 0
    }

    my $base_score_ = GetMyScore ( $$pnl_vec_ref_[0] - $pnl_base_level, $$volume_vec_ref_[0]/$$volume_vec_ref_[0] ) ;
    for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
    {
	my $minus_score_ = GetMyScore ( $$pnl_vec_ref_[($num_perturbations_ * $i) + 1], $$volume_vec_ref_[($num_perturbations_ * $i) + 1]/$$volume_vec_ref_[0] ) ;
	my $plus_score_ = GetMyScore ( $$pnl_vec_ref_[($num_perturbations_ * $i) + 2], $$volume_vec_ref_[($num_perturbations_ * $i) + 2]/$$volume_vec_ref_[0] ) ;
	if ( ( $base_score_ >= ( $minus_score_ - 1 ) ) &&
	     ( $base_score_ >= ( $plus_score_ - 1 ) ) )
	{
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "Halving CF ( %d ) = %f PNLS: %d %d %d \n", (1+$i),$change_fraction_vec_[$i], $base_score_, $minus_score_, $plus_score_ ) ; }
	    $change_fraction_vec_[$i] /= 2.0;
	}
	if ( ( $change_fraction_vec_[$i] > $significance_thresh_ ) &&
	     ( max ( abs ( $base_score_ - $plus_score_ ), abs ( $base_score_ - $minus_score_ ) ) >= ( 0.05 * abs ( $base_score_ ) ) ) ) # at least 5 percent PNL difference
	{
	    if ( $DEBUG ) { $main_log_file_handle_->printf ( "CF ( %d ) = %f still significant \n", (1+$i), $change_fraction_vec_[$i], $base_score_, $minus_score_, $plus_score_ ) ; }
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
	    $main_log_file_handle_->print ( "$calc_weights_algo_ $alpha_ $iter_count_\n" ) ;
	}

	# sanity checks
	if ( ( $#$pnl_vec_ref_ != $#$volume_vec_ref_ ) ||
	     ( $#$pnl_vec_ref_ < ( $num_perturbations_ * $num_indicators_ ) ) )
	{
	    my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
	    @t_best_indicator_weight_vec_ = @$tref_ ; # set to best
	    return @t_best_indicator_weight_vec_ ;
	}
	
	my $base_score_ = $$pnl_vec_ref_[0];
	my @score_gradient_vec_ = ();
	my @sign_gradient_vec_ = ();
	for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
	{
	    my $minus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $i) + 1];
	    my $plus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $i) + 2];
	    if ( ( $plus_score_ > $minus_score_ ) &&
		 ( $plus_score_ > $base_score_ ) )
	    {
		push ( @score_gradient_vec_, ( $plus_score_ - $base_score_ ) );
		push ( @sign_gradient_vec_, 1 );
	    }
	    elsif ( ( $minus_score_ > $plus_score_ ) &&
		    ( $minus_score_ > $base_score_ ) )
	    {
		push ( @score_gradient_vec_, ( $minus_score_ - $base_score_ ) );
		push ( @sign_gradient_vec_, -1 );
	    }
	    else
	    {
		push ( @score_gradient_vec_, 0 );
		push ( @sign_gradient_vec_, 0 );
	    }
	}
	my $sum_gradients_ = GetSum ( \@score_gradient_vec_ );
	
	if ( $sum_gradients_ > 0 )
	{
	    my $tref_ = $$indicator_weight_vec_vec_ref_[0];
	    @t_best_indicator_weight_vec_ = @$tref_ ; # set it to the first vector and return
	    
	    for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
	    {
		my $minus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $i) + 1];
		my $plus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $i) + 2];
		
		if ( $DEBUG ) { $main_log_file_handle_->printf ( "For indicator %d weight = %f\n", ($i+1), $sign_gradient_vec_[$i] * ( $alpha_ * $score_gradient_vec_[$i] / $sum_gradients_ ) ) ; }
		
		given ( $sign_gradient_vec_[$i] )
		{
		    when ( 1 )
		    {
			$tref_ = $$indicator_weight_vec_vec_ref_[($num_perturbations_ * $i) + 2];
			$t_best_indicator_weight_vec_[$i] += ( $alpha_ * $score_gradient_vec_[$i] / $sum_gradients_ ) * ( $$tref_[$i] - $t_best_indicator_weight_vec_[$i] ) ; # move by an amount proportional to gradient in the direction of this weight5 vec and since $i is the only index that is different in theis weight vec we only need to add that index value
		    }
		    when ( -1 )
		    {
			$tref_ = $$indicator_weight_vec_vec_ref_[($num_perturbations_ * $i) + 1];
			$t_best_indicator_weight_vec_[$i] += ( $alpha_ * $score_gradient_vec_[$i] / $sum_gradients_ ) * ( $$tref_[$i] - $t_best_indicator_weight_vec_[$i] ) ; # move by an amount proportional to gradient in the direction of this weight5 vec and since $i is the only index that is different in theis weight vec we only need to add that index value
		    }
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
	{ $alpha_ = ( ( $iter_count_ + 10 ) / ( $iter_count_ ) ) ; }

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

    if ( ( $calc_weights_algo_ eq "NR_0" ) )
    {
	# this may converge faster // Newton-Raphson
	# x_{n+1} = x_{n} + del_x ;
	# del_x = -del_f / del_del_f ;
        if ( $DEBUG )
        {
            print $main_log_file_handle_ "$calc_weights_algo_ 0 $iter_count_\n";
        }

        my $base_score_ = $$pnl_vec_ref_[0];
        my @score_gradient_vec_ = ();
        my @second_derivative_ = (); #should finer step size be used for calculation of this?
        my @sign_gradient_vec_ = ();
        for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
        {
            my $minus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $i) + 1];
            my $plus_score_ = $$pnl_vec_ref_[($num_perturbations_ * $i) + 2];
            if ( ( $plus_score_ > $minus_score_ ) &&
                 ( $plus_score_ > $base_score_ ) )
            {
                push ( @score_gradient_vec_, ( $plus_score_ - $base_score_ ) );
                push ( @sign_gradient_vec_, 1 );
            }
            elsif ( ( $minus_score_ > $plus_score_ ) &&
                    ( $minus_score_ > $base_score_ ) )
            {
                push ( @score_gradient_vec_, ( $minus_score_ - $base_score_ ) );
                push ( @sign_gradient_vec_, -1 );
            }
            else
            {
                push ( @score_gradient_vec_, 0 );
                push ( @sign_gradient_vec_, 0 );
            }
            push ( @second_derivative_ , $plus_score_ + $minus_score_ - 2*$base_score_ ) ;
        }

        my $sum_gradients_ = GetSum ( \@score_gradient_vec_ );
        if ( $sum_gradients_ > 0 )
        {
            my $tref_ = $$indicator_weight_vec_vec_ref_[0];
            @t_best_indicator_weight_vec_ = @$tref_ ; # set it to the first vector and return

            for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
            {
                given ( $sign_gradient_vec_[$i] )
                {
                    when ( 1 )
                    {
                        $tref_ = $$indicator_weight_vec_vec_ref_[($num_perturbations_ * $i) + 2];
                        my $step_scaling_ = abs ( $sign_gradient_vec_[$i] * $score_gradient_vec_[$i] / $second_derivative_[$i] ) ;
                        if ( $step_scaling_ > $MAXSTEPSCALE ) { $step_scaling_ = $MAXSTEPSCALE ; }
                        $t_best_indicator_weight_vec_[$i] += $step_scaling_ * ( $$tref_[$i] - $t_best_indicator_weight_vec_[$i] ) ;
                        if ( $DEBUG ) { printf $main_log_file_handle_ "For indicator %d weight = %f\n", ($i+1), $step_scaling_ ; }
                    }
                    when ( -1 )
                    {
                        $tref_ = $$indicator_weight_vec_vec_ref_[($num_perturbations_ * $i) + 1];
                        my $step_scaling_ = abs ( $sign_gradient_vec_[$i] * $score_gradient_vec_[$i] / $second_derivative_[$i] ) ;
                        if ( $step_scaling_ > $MAXSTEPSCALE ) { $step_scaling_ = $MAXSTEPSCALE ; }
                        $t_best_indicator_weight_vec_[$i] += $step_scaling_ * ( $$tref_[$i] - $t_best_indicator_weight_vec_[$i] ) ;
                        if ( $DEBUG ) { printf $main_log_file_handle_ "For indicator %d weight = %f\n", ($i+1), $step_scaling_ ; }
                    }
                    when ( 0 )
                    {
                        if ( $DEBUG ) { printf $main_log_file_handle_ "For indicator %d weight = %f\n", ($i+1), 0.0 ; }
                    }
                }
            }
        }
        else
        { # somehow all gradients are <= 0 .. should not happen since best_index_ is not 0, but anyway
            my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
            @t_best_indicator_weight_vec_ = @$tref_ ; # set it to the best vector and return
        }


    }


    if ( $#t_best_indicator_weight_vec_ < 0 )
    { # default = best1
	my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
	@t_best_indicator_weight_vec_ = @$tref_ ; # set it to the best vector and return
    }

    return @t_best_indicator_weight_vec_ ;
}
