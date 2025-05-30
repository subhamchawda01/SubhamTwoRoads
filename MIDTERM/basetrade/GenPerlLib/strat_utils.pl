use strict;
use warnings;
use List::Util qw /min max/;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";

require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; #CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; #GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/date_utils.pl"; # GetUTCTime
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

my @time_zones_ = ( "EST" , "CST", "CET", "BRT", "UTC", "KST", "HKT", "MSK", "IST", "JST", "BST", "AST", "PAR", "LON", "AMS"  );
my @filters_ = ("f0", "fst.5", "fst1", "fst2", "fsl2", "fsl3", "fsg.5", "fsg1", "fsg2", "fsr1_5", "fsr.5_3" , "flogit", "fv", "fsudm", "fsudm1", "fsudm2", "fsudm3" );
my @regressions_ = ( "L1_REGRESSION", "PCA_REGRESSION", "LASSO", "SLASSO", "SIGLR", "SIGLRF", "RANDOMFOREST", "CONSOLIDATEDBOOSTING", "PNLBOOSTING", "COARSEBOOSTING", "BOOSTING", "CONSOLIDATEDTREEBOOSTING", "COARSETREEBOOSTING", "TREEBOOSTING", "MLOGIT", "NEWMLOGIT", "EARTH", "FSLR", "FSHLR", "FSVLR", "FSHDVLR", "FSRR", "FSRLM", "FSRMFSS", "FSRMSH", "FSRSHRSQ", "MULTLR", "LR", "SOM", "LM" );
my @pred_algos_ = ( "na_t3_bd", "na_t1", "na_t3", "na_t5", "na_e1", "na_e3", "na_e5", "ac_e3", "ac_e5", "na_s4", "na_sa", "na_mult", "na_m4" );

sub CheckIfRegimeParam
{
    my $src_file_ = shift;
    my $num_regime_param_ = 0;
    my $num_regime_ind_ = 0;
    $num_regime_param_ = `awk '{if(\$1==\"PARAMFILELIST\"){print 1;}}' $src_file_ | wc -l`; chomp($num_regime_param_);
    $num_regime_ind_ = `awk '{if(\$1==\"INDICATOR\"){print 1;}}' $src_file_ | wc -l`; chomp($num_regime_ind_);
    return $num_regime_param_*$num_regime_ind_;
}

sub GetRegimeInd
{
    my $src_file_ = shift;
    my $regime_indicator_ = "NONE";
    if ( CheckIfRegimeParam ( $src_file_ ) != 0 )
    {
      $regime_indicator_ = `awk '{if(\$1==\"INDICATOR\"){print \$_;}}' $src_file_`; chomp($regime_indicator_);
    }
    return $regime_indicator_;
}


sub GetOrigName
{
  my $strat_ = shift;
  my $orig_strat_ = `basename $strat_`; chomp($orig_strat_);
  if (not -e $strat_)
  {
    return -1;
  }
  if(IsRollStrat($strat_))
  {
    my $roll_file_ = `awk '{print \$5;}' $strat_`; chomp($roll_file_);
    $roll_file_ = substr($roll_file_, 5);
    $orig_strat_ = `grep ORIGINAL $roll_file_ | awk '{print \$2;}'`; chomp($orig_strat_);
  }
  return $orig_strat_;
}

sub GetStrattype
{
  my $strat_ = shift;
  my $strattype_ = `cat $strat_ | head -n1 | awk '{print \$3}'`; chomp($strattype_);
  return $strattype_;
}

sub GetParam
{
  my $strat_ = shift;
  my $dt_ = "DEFAULT";
  if($#_>=0) {$dt_ = shift;}
  my $param_ = `cat $strat_ | head -n1 | awk '{print \$5}'`; chomp($param_);
  if(IsRollStrat($strat_))  {$param_ = GetRollParams($param_, $dt_);}
  return $param_;
}

sub GetModel
{
  my $strat_ = shift;
  my $model_ = `cat $strat_ | head -n1 | awk '{print \$4}'`; chomp($model_);
  return $model_;
}

sub IsRegimeStrat
{
  my $strat_ = shift;
  my $model_ = `cat $strat_ | head -n1 | awk '{print \$4}'`; chomp($model_);
  my $regime_ = `awk '{if(\$1=="REGIMEINDICATOR"){print 1;}}' $model_ | head -n1`; chomp($regime_);
  if ( $regime_ ) {return 1; }
  return 0;
}

sub GetRegression
{
  my $strat_ = shift;
  my $model_ = `cat $strat_ | head -n1 | awk '{print \$4}'`; chomp($model_);
  my $regression_ = `awk 'NR==2{print \$2;}' $model_`; chomp($regression_);
  return $regression_;
}

sub GetRollParams
{
  my $roll_paramfile_ = shift;
  my $dt_ = shift;
  #print "$roll_paramfile_ $dt_ \n";
  if(substr($roll_paramfile_, 0 ,5) eq "ROLL:") {$roll_paramfile_ = substr($roll_paramfile_, 5);}

  my $paramfile_ = `grep "DEFAULT DEFAULT" $roll_paramfile_ | head -n1 | awk '{print \$3}'`; chomp($paramfile_);
  if(not -e $paramfile_)  {PrintStacktraceAndDie("roll param $roll_paramfile_ don't have default param.\nPlease add <DEFAULT DEFAULT default_paramfile> on top of $roll_paramfile_\n");}

  if($dt_ eq "DEFAULT") {return $paramfile_;}
 
  $dt_ =  GetIsoDateFromStrMin1($dt_);
  open ROLLPARAM, "< $roll_paramfile_" or PrintStacktraceAndDie("Can't open file : $roll_paramfile_ for reading");
  while(my $line_ = <ROLLPARAM>)
  {
    my @words_ = split(" ", $line_);
    if(substr($words_[0],0,1) eq "#") {next;}
    if($#words_ >= 2)
    {
      if($words_[0] eq "DEFAULT" || $words_[1] eq "DEFAULT")  {next;}
      my $sd_ = GetIsoDateFromStrMin1($words_[0]);
      my $ed_ = GetIsoDateFromStrMin1($words_[1]);
      if($dt_ >= $sd_ && $dt_ <= $ed_)
      {
        #if(-e $words_[2])
        {
          $paramfile_ = $words_[2];
          last;
        }
      }
    }
  }
  close ROLLPARAM;
  return $paramfile_;
}

sub IsRollStrat
{
  my $strat_ = shift;
  my $param_ = `cat $strat_ | awk '{print \$5}'`; chomp($param_);
  if(substr($param_,0,5) eq "ROLL:")  {return 1;}
  return 0;
}

sub GetRollParamIfAllSame
{
  my $roll_paramfile_ = shift;
  if(substr($roll_paramfile_, 0 ,5) eq "ROLL:") {$roll_paramfile_ = substr($roll_paramfile_, 5);}

  my $last_param_ = "INVALIDFILE"; #will return this if only one roll param used
  open ROLLPARAM, "< $roll_paramfile_" or PrintStacktraceAndDie("Can't open file : $roll_paramfile_ for reading");
  while(my $line_ = <ROLLPARAM>)
  {
    my @words_ = split(" ", $line_);
    if(substr($words_[0],0,1) eq "#") {next;}
    if($#words_ >= 2)
    {
      if($words_[0] eq "DEFAULT" || $words_[1] eq "DEFAULT")  {next;}
      if($last_param_ eq "INVALIDFILE")
      {  
        $last_param_ = $words_[2];
      }
      elsif($words_[2] ne $last_param_)
      {
        close ROLLPARAM;
        return "MANYFILES";  #more than one roll params used
      }
    }
  }
  close ROLLPARAM;
  return $last_param_;
}

sub GetModelL1NormVec
{
  my $model_ = shift;
  my $sd_ = shift;
  my $ed_ = shift;
  my $st_ = shift;
  my $et_ = shift;
  my $t_vec_ref_ = shift;
  my $exec_cmd_ = "$MODELSCRIPTS_DIR/get_stdev_model.pl $model_ $sd_ $ed_ $st_ $et_ 2>/dev/null";
  my @output_lines_ = `$exec_cmd_`; chomp(@output_lines_);
  my $total_l1norm_ = -1;
  if ( $#output_lines_ >= 0 )
  {
    my @t_words_ = split(' ', $output_lines_[0]);
    $total_l1norm_ = $t_words_[1];
  }
  for ( my $i=1; $i<=$#output_lines_; $i++ )
  {
    my @t_words_ = split(' ', $output_lines_[$i]);
    if ( $t_words_[0] eq "REGIME")
    {
      push ( @$t_vec_ref_, $t_words_[3]);
    }
  }
  push ( @$t_vec_ref_, $total_l1norm_ ); #push complete l1norm in the end of vec
  return $t_vec_ref_;
}

sub GetTargetL1NormForShortcode
{
  my ( $t_shortcode_, $t_end_date_, $pool_tag_ ) = @_;
  my $target_stdev_filename_ = "/home/dvctrader/modelling/stratwork/".$t_shortcode_."/target_stdev_".$pool_tag_;
  open TSFILE, "< $target_stdev_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $target_stdev_filename_ for reading\n" );
  my $target_end_date_ = -1;
  my $target_start_date_ = -1;
  my $target_stdev_ = -1;
  my $current_end_date_ = -1;
  while ( my $tsline_ = <TSFILE> )
  {
    chomp($tsline_);
    my @ts_words_ = split(' ', $tsline_ );
    # Format of each row 
    # StartDate EndDate Stdev ModelFilePath            
    $current_end_date_ = $ts_words_[1];    
    #if ( $t_end_date_ > $current_end_date_ and ( $current_end_date_ > $target_end_date_ ) )
    {
      $target_end_date_ = $current_end_date_;
      $target_start_date_ = $ts_words_[0];
      $target_stdev_ = $ts_words_[2];
    }
  }
  return ($target_start_date_, $target_end_date_, $target_stdev_ );
}

sub AddL1NormToModel
{
  my $model_filename_ = shift;
  my $t_vec_ref_ = shift;

  #sanity check
  if ( $#$t_vec_ref_ < 0 ) { return ; }
  for ( my $i=0; $i<=$#$t_vec_ref_; $i++ )
  {
    if ( $$t_vec_ref_[$i] <= 0.0 )
    {
      return;
    }
  }

  my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;  
  my $temp_model_filename_ = "tmp_add_stdev_model_".$unique_gsm_id_;
  `rm -f $temp_model_filename_`;
  `cp $model_filename_ $temp_model_filename_`;

  my $l1_norm_line_ = "MODELINFO ".join(' ', @$t_vec_ref_);
  my $found_ = 0;
  open MFILE, "< $model_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $model_filename_ for reading\n" );
  open TMFILE, "> $temp_model_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $temp_model_filename_ for writing\n" );
  while ( my $mline_ = <MFILE> )
  {
    chomp($mline_);
    my @words_ = split(' ', $mline_);
    next if ( $#words_ < 0 || $words_[0] eq "MODELINFO" );
    if ( $words_[0] eq "MODELMATH" )
    {
      $found_ = 1;
      $mline_ = $mline_."\n$l1_norm_line_";
    }
    print TMFILE "$mline_\n";
  }
  if (not $found_)
  {
    print TMFILE "$l1_norm_line_\n";
  }
  close MFILE;
  close TMFILE;
  `mv $temp_model_filename_ $model_filename_`;
}


sub IsModelScalable
{
  my $model_filename_ = shift;
  my $modelmath_ = `awk '{if(\$1==\"MODELMATH\"){print \$2;}}' $model_filename_`; chomp($modelmath_);
  return ( $modelmath_ eq "LINEAR" || $modelmath_ eq "SIGLR" || $modelmath_ eq "SELECTIVENEW" || $modelmath_ eq "SELECTIVESIGLR" ) 
}

sub GetMinStratStartTime
{
  my $stratfile_ = shift;
  my $date_ = shift;

  my $min_st_timestamp_ = 0;
  my $min_st_ = 2359;

  open FILE , "< $stratfile_" or PrintStacktraceAndDie ( "could not open $stratfile_ for reading" );
  while ( my $stratline_ = <FILE> )
  {
    chomp($stratline_);
    my @words_ = split( ' ', $stratline_ );
    if ( $#words_ >= 6 && $words_[0] eq "STRATEGYLINE" )
    {
      my $t_st_ = $words_[5];
      my $t_st_timestamp_ = GetTimestampFromTZ_HHMM_DATE( $t_st_, $date_ );

      if ( $t_st_timestamp_ > 0 )
      {
        if ( $min_st_timestamp_ == 0 || $t_st_timestamp_ < $min_st_timestamp_ )
        {
          $min_st_ = $t_st_;
          $min_st_timestamp_ = $t_st_timestamp_;
        }
      }
    }
  } 
  close FILE;
  
  if ( $min_st_timestamp_ > 0 ) { return $min_st_ ; }
  else { return -1; }
}

sub IsStagedStrat
{
  my $stratbase_ = shift ;
  my $fullstrat_ = `ls $HOME_DIR/modelling/staged_strats/\*/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);                                              
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1); 
  }
  $fullstrat_ = `ls $HOME_DIR/modelling/staged_strats/\*/EBT/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1);
  }

  $fullstrat_ = `ls $HOME_DIR/modelling/NSEOptionsMM_staged/\*Strats/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1);
  }

  $fullstrat_ = `ls $HOME_DIR/modelling/wf_staged_strats/\*/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);                                              
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1); 
  }

  return (undef, 0);
}

sub IsPoolStrat
{
  my $stratbase_ = shift ;
  my $fullstrat_ = `ls $HOME_DIR/modelling/strats/\*/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);                                              
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1); 
  }
  $fullstrat_ = `ls $HOME_DIR/modelling/strats/\*/EBT/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1); 
  }

  $fullstrat_ = `ls $HOME_DIR/modelling/NSEOptionsMM/\*Strats/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1); 
  }

  $fullstrat_ = `ls $HOME_DIR/modelling/wf_strats/\*/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);                                              
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1); 
  }

  return (undef, 0);
}

sub IsStirStrat
{
  my $stratbase_ = shift ;
  my $fullstrat_ = `ls $HOME_DIR/modelling/stir_strats/*/$stratbase_ $HOME_DIR/modelling/stir_strats/*/*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_); 
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) { 
    return ($fullstrat_, 1); 
  }
  return (undef, 0);
}

sub IsPrunedStrat
{
  my $stratbase_ = shift ;
  my $fullstrat_ = `ls $HOME_DIR/modelling/pruned_strategies/\*/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);                                              
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1); 
  }
  $fullstrat_ = `ls $HOME_DIR/modelling/pruned_strategies/\*/EBT/\*/$stratbase_ 2>/dev/null | head -n1 ` ; chomp($fullstrat_);
  if ( $fullstrat_ && -f $fullstrat_ && -s $fullstrat_ ) {
    return ($fullstrat_, 1);
  }

  return (undef, 0);
}


sub GetCleanIndicatorString
{
  my $orig_ind_str_ = shift;
  my @orig_ind_words_ = split(' ', $orig_ind_str_);
  my $ret_str_ = "";
  if ( $#orig_ind_words_ < 2 ) { return $ret_str_; }
  else { $ret_str_ = $orig_ind_words_[2]; }  
  for ( my $i = 3; $i <= $#orig_ind_words_; $i++  )
  {
    last if ( index( $orig_ind_words_[$i] , "#" ) >= 0 ) ;
    $ret_str_ = $ret_str_." ".$orig_ind_words_[$i];
  }
  return $ret_str_;
}

=comment
Input : ( strat_basename )
Output : ( strat_fullname, strat_type )
strat_type : N - normal, S - staged, P - pruned, F - failed/StratNotFound
=cut

sub GetStratFullPathAndType
{
  my $strat_name_ = shift;
  my ($strat_path_, $is_valid_path_) = IsPoolStrat( $strat_name_ );
  if ( $is_valid_path_ && $strat_path_ ) {
    return ($strat_path_, "N");
  }

  #treating stir_strats as normal strats
  ($strat_path_, $is_valid_path_) = IsStirStrat( $strat_name_ );
  if ( $is_valid_path_ && $strat_path_ ) {
    return ($strat_path_, "N");
  }

  ($strat_path_, $is_valid_path_) = IsStagedStrat( $strat_name_ );
  if ( $is_valid_path_ && $strat_path_ ) {
    return ($strat_path_, "S");
  }

  ($strat_path_, $is_valid_path_) = IsPrunedStrat( $strat_name_ );
  if ( $is_valid_path_ && $strat_path_ ) {
    return ($strat_path_, "P");
  }

  return ("", "F");
}

=comment
Function: Computes the following properties of a strat.
(1) ShortCode
(2) Exec logic
(3) model 
(4) param
(5) start_time
(6) end_time
=cut
sub GetStratPropertiesVec
{
  my $strat_name_ = shift;
  my $ret_vec_ref_ = shift;

  my ( $strat_path_, $strat_type_ ) = GetStratFullPathAndType( $strat_name_ );
  if ( $strat_type_ eq "F" )
  {
    print "Error! No such strat: ".$strat_name_."\n";
    return 0;
  }

  $$ret_vec_ref_{"type"} = $strat_type_;

  my $exec_cmd_ = "cat $strat_path_"; 
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
  if ( $#exec_output_ < 0 ) 
  {
# for some reason, this strat is not complete
    print "Error! strat is empty. please look into it: ".$strat_path_."\n";
    return 0;
  }

  my @exec_out_words_ = split ( ' ' , $exec_output_ [ 0 ] );
  if ( $#exec_out_words_ < 6 ) 
  {
# for some reason, this strat is not complete
    print "Error! strat has less than 7 fields. please look into it: ".$strat_path_."\n";
    return 0;
  }

  $$ret_vec_ref_ { "shortcode" } = $exec_out_words_[1]; 
  $$ret_vec_ref_ { "execlogic" } = $exec_out_words_[2]; 
  $$ret_vec_ref_ { "modelfilename" } = $exec_out_words_[3]; 
  $$ret_vec_ref_ { "paramfilename" } = $exec_out_words_[4]; 
  $$ret_vec_ref_ { "start_time" } = $exec_out_words_[5]; 
  $$ret_vec_ref_ { "end_time" } = $exec_out_words_[6];

  my ( $start_desc_, $strat_approved_ ) = GetStratDBFields( $strat_name_ );
  $$ret_vec_ref_ { "description" } = $start_desc_;
  $$ret_vec_ref_ { "approved" } = $strat_approved_;
  
  return 1;
}

=comment
Function: Computes the following properties of a model.
(1) ShortCode
(2) ModelMath
(3) Regression
(4) Training Start Date
(5) Training End Date
(6) Training Start Time
(7) Training Start Time
(8) filter
(9) pred_dur
(10) pred_algo
(11) sample_timeouts
(12) stdev_or_l1norm
(13) change_or_return
=cut
sub GetModelPropertiesVec
{
  my $model_name_ = shift;
  my $ret_vec_ref_ = shift;
  if ( ! -s $model_name_ ) 
  {
    print "Error! No or 0-sized model: ".$model_name_."\n";
    return 0;
  }

  my $shc_ = `awk '{ if(\$1==\"MODELINIT\") {print \$3;} }' $model_name_`; chomp($shc_); 
  if ( $shc_ ne "" ) { $$ret_vec_ref_{ "shortcode" } = $shc_; }

  my $modelmath_ = `awk '{ if(\$1==\"MODELMATH\") {print \$2;} }' $model_name_`; chomp($modelmath_); 
  if ( $modelmath_ ne "" ) { $$ret_vec_ref_{ "modelmath" } = $modelmath_; }

  my $change_or_return_ = `awk '{ if(\$1==\"MODELMATH\") {print \$3;} }' $model_name_`; chomp($change_or_return_); 
  if ( $change_or_return_ ne "" ) { $$ret_vec_ref_{ "change_or_return" } = $change_or_return_; }
  
  my $l1norm_ = `awk '{ if(\$1==\"MODELINFO\") {print \$NF;} }' $model_name_`; chomp($l1norm_); 
  if ( $l1norm_ ne "" && $l1norm_ > 0 ) { $$ret_vec_ref_{ "stdev_or_l1norm" } = $l1norm_; }

  my $basename_ = `basename $model_name_`; chomp($basename_);

  foreach my $t_case_ ( @filters_ )
  {
    if ( $basename_ =~ m/.*[._]$t_case_[._].*/ )  
    { 
      $$ret_vec_ref_{ "filter" } = $t_case_; 
      last;
    }
  }

  foreach my $t_case_ ( @pred_algos_ )
  {

    if ( $basename_ =~ m/.*[._]$t_case_[._].*/ )  
    { 
      $$ret_vec_ref_{ "pred_algo" } = $t_case_; 
      last;
    }
  }

  foreach my $t_case_ ( @regressions_ )
  {
    if ( $basename_ =~ m/.*[._]$t_case_[._].*/ )  
    { 
      $$ret_vec_ref_{ "regression" } = $t_case_; 
      last;
    }
  }

  my ( $start_date_, $end_date_ ) = ( $basename_ =~ m/.*_(20[\d]{6})_(20[\d]{6})_.*/ );
  if ( ValidDate ( $start_date_ ) && ValidDate ( $end_date_ ) ) 
  {
    $$ret_vec_ref_{ "training_sd" } = $start_date_; 
    $$ret_vec_ref_{ "training_ed" } = $end_date_;

    my @words_ = split( "_", $basename_ );
    my $idx_ = -1;
    for ( my $i = 0; $i < $#words_ - 1 ; $i++ )
    {
      if ( $words_[$i] eq $start_date_ && $words_[$i + 1] eq $end_date_ )
      {
        $idx_ = $i;
        last;
      }
    }

    my $pd_idx_ = $idx_ - 3;
    if ( exists($$ret_vec_ref_{"pred_algo"}) )
    {
      my @temp_ = split( "_", $$ret_vec_ref_{"pred_algo"} );
      $pd_idx_ = $idx_ - max ( 3, $#temp_ + 2 );
    }
    if ( $pd_idx_ > 0 && ($words_[$pd_idx_] =~ m/^\d*\.?\d*$/ ) )
    {
      $$ret_vec_ref_{"pred_dur"} = $words_[$pd_idx_];
    }
    $idx_++;

    if ( $idx_ + 2 <= $#words_ )
    {
      if ( IsTimeZone ( $words_[$idx_ + 1] ) && ($words_[$idx_ + 2] =~ m/^\d+$/) &&  int ( $words_[$idx_ + 2] ) > 0 && int ( $words_[$idx_ + 2] ) < 2400 )
      {
        $$ret_vec_ref_{"training_st"} = $words_[$idx_ + 1]."_".$words_[$idx_ + 2];
        $idx_ += 2;
      }
      elsif ( ($words_[$idx_ + 1] =~ m/^\d+$/) && int ( $words_[$idx_ + 1] ) > 0 && int ( $words_[$idx_ + 1] ) < 2400 )
      {
        $$ret_vec_ref_{"training_st"} = $words_[$idx_ + 1];
        $idx_ += 1;
      }
    } 

    if ( $idx_ + 2 <= $#words_ )
    {
      if ( IsTimeZone ( $words_[$idx_ + 1] ) && ($words_[$idx_ + 2] =~ m/^\d+$/) && int ( $words_[$idx_ + 2] ) > 0 && int ( $words_[$idx_ + 2] ) < 2400 )
      {
        $$ret_vec_ref_{"training_et"} = $words_[$idx_ + 1]."_".$words_[$idx_ + 2];
        $idx_ += 2;
      }
      elsif ( ($words_[$idx_ + 1] =~ m/^\d+$/) && int ( $words_[$idx_ + 1] ) > 0 && int ( $words_[$idx_ + 1] ) < 2400 )
      {
        $$ret_vec_ref_{"training_et"} = $words_[$idx_ + 1];
        $idx_ += 1;
      }
    } 

    if (  $idx_ + 4 <= $#words_ )
    {
      $$ret_vec_ref_{"sample_timeouts"} = join(" ", @words_[($idx_ + 1) .. ($idx_ + 4)]);
    }
  }
  
  return 1;
}

sub IsTimeZone
{
  my $tz_str_ = shift;
  if ( grep { $_ eq $tz_str_ } @time_zones_  ) { return 1; }
  return 0;
}
