# \file GenPerlLib/create_sub_model_files.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use Math::Complex ; # sqrt
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $GENRSMWORKDIR=$SPARE_HOME."RSM/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}
if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
#######
require "$GENPERLLIB_DIR/get_indicator_lines_from_ilist.pl"; # GetIndicatorLinesFromIList
require "$GENPERLLIB_DIR/create_specific_length_model_file.pl"; # CreateSpecificLengthModelFile

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 5;
my $datagen_num_trades_timeout_ = 2;
my $to_print_on_economic_times_ = 0;

sub CreateSubModelFiles
{
    if ($USER eq "rahul") { return CreateSubModelFiles1 ( @_ ) };
    my $input_model_filename_ = shift;
#    my $num_inds_ = `sed -n '/^INDICATOR /p;' $input_model_filename_ | wc -l`; chomp($num_inds_);
    my $model_header_line_count_const_ = 3; # top three lines are not indicators
    my @indicator_lines_ = GetIndicatorLinesFromIList ( $input_model_filename_ );
    my $num_actual_indicators_ = 1 + $#indicator_lines_ ;

    my $min_indicator_count_to_keep_ = 8;

    # Just dont want to hardcode minimum number of inds. instead retain 50% top indicators.
    my $start_indicator_count_ = max ( 1, int ( $num_actual_indicators_ * 0.50 ) );
    if ( $start_indicator_count_ < $min_indicator_count_to_keep_ )
    { # if we were pruning to a very small size then don't
	$start_indicator_count_ = min ( $min_indicator_count_to_keep_, $num_actual_indicators_ );
    }

    my @sub_model_files_ = ();
    if ( ( $num_actual_indicators_ <= $min_indicator_count_to_keep_ ) # too few indicators ( < $min_indicator_count_to_keep_ ) to further cut
         || ( $num_actual_indicators_ > 16 ) # hopeless case .. So many indicators ( > 16 ) !!
	 || ( $start_indicator_count_ >= $num_actual_indicators_ ) ) # for whatever reason we are not pruning to any size smaller than num_actual_indicators_
    {
        push ( @sub_model_files_, $input_model_filename_ );
    }
    else
    {
        for ( my $this_indicator_count_ = $start_indicator_count_; $this_indicator_count_ <= $num_actual_indicators_ ; $this_indicator_count_ ++ )
        {
            my $this_sub_model_filename_ = $input_model_filename_."_ind_".( $this_indicator_count_ );
	    CreateSpecificLengthModelFile ( $input_model_filename_, $this_sub_model_filename_, $this_indicator_count_ );
            if ( -e $this_sub_model_filename_ ) 
	    { 
		push ( @sub_model_files_, $this_sub_model_filename_ ); 
	    }
        }
    }
    if ( $#sub_model_files_ < 0 )
    { # if still empty somehow
        push ( @sub_model_files_, $input_model_filename_ );
    }

    @sub_model_files_;
}

sub CreateSubModelFiles1
{
    my ($model_filename_, $datagen_start_yyyymmdd_, $datagen_end_yyyymmdd_, $datagen_start_hhmm_, $datagen_end_hhmm_) = @_;
    
    $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1($datagen_start_yyyymmdd_);
    $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1($datagen_end_yyyymmdd_);

    # print "$model_filename_  $datagen_start_yyyymmdd_, $datagen_end_yyyymmdd_, $datagen_start_hhmm_, $datagen_end_hhmm_\n";
    my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
#my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    
    my @model_coeff_vec_ = ();
    my @model_indicators_ = ();
    my $model_header_ = "";
    my $model_footer_ = "";
    
    my @new_model_files_ = ();

    my $mode_ = -1;
    open MFILE, "< $model_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $model_filename_ for reading\n" );
    while ( my $mline_ = <MFILE> )
    {
	chomp ( $mline_ );
	if ( $mline_ =~ /INDICATOR / )
	{
	    my @mwords_ = split ( ' ', $mline_ );
	    if ( $#mwords_ >= 2 )
	    {
		push ( @model_coeff_vec_, $mwords_[1] );
		push ( @model_indicators_, join(' ', @mwords_[2..$#mwords_]));
	    }
	$mode_ = 0;
	}
	else {
	    if( $mode_ == -1){
		$model_header_ .= "$mline_\n";
	    }
	    else {
		$model_footer_ .= "$mline_\n";
	    }
	}
    }
    close MFILE;

    my @sum_l1 = (0) * @model_coeff_vec_;
    my @sum_l2 = (0) * @model_coeff_vec_;
    my $count = 0; 	    
    my $num_inds_ = $#model_indicators_ + 1;

    if ( #( $num_inds_ < $min_model_length_const_ + $model_header_line_count_const_ + 1 ) ||                                                                                                   
	 ( $num_inds_ > 20 ) ) # hopeless case.. So many indicators!!                                                                                                                          
    {
	push ( @new_model_files_, $model_filename_ );
	return @new_model_files_;
    }
    my $tradingdate_ = $datagen_start_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;

    for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
    {
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     NoDataDate ( $tradingdate_ ) ||
	     IsDateHoliday ( $tradingdate_ ) )
	{
	    $tradingdate_ = CalcNextWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}
	
	if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	     ( $tradingdate_ > $datagen_end_yyyymmdd_ ) )
	{
	    last;
	}
	else 
	{   	    
	    my $this_day_timed_data_filename_ = $GENRSMWORKDIR."timed_data".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$unique_gsm_id_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_;
	    my $exec_cmd="$LIVE_BIN_DIR/datagen $model_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
#	print $main_log_file_handle_ "$exec_cmd\n";
	    `$exec_cmd`;
	    my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
	    `rm -f $datagen_logfile_`;
	    
	    if ( -e $this_day_timed_data_filename_ )
	    {
		open DFILE, "< $this_day_timed_data_filename_" or PrintStacktraceAndDie ( "Could not open this_day_timed_data_filename_ $this_day_timed_data_filename_ for reading\n" );
		while ( my $data_line_ = <DFILE> )
		{
		    chomp ( $data_line_ );
		    my @dwords_ = split ( ' ', $data_line_ );
		    
		    if ( $#dwords_ >= $#model_coeff_vec_ + 4 )
		    {
			my $this_model_value_ = 0;
			for ( my $j = 0; $j <= $#model_coeff_vec_ ; $j ++ )
			{
			    $this_model_value_ += ( $dwords_[ $j + 4 ] * $model_coeff_vec_[$j] ) ;
			
			    $sum_l1[$j] += $this_model_value_;
			    $sum_l2[$j] += $this_model_value_ * $this_model_value_ ;
			}
			$count ++;
		    }
		}
	    close DFILE;
		`rm -f $this_day_timed_data_filename_`;
	    }
	    
	$tradingdate_ = CalcNextWorkingDateMult ( $tradingdate_, 1 );
	}
	
    }
    
    my $start_index_ = int( $num_inds_ * 0.25) + 1;
    my @current_model_stdev_ = (-1) * $#model_coeff_vec_;

    if ( $count > 1 )
    {
	for (my $i=0; $i <= $#model_coeff_vec_; $i ++) {
	    $current_model_stdev_[$i] = sqrt ( ( $sum_l2[$i] - ( $sum_l1[$i] * $sum_l1[$i] / $count ) ) / ($count -1) ) ;
	}
	for (my $i=int($start_index_); $i <= $#model_coeff_vec_; $i ++) {
	    my $scale_factor_ = $current_model_stdev_[$#model_coeff_vec_] / $current_model_stdev_[$i];
	    my $new_model_file_name_ = $model_filename_ . "_ind_" . ($i+1) ;
	    open NFILE, "> $new_model_file_name_" or PrintStacktraceAndDie ( "Could not open $new_model_file_name_ for writing\n" );
	    print NFILE "$model_header_";
	    for ( my $j=0; $j <= $i; $j ++ ) {
		print NFILE "INDICATOR ". ($model_coeff_vec_[$j] * $scale_factor_) . " $model_indicators_[$j]\n";
	    }
	    print NFILE "$model_footer_";
	    close ( NFILE );
	    push(@new_model_files_, $new_model_file_name_); 
	}
    }
    
    if ( $#new_model_files_ < 0 )
    { # if still empty somehow                                                                                                                                                                 
	push ( @new_model_files_, $model_filename_ );
    }
    
    return @new_model_files_;
}

1
