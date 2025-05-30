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

require "$GENPERLLIB_DIR/strat_utils.pl"; #GetStratProperties
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

my @sampledata_table_cols = ( "shcid", "date", "feature_id", "sampling_duration", "value" );
my @ShortCodes_table_cols = ( "shortcode" );
my @Feature_table_cols = ( "feature" );

my $num_sampledata_table_cols = $#sampledata_table_cols + 1;

my $sampledata_insert_query = "INSERT INTO SampleData ( ".join(',' , @sampledata_table_cols)." ) VALUES ( ".join(',' , ('?') x @sampledata_table_cols)." )";
my $feature_insert_query = "INSERT INTO Features ( ".join(',' , @Feature_table_cols)." ) VALUES ( ".join(',' , ('?') x @Feature_table_cols)." )";
my $shortcode_insert_query = "INSERT INTO ShortCodes ( ".join(',' , @ShortCodes_table_cols)." ) VALUES ( ".join(',' , ('?') x @ShortCodes_table_cols)." )";
my $sampledata_delete_query = "DELETE FROM SampleData WHERE shcid = ? AND date = ? AND feature_id = ? AND sampling_duration = ?";

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
    $db_config_file_ = $write_db_config_file_;
    Connect();
  }
}

sub GetIdFromName
{
  if ( ! defined $dbh ) { return -1 };
  my $name = shift;
  my $type = shift;
  my $read_query = "";
  if ( $type eq "shortcode") 
  {
    $read_query = "SELECT shcid FROM ShortCodes WHERE shortcode = ?";
  }
  elsif ( $type eq "strat") 
  {
    $read_query = "SELECT stratid FROM strats WHERE sname = ?";
  } 
  elsif ( $type eq "model" )
  {
    $read_query = "SELECT modelid FROM models WHERE modelfilename = ?";
  }
  elsif ( $type eq "param" )
  {
    $read_query = "SELECT paramid FROM params WHERE paramfilename = ?";
  }
  elsif ( $type eq "feature" )
  {
    $read_query = "SELECT feature_id FROM Features WHERE feature = ?";
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

sub Insert
{
  SetWriteDB();
  if ( ! defined $dbh ) { return -1 };
  my $arg = shift;
  my $type = shift;
  my $arg_id = 0;
  if ( $type eq "shortcode") 
  {
  	$arg_id = GetIdFromName($arg, "shortcode");	
  }
  elsif ( $type eq "feature" )
  {
  	$arg_id = GetIdFromName($arg, "feature");	
  }
   
  if ( $arg_id >= 0 ) 
  {
    return $arg_id;
  }
  
  my @value_vec = (); 
  push ( @value_vec, $arg );
  
  my $insert_stmt = "";
  if ( $type eq "shortcode") {
  	$insert_stmt = $dbh->prepare_cached($shortcode_insert_query);
  }
  elsif ( $type eq "feature" ) {
  	$insert_stmt = $dbh->prepare_cached($feature_insert_query);
  }
  
  unless ( $insert_stmt->execute(@value_vec) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
  
  if ( $type eq "shortcode") {
  	return  GetIdFromName($arg, "shortcode");
  }
  elsif ( $type eq "feature" ) {
  	return  GetIdFromName($arg, "feature");
  }
}

sub FetchSampleData
{
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $feature = shift;
  my $date = shift;
  my $sampling_duration = 300;
  
  if ( $#_ >= 0 )
  {
   	$sampling_duration = shift;
  }
  
  my @strats_vec_ref_ = ();
  
  my $shortcode_id = Insert ( $shortcode, "shortcode" );
  my $feature_id = Insert ( $feature, "feature" );
  
  my @results_vec_ref_ = (); 
  push ( @results_vec_ref_, $shortcode_id );
  push ( @results_vec_ref_, $date );
  push ( @results_vec_ref_, $feature_id );
  push ( @results_vec_ref_, $sampling_duration );
  
  my $read_query = "SELECT value FROM SampleData WHERE shcid = ? AND date = ? AND feature_id = ? AND sampling_duration = ?";

  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute(@results_vec_ref_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  
  while (my $result_ref_ = $statement->fetchrow_arrayref) 
  {
    push ( @strats_vec_ref_, $$result_ref_[0] );
    print "$$result_ref_[0] ;; ";
  }
  return ( $#strats_vec_ref_ + 1 );
}

#Takes three arguments ( stratname, date and a space-separated string of results )
#And inserts the result with the corresponding values, returns 1 if successful and 0 otherwise
sub InsertSampleData
{
  SetWriteDB();
  if ( ! defined $dbh ) { return 0 };
 
print " @_ \n"; 
  if ( @_ < 4 ) 
  {
  	print "Usage: <exec> shortcode feature date values [sample_duration] \n";
#  	return 0;
  }
  
  my $shortcode = shift;
  my $feature = shift;
  my $date = shift;
  my $values = shift ;
  my $delete_prev = "false";
  my $sampling_duration = 300;
  
  if ( $#_ >= 0 )
  {
   	$delete_prev = shift;
  }
  
  if ( $#_ >= 0 )
  {
   	$sampling_duration = shift;
  }
  
  my $shortcode_id = Insert ( $shortcode, "shortcode" );
  my $feature_id = Insert ( $feature, "feature" );

  my @results_vec_ref_ = (); 
  push ( @results_vec_ref_, $shortcode_id );
  push ( @results_vec_ref_, $date );
  push ( @results_vec_ref_, $feature_id );
  push ( @results_vec_ref_, $values ); 
  push ( @results_vec_ref_, $sampling_duration ); 

  if ( $delete_prev eq "true") {
  	
	my @del_vec_ref_ = (); 
	push ( @del_vec_ref_, $shortcode_id );
	push ( @del_vec_ref_, $date );
	push ( @del_vec_ref_, $feature_id ); 
	push ( @del_vec_ref_, $sampling_duration ); 
  
	my $del_stmt = $dbh->prepare_cached($sampledata_delete_query); 
  	unless ( $del_stmt->execute(@del_vec_ref_) ) 
  	{
    	print "SQL Error: $DBI::errstr\n";
    	return 0;
  	}
  }

  my $strat_stmt = $dbh->prepare_cached($sampledata_insert_query); 
  unless ( $strat_stmt->execute(@results_vec_ref_) ) 
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  return 1;
}

sub END
{
  DisConnect();
}

InsertSampleData("HHI_0", "feature1", 20160530, "value1", "true", 300);