#!/usr/bin/perl
use strict;
use warnings;
use File::Basename; # for basename and dirname
use File::Path qw(mkpath);
use POSIX qw ( ceil floor );
#use Math::Round;

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO = "basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/array_ops.pl"; # NoDataDate
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; #GetFilteredDays

sub ReadDataFileIntoMatrix;
sub Histogram;
sub GenerateData;
sub RoundOff;

my $BASETRADEINFODIR="/spare/local/tradeinfo/";
my $SPARE_HOME="/spare/local/".$USER."/";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $MAX_INDICATORS = 18;

# start 
my $USAGE1="$0 MODELFILENAME DATAGEN_FILE ";
my $USAGE2="$0 MODELFILENAME TRADING_SDATE TRADING_EDATE STARTHHMM ENDHHMM MSECS_TIMEOUT L1_EVENTS_TIMEOUT NUM_TRADES_TIMEOUT TO_PRINT_ON_ECO [Dates Filter: Feature_FeatureAux_Lbound_Ubound / Feature_FeatureAux_Perc_LOW/HIGH_]";

my $model_file_name_ ;
my $datagen_start_date_ = `date +'%Y%m%d'` ;
my $datagen_end_date_ = `date +'%Y%m%d'`;
my $datagen_start_hhmm_ ;
my $datagen_end_hhmm_ ;
my $datagen_msecs_timeout_ ;
my $datagen_l1events_timeout_ ;
my $datagen_num_trades_timeout_ ;
my $to_print_on_economic_times_ ;
my $data_file_ = "NA";
my @filter_flags_;
my @dates_;
my @filter_dates_;
if ( $#ARGV == 1 )
{
    $model_file_name_ = $ARGV[0];
    $data_file_ = $ARGV[1];
}
elsif ( $#ARGV == 8 )
{
    $model_file_name_ = $ARGV[0];
    $datagen_start_date_ = $ARGV[1];
    $datagen_end_date_ = $ARGV[2];
    $datagen_start_hhmm_ = $ARGV[3];
    $datagen_end_hhmm_ = $ARGV[4];
    $datagen_msecs_timeout_ = $ARGV[5];
    $datagen_l1events_timeout_ = $ARGV[6];
    $datagen_num_trades_timeout_ = $ARGV[7];
    $to_print_on_economic_times_ = $ARGV[8];

}
elsif ( $#ARGV == 9 )
{
    $model_file_name_ = $ARGV[0];
    $datagen_start_date_ = $ARGV[1];
    $datagen_end_date_ = $ARGV[2];
    $datagen_start_hhmm_ = $ARGV[3];
    $datagen_end_hhmm_ = $ARGV[4];
    $datagen_msecs_timeout_ = $ARGV[5];
    $datagen_l1events_timeout_ = $ARGV[6];
    $datagen_num_trades_timeout_ = $ARGV[7];
    $to_print_on_economic_times_ = $ARGV[8];
    @filter_flags_ = split ( '_', $ARGV[9] );
    if ( scalar ( @filter_flags_ ) != 4 ) {
	print "filter dates format expects 4 tokens_, ex: AvgPrice_NA_0_20 || STDEV_NA_0.4_HIGH || CORR_ES_0_0.5_0.6\n";
	exit(0);
    }
}
else
{
    print "$USAGE1\n";
    print "$USAGE2\n";
    print "this script will only look at top 18 indicators \n";
    exit (-1);
}

my $work_dir_=$SPARE_HOME."AMF/";
mkpath $work_dir_;
chdir $work_dir_;

# Start Up
# Map from indicator index to indicator weight.
my %indicator_index_to_beta_map_ = ();
my %indicator_index_to_alpha_map_ = ();
my $indicator_count_ = 0;
my %hash_matrix_ = ( ) ;
my $shortcode_ = "NA" ;

# Start model info
open MODEL_FILE, "< $model_file_name_" or PrintStacktraceAndDie ( "Could not open $model_file_name_\n" );
my $is_siglr_ = 0 ;
while ( my $model_file_line_ = <MODEL_FILE> ) 
{
    chomp ($model_file_line_);
    my @model_file_lines_ = split ( ' ', $model_file_line_ );
   
    if ( $model_file_lines_[0] eq "MODELMATH" )
    {
	if ( $model_file_lines_[1] eq "SIGLR" )
	{
	    $is_siglr_ = 1;
	}
    }
    if ( $model_file_lines_[0] eq "INDICATOR" )
    {
	if ( $is_siglr_ == 1 )
	{
	    my ( $alpha_ , $beta_ ) = split ( /:/ , $model_file_lines_[1] );
	    # beta_ * ( 1 / ( 1 + exp ( - value_ * alpha_ ) ) - 0.5 ) ;
	    # if value_ == inf beta_ * 0.5 ... if value_ == -inf  beta_ * ( - 0.5 ) ;
	    # if value_ == 0 beta_ * 0 
	    $indicator_index_to_alpha_map_{$indicator_count_} = $alpha_ ; 
	    $indicator_index_to_beta_map_{$indicator_count_} = $beta_ ;
	}
	else
	{
	    $indicator_index_to_beta_map_{$indicator_count_} = $model_file_lines_[1];
	}
	$indicator_count_++;
    }
    if ( $model_file_lines_[0] eq "MODELINIT" ) 
    {
	$shortcode_ = $model_file_lines_[2];
    }
}
close MODEL_FILE;

my $min_price_increment_ = 1 ;
if ( $shortcode_ ne "NA" )
{
    $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $datagen_end_date_` ; chomp ( $min_price_increment_ ); # using single date in hope that min_inc doesnt change much
}

@dates_ = GetDatesFromStartDate( $shortcode_, $datagen_start_date_, $datagen_end_date_, "INVALIDFILE", 400 );


if ( scalar ( @filter_flags_ ) == 4 ) {
    if ( $filter_flags_[3] eq "HIGH" || $filter_flags_[3] eq "LOW" ) {
	GetFilteredDays ( $shortcode_, \@dates_, $filter_flags_[2], $filter_flags_[3], $filter_flags_[0], $filter_flags_[1], \@filter_dates_, $datagen_start_hhmm_, $datagen_end_hhmm_ );
    } else {
	GetFilteredDaysOnSampleBounds ( $shortcode_, \@dates_, $filter_flags_[2], $filter_flags_[3], $filter_flags_[0], $filter_flags_[1], \@filter_dates_, $datagen_start_hhmm_, $datagen_end_hhmm_ );
    }
} else {
    @filter_dates_ = @dates_;
}
# End model info
if ( $data_file_ eq "NA" )
{
    GenerateData ( ) ;
}

ReadDataFileIntoMatrix ( $data_file_ );
Histogram ( @{$hash_matrix_{ 0 } } );

#for ( my $ind_ = $indicator_count_ - 5 ; $ind_ < $indicator_count_ ; $ind_ ++ )
#{
#    Histogram ( 10 , @{$hash_matrix_{ $ind_ + 4 } } );
#}



# datagen start
sub GenerateData
{
    my $unique_amf_id_ = `date +%N`; chomp ( $unique_amf_id_ ); $unique_amf_id_ = int($unique_amf_id_) + 0;
    $data_file_ = basename ( $model_file_name_ )."_".$unique_amf_id_.".amf";
    my $temp_file_ = "tmp_".$unique_amf_id_.".amf";

    my $cmd_head_ = "$LIVE_BIN_DIR/datagen ".$model_file_name_." ";
    my $cmd_tail_ = $datagen_start_hhmm_." ".$datagen_end_hhmm_." ".$unique_amf_id_." ".$temp_file_." ".$datagen_msecs_timeout_." ".$datagen_l1events_timeout_." ".$datagen_num_trades_timeout_." ".$to_print_on_economic_times_." ADD_DBG_CODE -1";
    
    my $datagen_date_ = $datagen_end_date_;
    my $max_days_at_a_time_ = 2000;
    
    `> $data_file_`;
    for ( my $t_day_index_ = 0 ; $t_day_index_ < scalar(@filter_dates_) ; $t_day_index_ ++ ) 
    {
	$datagen_date_ = $filter_dates_[$t_day_index_];
	my $cmd_ = $cmd_head_." ".$datagen_date_." ".$cmd_tail_;
	# print $cmd_."\n";
	`$cmd_`;
	# print "cat $temp_file_ >> $data_file_\n";
	`cat $temp_file_ >> $data_file_`;
	`rm $temp_file_`;
    }
}
# datagen end

# function to read froom file to matrix
sub ReadDataFileIntoMatrix
{
    my ( $filename ) = @_;
    my $line ;
    open ( F, $filename ) || die "Could not open $filename: $!";
    my $count = 0 ;
    while ( $line = <F> )
    {
        chomp($line);
	my $matrix_name = "INDICATOR_".$count++;
        next if $line =~ /^\s*$/; # skip blank lines
        if ($line =~ /^([A-Za-z]\w*)/) 
	{
	    print "non-numeric items in datagen\n";
	    print $line."\n";
        } 
	else 
	{            
	    my (@data) = split (/\s+/, $line);
	    if ( scalar ( @data ) != 4 + $indicator_count_ )
	    {
		print "error malformed line in datagen \n";
		exit ( -1 );
	    }
	    my $weight_sum_ = 0 ;
	    for ( my $datagen_col_no_ = 4 ; $datagen_col_no_ <= $#data ; $datagen_col_no_++ )
	    {
		if ( $is_siglr_ == 1 )
		{
		    # beta_ * ( 1 / ( 1 + exp ( - value_ * alpha_ ) ) - 0.5 ) ;
		    my $ind_weight_ = $indicator_index_to_beta_map_{ $datagen_col_no_ - 4 }  * ( 1 / ( 1 + exp ( -1 * $indicator_index_to_alpha_map_ { $datagen_col_no_ - 4 } * $data [ $datagen_col_no_ ] ) ) - 0.5 ) ;
		    push ( @{$hash_matrix_{ $datagen_col_no_ }} , $ind_weight_ ) ;
		    $weight_sum_ += $ind_weight_ ;
		}
		else
		{
		    push ( @{$hash_matrix_{ $datagen_col_no_ }} , $indicator_index_to_beta_map_ { $datagen_col_no_ -4 } * $data [ $datagen_col_no_] ) ;
		    $weight_sum_ += $indicator_index_to_beta_map_ { $datagen_col_no_ - 4 } * $data [ $datagen_col_no_ ] ;
		}
	    }
	    push ( @ { $ hash_matrix_ { 0 } } , $weight_sum_ );
        }
    }
    close ( F );
}


# some sort of distribution representation 
sub Histogram
{
    my @list = @_;

    my @refined_data_ = ( ) ;

    # remove values > 10 * tick_value
    foreach my $val ( @list )
    {
	if ( abs ( $val ) < 10 * $min_price_increment_ )
	{
	    push ( @refined_data_ , $val ) ;
	}
    }
    #

    my $mean = GetAverage ( \@refined_data_ ) ;
    my $stdev = GetStdev ( \@refined_data_ ) ;
    my $median = GetMedianConst ( \@refined_data_ ) ;
    my $min = min ( @refined_data_ );
    my $max = max ( @refined_data_ );

    my $no_stdev = ( ( $max - $min ) / $stdev );
    my %histogram;

    print " $model_file_name_ \n";
    print "mean:$mean  :: stdev:$stdev :: no_of_stdev:$no_stdev :: percentage_of_outliers:@{[RoundOff(100*(1 - scalar(@refined_data_)/scalar(@list)),2)]}\n" ;

    my $stdev_factor_ = 0.5 ;#
    for ( $stdev_factor_ = 0.25 ; $stdev_factor_ <= 0.5 ; $stdev_factor_=$stdev_factor_*2 )
    {

	$histogram{ int ( ( $mean - $_ ) / ( $stdev_factor_ * $stdev ) ) }++ for @refined_data_;

	my @hist_values_ = ( values %histogram );
	my $density_sum_ = ( \@hist_values_ );

    # normal distribution map
    #"0.25 0.3867"
    #"0.5 0.3521"
    #"0.75 0.3011"
    #"1 0.242"
    #"1.25 0.1826"
    #"1.5 0.1295"
    #"1.75 0.0863"
    #"2 0.054"
    #"2.25 0.0317"
    #"2.5 0.0175"

    # this is not scientific enuf
    # for ( my $t = 0 ; $t < 5 ; $t ++ )
    # {
    #	if ( exists ( $histogram { $t } ) && exists ( $histogram { $t } ) )
    #	{
    #	    print  ( $histogram { $t } / $density_sum_ ) ;
    #	    print  ( $histogram { -$t } / $density_sum_ ) ;
    #	}
    # }

    ## TODO ## simply not required to take average ( but later on please adjust for bias )
	
	my $decrease_place_threshold =  (RoundOff(($mean + 3 * $stdev_factor_ * $stdev)/$min_price_increment_, 3) - RoundOff(($mean - 2 * $stdev_factor_ * $stdev)/$min_price_increment_, 3)) / 2 ; 
	my $place_threshold = (RoundOff(($mean + 5 * $stdev_factor_ * $stdev)/$min_price_increment_, 3) - RoundOff(($mean - 4 * $stdev_factor_ * $stdev)/$min_price_increment_, 3)) / 2  ;
	my $increase_place_threshold = (RoundOff(($mean + 7 * $stdev_factor_ * $stdev)/$min_price_increment_, 3) - RoundOff(($mean - 6 * $stdev_factor_ * $stdev)/$min_price_increment_, 3)) / 2 ;
	my $place_keep_diff = RoundOff(($stdev_factor_ * $stdev)/$min_price_increment_, 3) ;	
	print " stdev_factor:$stdev_factor_ increase_place_threshold:$increase_place_threshold place_threshold:$place_threshold decrease_place_threshold:$decrease_place_threshold place_keep_diff:$place_keep_diff \n";

	$decrease_place_threshold =  (RoundOff(($mean + 2 * $stdev_factor_ * $stdev)/$min_price_increment_, 3) - RoundOff(($mean - 2 * $stdev_factor_ * $stdev)/$min_price_increment_, 3)) / 2 ; 
	$place_threshold = (RoundOff(($mean + 4 * $stdev_factor_ * $stdev)/$min_price_increment_, 3) - RoundOff(($mean - 4 * $stdev_factor_ * $stdev)/$min_price_increment_, 3)) / 2  ;
	$increase_place_threshold = (RoundOff(($mean + 6 * $stdev_factor_ * $stdev)/$min_price_increment_, 3) - RoundOff(($mean - 6 * $stdev_factor_ * $stdev)/$min_price_increment_, 3)) / 2 ;
	$place_keep_diff = RoundOff(($stdev_factor_ * $stdev)/$min_price_increment_, 3) ;	
	print " stdev_factor:$stdev_factor_ increase_place_threshold:$increase_place_threshold place_threshold:$place_threshold decrease_place_threshold:$decrease_place_threshold place_keep_diff:$place_keep_diff \n";

#    for my $key ( sort { $a <=> $b } ( keys %histogram ) )
	for ( my $key = -15 ; $key < 16 ; $key ++ )
	{
	    if ( exists ( $histogram{$key} ) )
	    {
		print "( $key $histogram{$key} @{[RoundOff(($mean - $key * $stdev_factor_ * $stdev)/$min_price_increment_ , 3)]} ) ";
	    }
	}
	print "\n" ;
    }
}

sub RoundOff
{

    my ( $number , $accuracy ) = @_;
    return ( int ( $number * ( 10 ** $accuracy ) ) / ( 10 ** $accuracy ) )
}
