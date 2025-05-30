#!/usr/bin/perl

# \file scripts/score_indicators.pl
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
use FileHandle;
use List::Util qw/max min/; # for max

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO = "basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

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
sub GenerateInitialPopulation;
sub ReadIlistFile;
#sub Reproduce;
sub CrossOver;
sub UniformMutation;
sub Selection;
#sub NonUniformMutation;
sub Evaluate;
sub GetAverageSimPnlForPopulation;

my $USAGE="$0 ILIST_FILE STRAT_FILE(For Param file etc) START_DATE NUM_PREV_DAYS INITIAL_POPULATION_SIZE(multiple of 6) NUM_GENERATIONS OUTPUT_MODEL_FILE [VOLUME_LOWER_BOUND VOLUME_UPPER_BOUND D] ";
if ( $#ARGV < 6){ print $USAGE."\n"; exit ( 0 ); }

my $ilist_file_ = $ARGV [ 0 ];
my $strat_file_ = $ARGV [ 1 ];
my $start_date_ = $ARGV [ 2 ];
my $num_days_ = $ARGV [ 3 ];
my $num_weight_days_ = 5;
my $population_size_ = $ARGV [ 4 ];
my $new_population_size_ = int($population_size_/3);
my $num_generations_ = $ARGV [ 5 ];
my $out_modelfile_ = $ARGV [ 6 ];
my $sim_pnl_dir_ = "$HOME_DIR/sim_pnl";
my $base_param_file_ = "";
my $base_start_time_ = "";
my $base_end_time_ = "";
my $base_prog_id_ = "";
my $base_pbat_dat_ = "";
my $base_shortcode_ = "";
my $base_model_start_text_ = "";
my $strat_pre_text_ = "";
my $strat_post_text_ = "";

#TODO ideally we must have a file from these can be read for each product
#please change these depending upon the product
my $volume_lower_bound_ = 500;
my $volume_upper_bound_ = 5000;

if ($#ARGV>=8)
{
	my $volume_lower_bound_ = $ARGV[7];
	my $volume_upper_bound_ = $ARGV[8];
}

my $work_dir_ = "/spare/local/$USER/GA/"; 

if ( $#ARGV > 8 )
{
    if ( index ( $ARGV [ 9 ] , "D" ) != -1 ) { $DEBUG = 1; }
}


my @indicator_from_ilist_ = ( );
my @indicator_list_ = ( );


my $log_filename_ = $work_dir_."/log_ga_incremental";
open LOG, "> $log_filename_" or PrintStacktraceAndDie ( "Could not open log_filename_ $log_filename_ for writing\n" );

my $t = localtime();
print LOG $t."\n";


my %indicator_to_initial_corr_ = ( );

ReadIlistFile( );
ReadStratFile( );


$work_dir_ = $work_dir_."/".$base_shortcode_;
`mkdir -p $work_dir_ `; #make_work_dir

my %indicator_to_weight_lb_ = ( );
my %indicator_to_weight_ub_ = ( );
$strat_pre_text_ = "STRATEGYLINE ".$base_shortcode_." ".$base_pbat_dat_." ";
$strat_post_text_ = " ".$base_param_file_." ".$base_start_time_." ".$base_end_time_." ".$base_prog_id_;


GetULWeightBounds( );
my @initial_population_ ;
my @orig_initial_population_ = ( );
my @new_population_ = ( );
my @initial_population_to_score_ = ( );
my @initial_population_to_volume_score_ = ( );


my $current_pnl = 0;
my $prev_pnl = 0;
my $model_pre_text_ = $base_model_start_text_;
my @model_ilist_ = ( );
my $max_model_size_ = 10;


print LOG "\n\nLower Bound for all indicators\n\n";
print LOG Dumper(\%indicator_to_weight_lb_);
print LOG "\n\nUpper Bound for all indicators\n\n";
print LOG Dumper(\%indicator_to_weight_ub_);

while((($#model_ilist_+1)< $max_model_size_)) 
{
	print LOG "\n\n******Adding Next Indicator******\n\n";
	my $max_increment_ = -10000;
	my $max_index_ = -1;
	my $max_ind_weight_ = 0;
	my $max_volume_ = 0;
	for( my $i=0; $i<=$#indicator_list_;$i++)
	{
		my $ind_pnl_ = 0;
		my $ind_weight_ = 0;
		my $ind_volume_ = 0;
		if ( CheckIfAlreadyPresent($indicator_list_[$i]) )
		{
			next;
		}
		else
		{
			($ind_pnl_,$ind_volume_,$ind_weight_) = GetPnlForIndicator($model_pre_text_,$i);
			if($max_increment_ < ($ind_pnl_ - $current_pnl))
			{
				$max_increment_ = ($ind_pnl_ - $current_pnl);
				$max_index_ = $i;
				$max_ind_weight_ = $ind_weight_;
				$max_volume_ = $ind_volume_;
			}
		}
	}

        if( $#model_ilist_ > 0  &&  $max_increment_ < 0 )
        {
                last;
        }
	$prev_pnl = $current_pnl;
	$current_pnl += $max_increment_;
	$model_pre_text_ = $model_pre_text_."INDICATOR ".$max_ind_weight_." ".$indicator_list_[$max_index_]."\n";
	push (@model_ilist_,$indicator_list_[$max_index_]);
	print LOG "\n\nCurrent Model: \n\n";
	print LOG  $model_pre_text_;
	print LOG "\nCurrent Pnl : ".$current_pnl." ; Current Volume : ".$max_volume_."\n";
	print LOG "Maximum Increment :".$max_increment_,"\n";
	$t = localtime();
	print LOG "\n".$t."\n";	
}

open FINAL_MODEL, "> $out_modelfile_" or PrintStacktraceAndDie ( "Could not open log_filename_ $out_modelfile_ for writing\n" );
print FINAL_MODEL $model_pre_text_."INDICATOREND";

print LOG "\n";
$t = localtime();
print LOG $t."\n";
close LOG;
exit ( 0 );

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
	$base_param_file_ = $strat_words_[4];
	$base_start_time_ = $strat_words_[5];
	$base_end_time_ = $strat_words_[6];
	$base_prog_id_ = $strat_words_[7];
	return;
}

# Read the strategy file to get the model file, param file and other parameters
sub ReadIlistFile
{
	#please make sure the model file has scores with '#', right now there is no check for that
	if ( $DEBUG )
	{
		print "ReadIlistFile\n";
	}
	
	open ILIST_FILE, "< $ilist_file_" or PriceStacktraceAndDie ( "Could not open model file $ilist_file_ for reading\n" );
	
	print LOG "Reading Indicators from Ilist file\n";

        my $indicator_start_reaced_ = 0;

	while ( my $ilist_line_ = <ILIST_FILE> )
        {	
                chomp($ilist_line_);	
                my @ilist_words_ = split ' ', $ilist_line_;

                if( not $indicator_start_reaced_ )
                {
                    $base_model_start_text_ = $base_model_start_text_.$ilist_line_."\n";
                    if($ilist_words_[0] eq "INDICATORSTART")
                    {
                      $indicator_start_reaced_ = 1;
                    }
                }
		
		my @i_words_ = @ilist_words_;
		shift(@i_words_); shift(@i_words_);
		pop(@i_words_);pop(@i_words_);
		
		if($ilist_words_[0] eq "INDICATOR")
		{
			my $t_indicator_name_ = join(' ',@i_words_);
			push( @indicator_from_ilist_, $t_indicator_name_);
			print LOG $t_indicator_name_."\n";
			$indicator_to_initial_corr_{$t_indicator_name_} = $ilist_words_[-1];
		}
	}
	close ILIST_FILE;
	return;
}

sub CheckIfAlreadyPresent
{
	my ($indicator) = @_;
	if( $DEBUG )
	{
		print "CheckIfAlreadyPresent";
	}
	for( my $index=0; $index<=$#model_ilist_ ; $index++)
	{
		if($model_ilist_[$index] eq $indicator)
		{return 1;}
	}
	return 0;
}

sub GetPnlForIndicator
{
	my ($model_pre_text_, $ind_index) = @_;
	print LOG "\n\nINDICATOR $indicator_list_[$ind_index] \n\n";
	if( $DEBUG )
	{
		print "GetPnlForIndicator\n";
	}
	@initial_population_ = ( );
	GenerateInitialPopulation($ind_index);
	for( my $gen=0;$gen<$num_generations_;$gen++)
	{
		print LOG "***Generation ".($gen+1)."***\n";	
		print LOG "Population\n";
		print LOG "@initial_population_";
		print LOG "\n";
		@initial_population_to_score_ = ( );
		@initial_population_to_volume_score_ = ( );
		Evaluate($model_pre_text_,$indicator_list_[$ind_index]);
		print LOG "Avg Pnl Scores\n";
                print LOG "@initial_population_to_score_";
                print LOG "\n";
		print LOG "Avg Volume Scores\n";
		print LOG "@initial_population_to_volume_score_";
		print LOG "\n\n";
		Selection;
		@initial_population_ = @new_population_;
		@orig_initial_population_ = @initial_population_;
		@new_population_ = ( );
		UniformMutation($ind_index);
		CrossOver();
	}
	my $max_index_= -1;
	my $max_pnl_ = 0;
	for (my $member = 0; $member <=$#initial_population_to_score_; $member++)
	{
		if ($max_pnl_ < $initial_population_to_score_[$member] )
		{
			$max_pnl_ = $initial_population_to_score_[$member];
			$max_index_ = $member;
		}
	}
	return ($initial_population_to_score_[$max_index_],$initial_population_to_volume_score_[$max_index_],$initial_population_[0]);
}

# get the upper and lower bounds for the weights of each indicator on the basis of volume traded in sim by a strategy file
# having only the corresponding indicator
# TODO find a better way to do this
# ASSUMPTION volume changes monotonically with the increase in weights (not true)
sub GetULWeightBounds
{

	if ( $DEBUG )
	{
		print "GetULWeightBounds\n";
	}
	#these have to be hardcoded as of now....increase the range if you feel it is not sufficient...differs from product to product
	my @discrete_intervals_ = ( );
	push (@discrete_intervals_ , 0.0001);
	push (@discrete_intervals_ , 0.001);
	push (@discrete_intervals_ , 0.01);
	push (@discrete_intervals_ , 0.05);
	push (@discrete_intervals_ , 0.1);
	push (@discrete_intervals_ , 0.2);
	push (@discrete_intervals_ , 0.5);
	push (@discrete_intervals_ , 1);
	push (@discrete_intervals_ , 2);
	push (@discrete_intervals_ , 5);
	push (@discrete_intervals_ , 10);
	for ( my $i=0; $i <= $#indicator_from_ilist_ ; $i++ )
	{
		print  LOG "\nIndicator :".$indicator_from_ilist_[$i]."\n";
		my $t_sign_corr_ = $indicator_to_initial_corr_{$indicator_from_ilist_[$i]} > 0 ? 1 : -1;
		my $t_strat_filename_ = $work_dir_."tmp_ul_strat";
		open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );

		for (my $j=0; $j <= $#discrete_intervals_ ; $j++ )
		{
			my $t_model_filename_ = $work_dir_."tmp_ul_model".$j;
			my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_.$j."\n";
			my $t_output_ = $base_model_start_text_;
			if($t_sign_corr_ eq 1)
			{
				$t_output_ = $t_output_."INDICATOR ".($discrete_intervals_[$j])." ".$indicator_from_ilist_[$i]."\n";
			}			
			else
			{
				$t_output_ = $t_output_."INDICATOR ".(-($discrete_intervals_[$#discrete_intervals_ - $j]))." ".$indicator_from_ilist_[$i]."\n";				
			}
			
			$t_output_ = $t_output_."INDICATOREND\n";
			open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
			print OUTMODEL $t_output_;
			close OUTMODEL;
			print OUTSTRAT $t_strat_text_;	
		}
		close OUTSTRAT;
		
		
		my $tradingdate_ = $start_date_;
		my @t_sim_strategy_output_lines_ = ( );
		my @intervals_to_score_ = ( );
		
		my $num_strats_running = 0;
		my @sample_dates =( );

		for ( my $t_day_index_ = 0; $t_day_index_ < $num_weight_days_; $t_day_index_ ++ )
		{
		    if ( ( SkipWeirdDate ( $tradingdate_ ) ) ||
			 ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) || 
			 ( IsDateHoliday ( $tradingdate_ ) || ( ( $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) )
		    {
			$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
			next;
		    }
							
		    my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ 97869 $tradingdate_ ADD_DBG_CODE -1";	
		    my $log_=$work_dir_."sim_res_".$tradingdate_;
		    push (@sample_dates, $tradingdate_);
		    `$exec_cmd_ > $log_  & `; #run in background to parallelize
		    $num_strats_running+=1;
		    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
		}
		
		#wait till all background jobs have finished or not 
		#TODO think of a better way
		my $done_cnt_ = 0;
		while ( $done_cnt_ != $num_strats_running ) 
		{
			$done_cnt_ = 0;
			foreach $tradingdate_ ( @sample_dates )
			{
				my $log_=$work_dir_."sim_res_".$tradingdate_;
				$done_cnt_ += `tail -n 1 $log_ 2>/dev/null | grep SIMRESULT -c `;
			}
			select(undef, undef, undef, 1.0); #sleep for 1 secs
		}
		my $flag = 1;	
	
		foreach $tradingdate_ ( @sample_dates )
		{
			open(FILE, $work_dir_."sim_res_".$tradingdate_) or die("Unable to open file");
			# read file into an array
			while ( my $this_sim_strategy_output_line_ = <FILE> )
			{
			    if ( $this_sim_strategy_output_line_ =~ /SIMRESULT/ ) 
			    {
				push ( @t_sim_strategy_output_lines_, $this_sim_strategy_output_line_ );
			    }
			}
			# close file
			close(FILE);
 
			if ( $flag eq 1)
			{
			    for (my $j = 0; $j <=$#t_sim_strategy_output_lines_ ; $j ++ )
			    {
				push ( @intervals_to_score_ , 0 );
			    }
			    $flag = 0;
			}
			
			for ( my $j = 0; $j <= $#t_sim_strategy_output_lines_ ; $j ++ )
			{
			    my @t_sim_rwords_ = split ( ' ', $t_sim_strategy_output_lines_[$j] );
			    $intervals_to_score_[$j] += $t_sim_rwords_[2]/$num_weight_days_;		
			}

		}

		
		
		
		print LOG  "@intervals_to_score_";
		print LOG "\n";
		$flag = 0;
		for (my $t=0; $t <=$#intervals_to_score_;$t++)
		{
			if($intervals_to_score_[$t] > $volume_lower_bound_)
			{$flag = 1;}
		}
		
		#pushing the indicator only when it exceeds the volume barrier 
		if($flag==1)
		{
		push(@indicator_list_ , $indicator_from_ilist_[$i]);
		my $index = 0;
		while(($intervals_to_score_[$index] < $volume_lower_bound_ || $intervals_to_score_[$index] > $volume_upper_bound_ ) && $index <= $#discrete_intervals_ )
		{
			$index ++;		
		}
		$indicator_to_weight_lb_{$indicator_from_ilist_[$i]} = $t_sign_corr_ > 0 ? $discrete_intervals_[$index] : (-($discrete_intervals_[$#discrete_intervals_ - $index]));
		$index=$#discrete_intervals_;
		while(($intervals_to_score_[$index] < $volume_lower_bound_ || $intervals_to_score_[$index] > $volume_upper_bound_ ) && $index >= 0)
		{
				$index --;		
		}
		$indicator_to_weight_ub_{$indicator_from_ilist_[$i]} = $t_sign_corr_ > 0 ? $discrete_intervals_[$index] : (-($discrete_intervals_[$#discrete_intervals_ - $index]));
	
		#don't expect this to happen a lot but if happens, put these default weights, TODO think of something better
		if($indicator_to_weight_ub_{$indicator_from_ilist_[$i]} <= $indicator_to_weight_lb_{$indicator_from_ilist_[$i]})
		{
			if($t_sign_corr_ eq 1)
			{
				$indicator_to_weight_lb_{$indicator_from_ilist_[$i]} = 0;
				$indicator_to_weight_ub_{$indicator_from_ilist_[$i]} = 1;
			}
			else
			{
				$indicator_to_weight_lb_{$indicator_from_ilist_[$i]} = -1;
				$indicator_to_weight_ub_{$indicator_from_ilist_[$i]} = 0;
			}
		}
		}
		undef @intervals_to_score_;
	}
	return;
}

sub GenerateInitialPopulation
{
	my ($index) = @_;
	if ( $DEBUG )
	{
		print "GenerateInitialPopulation\n";
	}
	
	#add boundary values to the population
	#push (@initial_population_,$indicator_to_weight_lb_{$indicator_list_[$index]});
	#push (@initial_population_,$indicator_to_weight_ub_{$indicator_list_[$index]});

	#rest of the population is generated randomly in the range [lb,ub]
	for (my $i = 0; $i < $population_size_ ; $i++)
	{
                my $ind_weight = rand()*($indicator_to_weight_ub_{$indicator_list_[$index]} - $indicator_to_weight_lb_{$indicator_list_[$index]}) + $indicator_to_weight_lb_{$indicator_list_[$index]};
                push( @initial_population_ , $ind_weight);		
	}	
	return;
}

sub UniformMutation
{	
	#TODO not a proper step for incremental version....equivalent to generating a larger random sample at the very start than at every population
	my ($index) = @_;
	if ( $DEBUG )
	{
		print "UniformMutation\n";
	}
	
	for ( my $i = 0; $i <= $#orig_initial_population_ ; $i ++)
	{
		my $ind_weight = rand()*($indicator_to_weight_ub_{$indicator_list_[$index]} - $indicator_to_weight_lb_{$indicator_list_[$index]}) + $indicator_to_weight_lb_{$indicator_list_[$index]};
		push( @initial_population_ , $ind_weight);
	}
	return;
}

sub NonUniformMutation
{
	if ( $DEBUG )
	{
		print "NonUniformMutation\n";
	}
	return;
}

sub CrossOver
{
	#generate solutions around good solutions from the previous population.
	if ( $DEBUG )
	{
		print "CrossOver\n";
	}
	for (my $i = 0; $i <= $#orig_initial_population_ ; $i += 2)
	{
		my $r = rand();
		my $ind_weight1 = $r*$orig_initial_population_[$i] + (1 - $r)*$orig_initial_population_[$i+1];
		my $ind_weight2 = $r*$orig_initial_population_[$i+1] + (1 - $r)*$orig_initial_population_[$i];
		push ( @initial_population_ , $ind_weight1 );
		push ( @initial_population_ , $ind_weight2 );
	}
	return;
}

sub Selection
{
	#TODO constraining the volume too much might be an issue
	if ( $DEBUG )
	{
		print "Selection\n";
	}
	my $scores_file_ = $work_dir_."scores";
	`rm -f $scores_file_`;
	for (my $i=0; $i <= $#initial_population_; $i++)
	{
		my $tmp_score_ = $initial_population_to_score_[$i];
		`echo "$i $tmp_score_" >> $scores_file_`;
	}
	
	my @sort_output_lines_ = `cat $scores_file_ | sort -t " " -k2 -nr`;
	@new_population_ = ( );

	# here we can use additional constraints based volume, number of aggressive trades etc..
	for ( my $i=0; $i < $new_population_size_ ; $i++ )
	{
		my @sort_output_words_ = split ' ',$sort_output_lines_[$i];
		push ( @new_population_ , $initial_population_[int($sort_output_words_[0])] );
	}
	return;
}

sub Evaluate
{
	my ($model_pre_text_ , $indicator) = @_;

	if ( $DEBUG )
	{
		print "Evaluate\n";
	}
	
	#printing out the strat file with current population
	my $t_strat_filename_ = $sim_pnl_dir_."/"."tmp_strat";
	open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
	for ( my $i = 0; $i <  $population_size_ ; $i ++)
	{
		my $t_model_filename_ = $sim_pnl_dir_."/"."tmp_model".$i;
		my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."\n";
		my $t_output_ = $model_pre_text_;
	
		$t_output_ = $t_output_."INDICATOR ".$initial_population_[$i]." ".$indicator."\n";			

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
	
	#runs the strat file for "ten"(hardcoded) days at a time parallely to get a sense of 
	my ( $t_strat_filename_ ) = @_ ;
	
	if ( $DEBUG )
	{
		print "GetAveragePnlForPopulation\n";
	}
	my $tradingdate_ = $start_date_;
	my @t_sim_strategy_output_lines_ = ( );
	my @tmpsampledates = ( );
	my $rem_days_ = $num_days_;
	while( $rem_days_ > 0)
	{
		my @sample_dates =( );
		my $num_strats_running = 0;

		for ( my $t_day_index_ = 0; $t_day_index_ < (10 < $rem_days_ ? 10 : $rem_days_); $t_day_index_ ++ )
		{
			if ( SkipWeirdDate ( $tradingdate_ ) ||
		        ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) || 
		    	( IsDateHoliday ( $tradingdate_ , $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) 
			{
				$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
				$t_day_index_ --;
				next;
			}
		
			my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ 97869 $tradingdate_ ADD_DBG_CODE -1";
			my $log_=$work_dir_."sim_res_".$tradingdate_;
			push (@sample_dates, $tradingdate_);
			push (@tmpsampledates , $tradingdate_ );
			`$exec_cmd_ > $log_ & `; #run in background to parallelize
			$num_strats_running+=1;
			$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
		}
		
		my $done_cnt_ = 0;
		while ( $done_cnt_ != $num_strats_running )
		{
			$done_cnt_ = 0;
			foreach $tradingdate_ ( @sample_dates )
			{
	        	        my $log_=$work_dir_."sim_res_".$tradingdate_;
				$done_cnt_ += `tail -n 1 $log_ 2>/dev/null | grep SIMRESULT -c `;
			}	
			select(undef, undef, undef, 1.0); #sleep for 1 secs
		}
		$rem_days_ -= 10;
	}

	for (my $j = 0; $j < $population_size_ ; $j++ )
	{
		push ( @initial_population_to_score_ , 0 );
		push ( @initial_population_to_volume_score_ , 0 );
	}
		
	foreach $tradingdate_ ( @tmpsampledates )
	{
		open(FILE, $work_dir_."sim_res_".$tradingdate_) or die("Unable to open file");
		# read file into an array
		while ( my $this_sim_strategy_output_line_ = <FILE> )
		{
		    if ( $this_sim_strategy_output_line_ =~ /SIMRESULT/ ) 
		    {
			push ( @t_sim_strategy_output_lines_, $this_sim_strategy_output_line_ );
		    }
		}
		close(FILE);
		for (my $j = 0; $j < $population_size_ ; $j ++ )
		{
			my @t_sim_rwords_ = split ( ' ', $t_sim_strategy_output_lines_[$j] );
			$initial_population_to_score_[$j] += $t_sim_rwords_[1]/$num_days_;
			$initial_population_to_volume_score_[$j] += $t_sim_rwords_[2]/$num_days_;
		}	
		
	}	
	return;

}
