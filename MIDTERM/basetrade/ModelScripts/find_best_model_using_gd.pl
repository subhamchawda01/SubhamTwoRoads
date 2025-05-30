#!/usr/bin/perl
#
## \file ModelScripts/find_best_model_for_strategy_var_pert.pl
##
## \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
##  Address:
##       Suite No 162, Evoma, #14, Bhattarhalli,
##       Old Madras Road, Near Garden City College,
##       KR Puram, Bangalore 560049, India
##       +91 80 4190 3551
#


#import libraries:
use FileHandle;
use List::Util qw/max min/; # for max



#sub declarations
sub ReadStratFile;
sub CreateWorkDirectory;
sub InitializeLogFile;
sub ReadModelFile;
sub WriteOutModel;
sub CalcPnls;



#Global variables of strat file:
my $base_shortcode_ 	= "";
my $base_pbat_dat_ 		= "";
my $base_model_file_ 	= "";
my $base_param_file_ 	= "";
my $base_start_time_ 	= "";
my $base_end_time_ 		= "";
my $base_prog_id_ 		= "";



#initialise paths:
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $REPO="basetrade";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";


#Use functions from other perl files:
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir

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


require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllPIDSTerminated
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl";



#Usage declaration
my $DEBUG = 1;
my $USAGE="$0 input_strat_file output_model last_trading_date_ num_days_history_  [training_period_fraction_=0.7] [training_days_type_(ALL|HV|BD|VBD|ECO)]";


if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); } 
my $strat_file_ = $ARGV [ 0 ];
my $output_model_filename_ = $ARGV [ 1 ];
my $last_trading_date_ = GetIsoDateFromStrMin1 ( $ARGV[2] ) ;
my $num_days_ = max ( 1, int($ARGV [ 3 ]) );
my $training_period_fraction_ = $ARGV [ 4 ];
my $training_days_type_ = $ARGV[5];
my $use_fake_faster_data_ = 0;
my $sort_algo_ = "kCNAPnlAverage";

#Initializations:
( $base_shortcode_, $base_pbat_dat_, $base_model_file_, $base_param_file_, $base_start_time_, $base_end_time_, $base_prog_id_ ) = ReadStratFile ( $strat_file_ );
my $work_dir_ = CreateWorkDirectory ( $base_shortcode_ );
my $main_log_file_handle_ = InitializeLogFile ( $work_dir_ );
$main_log_file_handle_->print ( "Starting Logs" );


#read model file
my $base_model_start_text_ = "";
my $model_type_ = "";
my @indicator_list_ = ( );
my @weight_vec_ = ( );
my @sign_vec_ = ( );
my @siglr_alpha_vec_ = ( );
ReadModelFile( $base_model_file_, \$model_type_, \@indicator_list_, \@weight_vec_, \@sign_vec_, \@siglr_alpha_vec_ ) ;

#fill training date vector
my @training_date_vec_ = ();
my @outofsample_date_vec_ = ();
my $eco_date_time_file_ = "";
my %eco_date_time_map_;
FillDateVecs( \@training_date_vec_, \@outofsample_date_vec_,$last_trading_date_, $num_days_, $training_days_type_, $eco_date_time_file_, $eco_date_time_map_, $base_shortcode_, $training_period_fraction_ );

#initializations
my @indicator_weight_vec_vec_crossval_ = ( );
push (@indicator_weight_vec_vec_crossval_, \@weight_vec_);


my $strat_pre_text_ = "STRATEGYLINE ".$base_shortcode_." ".$base_pbat_dat_." ";
my $strat_post_text_ = " ".$base_param_file_." ".$base_start_time_." ".$base_end_time_." ".$base_prog_id_;
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

print "$#training_date_vec_\n";
while( 2>1 )
{
  my @stats_vec_crossval_ = ( );
  my @volume_vec_crossval_ = ( );
  my @ttc_vec_crossval_ = ( );
  my @sigma_vec_crossval_ = ( );
# my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );
  my $stats = CalcPnls ( \@indicator_weight_vec_vec_crossval_, \@stats_vec_crossval_, \@volume_vec_crossval_, \@ttc_vec_crossval_, \@sigma_vec_crossval_, 2, "train", \@training_date_vec_, 
             $work_dir_, $strat_pre_text_, $strat_post_text_, $base_model_start_text_, \@siglr_alpha_vec_, \@indicator_list_, $base_shortcode_, $use_fake_faster_data_, $sort_algo_ );

  print "$stats\n";

  for( my $i = 0; $i <= $#stats_vec_crossval_; $i ++ )
  {
      print "$sigma_vec_crossval_[$i]\n ";
  }

}
WriteOutModel ( \@weight_vec_, $model_type_, $base_model_file_, $output_model_filename_ );

###############################################################################
# Read the strategy file to get the model file, param file and other parameters
###############################################################################
sub ReadStratFile
{
  if ( $DEBUG ) { printf ( "Reading Strat File...\n" ); }

  my ( $strat_file_ ) = @_;
  open STRAT_FILE, "< $strat_file_" or PrintStacktraceAndDie ( "Could not open strategy file $strat_file_ for reading\n" );
  my $strat_line_ = <STRAT_FILE>;
  my @strat_words_ = split ' ', $strat_line_;
  return( $strat_words_[1], $strat_words_[2], $strat_words_[3], $strat_words_[4], $strat_words_[5], $strat_words_[6], $strat_words_[7] );
}


###############################################################################
#Create a directory for the log files and returns the path to that directory
###############################################################################
sub CreateWorkDirectory
{
  if ( $DEBUG ) { printf ( "Creating Working Directory...\n" ); }

  my ( $shortcode ) = @_; 
  my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
	my $SPARE_LOCAL="/spare/local/";
	my $hostname_ = `hostname`;
  my $USER = $ENV{'USER'};
	if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
	{
		$SPARE_LOCAL = GetSpareLocalDir();
	}
	my $work_dir_ = $SPARE_LOCAL."$USER/PerturbedModelTests/";
	$work_dir_ = $work_dir_."/".$shortcode."/".$unique_gsm_id_;

	for ( my $i = 0 ; $i < 30 ; $i ++ )
	{
		if ( -d $work_dir_ )
		{
			$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
			$work_dir_ = $SPARE_LOCAL."$USER/PerturbedModelTests/"."/".$base_shortcode_."/".$unique_gsm_id_;
		}
		else
		{
			last;
		}
	}
	if ( -d $work_dir_ ) { `rm -rf $work_dir_`; }
	`mkdir -p $work_dir_ `; #make_work_dir
  return $work_dir_
}

###############################################################################
#Initialize the file, make the handle for it and return the handle
###############################################################################
sub InitializeLogFile
{
  if ( $DEBUG ) { printf ( "Initializing Log file...\n" ); }

  my ( $work_dir_ ) = @_;
	my $main_log_file_ = $work_dir_."/main_log_file.txt";
	my $main_log_file_handle_ = FileHandle->new;
	$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
	$main_log_file_handle_->autoflush(1);
	my $start_time_ =`date +%s`; chomp ( $start_time_ );
	$main_log_file_handle_->print ( "Optimizing $strat_file_ starting at $start_time_\n" );
  return $main_log_file_handle_;
}


###############################################################################
# Read the strategy file to get the model file, param file and other parameters
###############################################################################
sub ReadModelFile
{
#please make sure the model file has scores with '#', right now there is no check for that
  if ( $DEBUG) { $main_log_file_handle_->print ( "ReadModelFile\n" ); }

  my ( $base_model_file_, $model_type_, $indicator_list_, $weight_vec_, $sign_vec_, $siglr_alpha_vec_ ) = @_;
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
      $$model_type_ = $model_words_[1];
    }

    if($model_words_[0] eq "INDICATOR")
    {
      my @i_words_ = @model_words_;
      shift(@i_words_); shift(@i_words_);
      #pop(@i_words_);pop(@i_words_);
      my $t_indicator_name_ = join(' ',@i_words_);

      push( @$indicator_list_, $t_indicator_name_);

      if ( $$model_type_ eq "SIGLR" )
      {
        my @t_siglr_words_ = split ':', $model_words_[1];
        push( @$siglr_alpha_vec_, $t_siglr_words_[0] );
        push( @$weight_vec_, $t_siglr_words_[1] );
        if ( ( $t_siglr_words_[0] + 0 ) * ( $t_siglr_words_[1] + 0 ) >= 0 ) { push ( @$sign_vec_, 1 ); }
        else { push (@$sign_vec_, -1); }
      }
      else
      {
        push( @$weight_vec_, $model_words_[1] + 0 );
        if ( $model_words_[1] + 0 >=0 ) { push (@$sign_vec_ , 1 ); }
        else { push (@$sign_vec_, -1); }
      }

    }
  }
  return ;
}

##############################################################################
# Write out the model in the output file, given the weights we want
###############################################################################
sub WriteOutModel
{
  if ( $DEBUG ) { $main_log_file_handle_->print ( "WriteOutModel\n" ); }
  my ( $tref_, $model_type_, $base_model_file_, $output_model_filename_ ) = @_;

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
      $model_words_[1] = ( $$tref_[$indicator_index_] + 0 );
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

##############################################################################
# Fill in the training and test date vectors
##############################################################################
sub FillDateVecs
{
  my ( $training_date_vec_ref_, $outsample_date_vec_ref_, $t_last_trading_date_, $t_num_days_, $training_days_type_, $eco_date_time_file_, $eco_date_time_map_, $base_shortcode_, $training_period_fraction_ ) = @_;
  if ( $DEBUG )
  {
    $main_log_file_handle_->print  ("FillDateVec $t_last_trading_date_ $t_num_days_\n" );
  }

  if($training_days_type_ eq "ECO")
  {
    open ECO_DATE_TIME_FILE, "< $eco_date_time_file_" or PrintStacktraceAndDie("can't open eco_date_time_file_ $eco_date_time_file_ for reading.\n");
    while (my $line_ = <ECO_DATE_TIME_FILE>)
    {
      chomp($line_);
      my @words_ = split ' ', $line_;
      if($#words_ >= 2)
      {
        push ( @$training_date_vec_ref_, $words_[0]);
        push ( @$outsample_date_vec_ref_, $words_[0]);	#for ECO days, outsample is same as insample, juts to see performance in mail....outsample don't make much sense in case of ECO
            $eco_date_time_map_{$words_[0]} = [ ( $words_[1], $words_[2] ) ];
      }
    }
    close ECO_DATE_TIME_FILE;
    return;
  }


  if($training_days_type_ eq "HV" || $training_days_type_ eq "BD" || $training_days_type_ eq "VBD")
  {   
    my @special_day_vec_ = ();
    if($training_days_type_ eq "HV")
    {
      GetHighVolumeDaysForShortcode ( $base_shortcode_, \@special_day_vec_ );
    }
    if($training_days_type_ eq "BD")
    {
      GetBadDaysForShortcode ( $base_shortcode_, \@special_day_vec_ );
    }
    if($training_days_type_ eq "VBD")
    {
      GetVeryBadDaysForShortcode ( $base_shortcode_, \@special_day_vec_ );
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
#with probability $training_period_fraction_ push in training set else in outsample set
      if ( grep {$_ eq $tradingdate_} @special_day_vec_ )
      {
        if ( rand( ) <= $training_period_fraction_ )
        {
          push ( @$training_date_vec_ref_, $tradingdate_ ); 
        }
        else
        {
          push ( @$outsample_date_vec_ref_, $tradingdate_ );
        }            
      }        
      $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
    if($#$training_date_vec_ref_ >= min(15, $t_num_days_/2))    #20 days for training are fine, if smaller span is given, then even less days are ok
    {
      if ( $training_period_fraction_ >= 1 )
      {
#degenrating this to every day being insample as well as outsample for comparision
        @$outsample_date_vec_ref_ = @$training_date_vec_ref_;
      }  
      return;
    }
    else
    {
      $main_log_file_handle_->print("can't find enough ".$training_days_type_." days. Training in normal days\n");
      print "very few( ".($#$training_date_vec_ref_ + 1)." )".$training_days_type_." days for training.\n";
      exit(0);
    }
  }

  @$training_date_vec_ref_ = ();
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
#with probability $training_period_fraction_ push in training set else in outsample set
    if ( rand( ) <= $training_period_fraction_ )
    {
      push ( @$training_date_vec_ref_, $tradingdate_ ); 
    }
    else
    {
      push ( @$outsample_date_vec_ref_, $tradingdate_ );
    }

    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
  }

  if ( $training_period_fraction_ >= 1 )     
  {        
#degenrating this to every day being insample as well as outsample for comparision
    @$outsample_date_vec_ref_ = @$training_date_vec_ref_;
  }
}


sub CalcPnls
{
  if ( $DEBUG ) { $main_log_file_handle_->print ( "CalcPnls\n" ); }

  my ( $indicator_weight_vec_vec_ref_, $stats_vec_ref_, $volume_vec_ref_, $ttc_vec_ref_, $sigma_vec_ref_, $iter_count_, $var_, $date_vec_ref_,
       $work_dir_, $strat_pre_text_, $strat_post_text_, $base_model_start_text_, $siglr_alpha_vec_, $indicator_list_, $base_short_code_, $use_fake_faster_data_, $sortalgo  ) = @_;
  my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
  my @date_vec_= ();

  my %models_list_map_ = ( );
  my $t_strat_filename_ = $work_dir_."/"."tmp_strat_".$iter_count_."_".$var_;
  my $t_models_list_ = $work_dir_."/"."tmp_models_list_".$iter_count_."_".$var_;
  my $local_results_base_dir = $work_dir_."/local_results_base_dir".$iter_count_."_".$var_;
  open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
  open OUTMODELLIST, "> $t_models_list_" or PrintStacktraceAndDie ( "Could not open output_models_list_filename_ $t_models_list_ for writing\n" );
  for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
  {
    my $t_model_filename_base_ = "tmp_model_".$iter_count_."_".$i."_".$var_;
    if($var_ eq "outofsample")
    {
      if($i==0)
      {
        $t_model_filename_base_ = "original_model"; #assuming 1st weight vec is original in outsample call
      }
      if($i==1)
      {
        $t_model_filename_base_ = "best_model"; #assuming 2nd weight vec is best in outsample call
      }
    }
    my $t_model_filename_ = $work_dir_."/".$t_model_filename_base_;
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
    print OUTMODELLIST $t_model_filename_base_."\n";
    $models_list_map_{$t_model_filename_base_}=$i;
  }
  close OUTSTRAT;
  close OUTMODELLIST;

  my @unique_sim_id_list_ = ( );
  my @independent_parallel_commands_ = ( );
  my @tradingdate_list_ = ( );
  my @temp_strategy_list_file_index_list_ = ( );
  my @temp_strategy_list_file_list_ = ( );
  my @temp_strategy_cat_file_list_ = ( );
  my @temp_strategy_output_file_list_ = ( );

  my @nonzero_tradingdate_vec_ = ( );
  my $start_date_ = "";
  my $end_date_ = "";

  for ( my $t_day_index_ = 0; $t_day_index_ <= $#$date_vec_ref_ ; $t_day_index_ ++ )
  {
    my $tradingdate_ = $$date_vec_ref_[$t_day_index_];
    push ( @tradingdate_list_ , $tradingdate_ );

    my $unique_sim_id_ = GetGlobalUniqueId ( );
    push ( @unique_sim_id_list_ , $unique_sim_id_ );

    my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$iter_count_."_".$tradingdate_."_".$var_;
    push ( @temp_strategy_output_file_list_ , $t_sim_res_filename_ );
    `> $t_sim_res_filename_`;

    my $this_strat_filename_ = $t_strat_filename_;
    if($training_days_type_ eq "ECO")
    {
#changing timings as per the event
      $this_strat_filename_ = $t_strat_filename_."_".$tradingdate_;
      my $start_time_ = $eco_date_time_map_{$tradingdate_}[0];
      my $end_time_ = $eco_date_time_map_{$tradingdate_}[1];
      `awk -vst=$start_time_ -vet=$end_time_ '{\$6=st; \$7=et; print \$_}' $t_strat_filename_ >  $this_strat_filename_`;
    }

    my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
    my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $this_strat_filename_ $unique_sim_id_ $tradingdate_ $market_model_index_ 0 0.0 $use_fake_faster_data_ ADD_DBG_CODE -1 > $t_sim_res_filename_ 2>&1";
    print "$exec_cmd_\n";
    push ( @independent_parallel_commands_ , $exec_cmd_ );
  }

  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; )
  {
    my @output_files_to_poll_this_run_ = ( );
    my @pids_to_poll_this_run_ = ( );
    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    print "core number = $THIS_MAX_CORES_TO_USE_IN_PARALLEL\n";
    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ )
    {
      my $t_sim_res_filename_ = $temp_strategy_output_file_list_ [ $command_index_ ];
      my $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ];

      push ( @output_files_to_poll_this_run_ , $t_sim_res_filename_ );

      print $main_log_file_handle_ $exec_cmd_."\n";

      my $pid_ = fork();
      die "unable to fork $!" unless defined($pid_);
      if ( !$pid_ )
      {
        #child process has pid 0
        exec($exec_cmd_);
      }

      #back to parent process
      print $main_log_file_handle_ "PID of last sim_strategy is $pid_\n";
      $command_index_ ++;
      sleep ( 1 );
    }

    my $t_pid_ = 9999;
    while ( $t_pid_ > 0 )
    { # there are still some sim-strats which haven't output SIMRESULT lines
      $t_pid_ = wait();
      print $main_log_file_handle_ "PID of completed process: $t_pid_\n";
    }
  }

  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
  {
#print "iter_count_ ".$iter_count_." command_index_ ".$command_index_."\n";

    my $t_sim_res_filename_ = $temp_strategy_output_file_list_ [ $command_index_ ];
    my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];
    my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];

    my @sim_strategy_output_lines_=();
    my %unique_id_to_pnlstats_map_ =();

    if ( ExistsWithSize ( $t_sim_res_filename_ ) )
    {	
      push (@nonzero_tradingdate_vec_, $tradingdate_);
      @sim_strategy_output_lines_=`cat $t_sim_res_filename_`;
    }
    my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_sim_id_);
    my $this_logfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_sim_id_);
#print "trades file name ".$this_tradesfilename_."\n";
    if ( ExistsWithSize ( $this_tradesfilename_ ) )
    {
      my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";
      print "This is $exec_cmd\n";
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
      `rm -f $this_tradesfilename_`;
      `rm -f $this_logfilename_`;
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
      if ($start_date_ eq ""){  $start_date_ = $tradingdate_; }
      if($tradingdate_ > $end_date_){ $end_date_ = $tradingdate_; }
      if($tradingdate_ < $start_date_){ $start_date_ = $tradingdate_; }
    }
  }
  my $statistics_result_file_ = $work_dir_."/"."stats_res_file_".$iter_count_."_".$var_;
  my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $sort_algo_ > $statistics_result_file_";
#print $main_log_file_handle_ "$exec_cmd\n";
  my $results_ = `$exec_cmd`;
  $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $sort_algo_ 0 INVALIDFILE 0";
  my $stats_string_ = `$exec_cmd`;

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
        $$sigma_vec_ref_[$index]=$t_stat_rwords_[3];
      }
    }
  }
  return $stats_string_;

}

