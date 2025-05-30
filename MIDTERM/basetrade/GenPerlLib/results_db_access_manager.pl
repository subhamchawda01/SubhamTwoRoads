#!/usr/bin/perl

use strict; 
use warnings;
use DBI; 
use File::Basename; # for basename and dirname

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $WF_SCRIPTS_DIR=$HOME_DIR."/".$REPO."/walkforward";

require "$GENPERLLIB_DIR/strat_utils.pl"; #GetStratProperties
require "$GENPERLLIB_DIR/config_utils.pl"; #GetConfigId
require "$GENPERLLIB_DIR/array_ops.pl"; #FirstIndexOfElementFromArrayRef
require "$GENPERLLIB_DIR/sql_utils.pl"; #ConnectToDB
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

#We might want to access a local DB just for reading while central DB for writing
#Thus support for different config files for ReadDB and WriteDB
my $read_db_config_file_ = "/spare/local/files/DBCONFIG_results";
my $write_db_config_file_ = "/spare/local/files/DBCONFIG_results";


my $is_writing_ = 0;
my $initialize_ = 0;
my $db_config_file_ = $read_db_config_file_;

my $dbh ;    
Connect(); #should we move this inside fetch functions ?

#DB SCHEMA INFO, we expect that InsertResults will recieve pnl_stats in the same order as results_table_stats_cols
#Add an entry in results_table_stats_cols, when any extra stat is added to the results
my @results_table_stats_cols = ( "pnl" ,"vol" ,"supp_per" ,"best_per" ,"agg_per" ,"imp_per" ,"apos" ,"median_ttc" ,"avg_ttc" ,"med_closed_trd_pnl" ,"avg_closed_trd_pnl" ,"std_closed_trd_pnl" ,"sharpe_closed_trade_pnls_" ,"fracpos_closed_trd_pnl" ,"min_pnl" ,"max_pnl" ,"drawdown" ,"max_ttc" ,"msg_count" ,"vol_norm_avg_ttc" ,"otl_hits" ,"abs_open_pos" ,"uts" ,"ptrds" ,"ttrds" );
my @results_table_extra_cols = ( "date" ,"stratid" ,"pnl_samples", "regenerate" );

my @results_table_cols = ( @results_table_stats_cols, @results_table_extra_cols );

my @wf_results_table_extra_cols = ( "date" ,"configid" ,"pnl_samples", "regenerate" );
my @wf_results_table_cols = ( @results_table_stats_cols, @wf_results_table_extra_cols );

#not including stratid, modelid, paramid columns in following arrays because it is not needed and passing a garbage value to auto_increment field could be disastrous as well
#TODO: find a better field trype for id assignment which don't have overflow problem like auto-increment
my @strats_table_cols = ( "sname", "shortcode" ,"execlogic" ,"modelid" ,"paramid" ,"start_time" ,"end_time", "type"  );
my @models_table_cols = ( "modelfilename" , "shortcode", "modelmath", "regression", "training_sd" ,"training_ed" ,"training_st" ,"training_et" ,"filter" ,"pred_dur" ,"pred_algo" ,"sample_timeouts" ,"stdev_or_l1norm", "change_or_return" );
my @params_table_cols = ( "paramfilename" );

my $num_results_table_stats_cols = $#results_table_stats_cols + 1;

my $result_insert_query = "INSERT INTO results ( ".join(',' , @results_table_cols)." ) VALUES ( ".join(',' , ('?') x @results_table_cols)." )";
my $wf_result_insert_query = "INSERT INTO wf_results ( ".join(',' , @wf_results_table_cols)." ) VALUES ( ".join(',' , ('?') x @wf_results_table_cols)." )";

my $strat_insert_query = "INSERT INTO strats ( ".join(',' , @strats_table_cols)." ) VALUES ( ".join(',' , ('?') x @strats_table_cols)." )";
my $model_insert_query = "INSERT INTO models ( ".join(',' , @models_table_cols)." ) VALUES ( ".join(',' , ('?') x @models_table_cols)." )";
my $param_insert_query = "INSERT INTO params ( ".join(',' , @params_table_cols)." ) VALUES ( ".join(',' , ('?') x @params_table_cols)." )";
  
my $result_read_query = "SELECT sname, date, ".join( ',', @results_table_stats_cols)." FROM strats, results WHERE strats.stratid = results.stratid AND shortcode = ? AND date = ? ";
my $wf_result_read_query = "SELECT cname, date, ".join( ',', @results_table_stats_cols)." FROM wf_configs, wf_results WHERE wf_configs.configid = wf_results.configid AND shortcode = ? AND date = ? "; 

my $dummy_model_name_ = "DUMMY_MODEL";
my $dummy_param_name_ = "DUMMY_PARAM";
#for cacheing these ids
my $dummy_model_id_ = -1;
my $dummy_param_id_ = -1;

sub SetBacktest
{
  if (defined $ENV{'USE_BACKTEST'}) {
    $read_db_config_file_ = "/spare/local/files/DBCONFIG_backtest_results";
    $write_db_config_file_ = "/spare/local/files/DBCONFIG_backtest_results";
    $db_config_file_ = $read_db_config_file_;
  }
}

sub Connect
{
  DisConnect();
  $dbh = ConnectToDB($db_config_file_);
}

sub DisConnect
{
  if ( defined $dbh ) { $dbh->disconnect; }
}

sub SetWriteDB
{
  if ( ! $is_writing_ )
  {
    $is_writing_ = 1;
    SetBacktest();
    $db_config_file_ = $write_db_config_file_;
    Connect();
  }
}

sub CheckConnection
{
  if (! $dbh->ping) {
    $dbh->clone() or die "Stale connection! Unable to re-connect!";
  }
}

sub ReturnDBH
{
  return $dbh;
}

#returns reference to a vec containing all shc in DB strats table
sub GetAllShcVecRef
{
  my @shc_vec_ = ();
  CheckConnection();
  if ( ! defined $dbh ) { return \@shc_vec_ };

  my $read_query_ = "SELECT DISTINCT(shortcode) from strats";
  my $sth_ = $dbh->prepare_cached($read_query_);
  unless ( $sth_->execute() )
  {
    print "SQL Error: $DBI::errstr\n";
    return \@shc_vec_;
  }

  while ( my $row_ref_ = $sth_->fetchrow_arrayref )
  {
    push ( @shc_vec_, $$row_ref_[0] );
  }

  $sth_->finish;
  return \@shc_vec_;
}

#Takes 3 arguments ( shortcode date  and ref to return_vec_ )
#Creates a vector of vector for the corresponding results in the passed_reference, returns the no of results fetched
sub FetchResults
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $date = shift;
  my $results_vec_ref_ = shift;
  @$results_vec_ref_ = ();
  #by default return for all strats without pnl_samples
  #this can be out to a wise use like returning only fresh results , only for pool strats etc.
  my $return_mode_ = "A"; 
  if ( $#_ >= 0 )
  {
    $return_mode_ = shift;
  }

  my $fetch_from_wf_db_ = 0;
  if ($#_ >= 0){
  	$fetch_from_wf_db_ = shift;
  }
  
  
  my $local_read_query = $result_read_query;
  if ($fetch_from_wf_db_ > 0 ){
  	#fetch from wf_results table
  	$local_read_query = $wf_result_read_query;
  }
  #by default , everything except pruned
  
  my $read_query = $local_read_query." AND type != 'P'"; 
  if ( $return_mode_ eq "N" )
  {
    #only pool
    $read_query = $local_read_query." AND type = 'N'"; 
  }
  elsif ( $return_mode_ eq "S" )
  {
    #only staged
    $read_query = $local_read_query." AND type = 'S'"; 
  }
  elsif ( $return_mode_ eq "E" )
  {
    #every thing in DB including pruned
    $read_query = $local_read_query; 
  }


  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute($shortcode, $date) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_row_ref_ = $statement->fetchrow_arrayref) 
  {
    my $null_pos_ = FirstIndexOfElementFromArrayRef ( $result_row_ref_, undef ); 
    if ( $null_pos_ >= 0 ) 
    {
      #remove any trailing missing stats 
      splice @$result_row_ref_, $null_pos_;
    }

    push ( @$results_vec_ref_, [@$result_row_ref_] );
  }
  return ($#$results_vec_ref_ + 1 ) ;
}

#Takes 3 arguments ( shortcode date  and ref to return_vec_ )
#Creates a vector of vector for the corresponding pnl_samples in the passed_reference and return no of results fetched
sub FetchPnlSamples
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $date = shift;
  my $results_vec_ref_ = shift;
  my $volume_map_ref_ = shift;
  my $fetch_from_wf_db_ = 0;
  if ($#_ >= 0){
  	$fetch_from_wf_db_ = shift;
  }
  
  @$results_vec_ref_ = ();

  my $read_query = "SELECT strats.sname, results.pnl_samples, vol FROM strats, results WHERE strats.stratid = results.stratid AND shortcode = ? AND date = ?";
  if ($fetch_from_wf_db_ > 0 ){
  	$read_query =  "SELECT wf_configs.cname, wf_results.pnl_samples, vol FROM wf_configs, wf_results WHERE wf_configs.configid = wf_results.configid AND shortcode = ? AND date = ?";
  } 
  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute($shortcode, $date) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_row_ref_ = $statement->fetchrow_arrayref) 
  {
    my $sname = $$result_row_ref_[0];
    my $t_pnl_sample_str_ = $$result_row_ref_[1];
    next if ( ! defined $t_pnl_sample_str_ ) ;

    my @t_pnl_sample_vec_ = split ( ' ', $t_pnl_sample_str_ );
    next if ( $#t_pnl_sample_vec_ < 1 );

    unshift @t_pnl_sample_vec_, $sname;
    push(@$results_vec_ref_, \@t_pnl_sample_vec_ );

    if ( defined $volume_map_ref_ ) {
      $$volume_map_ref_{ $sname } = $$result_row_ref_[2];
    }
  }
  $statement->finish;
  return ( $#$results_vec_ref_ + 1 );
}

#Takes 4 arguments ( shortcode date strat and ref to return_vec_ )
#Creates a vector of vector for the corresponding pnl_samples in the passed_reference and return no of results fetched
sub FetchPnlSamplesSingleStratQuery
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $date = shift;
  my $strat_ = shift;
  my $results_vec_ref_ = shift;
  my $volume_ref_ = shift;
  my $use_wf_db_ = shift;
  @$results_vec_ref_ = ();
  
  my $read_query = "";
  my $configid = 0;
  if ( $use_wf_db_ == -1) {
  	$configid = GetConfigId($strat_);
  }
  if ( $use_wf_db_ == 1 || $configid > 1 ){
  	$read_query = "SELECT wf_results.pnl_samples, vol FROM wf_configs, wf_results WHERE wf_results.configid = wf_configs.configid AND wf_configs.cname = ? AND shortcode = ? AND date = ?";  	
  }
  else{
  	$read_query = "SELECT results.pnl_samples, vol FROM strats, results WHERE results.stratid = strats.stratid AND strats.sname = ? AND shortcode = ? AND date = ?";
  } 
  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute($strat_, $shortcode, $date) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_row_ref_ = $statement->fetchrow_arrayref) 
  {
    my $t_pnl_sample_str_ = $$result_row_ref_[0];
    next if ( ! defined $t_pnl_sample_str_ ) ;

    @$results_vec_ref_ = split ( ' ', $t_pnl_sample_str_ );

    if ( defined $volume_ref_ ) {
      $$volume_ref_ = $$result_row_ref_[1];
    }
    last;
  }
  $statement->finish;
  return;
}

# ARGS: stratname, date, a space-separated string of results [, pnlsamples]
# Inserts the result with the corresponding values, returns 1 if successful and 0 otherwise
sub InsertResults
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $stratname = shift;
  my $date = shift;
  my $results_vec_ref_ = shift;

  my $pnl_samples_string_ ;
  if ( $#_ >= 0 ) {
    my $t_pnl_samples_string_ = shift;
    if ( defined $t_pnl_samples_string_ ) {
      my @t_words_ = split(" ", $t_pnl_samples_string_);
      if ( $#t_words_ >=  1 ) {
        $pnl_samples_string_ = $t_pnl_samples_string_;
      }
    }
  }

  my $shc_ = "INVALID";
  $shc_ = shift if $#_ >= 0;

  if ( $#$results_vec_ref_ < 1 ) { 
    print "Error! Either Pnl and Vol not specified or Vol <=0. Can't Insert.\n";
    return 0;
  }

  if ( ! IsValidConfig($stratname) ) {
    print "Error! Not a valid config $stratname\n";
    return 0;
  }

  if ( $#$results_vec_ref_ + 1 > $num_results_table_stats_cols ) { splice @$results_vec_ref_, $num_results_table_stats_cols; }
  if ( $#$results_vec_ref_ + 1 < $num_results_table_stats_cols ) {
    push ( @$results_vec_ref_, ( 0 ) x ($num_results_table_stats_cols - $#$results_vec_ref_ - 1) );
#print "Warning! All arguments not provided for $stratname, $date to InsertResults. Provided ".($#$results_vec_ref_ + 1).", required $num_results_table_stats_cols. Filling NULL for others\n";
  }

  my $configid = GetConfigId($stratname);

  #Filling fields in results_table_extra_cols 
  push ( @$results_vec_ref_, $date );
  push ( @$results_vec_ref_, $configid );
  push ( @$results_vec_ref_, $pnl_samples_string_ ); 

  if ( $$results_vec_ref_[0] == 0 && $$results_vec_ref_[1] == 0) {
    push ( @$results_vec_ref_, "Y" ); #dont regenerate, these are fresh results
  }
  else {
    push ( @$results_vec_ref_, "N" ); #dont regenerate, these are fresh results
  }

  #Remove results if already exists for the given strat
  my $delete_query = "DELETE FROM wf_results WHERE configid = ? AND date = ?"; 
  my $strat_stmt = $dbh->prepare_cached($delete_query); 
  unless ( $strat_stmt->execute($configid, $date) ) {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  $strat_stmt = $dbh->prepare_cached($wf_result_insert_query); 
  unless ( $strat_stmt->execute(@$results_vec_ref_) ) {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  return 1;
}

sub GetLastUpdate
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };
  my $shortcode = shift;
  my $date = shift;
  my $type = shift;
  my $fetch_from_wf_db_ = 0;
  if ($#_ >= 0){
  	$fetch_from_wf_db_ = shift;
  }
  
  my $last_update_query = "select max(last_update) from results inner join strats on results.stratid=strats.stratid and shortcode=? and date=? and type=?" ;
  if ( $fetch_from_wf_db_ > 0){
  	$last_update_query = "select max(last_update) from wf_results inner join wf_configs on wf_results.configid = wf_configs.configid and shortcode=? and date=? and type=?" ;
  }
  my $statement = $dbh->prepare_cached($last_update_query);
  unless ( $statement->execute($shortcode, $date, $type ) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;
    return $$row_ref_[0];
  }
}


#Takes stratname
#Inserts the strat if not already there 
#Returns stratid or -1 if fails
sub InsertStrat
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };
  my $stratname = shift;
  my $shc_ = "INVALID";
  if ( $#_ >= 0 )
  {
    $shc_ = shift;
  }

#check if strat already exists 
  my $strat_id = GetIdFromName($stratname, "strat"); 
  if ( $strat_id >= 0 ) 
  {
    return $strat_id;
  }

#insert strat
  my %strat_prop_vec = (); 
  my $modelid = -1;
  my $paramid = -1;

  if ( $shc_ ne "INVALID" )
  {
    #inserting with a dummy param/model 
    #giving a valid shc implies we don't want to parse this strat - helpful for stir strats etc.

    $modelid = GetDummyModelId ();
    if ( $modelid < 0 ) { return -1; } 
    else { $strat_prop_vec{"modelfilename"} = $dummy_model_name_; }

    $paramid = GetDummyParamId ();
    if ( $paramid < 0 ) { return -1; } 
    else { $strat_prop_vec{"paramfilename"} = $dummy_param_name_; }

    $strat_prop_vec{"shortcode"} = $shc_ ;
    $strat_prop_vec{"type"} = 'N'; 
  } 
  elsif ( GetStratPropertiesVec($stratname, \%strat_prop_vec) > 0 ) 
  { 
    #strat properly parsed
    $modelid = InsertModel ( $strat_prop_vec{"modelfilename"} );
    if ( $modelid < 0 ) 
    { 
      #try dummy model, we don't want to waste the result computation
      $modelid = GetDummyModelId ();
      if ( $modelid < 0 ) { return -1; } 
      else { $strat_prop_vec{"modelfilename"} = $dummy_model_name_; }
    }

    $paramid = InsertParam ( $strat_prop_vec{"paramfilename"} );
    if ( $paramid < 0 ) 
    { 
      #try dummy param, we don't want to waste the result computation
      $paramid = GetDummyParamId ();
      if ( $paramid < 0 ) { return -1; } 
      else { $strat_prop_vec{"paramfilename"} = $dummy_param_name_; }
    }
  }
  else
  {
    #invalid shc and strat can't be parsed
    return -1;
  }
  
  $strat_prop_vec{"sname"} = $stratname;
  $strat_prop_vec{"modelid"} = $modelid;
  $strat_prop_vec{"paramid"} = $paramid;

  #properties not in map, will be undef in value_vec and NULL in DB
  my @value_vec = map { $strat_prop_vec{$_} } @strats_table_cols;

  my $insert_stmt = $dbh->prepare_cached($strat_insert_query);   
  unless ( $insert_stmt->execute(@value_vec) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
  return GetIdFromName($stratname, "strat");
}

#Takes modelname
#Inserts the model if not already there 
#Returns modelid or -1 if fails
sub InsertModel
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };
  my $modelname = shift;

#check if model already exists 
  my $modelid = GetIdFromName($modelname, "model"); 
  if ( $modelid >= 0 ) 
  {
    return $modelid;
  }

#insert model
  my %model_prop_vec = (); 
  if ( GetModelPropertiesVec($modelname, \%model_prop_vec) <= 0 )
  {
    return -1;
  }

  $model_prop_vec{"modelfilename"} = $modelname;

  #properties not in map, will be undef in value_vec and NULL in DB
  my @value_vec = map { $model_prop_vec{$_} } @models_table_cols;

  my $insert_stmt = $dbh->prepare_cached($model_insert_query);   
  unless ( $insert_stmt->execute(@value_vec) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
  return GetIdFromName($modelname, "model");
}

#Takes modelname
#Inserts the model if not already there 
#Returns modelid or -1 if fails
sub InsertParam
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };
  my $paramname = shift;

#check if model already exists 
  my $paramid = GetIdFromName($paramname, "param"); 
  if ( $paramid >= 0 ) 
  {
    return $paramid;
  }

#insert model
  my %param_prop_vec = ();
  $param_prop_vec{"paramfilename"} = $paramname;

  #properties not in map, will be undef in value_vec and NULL in DB
  my @value_vec = map { $param_prop_vec{$_} } @params_table_cols;

  my $insert_stmt = $dbh->prepare_cached($param_insert_query);   
  unless ( $insert_stmt->execute(@value_vec) ) 
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
  return GetIdFromName($paramname, "param");
}

sub GetIdFromName
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };
  my $name = shift;
  my $type = shift;
  my $read_query = "";
  if ( $type eq "strat") 
  {
    $read_query = "SELECT stratid FROM strats WHERE sname = ?";
  }
  elsif ( $type eq "wf_config") {
    $read_query = "SELECT configid FROM wf_configs WHERE cname = ? ";
  } 
  elsif ( $type eq "model" )
  {
    $read_query = "SELECT modelid FROM models WHERE modelfilename = ?";
  }
  elsif ( $type eq "param" )
  {
    $read_query = "SELECT paramid FROM params WHERE paramfilename = ?";
  }
  else
  {
    return -1;
  }

  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute($name) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref) 
  {
    $statement->finish;
    return $$row_ref_[0];
  }
  return -1;
}


sub FetchStratsWithResults
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $date = shift;
  my $strats_vec_ref_ = shift;
  @$strats_vec_ref_ = ();
  #this can be out to a wise use like returning only fresh results etc., curruntly returing all
  my $return_mode_ = "A"; 
  if ( $#_ >= 0 )
  {
    $return_mode_ = shift;
  }

  my $fetch_from_wf_db_ = 0;
  if ( $#_ >= 0 ){
  	$fetch_from_wf_db_ = shift;
  }
  #print "FetchStratsWithResults: $shortcode, $date $strats_vec_ref_ $return_mode_ $fetch_from_wf_db_ \n";
  my $read_query = "SELECT strats.sname FROM strats, results WHERE strats.stratid = results.stratid AND shortcode = ? AND date = ?";
  if ( $fetch_from_wf_db_ > 0){
  	$read_query = "SELECT wf_configs.cname FROM wf_configs, wf_results WHERE wf_configs.configid = wf_results.configid AND shortcode = ? AND date = ?";
  	if ( $return_mode_ eq "NP" )
  	{
    	#return only strats with new results with pnl_samples, for which recomputation is not needed
    	$read_query = "SELECT wf_configs.cname FROM wf_configs, wf_results WHERE wf_configs.configid = wf_results.configid AND shortcode = ? AND date = ? AND regenerate = 'N' AND pnl_samples IS NOT NULL"; 
  	}
  	elsif ( $return_mode_ eq "N" )
  	{
    	#return only strats with new results with/without pnl_samples, for which recomputation is not needed
    	$read_query = "SELECT wf_configs.cname FROM wf_configs , wf_results WHERE wf_configs.configid= wf_results.configid AND shortcode = ? AND date = ? AND regenerate = 'N'"; 
  	} 
  	
  }else{
  	 
  	if ( $return_mode_ eq "NP" )
  	{
    	#return only strats with new results with pnl_samples, for which recomputation is not needed
    	$read_query = "SELECT strats.sname FROM strats, results WHERE strats.stratid = results.stratid AND shortcode = ? AND date = ? AND regenerate = 'N' AND pnl_samples IS NOT NULL"; 
  	}
  	elsif ( $return_mode_ eq "N" )
  	{
    	#return only strats with new results with/without pnl_samples, for which recomputation is not needed
    	$read_query = "SELECT strats.sname FROM strats, results WHERE strats.stratid = results.stratid AND shortcode = ? AND date = ? AND regenerate = 'N'"; 
  	} 
  }
  #print "FetchCommand: $read_query\n";
  
  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute($shortcode, $date) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_ref_ = $statement->fetchrow_arrayref) 
  {
    push ( @$strats_vec_ref_, $$result_ref_[0] );
  }
  return ($#$strats_vec_ref_ + 1 ) ;
}

sub GetSimulaApprovedStrats
{
  my $shortcode_ = shift;
  my $strats_vec_ref_ = shift;
  my $use_wf_results_ = 0;
  if ( $#_ >= 0){
  	$use_wf_results_ = shift;
  }
  
  my $read_query = "SELECT strats.sname FROM strats WHERE shortcode = ? AND simula_approved = true";
  if ( $use_wf_results_ > 0 ){
  	$read_query = "SELECT wf_configs.cname FROM wf_configs WHERE shortcode = ?";
  }
  
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($shortcode_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_ref_ = $statement->fetchrow_arrayref)
  {
    $$strats_vec_ref_ {$$result_ref_[0]} = 1;
  }
  return 1;
}

sub GetDummyModelId
{
  if ( ! defined $dbh ) { return -1 };

  if ( $dummy_model_id_ >= 0 ) { return $dummy_model_id_; } 
  
#check if model already exists 
  my $modelid = GetIdFromName($dummy_model_name_, "model"); 
  if ( $modelid >= 0 ) 
  {
    $dummy_model_id_ = $modelid;
    return $modelid;
  }

  SetWriteDB();

#insert model
  my %model_prop_vec = (); 
  $model_prop_vec{"modelfilename"} = $dummy_model_name_;

  #properties not in map, will be undef in value_vec and NULL in DB
  my @value_vec = map { $model_prop_vec{$_} } @models_table_cols;

  my $insert_stmt = $dbh->prepare_cached($model_insert_query);   
  unless ( $insert_stmt->execute(@value_vec) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
  $dummy_model_id_ = GetIdFromName($dummy_model_name_, "model");
  return $dummy_model_id_;
}

sub GetDummyParamId
{
  if ( ! defined $dbh ) { return -1 };

  if ( $dummy_param_id_ >= 0 ) { return $dummy_param_id_; } 

#check if param already exists 
  my $paramid = GetIdFromName($dummy_param_name_, "param"); 
  if ( $paramid >= 0 ) 
  {
    $dummy_param_id_ = $paramid;
    return $paramid;
  }

  SetWriteDB();

#insert param
  my %param_prop_vec = (); 
  $param_prop_vec{"paramfilename"} = $dummy_param_name_;

  #properties not in map, will be undef in value_vec and NULL in DB
  my @value_vec = map { $param_prop_vec{$_} } @params_table_cols;

  my $insert_stmt = $dbh->prepare_cached($param_insert_query);   
  unless ( $insert_stmt->execute(@value_vec) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }

  $dummy_param_id_ = GetIdFromName($dummy_param_name_, "param"); 
  return $dummy_param_id_;
}

sub SetStratType
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  my $stratname_ = shift; $stratname_ = basename($stratname_);
  my $newtype_ = shift;
  my $query_ = "UPDATE strats SET type = ? WHERE sname = ?";
  
  return ExecuteWriteSQLQuery( $dbh, $query_, $newtype_, $stratname_ );
}

sub SetConfigType
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  my $stratname_ = shift; $stratname_ = basename($stratname_);
  my $newtype_ = shift;
  my $query_ = "UPDATE wf_configs SET type = ? WHERE cname = ?";
  
  return ExecuteWriteSQLQuery( $dbh, $query_, $newtype_, $stratname_ );
}



sub SetStratDescription
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  my $stratname_ = shift; $stratname_ = basename($stratname_);
  my $description_ = shift;
  my $query_ = "UPDATE strats SET description = ? WHERE sname = ?";

  return ExecuteWriteSQLQuery( $dbh, $query_, $description_, $stratname_ );
}

#Simula-approval of strats
sub ApprovePoolConfig
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  my $stratname_ = shift; $stratname_ = basename($stratname_);
  my $query_ = "UPDATE wf_configs SET simula_approved = true WHERE cname = ?";

  return ExecuteWriteSQLQuery( $dbh, $query_, $stratname_ );
}


sub GetStratDBFields
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 0 )
  {
    print "Usage: GetStratDBFields() sname";
    return (undef, undef);
  }
  my $sname_ = shift;

  my $strat_fetch_query_ = "SELECT description, simula_approved from strats where sname = ?  ";
  my $statement = $dbh->prepare_cached($strat_fetch_query_);
  unless ( $statement->execute($sname_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return (undef, undef);
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;
    if ( defined $row_ref_ && defined $$row_ref_[1] ) {
      return ($$row_ref_[0], $$row_ref_[1]);
    }
  }
  return (undef,undef);
}

sub GetPickstratConfigNameForId
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 1 )
  {
    print "Usage: GetPickstratConfigId() configid";
    return undef;
  }
  my $configid = shift;

  my $configname_fetch_query_ = "SELECT config_name FROM PickstratConfig WHERE config_id = ? ";
  my $statement = $dbh->prepare_cached($configname_fetch_query_);
  unless ( $statement->execute($configid) )
  {
    print "SQL Error: $DBI::errstr\n";
    return undef;
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;
    if ( defined $row_ref_ && defined $$row_ref_[0] ) {
      return $$row_ref_[0];
    }
  }
  return undef;
}

sub GetPickstratConfigIdWithMatchingQueryIds
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 1 )
  {
    print "Usage: GetPickstratConfigId() query_ids_to_check [config_ids_to_exclude]";
    return -1;
  }
  my $qid_ref_ = shift;
  my $exclude_configid_ = shift;

  my $cid_qids_query_ = "SELECT config_id, start_queryid, end_queryid FROM PickstratConfig";
  my $statement = $dbh->prepare_cached($cid_qids_query_);
  unless ( $statement->execute() )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }

  my $matching_configid_ = 0;
  while (my $row_ref_ = $statement->fetchrow_arrayref) {
    next if defined $exclude_configid_ and $$row_ref_[0] eq $exclude_configid_;

    if (grep {$_ >= $$row_ref_[1] && $_ <= $$row_ref_[2]} @$qid_ref_) {
      $matching_configid_ = $$row_ref_[0];
      last;
    }
  }
  $statement->finish;

  return $matching_configid_;
}

sub GetPickstratConfigId
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 1 )
  {
    print "Usage: GetPickstratConfigId() configname";
    return (undef,undef,undef);
  }
  my $configname_ = shift;

  my $configid_fetch_query_ = "SELECT config_id, start_queryid, end_queryid FROM PickstratConfig WHERE config_name = ? ";
  my $statement = $dbh->prepare_cached($configid_fetch_query_);
  unless ( $statement->execute($configname_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return (undef,undef,undef);
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;
    if ( defined $row_ref_ && defined $$row_ref_[0] ) {
      return ($$row_ref_[0], $$row_ref_[1], $$row_ref_[2]);
    }
  }
  return (undef,undef,undef);
}

sub GetActivePickstratConfigs
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 2 )
  {
    print "Usage: GetActivePickstratConfigs() last_date name_to_details_map";
  }
  my $last_date_ = shift;
  my $configname_to_details_ref_ = shift;

  my $configid_fetch_query_ = "SELECT config_name, PickstratConfig.config_id, MAX(date), start_queryid, end_queryid FROM PickstratConfig 
    INNER JOIN PickstratRecords ON PickstratRecords.config_id = PickstratConfig.config_id WHERE date >= ? GROUP BY PickstratConfig.config_id";

  my $statement = $dbh->prepare_cached($configid_fetch_query_);
  unless ( $statement->execute($last_date_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( defined $row_ref_ && $#$row_ref_ >= 2 ) {
      $$configname_to_details_ref_{ $$row_ref_[0] } = [ @$row_ref_[1..$#$row_ref_] ];
    }
  }
  $statement->finish;
}

sub InsertSimRealPnlsForMRT
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };
  my $qid_ = shift;
  my $date_ = shift;
  my $sname_ = shift;
  my $real_pnl_ = shift;
  my $simreal_sim_pnl_ = shift;
  my $sim_pnl_ = shift;
  my $update_existing_ = shift || 0;

  my $insert_query_ = "INSERT INTO SimRealPnlsMRT (query_id, date, sname, real_pnl, simreal_sim_pnl, sim_pnl) VALUES (?,?,?,?,?,?)";

  if ( $update_existing_ == 1 ) {
    $insert_query_ .= "ON DUPLICATE KEY UPDATE sname=VALUES(sname), real_pnl=VALUES(real_pnl), simreal_sim_pnl=VALUES(simreal_sim_pnl), sim_pnl=VALUES(sim_pnl)";
  }

  my $statement = $dbh->prepare_cached($insert_query_);
  unless ( $statement->execute($qid_, $date_, $sname_, $real_pnl_, $simreal_sim_pnl_, $sim_pnl_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
}

sub GetLastPickstratRecord
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 2 )
  {
    print "Usage: GetLastPickstratRecord() config_id last_date";
  }
  my $configid_ = shift;
  my $last_date_ = shift;

  my $record_fetch_query_ = "SELECT date, time, num_queries, global_maxloss, sum_maxlosses, computed_oml FROM PickstratRecords
    WHERE config_id = ? AND date >= ? ORDER BY date DESC, time DESC LIMIT 1";

  my $statement = $dbh->prepare_cached($record_fetch_query_);
  unless ( $statement->execute($configid_, $last_date_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;
    if ( defined $row_ref_ && $#$row_ref_ >= 4 ) {
      return @$row_ref_;
    }
  }
  return;
}

sub InsertPickstratConfigId
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 2 )
  {
    print "Usage: GetPickstratConfigId() configname start_queryid [end_queryid]";
    return;
  }
  my ($configname_, $start_id_, $end_id_) = @_;

  my $config_insert_query_ = "INSERT INTO PickstratConfig (config_name, start_queryid, end_queryid) VALUES (?,?,?) ON DUPLICATE KEY UPDATE 
    start_queryid=VALUES(start_queryid), end_queryid=VALUES(end_queryid)";
  
  my $statement = $dbh->prepare_cached($config_insert_query_);
  unless ( $statement->execute($configname_, $start_id_, $end_id_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }

  my ($config_id_) = GetPickstratConfigId ( $configname_ );
  return $config_id_;
}

sub InsertPickstratRecord
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  my ($config_id_, $date_, $time_, $nqueries_, $global_maxloss_, $sum_maxlosses_, $computed_maxloss_, $query_configids_,
      $uts_inorder_, $pickstrats_logic_) = @_;

  my $ps_record_query_ = "INSERT INTO PickstratRecords (config_id, date, time, num_queries, global_maxloss, sum_maxlosses,
   computed_oml, query_config_ids, uts_in_order, pick_strats_logic) VALUES (?,?,?,?,?,?,?,?,?,?)";

  my $statement = $dbh->prepare_cached($ps_record_query_);
  unless ( $statement->execute($config_id_, $date_, $time_, $nqueries_, $global_maxloss_, $sum_maxlosses_, $computed_maxloss_,
      $query_configids_, $uts_inorder_, $pickstrats_logic_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  return;
}

sub FetchRealPnlsForDate
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 2 )
  {
    print "Usage: FetchRealPnlsForDate() date shc_session_to_pnl_map_ref shc_session_to_vol_map_ref";
    return;
  }
  my ($date_, $shc_sess_to_pnl_ref_, $shc_sess_to_vol_ref_) = @_;

  my $pnlfetchquery_ = "SELECT shortcode, session, pnl, volume FROM RealPNLS WHERE date = ?";

  my $statement = $dbh->prepare_cached($pnlfetchquery_);
  unless ( $statement->execute($date_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( defined $row_ref_ && $#$row_ref_ >= 1 ) {
      my $shc_tp_ = $$row_ref_[0]." ".$$row_ref_[1];
      $$shc_sess_to_pnl_ref_{ $shc_tp_ } = $$row_ref_[2];
      $$shc_sess_to_vol_ref_{ $shc_tp_ } = $$row_ref_[3];
    }
  }
  $statement->finish;
}

sub AddPnlsToDB
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 2 )
  {
    print "Usage: AddPnlsToDB() date session shc_to_pnl_map_ shc_to_vol_map_ [update_existing]";
    return;
  }
  my ($date_, $session_, $shc_to_pnl_ref_, $shc_to_vol_ref_, $update_) = @_;
  $update_ = 0 if ! defined $update_;

  my $insertpnls_ = "INSERT INTO RealPNLS (shortcode, session, date, pnl, volume) VALUES (?,?,?,?,?)";

  if ( $update_ ) { $insertpnls_ .= " ON DUPLICATE KEY UPDATE pnl=VALUES(pnl), volume=VALUES(volume)"; }

  foreach my $shc_ (keys %$shc_to_pnl_ref_ ) {
    my $statement = $dbh->prepare_cached($insertpnls_);
    my $this_pnl_;
    $this_pnl_ = int( 0.5 + $$shc_to_pnl_ref_{$shc_} ) if defined $$shc_to_pnl_ref_{$shc_};
    unless ( $statement->execute($shc_, $session_, $date_, $this_pnl_, $$shc_to_vol_ref_{$shc_} ) )
    {
      print "SQL Error: $DBI::errstr\n";
      next;
    }
    $statement->finish;
  }
}

sub SanitizeStratType
{
  my $_strat_ = shift;
  my ( $full_path_ , $t_strat_type_ ) = GetStratFullPathAndType($_strat_);
  if ( $t_strat_type_ eq "F" )
  {
    return -1;
  }
  return SetStratType( $_strat_, $t_strat_type_ );
}

sub DeleteStratFromDB
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  my $_strat_ = shift; $_strat_ = basename($_strat_);
  my $num_res_removed_ = ExecuteWriteSQLQuery( $dbh, "DELETE results FROM results JOIN strats ON results.stratid = strats.stratid AND sname = ?", $_strat_ );
  $num_res_removed_ += ExecuteWriteSQLQuery( $dbh, "DELETE FROM strats WHERE sname = ?", $_strat_ );

  return $num_res_removed_;
}

sub GetConfigTypeApproved
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 0 )
  {
    print "Usage: GetStratDBFields() cname";
    return (undef, undef);
  }
  my $sname_ = shift;

  my $strat_fetch_query_ = "SELECT type, simula_approved from wf_configs where cname = ?  ";
  my $statement = $dbh->prepare_cached($strat_fetch_query_);
  unless ( $statement->execute($sname_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return (undef, undef);
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;
    if ( defined $row_ref_ && defined $$row_ref_[1] ) {
      return ($$row_ref_[0], $$row_ref_[1]);
    }
  }
  return (undef,undef);
}

sub GetTstampFromCname
{
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  if ( @_ < 0 )
  {
    print "Usage: GetTstampFromCname() cname";
    return;
  }
  my $cname_ = shift;

  my $strat_fetch_query_ = "SELECT tstamp from wf_configs where cname = ?  ";
  my $statement = $dbh->prepare_cached($strat_fetch_query_);
  unless ( $statement->execute($cname_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return (undef, undef);
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;
    if ( defined $row_ref_ && defined $$row_ref_[0] ) {
      return $$row_ref_[0];
    }
  }
  return;
}

sub ExecuteWriteQueryOnResultsDB
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return -1 };

  return ExecuteWriteSQLQuery( $dbh, @_ );
}

sub ExecuteReadQueryOnResultsDB
{
  return ExecuteReadSQLQuery( $dbh, @_ );
}

sub END
{
  DisConnect();
}

1
