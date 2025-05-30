#!/usr/bin/perl

# \file scripts/inc_sim_pnl_modelling.pl
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
use feature "switch";
use List::Util qw/max min/; # for max
use List::Util qw(shuffle);
use Math::Complex ; # sqrt
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO = "basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/is_model_corr_consistent.pl"; # IsModelCorrConsistent
require "$GENPERLLIB_DIR/get_bad_days_for_shortcode.pl"; # GetBadDaysForShortcode
require "$GENPERLLIB_DIR/get_very_bad_days_for_shortcode.pl"; # GetVeryBadDaysForShortcode
require "$GENPERLLIB_DIR/get_high_volume_days_for_shortcode.pl"; # GetHighVolumeDaysForShortcode
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
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
require "$GENPERLLIB_DIR/name_strategy_from_model_and_param_index.pl"; # for NameStrategyFromModelAndParamIndex
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo
require "$GENPERLLIB_DIR/get_indicator_lines_from_ilist.pl"; # GetIndicatorLinesFromIList
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec
require "$GENPERLLIB_DIR/install_strategy_modelling.pl"; # InstallStrategyModelling
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; # MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_substr.pl"; # MakeStratVecFromDirSubstr
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/get_min_pnl_per_contract_for_shortcode.pl"; # GetMinPnlPerContractForShortcode
require "$GENPERLLIB_DIR/get_min_volume_for_shortcode.pl"; # GetMinVolumeForShortcode
require "$GENPERLLIB_DIR/get_min_num_files_to_choose_for_shortcode.pl"; # GetMinNumFilesToChooseForShortcode
require "$GENPERLLIB_DIR/get_business_days_between.pl"; # GetBusinessDaysBetween

use Data::Dumper;

my $DEBUG = 0;

#sub declarations
sub ReadConfigFile;
sub PickDates;
sub GenerateInitialPopulation;
sub ReadIlistFile;
sub InOutSampleCheck;
sub Reproduce;
sub CrossOver;
sub UniformMutation;
sub Selection;
sub NonUniformMutation;
sub Evaluate;
sub GetAverageSimPnlForPopulation;
sub gaussian_rand;

my $USAGE="$0 CONFIG_FILE [D]";
if ( $#ARGV < 0){ print $USAGE."\n"; exit ( 0 ); }

my $config_file_ = $ARGV[0];
my $num_sim_strats_ = 8;
my $initial_population_size_ = 10;
my $new_population_size_ = 5;
my $num_generations_ = 2;
my @paramfile_vec_ = ();
my @indicator_list_file_vec_ = ();
my $start_date_ = "";
my $end_date_ = "";
my $outsample_start_date_ = "";
my $outsample_end_date_ = "";
my $start_time_ = "";
my $end_time_ = "";
my $exec_logic_ = "";
my $shortcode_ = "";
my $prog_id_ = int(rand()*1000);
my $base_model_start_text_ = "";
my $sort_algo_ = "";
my $ct_measure_ = "";
my $mail_address_ = "";
my $mail_body_ = "";

my $use_random_dates_ = 1;
my $prob_insample_ = 0.6;  # hardcoded
my $minimum_aggressive_trades_ = 0;
my $minimum_volume_ = 0;

my $weight_lower_bound_ = 0.0001;
my $weight_upper_bound_ = 500;

my $log_weight_lower_bound_ = log($weight_lower_bound_)/log(1.7);
my $log_weight_upper_bound_ = log($weight_upper_bound_)/log(1.7);

my $dates_count_ = 0;

ReadConfigFile();

print $start_date_."\n";
print $end_date_."\n";

my $t_param_idx_ = int(rand()*($#paramfile_vec_+1));
my $param_file_ = $paramfile_vec_[$t_param_idx_];
$mail_body_ .= "Param_File : $param_file_ \n";
print $param_file_."\n";
my $t_idx_ = int(rand()*($#indicator_list_file_vec_+1));
my $ilist_file_ = $indicator_list_file_vec_[$t_idx_];
$mail_body_ .= "Ilist : $ilist_file_ \n";
print $ilist_file_."\n";
my $ilist_ = (split(/\//, $ilist_file_))[-1];

my $work_dir_ = "/spare/local/$USER/inc_strats/";

my $add_factor_ = ($log_weight_upper_bound_ - $log_weight_lower_bound_)/($initial_population_size_-1);

my @indicator_from_ilist_ = ( );
my @indicator_list_ = ( );

my $out_modelfile_ = $work_dir_.$shortcode_."/w_inc_simpnl_model_".$ilist_."_".$start_date_."_".$end_date_."_pfi_".$t_param_idx_; 
my $out_stratfile_ = $work_dir_.$shortcode_."/w_inc_simpnl_strategy_".$ilist_."_".$start_date_."_".$end_date_."_pfi_".$t_param_idx_;


my $shortcode_strats_dir_ = $work_dir_.$shortcode_;
`mkdir -p $work_dir_`; 
`mkdir -p $shortcode_strats_dir_`;

my $log_filename_ = $work_dir_."log_ga_incremental.$shortcode_".int(rand()*1000);
open LOG, "> $log_filename_" or PrintStacktraceAndDie ( "Could not open log_filename_ $log_filename_ for writing\n" );
$mail_body_ .= "Log : $log_filename_\n";
my $t = localtime();
print LOG $t."\n";

my %indicator_to_initial_corr_ = ( );

ReadIlistFile( );
@indicator_list_ = @indicator_from_ilist_;

my $strat_pre_text_ = "STRATEGYLINE ".$shortcode_." ".$exec_logic_." ";
my $strat_post_text_ = " ".$param_file_." ".$start_time_." ".$end_time_." ".$prog_id_;
print $log_filename_."\n";

my @initial_population_ ;
my @orig_initial_population_ = ( );
my @new_population_ = ( );
my @insample_dates_ = ( );
my @outsample_dates_ = ( );
my @initial_population_to_score_ = ( );
my @initial_population_to_pnl_score_ = ( );
my @initial_population_to_pnl_squared_score_ = ( );
my @initial_population_to_volume_score_ = ( );
my @initial_population_to_agg_score_ = ( );
my $reg_data_file_ = "$work_dir_/reg_data_inds";

my $current_score_ = 0;
my $prev_score_ = 0;
my $model_pre_text_ = $base_model_start_text_;
my @model_ilist_ = ( );
my $max_model_size_ = 10;

print LOG "PARAM FILE\n";
my @param_output_lines_ = `cat $param_file_`;
print LOG "@param_output_lines_";print LOG "\n";
PickDates();

my $flag_threshold_ = 1;
while((($#model_ilist_+1)< $max_model_size_)) 
{
    print LOG "\n\n******Adding Next Indicator******\n\n";
    my $max_increment_ = -10000000000;

    my $max_index_ = -1;
    my $max_ind_weight_ = 0;
    my $max_volume_ = 0;
    my @new_indicator_list_ = ( );
    print LOG "Number of eligible indicators:".($#indicator_list_)."\n";
    for( my $i=0; $i<=$#indicator_list_;$i++)
    {
	my $ind_score_ = 0;
	my $ind_pnl_ = 0;
	my $ind_weight_ = 0;
	my $ind_volume_ = 0;
	my $ind_agg_ = 0;
	if ( CheckIfAlreadyPresent($indicator_list_[$i]) )
	{
	    next;
	}
	else
	{
	    ($ind_score_,$ind_pnl_, $ind_agg_ ,$ind_volume_,$ind_weight_) = GetScoresForIndicator($model_pre_text_,$i);
	    my $increment_ = $ind_score_ - $current_score_ ;
	    if($max_increment_ < $increment_ && $ind_agg_ >= $minimum_aggressive_trades_ && $ind_volume_ >= $minimum_volume_)
	    {
		$max_increment_ = $increment_;
		$max_index_ = $i;
		$max_ind_weight_ = $ind_weight_;
		$max_volume_ = $ind_volume_;
	    }
	    
	    print LOG "Increment : ".$increment_."\n";
	    if( ($#model_ilist_+1) > 0 &&  $increment_ > 0.01*abs($current_score_))
	    {
		push(@new_indicator_list_, $indicator_list_[$i]);
	    }
	}
    }

    if( $#model_ilist_ > 0  &&  $max_increment_ < 0 )
    {
	last;
    }

    $prev_score_ = $current_score_;
    $current_score_ += $max_increment_;
    my $t_sign_corr_ = $indicator_to_initial_corr_{$indicator_list_[$max_index_]}>0?1:-1;
    $model_pre_text_ = $model_pre_text_."INDICATOR ".($t_sign_corr_*(1.7**$max_ind_weight_))." ".$indicator_list_[$max_index_]."\n";

    push (@model_ilist_,$indicator_list_[$max_index_]);

    print LOG "\n\nCurrent Model: \n\n";
    print LOG  $model_pre_text_;
    print LOG "\nCurrent Score : ".$current_score_." ; Current Volume : ".$max_volume_."\n";
    print LOG "Maximum Increment :".$max_increment_,"\n";

    if($#model_ilist_ > 0)
    {
	@indicator_list_ = ();
	@indicator_list_ = @new_indicator_list_;
	print LOG "New Indicator List Size :".($#new_indicator_list_+1)."\n";
    }

    $t = localtime();
    print LOG "\n".$t."\n";	

}
print $out_modelfile_."\n";
open FINAL_MODEL, "> $out_modelfile_" or PrintStacktraceAndDie ( "Could not open log_filename_ $out_modelfile_ for writing\n" );
print FINAL_MODEL $model_pre_text_."INDICATOREND";
close FINAL_MODEL;

print $out_stratfile_."\n";
open FINAL_STRAT, "> $out_stratfile_" or PrintStacktraceAndDie ( "Could not open log_filename_ $out_stratfile_ for writing\n" );
print FINAL_STRAT $strat_pre_text_.$out_modelfile_.$strat_post_text_;
close FINAL_STRAT;

my $results_file_ = $out_stratfile_."_results";
InOutSampleCheck();
$mail_body_ .= "Model File : $out_modelfile_\n";
$mail_body_ .= "Strategy File : $out_stratfile_\n";

open(MAIL, "|/usr/sbin/sendmail -t");
	    
my $hostname_=`hostname -s`;
#Mail Header
print MAIL "To: $mail_address_\n";
my $from_mail_address_ = $USER."@".$hostname_;
print MAIL "From: $from_mail_address_\n";
my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
print MAIL "Subject: incremental_simpnl_modelling ( $config_file_ ) $yyyymmdd_ \n\n";
 ## Mail Body
print MAIL $mail_body_ ;
close(MAIL);


print LOG "\n";
$t = localtime();
print LOG $t."\n";
close LOG;
exit ( 0 );

# Read the strategy file to get the model file, param file and other parameters

sub ReadConfigFile
{
    open CONFIG_FILE, "< $config_file_" or PriceStacktraceAndDie ( "Could not open model file $config_file_ for reading\n" );	
    my $current_instruction_="";
    my $current_instruction_set_ = 0;
    while ( my $thisline_ = <CONFIG_FILE> ) 
    {
		chomp ( $thisline_ ); # remove newline
		
		my @this_words_ = split ( ' ', $thisline_ );
		if ( $#this_words_ < 0 ) 
		{ # empty line hence set $current_instruction_set_ 0
		    $current_instruction_ = "";
		    $current_instruction_set_ = 0;
		    next;
		} 
		else 
		{ # $#this_words_ >= 0
		    
		    if ( substr ( $this_words_[0], 0, 1) ne '#' )
		    {
			
				if ( $current_instruction_set_ == 0 ) 
				{ 
				    # no instruction set currently being processed
				    $current_instruction_ = $this_words_[0];
				    $current_instruction_set_ = 1;
				} 
				else 
				{ # $current_instruction_ is valid
				    given ( $current_instruction_ ) 
				    {
				    	when ("SHORTCODE") 
						{
						    $shortcode_ = $this_words_[0];
						}
						when ("EXEC_LOGIC")
						{
						 	$exec_logic_ = $this_words_[0];
						}
						when ("PARAMFILE_LIST") 
						{
							if ( ExistsWithSize ( $this_words_ [ 0 ] ) )
							{ # Eliminate non-existant ilists.
						 		push ( @paramfile_vec_ , $this_words_ [ 0 ] );
						 	}
						}
						when ("ILIST_LIST")
						{
							if ( ExistsWithSize ( $this_words_ [ 0 ] ) )
						 	{ # Eliminate non-existant ilists.
						 		push ( @indicator_list_file_vec_ , $this_words_[ 0 ] );
						 	}
						}
						when ("USE_RANDOM_DATES")
						{
							$use_random_dates_ = $this_words_[0];
						}
						when ("START_END_DATE")
						{
							$dates_count_ +=1;
							if(rand()< (1/$dates_count_))
							{
								$start_date_ = GetIsoDateFromStrMin1 ( $this_words_[0] );
						 		$end_date_  =  GetIsoDateFromStrMin1 ( $this_words_[1] );
							}
						}
						when ("OUTSAMPLE_START_END_DATE")
						{
							$outsample_start_date_ = GetIsoDateFromStrMin1 ( $this_words_[0] );
							$outsample_end_date_ = GetIsoDateFromStrMin1 ( $this_words_[1] );
						}
						when ("START_END_TIME")
						{
						 	$start_time_ = $this_words_[0];
						 	$end_time_ = $this_words_[1];
						}
						when ("SORT_ALGO")
						{
						 	$sort_algo_ = $this_words_[0];
						}	
						when ("WEIGHT_BOUNDS")
						{
						 	$weight_lower_bound_ = $this_words_[0];
						 	$weight_upper_bound_ = $this_words_[1];
						}
						when ("MINIMUM_VOLUME")
						{
						 	$minimum_volume_ = $this_words_[0];
						}
						when ("MINIMUM_AGGRESSIVE_TRADES")
						{
						 	$minimum_aggressive_trades_ = $this_words_[0];
						}
						when ("INITIAL_POPULATION_SIZE")
						{
						 	$initial_population_size_ = $this_words_[0];
						}
						when ("NEW_POPULATION_SIZE")
						{
						 	$new_population_size_ = $this_words_[0];
						}
						when ("NUM_GENERATIONS")
						{
						 	$num_generations_ = $this_words_[0];
						}
						when ("MEASURE_OF_CENTRAL_TENDENCY")
						{
						 	$ct_measure_ = $this_words_ [0];
						}
						when ("MAIL_ADDRESS")
						{
							$mail_address_ = $this_words_ [0];
						}
						default
						{}				    	
					}
				}
			 }
		}			
    }
	close CONFIG_FILE;
}

sub PickDates
{
    if ( $DEBUG )
    {
        print "PickRandomDates\n";
    }
    
    for (my $tradingdate_ = $start_date_; $tradingdate_ < $end_date_ ; $tradingdate_ = CalcNextWorkingDateMult ( $tradingdate_, 1) )
    {
	if ( SkipWeirdDate ( $tradingdate_ ) || NoDataDateForShortcode ( $tradingdate_ , $shortcode_ )  ||
	   ( IsDateHoliday ( $tradingdate_ , $shortcode_ ) &&  IsProductHoliday ( $tradingdate_, $shortcode_ ) ) )
        {    
            next;
        }
        if($use_random_dates_==1 && rand()>$prob_insample_)
	{
	    push(@outsample_dates_,$tradingdate_);
	}
	else
	{
	    push(@insample_dates_,$tradingdate_);
	}
    }
    for (my $tradingdate_ = $outsample_start_date_; $tradingdate_ < $outsample_end_date_ ; $tradingdate_ = CalcNextWorkingDateMult ($tradingdate_ , 1) )
    {
	if ( SkipWeirdDate ( $tradingdate_ ) || NoDataDateForShortcode ( $tradingdate_ , $shortcode_ )  ||
           ( IsDateHoliday ( $tradingdate_ , $shortcode_ ) &&  IsProductHoliday ( $tradingdate_, $shortcode_ ) ) )
        {
            next;
        }
	push(@outsample_dates_,$tradingdate_);
    }
}

sub InOutSampleCheck
{
    open RESULTS, "> $results_file_" or PrintStacktraceAndDie ( "Could not open log_filename_ $results_file_ for writing\n" );	
    print RESULTS "Insample Results\n";
    $mail_body_ .= "Insample Results\n";
    my $tmp_file_ = $work_dir_."/tmp_file";
   `rm -f $tmp_file_`;
    for(my $i = 0; $i<=$#insample_dates_; $i++)
    {
	my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $out_stratfile_ 97869 $insample_dates_[$i] ADD_DBG_CODE -1 | grep SIMRESULT";
	print RESULTS "$insample_dates_[$i]\t";
	$mail_body_ .= "$insample_dates_[$i]\t";
	my $sim_results_ = `$exec_cmd_`;
	$mail_body_ .= $sim_results_;
	chomp($sim_results_);
	`echo "$sim_results_" >> $tmp_file_`;
	print RESULTS $sim_results_."\n";
	my $pnl_stats_ = `$MODELSCRIPTS_DIR/get_pnl_stats_2.pl /spare/local/logs/tradelogs/trades.$insample_dates_[$i].97869`;
	chomp($pnl_stats_);
	$mail_body_ .= "Pnl Stats : $pnl_stats_\n";
	print RESULTS $pnl_stats_;
    }
    my $insample_stats_ = `cat $tmp_file_ | awk \'{pnl+=\$2; vol+=\$3;}END{print pnl/NR, \" \" , vol/NR }\'`;
    print RESULTS "\n Insample Statistics : ".$insample_stats_."\n";
    print RESULTS "\n\nOutSample Results\n\n";
    $mail_body_ .= "\n Insample Statistics : ".$insample_stats_."\n";
    $mail_body_ .= "\n\nOutSample Results\n\n";
    `rm -f $tmp_file_`;	
    for(my $i=0; $i<=$#outsample_dates_; $i++)
    {
	my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $out_stratfile_ 97869 $outsample_dates_[$i] ADD_DBG_CODE -1 | grep SIMRESULT";
	print RESULTS "$outsample_dates_[$i]\t";
	$mail_body_ .= "$outsample_dates_[$i]\t";
	my $sim_results_ = `$exec_cmd_`;
	$mail_body_ .= $sim_results_;
	chomp($sim_results_);
	`echo "$sim_results_" >> $tmp_file_`;
	print RESULTS $sim_results_."\n";
	my $pnl_stats_ = `$MODELSCRIPTS_DIR/get_pnl_stats_2.pl /spare/local/logs/tradelogs/trades.$outsample_dates_[$i].97869`;	
	chomp($pnl_stats_);
	$mail_body_ .= "Pnl Stats : $pnl_stats_\n";	
	print RESULTS $pnl_stats_."\n";
    }
    my $outsample_stats_ = `cat $tmp_file_ | awk '{pnl+=\$2; vol+=\$3;}END{print pnl/NR, " " ,vol/NR}'`;
    print RESULTS "\n OutSample Statistics : ".$outsample_stats_."\n";
    $mail_body_ .= "\n OutSample Statistics : ".$outsample_stats_."\n";
    print "*************************************\n";
    `rm -f $tmp_file_`;
    close RESULTS;
}


sub ReadIlistFile
{
    #please make sure the model file has scores with '#', right now there is no check for that
    #this is needed to assign sign for the weight of each indicator
    #if not already present please add a dummy value with expected sign
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

sub GenerateRegDataFile
{
    my $tradingdate_ = $start_date_;
    for ( my $tradingdate_ = $start_date_; $tradingdate_ <= $end_date_; $tradingdate_ = CalcNextWorkingDateMult ( $tradingdate_ , 1 ) )
    {
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	   ( NoDataDateForShortcode ( $tradingdate_ , $shortcode_ ) ) ||
	   ( IsDateHoliday ( $tradingdate_ , $shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $shortcode_ ) ) ) )
	{
	    next;
	}
	my $tmp_datagen_file_ = "$work_dir_/tmp_datagen_out";
	my $tmp_reg_data_file_= "$work_dir_/tmp_reg_data_out";
	my $exec_cmd_ = "$LIVE_BIN_DIR/datagen $ilist_file_ $tradingdate_ 830 1430 3333 $tmp_datagen_file_ 3000 6 2 0";
	`$exec_cmd_`;
	$exec_cmd_ = "$LIVE_BIN_DIR/timed_data_to_reg_data $ilist_file_ $tmp_datagen_file_ 32 na_e3 $tmp_reg_data_file_";
	`$exec_cmd_`;
	`cat $tmp_reg_data_file_ >> $reg_data_file_`;
	`rm -f $tmp_datagen_file_ $tmp_reg_data_file_`;
    }
    my $t_file_ = "$work_dir_/tmp_file";
    `cp $reg_data_file_ $t_file_`;
    `cat $t_file_ |  awk \'{sub(\$1FS,x)}1\' > $reg_data_file_`;	
    `rm -f $t_file_`;
}

sub RemoveIndicatorFromRegData
{
    my $indicator_index_ = @_;
    my $t_file_ = "$work_dir_/tmp_file";
    `cp $reg_data_file_ $t_file_`;
    my $t_str_ = $indicator_index_."FS";
    `cat $t_file_ |  awk \'{sub(\$$t_str_,x)}1\' > $reg_data_file_`;            
    `rm -f $t_file_`;			
}

sub RemoveHighlyCorrelatedIndicators
{
    my $indicator_index_ = @_;
    my $t_ind_reg_data_file_ = "$work_dir_/tmp_ind_reg_data_file_";	
    `cat $reg_data_file_ | awk \'{print \$$indicator_index_ , \"\",\$0}\' > $t_ind_reg_data_file_`;
    my $t_file_ = "$work_dir_/tmp_file";
    `cp $t_ind_reg_data_file_ $t_file_`;
    my $t_str_ = ($indicator_index_+1)."FS";
    `cat $t_file_ |  awk \'{sub(\$$t_str_,x)}1\' > $t_ind_reg_data_file_`;
    `rm -f $t_file_`;
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

sub GetScoresForIndicator
{
    my ($model_pre_text_, $ind_index) = @_;
    print LOG "\n\nINDICATOR $indicator_list_[$ind_index] \n\n";
    if( $DEBUG )
    {
	print "GetScoresForIndicator\n";
    }
    @initial_population_ = ( );
    $flag_threshold_ = 1;
    GenerateInitialPopulation($ind_index);
    for( my $gen=0;$gen<$num_generations_;$gen++)
    {
	if($flag_threshold_ eq 0)
	{last;}
	$flag_threshold_ = 1;
	print LOG "***Generation ".($gen+1)."***\n";	
	print LOG "Population\n";
	print LOG "@initial_population_";
	print LOG "\n";
	@initial_population_to_score_ = ( );
	@initial_population_to_pnl_score_ = ( );
	@initial_population_to_volume_score_ = ( );
	@initial_population_to_agg_score_ = ( );
	Evaluate($model_pre_text_,$indicator_list_[$ind_index]);
	Selection;
	print LOG "Avg Scores\n";
	print LOG "@initial_population_to_score_";
	print LOG "\n";
	print LOG "Avg Pnl Scores\n";
	print LOG "@initial_population_to_pnl_score_";
	print LOG "\n";
	print LOG "Avg Aggressive Scores\n";
	print LOG "@initial_population_to_agg_score_";
	print LOG "\n";
	print LOG "Avg Volume Scores\n";
	print LOG "@initial_population_to_volume_score_";
	print LOG "\n\n";
	UniformMutation($ind_index);
	NonUniformMutation($gen+1,$ind_index);
    }
    my $max_index_= -1;
    my $max_pnl_ = -2000000;
    for (my $member = 0; $member <=$#initial_population_to_score_; $member++)
    {
	if ($max_pnl_ < $initial_population_to_score_[$member] && $initial_population_to_volume_score_[$member] >= $minimum_volume_  && $initial_population_to_agg_score_[$member] >= $minimum_aggressive_trades_)
	{
	    $max_pnl_ = $initial_population_to_score_[$member];
	    $max_index_ = $member;
	}
    }
    return ($initial_population_to_score_[$max_index_],$initial_population_to_pnl_score_[$max_index_],$initial_population_to_agg_score_[$max_index_], $initial_population_to_volume_score_[$max_index_],$initial_population_[$max_index_]);
}

sub GenerateInitialPopulation
{
    my ($index) = @_;
    if ( $DEBUG )
    {
	print "GenerateInitialPopulation\n";
    }
    
    for (my $i = 0; $i < $initial_population_size_ ; $i++)
    {
	my $ind_weight = $log_weight_lower_bound_ + $add_factor_*$i;
        push( @initial_population_ , $ind_weight);		
    }	
    return;
}

sub gaussian_rand 
{
    my ($u1, $u2);  # uniformly distributed random numbers
    my $w;          # variance, then a weight
    my ($g1, $g2);  # gaussian-distributed numbers

    do {
        $u1 = 2 * rand() - 1;
        $u2 = 2 * rand() - 1;
        $w = $u1*$u1 + $u2*$u2;
    } while ( $w >= 1 );

    $w = sqrt( (-2 * log($w))  / $w );
    $g2 = $u1 * $w;
    $g1 = $u2 * $w;
    return $g1;
}


sub NonUniformMutation
{	
    #TODO not a proper step for incremental version....equivalent to generating a larger random sample at the very start than at every population
    my ($gen_,$index) = @_;
    if ( $DEBUG )
    {
	print "NonUniformMutation\n";
    }
    #my $fine_add_factor_ = 1.9*$add_factor_/10;
    my $count_ = $#orig_initial_population_+1;
    while($count_ < $initial_population_size_)
    {	
	for ( my $i = 0; $i <= $#orig_initial_population_ && $count_< $initial_population_size_ ; $i ++)
	{
	    my $feasible = 1;
	    my $ind_weight = 0;
	    while($feasible eq 1)
	    {
		$ind_weight = $orig_initial_population_[$i] + gaussian_rand()/$gen_;
		#$ind_weight = $orig_initial_population_[$i] - $add_factor_ + $fine_add_factor_*($j+1);
		if($ind_weight <= $log_weight_upper_bound_ && $ind_weight >=$log_weight_lower_bound_)
		{
		    $feasible = 0;
		}
	    }
	    push( @initial_population_ , $ind_weight);
	    $count_++;
	}
    }
    return;
}

sub UniformMutation
{
    my ($index) = @_;
    if ( $DEBUG )
    {
	print "UniformMutation\n";
    }

    @initial_population_ = ();
    @orig_initial_population_ = @new_population_;
    @initial_population_ = @new_population_;
    @new_population_ = ( );
    return;
}

sub CrossOver
{
    #generate solutions around good solutions from the previous population.
    if ( $DEBUG )
    {
	print "CrossOver\n";
    }
    my @t_shuffled_population_ = shuffle(@orig_initial_population_);
    for (my $i = 0; $i <= $#t_shuffled_population_ ; $i += 2)
    {
	my $r = rand();
	my $ind_weight1 = $r*$t_shuffled_population_[$i] + (1 - $r)*$t_shuffled_population_[$i+1];
	my $ind_weight2 = $r*$t_shuffled_population_[$i+1] + (1 - $r)*$t_shuffled_population_[$i];
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
	my $tmp_score_ = 0;
	if($sort_algo_==1)
	{$tmp_score_ = $initial_population_to_pnl_score_[$i];}
	elsif($sort_algo_==2)
	{$tmp_score_ = ($initial_population_to_pnl_score_[$i]<0)?($initial_population_to_pnl_score_[$i]/($initial_population_to_volume_score_[$i]**0.5)):($initial_population_to_pnl_score_[$i]*($initial_population_to_volume_score_[$i]**0.5));}
	else
	{$tmp_score_ = ($initial_population_to_pnl_score_[$i]<0)?($initial_population_to_pnl_score_[$i]/($initial_population_to_volume_score_[$i])):($initial_population_to_pnl_score_[$i]*($initial_population_to_volume_score_[$i]));	}			
	
        $initial_population_to_score_[$i] = $tmp_score_;
	
	`echo "$i $tmp_score_" >> $scores_file_`;
    }
    
    my @sort_output_lines_ = `cat $scores_file_ | sort -t " " -k2 -nr`;
    @new_population_ = ( );

    # here we can use additional constraints based volume, number of aggressive trades etc..
    my @not_picked_ = ( );
    $flag_threshold_ =1;
    for ( my $i=0; $i < $initial_population_size_ ; $i++ )
    {
	my @sort_output_words_ = split ' ',$sort_output_lines_[$i];
	if( $initial_population_to_agg_score_[int($sort_output_words_[0])] >= $minimum_aggressive_trades_  && $initial_population_to_volume_score_[int($sort_output_words_[0])] >= $minimum_volume_ )
	{
	    push ( @new_population_ , $initial_population_[int($sort_output_words_[0])] );
	}
	else
	{ push(@not_picked_,int($sort_output_words_[0]));}
	if($#new_population_+1>=$new_population_size_)
	{last;}
    }
    if($#new_population_ < 0)	
    {$flag_threshold_ = 0;}
    my $to_add_ = $new_population_size_-$#new_population_-1;
    for ( my $i=0; $i < $to_add_; $i++ )
    {
	push(@new_population_ , $initial_population_[$not_picked_[$i]]);
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
    my $t_sign_corr_ = $indicator_to_initial_corr_{$indicator} > 0 ? 1 : -1;	
    #printing out the strat file with current population
    my $t_strat_filename_ = $work_dir_."/"."tmp_strat";
    open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
    for ( my $i = 0; $i <  $initial_population_size_ ; $i ++)
    {
	my $t_model_filename_ = $work_dir_."/"."tmp_model".$i;
	my $t_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."\n";
	my $t_output_ = $model_pre_text_;
	$t_output_ = $t_output_."INDICATOR ".($t_sign_corr_*(1.7**$initial_population_[$i]))." ".$indicator."\n";			

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
    my $tradingdate_ = $insample_dates_[0];
    my @t_sim_strategy_output_lines_ = ( );
    my @tmpsampledates = ( );
    my $rem_days_ = $#insample_dates_+1;
    my $day_count_ = 0;
    while( $rem_days_ > 0)
    {
	my @sample_dates =( );
	my $num_strats_running = 0;

	for ( my $t_day_index_ = 0; $t_day_index_ < ($num_sim_strats_ < $rem_days_ ? $num_sim_strats_ : $rem_days_); $t_day_index_ ++ )
	{
	    my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ 97869 $tradingdate_ ADD_DBG_CODE -1 | grep SIMRESULT";
	    my $log_=$work_dir_."sim_res_".$tradingdate_;
	    push (@sample_dates, $tradingdate_);
	    push (@tmpsampledates , $tradingdate_ );
	    `$exec_cmd_ > $log_ & `; #run in background to parallelize
	    $num_strats_running+=1;
	    $day_count_ ++;
	    $tradingdate_ = $insample_dates_[$day_count_]; 
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
	$rem_days_ -= $num_sim_strats_;
    }
    for (my $j = 0; $j < $initial_population_size_ ; $j++ )
    {
	push ( @initial_population_to_score_ , 0 );
	push ( @initial_population_to_pnl_score_ , 0 );
	push ( @initial_population_to_agg_score_ , 0 );
	push ( @initial_population_to_volume_score_ , 0 );
    }
    
    foreach $tradingdate_ ( @tmpsampledates )
    {
	open(FILE, $work_dir_."sim_res_".$tradingdate_) or die("Unable to open file");
	# read file into an array
	@t_sim_strategy_output_lines_ = <FILE>;
	close(FILE);
	for (my $j = 0; $j < $initial_population_size_ ; $j ++ )
	{
	    chomp( $t_sim_strategy_output_lines_[$j]);
	    my @t_sim_rwords_ = split ( ' ', $t_sim_strategy_output_lines_[$j] );
	    $initial_population_to_pnl_score_[$j] += $t_sim_rwords_[1]/($#insample_dates_+1);
	    $initial_population_to_agg_score_[$j] += $t_sim_rwords_[5]/($#insample_dates_+1);
	    $initial_population_to_volume_score_[$j] += $t_sim_rwords_[2]/($#insample_dates_+1);
	}	
	
    }	
    return;
}
