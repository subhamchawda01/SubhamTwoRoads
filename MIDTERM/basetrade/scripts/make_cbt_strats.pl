#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);
use FileHandle;
use File::Path qw(mkpath);
use List::Util qw/max min/; # for max

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $GENPERLLIB_DIR = "$HOME_DIR/basetrade_install/GenPerlLib" ;
my $LIVE_BIN_DIR = "$HOME_DIR/LiveExec/bin" ;
my $MODELSCRIPTS_DIR = "$HOME_DIR/basetrade_install/ModelScripts" ;
my $SCRIPTS_DIR = "$HOME_DIR/basetrade_install/scripts" ;

require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/check_ilist_data.pl"; # CheckIndicatorData
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_min_volume_for_shortcode.pl"; # GetMinVolumeForShortcode
require "$GENPERLLIB_DIR/get_min_num_files_to_choose_for_shortcode.pl"; # GetMinNumFilesToChooseForShortcode
require "$GENPERLLIB_DIR/is_strat_dir_in_timeperiod.pl"; # IsStratDirInTimePeriod
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/get_business_days_between.pl"; # GetBusinessDaysBetween
require "$GENPERLLIB_DIR/install_strategy_modelling.pl"; # InstallStrategyModelling
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/array_ops.pl" ; # stats 


if ( scalar ( @ARGV ) < 4 ) 
{
    print "mtemplate stemplate config shc\n" ;
    exit ( 0 ) ;
}


######## CBT specific can be avoided ### 

my $mtemplate = $ARGV [ 0 ] ;
my $stemplate = $ARGV [ 1 ] ;
my $config = $ARGV [ 2 ] ;
my $shc = $ARGV [ 3 ] ;

my $task = "_STUDY" ;

my @lines = `grep $shc $config` ;
chomp ( @lines ) ;

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $MODELING_MODELS_DIR=$MODELING_BASE_DIR."/models"; # this directory is used to store the chosen model files
my $MODELING_PARAMS_DIR=$MODELING_BASE_DIR."/params"; # this directory is used to store the chosen param files


############GOES IN A FILE 

my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-1" ) ;
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-180" ) ;

my $instructionfilename_ = $task ;
my $strategy_ = "PriceBasedTrading" ;
my $paramfile = "/home/dvctrader/modelling/CBT/cbt.di.base.param" ;
my $trading_start_hhmm_ = "BRT_920" ;
my $trading_end_hhmm_ = "BRT_1500" ;
my $trading_start_end_str_ = "BRT_920-BRT_1500" ;
my $pid = 22897 ;
my @trading_exclude_days_ = ( ) ;

my $recent_trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-1" ) ;
my $recent_trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( "TODAY-7" ) ;



################params for choosing strats ( AveragePnl Sharpe Consistent lower_TTC higher_VOL ) ####### 


my $mail_address_ = "kp\@circulumvite.com" ;
my $min_num_files_to_choose_ = 15 ;
my $num_files_to_choose_ = 15 ;
my $historical_sort_algo_ = "kCNAPnlAverage" ;
my $min_volume_to_allow_ = 1 ;
my $min_pnl_per_contract_to_allow_ = -1 ;
my $max_ttc_to_allow_ = 8000 ;


#####################

##############################################################################################

############ holders 

my @strategy_filevec_ = ( ) ;
my @unique_results_filevec_ = ( ); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

my $MAX_STRAT_FILES_IN_ONE_SIM = 100 ;
my $delete_intermediate_files_ = 1 ;
my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $SAVE_TRADELOG_FILE = 0 ;

my @pnl_q = (); 
my @ttc_q = ();
my @vol_q = ();
my @ppc_q = ();

##############################################################################################

my %indicators1 = qw( 
pr.in DIPricingIndicator  
pr.in.me DIPricingIndicatorMktEvents

st DI1CurveAdjustedSimpleTrend
st.me DI1CurveAdjustedSimpleTrendMktEvents
ln.st DI1CurveAdjustedLinearSimpleTrend
ln.st.me DI1CurveAdjustedLinearSimpleTrendMktEvents

st.mo DI1CurveAdjustedSimpleTrendMomentum
st.me.mo DI1CurveAdjustedSimpleTrendMktEventsMomentum
ln.st.mo DI1CurveAdjustedLinearSimpleTrendMomentum
ln.st.me.mo DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum

);

my %indicators2 = qw( 
le.st  DI1LeveredSimpleTrend
le.st.me  DI1LeveredSimpleTrendMktEvents
);

my %indicators3 = qw(
pr DI1CurveAdjustedPrice
);

my $modelfile = "" ;
my $stratfile = "" ;


my $instbase_ = basename ( $instructionfilename_ ); 
chomp ( $instbase_ );

my $SPARE_HOME="/spare/local/".$USER."/";
my $GENSTRATWORKDIR=$SPARE_HOME."GSW/CBT";

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my $work_dir_ = $GENSTRATWORKDIR.$instbase_."/".$unique_gsm_id_."_$shc"; 

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $GENSTRATWORKDIR.$instbase_."/".$unique_gsm_id_."_$shc";
    }
    else
    {
	last;
    }
}


my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;

`mkdir -p "$work_dir_"`;
`mkdir -p "$work_dir_/models"`;
`mkdir -p "$work_dir_/strats"`;

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

foreach my $line ( @lines )
{
    my @tkns = split ( ' ' , $line ) ;
    next if ( $tkns[ 0 ] ne $shc ) ;

    if ( scalar ( @tkns ) == 4 )
    {
	foreach my $key ( keys % indicators1 )
	{
	    $modelfile = "$work_dir_/models/cbt.di.model.$tkns[ 1 ].$tkns[ 2 ].$tkns[ 3 ].$key";  
	    `cp $mtemplate $modelfile`;
	    `sed -i 's/__DEP__/$shc/g' $modelfile` ;
	    `sed -i 's/__INDEP1__/$tkns[ 1 ]/g' $modelfile` ;
	    `sed -i 's/__INDEP2__/$tkns[ 2 ]/g' $modelfile` ;
	    `sed -i 's/__DUR__/$tkns[ 3 ]/g' $modelfile` ;
	    `sed -i 's/__INDICATOR__/$indicators1{ $key }/g' $modelfile` ;
	    
	    $stratfile = "$work_dir_/strats/cbt.di.strat.$tkns[ 1 ].$tkns[ 2 ].$tkns[ 3 ].$key";  
	    `cp $stemplate $stratfile`;
	    `sed -i 's/__DEP__/$shc/g' $stratfile` ;
	    `sed -i 's/__TSTRATEGY__/$strategy_/g' $stratfile` ;
	    `sed -i 's|__MODELFILE__|$modelfile|g' $stratfile` ;
	    `sed -i 's|__PARAMFILE__|$paramfile|g' $stratfile` ;
	    `sed -i 's/__STIME__/$trading_start_hhmm_/g' $stratfile` ;
	    `sed -i 's/__ETIME__/$trading_end_hhmm_/g' $stratfile` ;
	    `sed -i 's/__SID__/$pid/g' $stratfile` ;

	    $pid++ ;
	    push ( @strategy_filevec_ , $stratfile ) ;
	    
	}	

    }    
    elsif ( scalar ( @tkns ) == 3 )
    {	
	foreach my $key ( keys % indicators2 )
	{
	    $modelfile = "$work_dir_/models/cbt.di.model.$tkns[ 1 ].$tkns[ 2 ].$key";  
	    `cp $mtemplate $modelfile`;
	    `sed -i 's/__DEP__/$shc/g' $modelfile` ;
	    `sed -i 's/__INDEP1__/$tkns[ 1 ]/g' $modelfile` ;
	    `sed -i 's/__INDEP2__//g' $modelfile` ;
	    `sed -i 's/__DUR__/$tkns[ 2 ]/g' $modelfile` ;
	    `sed -i 's/__INDICATOR__/$indicators2{ $key }/g' $modelfile` ;

	    $stratfile = "$work_dir_/strats/cbt.di.strat.$tkns[ 1 ].$tkns[ 2 ].$key";  
	    `cp $stemplate $stratfile`;
	    `sed -i 's/__DEP__/$shc/g' $stratfile` ;
	    `sed -i 's/__TSTRATEGY__/$strategy_/g' $stratfile` ;
	    `sed -i 's|__MODELFILE__|$modelfile|g' $stratfile` ;
	    `sed -i 's|__PARAMFILE__|$paramfile|g' $stratfile` ;
	    `sed -i 's/__STIME__/$trading_start_hhmm_/g' $stratfile` ;
	    `sed -i 's/__ETIME__/$trading_end_hhmm_/g' $stratfile` ;
	    `sed -i 's/__SID__/$pid/g' $stratfile` ;

	    $pid++ ;
	    push ( @strategy_filevec_ , $stratfile ) ;
	    
	}	
	
    }
    
}

RunSimulationOnCandidates ( ) ;

if ( $task eq "INSTALL" )
{
    SummarizeLocalResultsAndChoose ( );
}

#***********************************************************************#

sub RunSimulationOnCandidates
{
    
    my @non_unique_results_filevec_=();
    my $tradingdate_ = $trading_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000;

    print $main_log_file_handle_ "start_date : $trading_start_yyyymmdd_ :: end_date : $trading_end_yyyymmdd_ \n";

    for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
    {
	print $main_log_file_handle_ "date under consideration : $tradingdate_ \n";
	
	if ( ! defined $tradingdate_ || $tradingdate_ eq '' ) { 
	    print "the tradingdate_ is missing. File: ",__FILE__, " Line: ", __LINE__, "\n t_day_index=$t_day_index_, trading_start_end_date=[$trading_start_yyyymmdd_] [$trading_end_yyyymmdd_]\n";
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next; 
	}
	if ( ( $tradingdate_ < 20101231 ) || ( $tradingdate_ > 20200220 ) )
	{
	    last;
	}
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     ( NoDataDateForShortcode ( $tradingdate_ , $shc ) ) || 
	     ( IsDateHoliday ( $tradingdate_ ) || 
	       ( ( $shc ) && ( IsProductHoliday ( $tradingdate_, $shc ) ) ) ) )
	{
	    print $main_log_file_handle_ "skipping date : $tradingdate_ ( weird_date || no_data || holiday ) \n";
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}
	
	if ( FindItemFromVec ( $tradingdate_ , @trading_exclude_days_ ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}
	
	if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	     ( $tradingdate_ < $trading_start_yyyymmdd_ ) )
	{
	    print $main_log_file_handle_ "not a valid date or greater than start_date exiting : $tradingdate_ , $trading_start_yyyymmdd_ \n";
	    last;
	}
	else 
	{
	    # for this $tradingdate_ break the @strategy_filevec_ into blocks of size MAX_STRAT_FILES_IN_ONE_SIM
	    # for a block print the filenames in a $temp_strategy_list_file_
	    # run sim_strategy, store output in @sim_strategy_output_lines_
	    # print lines in @sim_strategy_output_lines_ that have the word "SIMRESULT" into $temp_results_list_file_.
            # run add_results_to_local_database.pl ... push the file written to @non_unique_results_filevec_
	    my $strategy_filevec_front_ = 0;
	    my $temp_strategy_list_file_index_ = 0;
	    while ( $strategy_filevec_front_ <= $#strategy_filevec_ )
	    { # till there are more files in the list to service
		
		my $strategy_filevec_back_ = min ( ( $strategy_filevec_front_ + $MAX_STRAT_FILES_IN_ONE_SIM - 1 ), $#strategy_filevec_ ) ;
		
		my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
		my $temp_strategy_cat_file_ = $work_dir_."/temp_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
		open TSLF, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_ for writing\n" );
		if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
		for ( my $t_strategy_filevec_index_ = $strategy_filevec_front_; $t_strategy_filevec_index_ <= $strategy_filevec_back_; $t_strategy_filevec_index_ ++ )
		{
		    print TSLF $strategy_filevec_[$t_strategy_filevec_index_]."\n";
		    `cat $strategy_filevec_[$t_strategy_filevec_index_] >> $temp_strategy_cat_file_`;
		}

		close TSLF;
		
		my @sim_strategy_output_lines_=(); # stored to seive out the SIMRESULT lines
		my %unique_id_to_pnlstats_map_=(); # stored to write extended results to the database
		{
		    
		    my $market_model_index_ = 0 ;
		    $market_model_index_ = GetMarketModelForShortcode ( $shc );
		    
		    my $sim_strat_cerr_file_ = $temp_strategy_cat_file_."_cerr";
		    my $exec_cmd="$LIVE_BIN_DIR/sim_strategy SIM $temp_strategy_cat_file_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 2>$sim_strat_cerr_file_"; 
		    if ( CheckIndicatorData ($tradingdate_, $temp_strategy_cat_file_) == 0 ){
			print $main_log_file_handle_ "$exec_cmd\n";
			@sim_strategy_output_lines_=`$exec_cmd`;
		    }
		    else {
		    	@sim_strategy_output_lines_ = ();
		    }
		    
		    my @cerr_ = `cat $sim_strat_cerr_file_`;
		    foreach my $t_sim_strategy_line_ ( @cerr_ )
		    {
			if ( index ( $t_sim_strategy_line_ , "glibc" ) >= 0 ||
			     index ( $t_sim_strategy_line_ , "HUGE"  ) >= 0 )
			{
			    print @cerr_."\n"; # Print if glibc detected.
			    print $main_log_file_handle_ "SimOutPut: @cerr_\n";
			    last;
			}
		    }
		    `rm -f  $sim_strat_cerr_file_`;
		    # To detect glibcs.
		    print $main_log_file_handle_ @sim_strategy_output_lines_."\n";
		    
		    my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
		    if ( ExistsWithSize ( $this_tradesfilename_ ) )
		    {
			$exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";
			print $main_log_file_handle_ "$exec_cmd\n";
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
		    
		    # added deletion of tradesfiles
		    if ( $SAVE_TRADELOG_FILE == 0 ) 
		    { 
			`rm -f $this_tradesfilename_`; 
			my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_gsm_id_);
			`rm -f $this_tradeslogfilename_`;
		    }
		}
		
		
		my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
		
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
				 ( ( $shc =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day
			    {
				my $unique_sim_id_ = GetUniqueSimIdFromCatFile ( $temp_strategy_cat_file_, $psindex_ );
				if ( ! exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
				{
				    $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0 0 0";
#				PrintStacktraceAndDie ( "unique_id_to_pnlstats_map_ missing $unique_sim_id_ for listfile: $temp_results_list_file_ catfile: $temp_strategy_cat_file_ rline: $remaining_simresult_line_\n" );
				}
				printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_ ;
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
		    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database.pl $temp_strategy_list_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir"; # TODO init $local_results_base_dir
		    print $main_log_file_handle_ "$exec_cmd\n";
		    my $this_local_results_database_file_ = `$exec_cmd`;
		    push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
		}
		
		if ( $delete_intermediate_files_ )
		{
		    if ( -e $temp_strategy_cat_file_ ) { `rm -f $temp_strategy_cat_file_`; }
		    if ( -e $temp_strategy_list_file_ ) { `rm -f $temp_strategy_list_file_`; }
		    if ( -e $temp_results_list_file_ ) { `rm -f $temp_results_list_file_`; }
		}
		$strategy_filevec_front_ = $strategy_filevec_back_ + 1; # front is now set to the first item of the next block
		$temp_strategy_list_file_index_ ++;
	    }
	}
	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
    @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ ); # the same result file is written to by all blocks of strategy files on the same date, that's why @non_unique_results_filevec_ might have duplicate entries
}


#*************************************************************************#


sub SummarizeLocalResultsAndChoose
{
##SINGLE RECORD 1 
#DATE PNL 2 
#VOLUME 3 
#MEDIAN_TTC 4 
#MEAN_TTC 5 
#MAX_TTC 6 
#MIN_PNL 7
#MAX_PNL 8
#MIN_ADJUST_PNL 9
#PNL_ZSCORE 10
#SUPPORT_FILL 11
#BEST_FILL 12
#AGRESS_FILL MEAN_ABS_POS 13
    
    my %slrdacba_STATISTICS_line_indices = qw(
DUMMY 0
MEAN_PNL 1
STD_PNL 2
MEAN_VOL 3
SHARPE_PNL 4 
"MU-SD/3" 5
MEAN_MIN_ADJUST_PNL 6
MEDIAN_MEAN_TTC 7
MEAN_PPC 8
S_FP 9
B_FP 10
A_FP 11
MEAN_MAX_DD 12
);

#for each strategy for n days summarize_strategy_results
    my %ssr_line_indices = qw(
DUMMY 0
SNAME 1
MEAN_PNL 2
STD_PNL 3
MEAN_VOL 4
SHARPE_PNL 5 
CPNL 6
MEDIAN_AVERAGE_PNL 7 
MEDIAN_MEAN_TTC 8
ZSCORE_MEAN_PNL 9
MEAN_MIN_ADJUST_PNL 10
MEAN_PPC 11
S_FP 12
B_FP 13
A_FP 14
MEAN_MAX_DD 15
DD_ADJUST_MEAN_PNL 16
);


    print $main_log_file_handle_ "SummarizeLocalResultsAndChoose\n";
    
    if ( $#unique_results_filevec_ >= 0 )
    {
	# hoping one day we will add for all products and take only this route
	$min_num_files_to_choose_ = max ( $min_num_files_to_choose_ , GetMinNumFilesToChooseForShortcode ( $shc ) );	
	$min_volume_to_allow_ = max ( $min_volume_to_allow_ , GetMinVolumeForShortcode ( $shc ) );	
	
	my $mail_body_ = "";	
	
	# get current strat pool quantiles ad cut off i.e using global results  
	print $main_log_file_handle_ "retreiving current stats profile \n";
	my $num_strats_in_global_results_=0;
	my $global_results_dir_path = "/NAS1/ec2_globalresults/";
	my $srv_name_=`hostname | cut -d'-' -f2`; chomp($srv_name_);

	if ( $srv_name_ =~ "crt" )
	{
	    $global_results_dir_path = $HOME_DIR."/ec2_globalresults/";
	}
	
	my $timeperiod_ = GetTimePeriodFromTime();
	print $main_log_file_handle_ "TimePeriod: $timeperiod_\n";
	my $cstempfile_ = GetCSTempFileName ( $work_dir_."/cstemp" );
	
	open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
	
	my @exclude_tp_dirs_ = ();
	my @all_strats_in_dir_ = MakeStratVecFromDirInTpExcludingSets ( $MODELING_STRATS_DIR."/".$shc, $timeperiod_, @exclude_tp_dirs_ );
	
	print $main_log_file_handle_ "all global strats: ".join("\n", @all_strats_in_dir_)."\n";
	
	for(my $i=0; $i < $#all_strats_in_dir_; $i++)
	{
	    my $t_strat_file_ = basename($all_strats_in_dir_[$i]);
	    print CSTF "$t_strat_file_\n";
	}

	$num_strats_in_global_results_ = $#all_strats_in_dir_ + 1;
	close CSTF;
	
	if( "$num_strats_in_global_results_" eq "")
	{ 
	    print STDERR "No previous strats in the pool";
	    $num_strats_in_global_results_ = 0; 
	}
	    
	print $main_log_file_handle_ "num of strats in global result: $num_strats_in_global_results_\n"; 
	my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shc $cstempfile_ $global_results_dir_path $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ INVALIDFILE $historical_sort_algo_";
	print $main_log_file_handle_ $exec_cmd."\n";
	my @global_res_out_ = `$exec_cmd`; 
	print $main_log_file_handle_ "@global_res_out_\n";chomp(@global_res_out_);
	
	# already sorted by sort_algo lets says PnlAverage 
	my $nstrats = scalar ( @global_res_out_ ) ;
	
	if ( $nstrats > 0 ) 
	{
	    my @pnl_vec = ( ) ;
	    my @ttc_vec = ( ) ;
	    my @vol_vec = ( ) ;
	    my @ppc_vec = ( ) ;
	    
	    for ( my $j = 0 ; $j < $nstrats ; $j++ )
	    {
		my @res_line_words = split ( ' ', $global_res_out_ [ $j ] );		
		push ( @pnl_vec, $res_line_words[ $ssr_line_indices { "MEAN_PNL" } ] ) ; 
		push ( @ttc_vec, $res_line_words[ $ssr_line_indices { "MEDIAN_MEAN_TTC" } ] ) ; 
		push ( @vol_vec, $res_line_words[ $ssr_line_indices { "MEAN_VOL" } ] ) ;  
		push ( @ppc_vec, $res_line_words[ $ssr_line_indices { "MEAN_PPC" } ] ) ; 
	    }	
	    
	    # pnl_vec is already in desc order 
	    @ttc_vec = sort {$a <=> $b} @ttc_vec ;
	    @ppc_vec = sort {$b <=> $a} @ppc_vec ;
	    @vol_vec = sort {$b <=> $a} @vol_vec ;
	    
	    push ( @pnl_q , $pnl_vec [ 0 ] , $pnl_vec [ int ( ( $#pnl_vec + 1 ) * 0.25 ) - 1 ] , $pnl_vec [ int ( ( $#pnl_vec + 1 ) * 0.5 ) - 1 ] , $pnl_vec [ int ( ( $#pnl_vec + 1 ) * 0.75 ) - 1 ] , $pnl_vec [ int ( ( $#pnl_vec + 1 ) * 1 ) - 1 ] ) ;
	    push ( @ttc_q , $ttc_vec [ 0 ] , $ttc_vec [ int ( ( $#ttc_vec + 1 ) * 0.25 ) - 1 ] , $ttc_vec [ int ( ( $#ttc_vec + 1 ) * 0.5 ) - 1 ] , $ttc_vec [ int ( ( $#ttc_vec + 1 ) * 0.75 ) - 1 ] , $ttc_vec [ int ( ( $#ttc_vec + 1 ) * 1 ) - 1 ] ) ;
	    push ( @vol_q , $vol_vec [ 0 ] , $vol_vec [ int ( ( $#vol_vec + 1 ) * 0.25 ) - 1 ] , $vol_vec [ int ( ( $#vol_vec + 1 ) * 0.5 ) - 1 ] , $vol_vec [ int ( ( $#vol_vec + 1 ) * 0.75 ) - 1 ] , $vol_vec [ int ( ( $#vol_vec + 1 ) * 1 ) - 1 ] ) ;
	    push ( @ppc_q , $ppc_vec [ 0 ] , $ppc_vec [ int ( ( $#ppc_vec + 1 ) * 0.25 ) - 1 ] , $ppc_vec [ int ( ( $#ppc_vec + 1 ) * 0.5 ) - 1 ] , $ppc_vec [ int ( ( $#ppc_vec + 1 ) * 0.75 ) - 1 ] , $ppc_vec [ int ( ( $#ppc_vec + 1 ) * 1 ) - 1 ] ) ;

	    print $main_log_file_handle_ "num of old strats in modelling strats dir with results : $nstrats \n";	    
	    print $main_log_file_handle_ "current pnl quantiles @pnl_q " ;
	    print $main_log_file_handle_ "current ttc quantiles @ttc_q " ;
	    print $main_log_file_handle_ "current vol quanitles @vol_q " ;
	    print $main_log_file_handle_ "current ppc quantiles @ppc_q " ;

	    $mail_body_ = $mail_body_."num of old strats in modelling strats dir with results : $nstrats \n";
	    $mail_body_ = $mail_body_."current PNL percentile cutoffs : @pnl_q\n" ;
	    $mail_body_ = $mail_body_."current TTC percentile cutoffs : @ttc_q\n" ;
	    $mail_body_ = $mail_body_."current VOL percentile cutoffs : @vol_q\n" ;
	    $mail_body_ = $mail_body_."current PPC percentile cutoffs : @ppc_q\n\n" ;
		
	}
	else
	{
	    print $main_log_file_handle_ "num of old strats in modelling strats dir with results : $nstrats \n";  # i.e 0 
	}
	
	# now use quantile cut offs on local results and pick strats 
	$mail_body_ = $mail_body_." result set generated using from $trading_start_yyyymmdd_ to $trading_end_yyyymmdd_ \n" ;
	
	print $main_log_file_handle_ " using min_num_files_to_choose_=".$min_num_files_to_choose_." min_volume_to_allow_=".$min_volume_to_allow_."\n";
	$exec_cmd="$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo $historical_sort_algo_ $min_num_files_to_choose_ $num_files_to_choose_ $min_pnl_per_contract_to_allow_ $min_volume_to_allow_ $max_ttc_to_allow_ $local_results_base_dir";
	print $main_log_file_handle_ "$exec_cmd\n";
	my @new_strats_results_summary_=`$exec_cmd`;
	print $main_log_file_handle_ @new_strats_results_summary_;
	print $main_log_file_handle_ "\n";
	
	my @strat_files_selected_ = ();
	my %strat_indices_ = ();

	my $last_strat_file_selected_ = "";
	
	for ( my $t_new_strats_results_summary_index_ = 0 ; $t_new_strats_results_summary_index_ <= $#new_strats_results_summary_ ; $t_new_strats_results_summary_index_++ )
	{
	    my $this_line_ = $new_strats_results_summary_[ $t_new_strats_results_summary_index_ ];
	    if ( $this_line_ =~ /STRATEGYFILEBASE/ )
	    { # STRATEGYFILEBASE basename of strategyfile
		my @strat_line_words_ = split ( ' ', $this_line_ );
		if ( $#strat_line_words_ >= 1 )
		{ # STRATEGYFILEBASE w_strategy_ilist_ZB_EU_PBT_30_na_e3_20110629_20110729_CET_805_EST_800_fsg.5_FSRR.0.5.0.01.0.0.0.85.tt_CET_805_EST_800.pfi_0
		    $last_strat_file_selected_ = $strat_line_words_[1];
		    print $main_log_file_handle_ "strat under consideration $last_strat_file_selected_  \n";
		}
		else
		{
		    $last_strat_file_selected_ = "" ;
		    print $main_log_file_handle_ "STRATEGYFILEBASE line as $#strat_line_words_ , expected >=1 \n "
		}
	    }
	    if ( $this_line_ =~ /STATISTICS/ )
	    { # STATISTICS ....
		print $main_log_file_handle_ "STATISTICS LINE FOUND for  $last_strat_file_selected_\n " ;
		if ( length ( $last_strat_file_selected_ ) > 0 ) 
		{ 
		    my @stat_line_words_ = split ( ' ', $this_line_ );
		    if ( $#stat_line_words_ == 12 )
		    {
			my $npnl_ = $stat_line_words_[$slrdacba_STATISTICS_line_indices { "MEAN_PNL" } ]; 
			my $nttc_ = $stat_line_words_[$slrdacba_STATISTICS_line_indices { "MEDIAN_MEAN_TTC" } ]; 
			my $nvol_ = $stat_line_words_[$slrdacba_STATISTICS_line_indices { "MEAN_VOL" } ]; 
			my $nppc_ = $stat_line_words_[$slrdacba_STATISTICS_line_indices { "MEAN_PPC" } ]; 
			
			my $pnl_r = 5 ; my $ttc_r = 5 ; my $vol_r = 5 ; my $ppc_r = 5 ;

			for ( my $i  = 0 ; $i < scalar ( @pnl_q ) ; $i ++ )
			{
			    if ( $npnl_ > $pnl_q[ $i ] )
			    { 
				$pnl_r  = $i ; last ; 
			    }
			}
			for ( my $i  = 0 ; $i < scalar ( @ttc_q ) ; $i ++ )
			{
			    if ( $nttc_ < $ttc_q [ $i ] ) 
			    { 
				$ttc_r  = $i ; last ; 
			    }
			}
			for ( my $i  = 0 ; $i < scalar ( @vol_q ) ; $i ++ ) 
			{
			    if ( $nvol_ > $vol_q [ $i ] ) 
			    { 
				$vol_r  = $i ; last ; 
			    }
			}
			for ( my $i  = 0 ; $i < scalar ( @ppc_q ) ; $i ++ )
			{
			    if ( $nppc_ > $ppc_q [ $i ] ) 
			    { 
				$ppc_r  = $i ; last ; 
			    }
			}
			
			## decision tree ## ignoring ppc for now ( ??  )
			if ( ( $pnl_r == 0 ) || 
			     ( $pnl_r == 1 && $ttc_r < 5 && $vol_r < 5 ) || 
			     ( $pnl_r == 2 && $ttc_r < 4 && $vol_r < 4 ) || 
			     ( $pnl_r == 3 && $ttc_r < 3 && $vol_r < 3 ) || 
			     ( $pnl_r == 4 && $ttc_r < 2 && $vol_r < 2 ) || 
			     ( $pnl_r == 5 && scalar ( @pnl_q ) == 0 ) )
			    {
				print $main_log_file_handle_ "selected $last_strat_file_selected_\n " ;
				push ( @strat_files_selected_, $last_strat_file_selected_ ) ;
				$mail_body_ .= "\n".$last_strat_file_selected_."\n".$this_line_;
				$mail_body_ .= "percentile ranks in the existing Pool(of Size: $num_strats_in_global_results_): PNL : $pnl_r || TTC : $ttc_r || VOL : $vol_r || PPC : $ppc_r \n\n";
				$last_strat_file_selected_ = "" ;
			    }
			else
			{
			    print $main_log_file_handle_  " failed with ranks $pnl_r $ttc_r $vol_r \n" ;
			}
		    }
		    else
		    {
			print $main_log_file_handle_ "STATISTIC line as $#stat_line_words_ expected 12 " ;
		    }
		}
		else
		{
		    print $main_log_file_handle_ "couldn't find STRATEGYFILEBASE line correposing to this STATISTICS line \n"
		}
	    }
	}
	
	print $main_log_file_handle_ " no of strat files selected are $#strat_files_selected_ \n " ;
	
	if ( $#strat_files_selected_ >= 0 )
	{
	    $mail_body_ = $mail_body_." FOR SELECTED STRATS RESULTS SAMPLED FROM $recent_trading_start_yyyymmdd_ $recent_trading_end_yyyymmdd_ \n" ;
	    for ( my $i = 0 ; $i <= $#strat_files_selected_; $i ++ )
	    {
		$mail_body_ = $mail_body_."\nfor selected strat: ".$strat_files_selected_[$i]."\n";
		print $main_log_file_handle_ "for strat: ".$strat_files_selected_[$i]."\n";
		my $strat_fl_ = FindItemFromVecWithBase ( $strat_files_selected_[$i], @strategy_filevec_  );
		
		my @t_recent_text_ = ();
		
		my $recent_sim_day_count_ = GetBusinessDaysBetween ( $recent_trading_start_yyyymmdd_, $recent_trading_end_yyyymmdd_ );
		print $main_log_file_handle_ "recent days between $recent_trading_start_yyyymmdd_ $recent_trading_end_yyyymmdd_ = $recent_sim_day_count_\n";
		if ( $recent_sim_day_count_ > 3 )
		{
		    # last 5 days results
		    $exec_cmd="$LIVE_BIN_DIR/summarize_single_strategy_results local_results_base_dir $strat_files_selected_[$i] $work_dir_ $recent_trading_start_yyyymmdd_ $recent_trading_end_yyyymmdd_";
		    print $main_log_file_handle_ "$exec_cmd\n";
		    @t_recent_text_ = `$exec_cmd`;
		    $mail_body_ = $mail_body_.join ( "", @t_recent_text_ ) ;
		    print $main_log_file_handle_ @t_recent_text_;
		    print $main_log_file_handle_ "\n";
		}

		my $t_temp_strategy_filename_ = FindItemFromVecWithBase ( $strat_files_selected_[$i], @strategy_filevec_  ) ;
		printf $main_log_file_handle_ "InstallStrategyModelling ( $t_temp_strategy_filename_, $shc , $trading_start_end_str_, $trading_start_end_str_ )";
		InstallStrategyModelling ( $t_temp_strategy_filename_, $shc, $trading_start_end_str_, $trading_start_end_str_ );
	    }
	    
	}
	else 
	{
	    if ($#new_strats_results_summary_>=0)
	    {
		$mail_body_ .= "*ERROR*: None of the strategies could cross the pass cutoffs \n\n" ;
	    }
	}
	if ( ( $mail_address_ ) &&
	     ( $mail_body_ ) )
	{
	    open(MAIL, "|/usr/sbin/sendmail -t");
	    
	    my $hostname_=`hostname`;
	    ## Mail Header
	    print MAIL "To: $mail_address_\n";
	    print MAIL "From: $mail_address_\n";
	    print MAIL "Subject: genstrat ( $instructionfilename_ ) $yyyymmdd_ $hhmmss_ $hostname_\n\n";
	    ## Mail Body
	    print MAIL $mail_body_ ;
	    
	    close(MAIL);
	    
	    print $main_log_file_handle_ "Mail Sent to $mail_address_\n$mail_body_\n";
	}
    }
}

#************************************************************************************************#
#**************************************************************************************************************#

sub GetTimePeriodFromTime 
{
    my $dir_name_ = "$trading_start_hhmm_-$trading_end_hhmm_";
    my $retval_ = $dir_name_;
    if    (IsStratDirInTimePeriod($dir_name_, "EU_MORN_DAY") ) { $retval_ = "EU_MORN_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN_DAY") ) { $retval_ = "US_MORN_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "AS_MORN")     ) { $retval_ = "AS_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "AS_DAY")      ) { $retval_ = "AS_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_EARLY_MORN")){ $retval_ = "US_EARLY_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN")     ) { $retval_ = "US_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_DAY")      ) { $retval_ = "US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_MORN")     ) { $retval_ = "US_MORN"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "US_DAY")      ) { $retval_ = "US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "EU_MORN_DAY_US_DAY")) { $retval_ = "EU_MORN_DAY_US_DAY"; }
    elsif (IsStratDirInTimePeriod($dir_name_, "EU_US_MORN_DAY")) { $retval_ = "EU_US_MORN_DAY";}
    $retval_;
}
