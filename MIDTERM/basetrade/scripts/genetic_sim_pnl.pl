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
#sub Reproduce;
sub CrossOver;
sub UniformMutation;
sub Selection;
#sub NonUniformMutation;
sub Evaluate;
sub GetAverageSimPnlForPopulation;

my $USAGE="$0 STRAT_FILE START_DATE NUM_PREV_DAYS INITIAL_POPULATION_SIZE(multiple of 6) NUM_GENERATIONS [D]";
if ( $#ARGV < 4){ print $USAGE."\n"; exit ( 0 ); }

my $strat_file_ = $ARGV [ 0 ];
my $start_date_ = $ARGV [ 1 ];
my $num_days_ = $ARGV [ 2 ];
my $population_size_ = $ARGV [ 3 ];
my $new_population_size_ = int($population_size_/3);
my $num_generations_ = $ARGV [ 4 ];
my $sim_pnl_dir_ = "$HOME_DIR/sim_pnl";
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
my $volume_upper_bound_ = 2000;
my $volume_lower_bound_ = 200;
my $work_dir_ = "/spare/local/$USER/GA/"; 

if ( $#ARGV > 4 )
{
    if ( index ( $ARGV [ 5 ] , "D" ) != -1 ) { $DEBUG = 1; }
}


my @indicator_list_ = ( );

my $t = localtime();
print $t."\n";

ReadStratFile( );

my %indicator_to_initial_corr_ = ( );
my %indicator_to_weight_base_model_ = ( );
ReadModelFile( );
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


GenerateInitialPopulation();


for (my $i = 0; $i < $num_generations_ ; $i++)
{
	@initial_population_to_score_ = ( );
	Evaluate( );
	print "@initial_population_to_score_";
	print "\n";
	print max(@initial_population_to_score_);
	print "\n";
	Selection( );
	@initial_population_ = @new_population_;
	@orig_initial_population_ = @initial_population_;
	@new_population_ = ( );
	UniformMutation( );
	CrossOver( );
}

print max(@initial_population_to_score_);
print "\n";
$t = localtime();
print $t."\n";

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
			$indicator_to_initial_corr_{$t_indicator_name_} = $model_words_[-1];
			$indicator_to_weight_base_model_{$t_indicator_name_} = $model_words_[1];
		}
	}
	#print "@indicator_list_";
	#print "\n";
	#print Dumper(\%indicator_to_initial_corr_);
	#print "\n";
	#print Dumper(\%indicator_to_weight_base_model_);
	
	return;
}

# get the upper and lower bounds for the weights of each indicator on the basis of volume traded in sim by a strategy file
# having only the corresponding indicator
sub GetULWeightBounds
{
	if ( $DEBUG )
	{
		print "GetULWeightBounds\n";
	}

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
	for ( my $i=0; $i <= $#indicator_list_ ; $i++ )
	{
		my $t_sign_corr_ = $indicator_to_initial_corr_{$indicator_list_[$i]} > 0 ? 1 : -1;
		my $t_strat_filename_ = $sim_pnl_dir_."/"."tmp_ul_strat";
		open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );

		for (my $j=0; $j <= $#discrete_intervals_ ; $j++ )
		{
			my $t_model_filename_ = $sim_pnl_dir_."/"."tmp_ul_model".$j;
			my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_.$j."\n";
			my $t_output_ = $base_model_start_text_;
			if($t_sign_corr_ eq 1)
			{
				$t_output_ = $t_output_."INDICATOR ".($discrete_intervals_[$j])." ".$indicator_list_[$i]."\n";
			}			
			else
			{
				$t_output_ = $t_output_."INDICATOR ".(-($discrete_intervals_[$#discrete_intervals_ - $j]))." ".$indicator_list_[$i]."\n";				
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
		my $flag = 1;
		my @intervals_to_score_ = ( );
		
		my $num_strats_running = 0;
		my @sample_dates =( );

		for ( my $t_day_index_ = 0; $t_day_index_ < $num_days_; $t_day_index_ ++ )
		{
			if ( SkipWeirdDate ( $tradingdate_ ) ||
		     ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) || 
		     ( IsDateHoliday ( $tradingdate_ ) || ( ( $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) )
			{
				$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
				next;
			}
		
			my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ 99919 $tradingdate_ ADD_DBG_CODE -1";	
			my $log_=$work_dir_."sim_res_".$tradingdate_;
			push (@sample_dates, $tradingdate_);
			`$exec_cmd_ > $log_ & `; #run in background to parallelize
			$num_strats_running+=1;
			$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
		}
		
		#wait till all background jobs have finished or not 
		#TODO think of a better way
		my $done_cnt_ = 0;
		while ( $done_cnt_ != $num_strats_running ) {
			$done_cnt_ = 0;
			foreach $tradingdate_ ( @sample_dates )
			{
				my $log_=$work_dir_."sim_res_".$tradingdate_;
				$done_cnt_ += `tail -n 1 $log_ 2>/dev/null | grep SIMRESULT -c `;
			}
			select(undef, undef, undef, 1.0); #sleep for 1 secs
		}
		
		foreach $tradingdate_ ( @sample_dates )
		{
			open(FILE, $work_dir_."sim_res_".$tradingdate_) or die("Unable to open file");
			# read file into an array
			@t_sim_strategy_output_lines_ = <FILE>;
			# close file
			close(FILE);
 
			if( $flag eq 1)
			{
				for (my $j = 0; $j <=$#t_sim_strategy_output_lines_ ; $j ++ )
				{
					push ( @intervals_to_score_ , 0 );
				}
				$flag = 0;
			}
		
			for (my $j = 0; $j <=$#t_sim_strategy_output_lines_ ; $j ++ )
			{
				my @t_sim_rwords_ = split ( ' ', $t_sim_strategy_output_lines_[$j] );
				$intervals_to_score_[$j] += $t_sim_rwords_[2]/$num_days_;		
			}

		}
		
		my $index=0;		
		while(($intervals_to_score_[$index] < $volume_lower_bound_ || $intervals_to_score_[$index] > $volume_upper_bound_ ) && $index <= $#discrete_intervals_ )
		{
				$index ++;		
		}
		$indicator_to_weight_lb_{$indicator_list_[$i]} = $t_sign_corr_ > 0 ? $discrete_intervals_[$index] : (-($discrete_intervals_[$#discrete_intervals_ - $index]));
		$index=$#discrete_intervals_;
		while(($intervals_to_score_[$index] < $volume_lower_bound_ || $intervals_to_score_[$index] > $volume_upper_bound_ ) && $index >= 0)
		{
				$index --;		
		}
		$indicator_to_weight_ub_{$indicator_list_[$i]} = $t_sign_corr_ > 0 ? $discrete_intervals_[$index] : (-($discrete_intervals_[$#discrete_intervals_ - $index]));
	
		if($indicator_to_weight_ub_{$indicator_list_[$i]} <= $indicator_to_weight_lb_{$indicator_list_[$i]})
		{
			if($t_sign_corr_ eq 1)
			{
				$indicator_to_weight_lb_{$indicator_list_[$i]} = 0;
				$indicator_to_weight_ub_{$indicator_list_[$i]} = 1;
			}
			else
			{
				$indicator_to_weight_lb_{$indicator_list_[$i]} = -1;
				$indicator_to_weight_ub_{$indicator_list_[$i]} = 0;
			}
		}
		undef @intervals_to_score_;
	}
	return;
}

sub GenerateInitialPopulation
{
	if ( $DEBUG )
	{
		print "GenerateInitialPopulation\n";
	}
	
	for (my $i = 0; $i < $population_size_ ; $i++)
	{
		my %t_model_ = ( );
		for (my $j = 0; $j <= $#indicator_list_; $j++)
		{
			$t_model_{$indicator_list_[$j]} = rand()*($indicator_to_weight_ub_{$indicator_list_[$j]} - $indicator_to_weight_lb_{$indicator_list_[$j]}) + $indicator_to_weight_lb_{$indicator_list_[$j]};
		}
		push(@initial_population_,\%t_model_);
	}
	return;
}

sub UniformMutation
{	
	if ( $DEBUG )
	{
		print "UniformMutation\n";
	}
	for ( my $i = 0; $i <= $#orig_initial_population_ ; $i ++)
	{
		my $t_index = int(rand()*$#indicator_list_) + 1;
		my %t_model_ = ( );
		for (my $j = 0; $j <= $#indicator_list_ ; $j ++)
		{
			$t_model_{$indicator_list_[$j]} = $orig_initial_population_[$i]{$indicator_list_[$j]};
		}
		$t_model_{$indicator_list_[$t_index]} = rand()*($indicator_to_weight_ub_{$indicator_list_[$t_index]} - $indicator_to_weight_lb_{$indicator_list_[$t_index]}) - $indicator_to_weight_lb_{$indicator_list_[$t_index]};
		push( @initial_population_ , \%t_model_);
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
	if ( $DEBUG )
	{
		print "CrossOver\n";
	}
	for (my $i = 0; $i <= $#orig_initial_population_ ; $i += 2)
	{
		my $feasible = 0;
		#relaxing this for a while
		#while(!$feasible)
		{
			$feasible = 1;
			# not checking if population is odd...
			my $r = rand(); #random mixing proportion
			my %t_model1_ = ( );	# first offspring
			my %t_model2_ = ( );	# second offspring	
			
			for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
			{
				$t_model1_{$indicator_list_[$j]} = $r*$orig_initial_population_[$i]{$indicator_list_[$j]} + (1 - $r)*$orig_initial_population_[$i+1]{$indicator_list_[$j]};
				$t_model2_{$indicator_list_[$j]} = $r*$orig_initial_population_[$i+1]{$indicator_list_[$j]} + (1 - $r)*$orig_initial_population_[$i]{$indicator_list_[$j]};
				if ( ($t_model1_{$indicator_list_[$j]} < $indicator_to_weight_lb_{$indicator_list_[$j]} || $t_model1_{$indicator_list_[$j]} > $indicator_to_weight_ub_{$indicator_list_[$j]}) ||
					 ($t_model2_{$indicator_list_[$j]} < $indicator_to_weight_lb_{$indicator_list_[$j]} || $t_model2_{$indicator_list_[$j]} > $indicator_to_weight_ub_{$indicator_list_[$j]}) )
				{
					$feasible = 0;
				}
			}
			
			if($feasible)
			{
				push ( @initial_population_ , \%t_model1_ );
				push ( @initial_population_ , \%t_model2_ );
			}
		}
	}
	return;
}

sub Selection
{
	if ( $DEBUG )
	{
		print "Selection\n";
	}
	`rm -f $sim_pnl_dir_/scores`;
	for (my $i=0; $i <= $#initial_population_; $i++)
	{
		
		`echo "$i $initial_population_to_score_[$i]" >> $sim_pnl_dir_/scores`;
	}
	
	my @sort_output_lines_ = `cat scores | sort -t " " -k2 -nr`;

	@new_population_ = ( );

	for ( my $i=0; $i < $new_population_size_ ; $i++ )
	{
		my @sort_output_words_ = split ' ',$sort_output_lines_[$i];
		push ( @new_population_ , $initial_population_[int($sort_output_words_[0])] );
	}
	return;
}

sub Reproduce
{
	return;
}

sub Evaluate
{
	if ( $DEBUG )
	{
		print "Evaluate\n";
	}
	my $t_strat_filename_ = $sim_pnl_dir_."/"."tmp_strat";
	open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
	for ( my $i = 0; $i <= $#initial_population_ ; $i ++)
	{
		my $t_model_filename_ = $sim_pnl_dir_."/"."tmp_model".$i;
		my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."\n";
		my $t_output_ = $base_model_start_text_;

		for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
		{
			$t_output_ = $t_output_."INDICATOR ".$initial_population_[$i]{$indicator_list_[$j]}." ".$indicator_list_[$j]."\n";			
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
		push (@sample_dates, $tradingdate_);
		#print "$exec_cmd_ > $log_ & \n";
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
	for (my $j = 0; $j <=$#initial_population_ ; $j++ )
	{
		push ( @initial_population_to_score_ , 0 );
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
			$initial_population_to_score_[$j] += $t_sim_rwords_[1]/$num_days_;
		}
		$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
	}
	return;
}

