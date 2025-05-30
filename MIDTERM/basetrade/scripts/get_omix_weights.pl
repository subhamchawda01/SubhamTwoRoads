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

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
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
my $work_dir_ = "/spare/local/$USER/OMIX/";
#sub declarations
sub SetUpVolumes;
sub ReadOMixFile;
sub ReadBuildStratFile;
sub GenerateParamFiles;
sub ReadParamFile;
sub GenerateInitialPopulation;
sub CrossOver;
sub UniformMutation;
sub Selection;
sub Evaluate;
sub GetAverageSimPnlForPopulation;


my $USAGE="$0 SHC STRAT_FILE(Concatenated) START_DATE NUM_PREV_DAYS DESIRED_VOLUME NUMBER_PARAMS D";
if ( $#ARGV < 5 ) { print $USAGE."\n"; exit ( 0 ); }


my $shortcode_ = $ARGV[ 0 ];
my $strat_file_ = $ARGV [ 1 ];
my $zk_param_count_ = $ARGV[ 5 ];
my $param_pre_text_ = "";
my $param_post_text_ = "";
my $num_strats_ = 0;
my $start_date_ = $ARGV [ 2 ];
my $num_days_ = $ARGV [ 3 ];
my $population_size_ = 0 ;
my $sim_pnl_dir_ = "$HOME_DIR/sim_pnl";
my $omix_file_ = "/spare/local/ankit/OMIX/offline_mix_mms_test.txt";
my $omix_pretext_ = "";
my $omix_posttext_ = "";
my $param_file_="";
my $desired_volume_ = $ARGV[4];
my $base_zeropos_keep_ = 0;
my $param_zk_pre_text_ = ""; my $param_zk_post_text_ = ""; 
my $flag_zk_ = 0;
my $log_filename_ = "/home/ankit/foow.log.$shortcode_.$start_date_.$num_days_";

if ( $#ARGV > 5 )
{
    if ( index ( $ARGV [ 6 ] , "D" ) != -1 ) { $DEBUG = 1; }
}

my %product_to_volume_ = ();

my @price_type_list_ = ( );
push (@price_type_list_ , "Mid");
push (@price_type_list_ , "Mkt");
push (@price_type_list_ , "Sin");
push (@price_type_list_ , "Owp");

my $omix_strat_file_ = "/home/ankit/".$shortcode_."_omix_strats";
my @initial_population_ = ( );
my @initial_population_to_score_ = ( );
my @initial_population_to_volume_score_ = ( );

ReadOMixFile();
ReadBuildStratFile();
GenerateParamFiles();
GenerateInitialPopulation();

print "Mid Mkt Sin Owp Pnl Volume Pnl/Volume\n";
@initial_population_to_score_ = ( );
@initial_population_to_volume_score_ = ( );
Evaluate( );
exit ( 0 );


sub SetUpVolumes
{
	if ( $DEBUG )
	{
		print "SetUpVolumes\n";
	}
	#$product_to_volume_{"BAX_1"} = 50;
	#$product_to_volume_{"BAX_2"} = 60; 
	#$product_to_volume_{"BAX_3"} = 150;
	#$product_to_volume_{"BAX_4"} = 200;
	#$product_to_volume_{"BAX_5"} = 60; 
	$product_to_volume_{"CGB_0"} = 800;
	$product_to_volume_{"BR_DOL_0"} = 5000;
	$product_to_volume_{"LFR_0"} = 1200;
	$product_to_volume_{"JFFCE_0"} = 6000;
	$product_to_volume_{"BR_IND_0"} = 6000;
	$product_to_volume_{"BR_WIN_0"} = 7000; 
	$product_to_volume_{"ZF_0"} = 14000;
	$product_to_volume_{"ZN_0"} = 11000; 
	$product_to_volume_{"ZB_0"} = 4000;
	$product_to_volume_{"UB_0"} = 4000; 
	$product_to_volume_{"FOAT_0"} = 400;
	$product_to_volume_{"FESX_0"} = 2500; 
	$product_to_volume_{"FGBS_0"} = 800;
	$product_to_volume_{"FGBM_0"} = 4000;
	$product_to_volume_{"FGBL_0"} = 800;
}

sub ReadOMixFile
{
    if ( $DEBUG ) 
    { 
		print "ReadOMixFile\n"; 
    }
	
	open OMIX_FILE, "< $omix_file_" or PrintStacktraceAndDie ( "Could not open strategy file $omix_file_ for reading\n" );

	
	my $flag = 0;
	while ( my $omix_line_ = <OMIX_FILE> )
	{
		chomp($omix_line_);		
		my @omix_words_ = split ' ', $omix_line_;
		if($omix_words_[0] eq $shortcode_)
		{
			$flag = 1;
		}
		else
		{
			if($flag eq 0)
			{ $omix_pretext_ = $omix_pretext_.$omix_line_."\n";}
			else
			{ $omix_posttext_ = $omix_posttext_.$omix_line_."\n";}
		}
	}

    return;
}


sub ReadParamFile
{
	if ( $DEBUG )	
	{
		print "ReadParamFile\n";
	}
	
	open PARAM_FILE, "< $param_file_" or PrintStacktraceAndDie ( "Could not open param file $param_file_ for reading\n" );
	
        while ( my $param_line_ = <PARAM_FILE> )
        {
                chomp($param_line_);
                my @param_words_ = split ' ', $param_line_;
		if($#param_words_>0  && $param_words_[1] eq "ZEROPOS_KEEP")
		{
			$base_zeropos_keep_ = $param_words_[2];
			print $base_zeropos_keep_."\n";
			$flag_zk_ = 1;
		}
		else
                {
                        if($flag_zk_ eq 0)
                        {$param_zk_pre_text_ = $param_zk_pre_text_.$param_line_."\n";}
                        else
                        {$param_zk_post_text_ = $param_zk_post_text_.$param_line_."\n";}
                }

        }

	close PARAM_FILE
}

sub GenerateParamFiles
{
	if ($flag_zk_ eq 1)
	{
		my $tmp_zeropos_keep_ = 0;
		for(my $i=0;$i<$zk_param_count_;$i++)
		{
			my $test_param_file_ = $work_dir_."test_param_file_".$shortcode_.$i;
			open TEST_PARAM_FILE, "> $test_param_file_" or PrintStacktraceAndDie ( "Could not open param file $test_param_file_ for writing\n" );
			print TEST_PARAM_FILE $param_zk_pre_text_;
			$tmp_zeropos_keep_ =  $base_zeropos_keep_*$i/10;
			print TEST_PARAM_FILE "PARAMVALUE ZEROPOS_KEEP ".$tmp_zeropos_keep_."\n";
			print $tmp_zeropos_keep_."\n";
			print TEST_PARAM_FILE $param_zk_post_text_;
		}
	}
}


sub GenerateInitialPopulation
{
	if ( $DEBUG )
	{
		print "GenerateInitialPopulation\n";
	}
	
		my %t_mix_ = ( );
		my $t_mix_sum_ = 0;
		my $eps = 0.001;
		for (my $j = 0; $j <= 1+$eps; $j=$j+0.2)
		{
			$t_mix_{$price_type_list_[0]} = $j;
			
			for (my $k = 0; $k <= (1+$eps-$j) ; $k=$k+0.2)
			{
				$t_mix_{$price_type_list_[1]} = $k;

				for (my $l = 0; $l <= (1+$eps-$k-$j); $l=$l+0.2)
				{
					$t_mix_{$price_type_list_[2]} = $l;

					$t_mix_{$price_type_list_[3]} = 1-$j-$k-$l;					
					my %tt_mix_ = ( );
					for (my $p = 0; $p<=$#price_type_list_; $p++)
					{
						$tt_mix_{$price_type_list_[$p]} = $t_mix_{$price_type_list_[$p]};
					}
					push(@initial_population_,\%tt_mix_);
					$population_size_ ++;
					
				}
			}
    		}
		
	return;
}



sub ReadBuildStratFile
{
	if( $DEBUG )
	{
		print "ReadBuildStratFile\n";
	}
	open STRAT_FILE, "< $strat_file_" or PrintStacktraceAndDie ( "Could not open strategy file $strat_file_ for reading\n" );
	open OMIX_STRAT_FILE, "> $omix_strat_file_" or PrintStacktraceAndDie ( "Could not open strategy file $omix_strat_file_ for writing\n" );
	my $strat_count = 0;
	my $t_strat_text_ = "";
	my $strat_line_ = <STRAT_FILE>; 
	chomp($strat_line_);
	my @strat_words_ = split ' ', $strat_line_;
	my $model_file_ = $strat_words_[3];
	$param_file_ = $strat_words_[4];
	ReadParamFile();
	$num_strats_ = $flag_zk_*$zk_param_count_;
	for (my $i=0; $i<$num_strats_; $i++)
	{
		my $test_param_file_ = $work_dir_."test_param_file_".$shortcode_.$i;
		$strat_words_[4] = $test_param_file_;
		print OMIX_STRAT_FILE "@strat_words_"; print OMIX_STRAT_FILE "$i\n";
	}
	close OMIX_STRAT_FILE;
}


sub Evaluate
{
	if ( $DEBUG )
	{
		print "Evaluate\n";
	}

	for (my $i = 0; $i < $population_size_ ; $i ++ )
	{

		push ( @initial_population_to_score_ , 0 );
		push ( @initial_population_to_volume_score_ , 0 );
	}

	for (my $i= 0; $i < $population_size_ ; $i ++ )
	{

		open OUTOMIXFILE, "> $omix_file_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $omix_file_ for writing\n" );
		print $initial_population_[$i]{$price_type_list_[0]}." ".$initial_population_[$i]{$price_type_list_[1]}." ".$initial_population_[$i]{$price_type_list_[2]}." ".$initial_population_[$i]{$price_type_list_[3]};
		my $t_omix_text_ = $omix_pretext_.$shortcode_." ".$initial_population_[$i]{$price_type_list_[0]}." ".$initial_population_[$i]{$price_type_list_[1]}." ".$initial_population_[$i]{$price_type_list_[2]}." ".$initial_population_[$i]{$price_type_list_[3]}."\n".$omix_posttext_;
		print OUTOMIXFILE $t_omix_text_;
		close OUTOMIXFILE;

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
                	( NoDataDateForShortcode ( $tradingdate_ , $shortcode_ ) ) ||
  	       		( IsDateHoliday ( $tradingdate_ ) || ( ( $shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $shortcode_ ) ) ) ) )
			{
        	 		$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
             	   		$t_day_index_ --;
             	    		next;
                	}

	             	my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $omix_strat_file_ 97862 $tradingdate_ ADD_DBG_CODE -1";
        	      	my $log_=$work_dir_."sim_res_".$tradingdate_;
                	push (@sample_dates, $tradingdate_);
	                push (@tmpsampledates , $tradingdate_ );
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
 	            	select(undef, undef, undef, 0.2); #sleep for 1 secs
        	     }
	             $rem_days_ -= 10;
        	}
                my @avg_volumes_=();
                my @avg_pnl_ = ();
                for (my $j=0; $j<$num_strats_; $j++)
                {
                        push ( @avg_pnl_ , 0); push( @avg_volumes_ , 0);
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
			
 	        	for (my $j = 0; $j <$num_strats_ ; $j ++ )
            		{
                                my @t_sim_rwords_ = split ( ' ', $t_sim_strategy_output_lines_[$j] );
				$avg_volumes_[$j] += $t_sim_rwords_[2]/($num_days_);
				$avg_pnl_[$j] += $t_sim_rwords_[1]/($num_days_);
				#print "Pnl,Volume:".$t_sim_rwords_[1].",".$t_sim_rwords_[2]."\n";
            		}
        	}
		my $t_volume_diff_ = 200000;
		my $t_volume_ = 0;
		my $t_pnl_ = 0;
		for ( my $j=0; $j<$num_strats_; $j++)
		{
			if($t_volume_diff_ > abs($desired_volume_ - $avg_volumes_[$j]))
			{
				$t_volume_diff_ = abs($desired_volume_ - $avg_volumes_[$j]);
				$t_volume_ = $avg_volumes_[$j];
				$t_pnl_ = $avg_pnl_[$j];
			}	
		}
		if($t_volume_diff_ >  $desired_volume_/2)
		{print "Next\n";next;}
		$initial_population_to_score_[$i]=int($t_pnl_); $initial_population_to_volume_score_[$i] = int ($t_volume_);
		print $initial_population_[$i]{$price_type_list_[0]}." ".$initial_population_[$i]{$price_type_list_[1]}." ".$initial_population_[$i]{$price_type_list_[2]}." ".$initial_population_[$i]{$price_type_list_[3]};
		print " ".$initial_population_to_score_[$i]." ".$initial_population_to_volume_score_[$i]." ".($initial_population_to_score_[$i]/$initial_population_to_volume_score_[$i])."\n";
	}		
	return;
}
