use strict; 
use warnings;
use DBI; 
use File::Basename; # for basename and dirname
use List::Util qw( min max );

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
my $db_config_file_ = "/spare/local/files/DBCONFIG_lrdb";

my $is_writing_ = 0;
my $initialize_ = 0;

my $dbh ;    
Connect();

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

sub FetchExchanges
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $exchanges_ref_ = shift;

  my $read_query = "SELECT DISTINCT exchange FROM session_timings";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute() ) {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 0 ) {
      push ( @$exchanges_ref_, $$row_ref_[0] );
    }
  }
  $statement->finish;
}

sub GetExchangeSessions
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $exchange = shift;
  my $sessionid_to_start_end_ref_ = shift;
 
  my $read_query = "SELECT  session_id,start_time,end_time FROM session_timings WHERE exchange = ?";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($exchange) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 2 ) {
      push ( @$sessionid_to_start_end_ref_, [@$row_ref_] );
    }
  }
  $statement->finish;
}

sub GetDepsForExchange
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $exchange = shift;
  my $deps_ref_ = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "RetLRDB_Pair_Timings" : "LRDB_Pair_Timings";

  my $read_query = "SELECT DISTINCT dep FROM $dbtable_ INNER JOIN session_timings ON $dbtable_.session_id = session_timings.session_id WHERE exchange = ?";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($exchange) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 0 ) {
      push ( @$deps_ref_, @$row_ref_ );
    }
  }
  $statement->finish;
}

sub FetchLRDBPair
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $dep = shift;
  my $indep = shift;
  my $session_id = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "RetLRDB_Pair_Timings" : "LRDB_Pair_Timings";

  #@$results_vec_ref_ = ();

  my $read_query = "SELECT  start_time ,end_time FROM $dbtable_ WHERE dep = ? AND indep = ? AND session_id = ?";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($dep, $indep, $session_id) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  my $retarray_ref_ = $statement->fetchrow_arrayref;
  $statement->finish;
  if ( defined $retarray_ref_) { return @{$retarray_ref_}; }
  else { return; }
}

sub FetchAllLRDBPairs
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $pairs_ref_ = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "RetLRDB_Pair_Timings" : "LRDB_Pair_Timings";

  my $read_query = "SELECT dep, indep, session_id FROM $dbtable_";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute() )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 2 ) {
      push ( @$pairs_ref_, [ @$row_ref_ ] );
    }
  }
  $statement->finish;
}

sub FetchAllLRDBForPair
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $dep = shift;
  my $indep = shift;
  my $session_id = shift;
  my $values_ref_ = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "RetLRDB_Pair_Timings" : "LRDB_Pair_Timings";

  my $read_query = "SELECT beta, correlation, date_generated, dirty_bit FROM $dbtable_ WHERE dep = ? AND indep = ? AND session_id = ? ORDER BY date_generated";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($dep, $indep, $session_id) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 2 ) {
      push ( @$values_ref_, [ @$row_ref_ ] );
    }
  }
  $statement->finish;
}

sub FetchLRDBPairsForSession
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $sessionid_ = shift;
  my $lrdb_pairs_ref_ = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "RetLRDB_Pair_Timings" : "LRDB_Pair_Timings";

  my $read_query = "SELECT dep, indep, start_time ,end_time FROM $dbtable_ WHERE session_id = ?";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($sessionid_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 3 ) {
      push ( @$lrdb_pairs_ref_, [@$row_ref_] );
    }
  }
  $statement->finish;
}

sub InsertLRDBPair
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $dep = shift;
  my $indep = shift;
  my $session_id = shift;
  my $start_time = shift;
  my $end_time = shift;
  my $update_existing_ = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "RetLRDB_Pair_Timings" : "LRDB_Pair_Timings";

  my $lrdb_pair_insert_query_ = "INSERT IGNORE INTO $dbtable_ (dep,indep, start_time, end_time, session_id)
    VALUES (?,?,?,?,?)";
  my $statement = $dbh->prepare_cached($lrdb_pair_insert_query_);
  unless ( $statement->execute($dep, $indep, $start_time, $end_time, $session_id) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  $statement->finish;

  if ( $update_existing_ == 1 ) {
    my $lrdb_pair_update_query_ = "UPDATE $dbtable_ SET start_time=?, end_time=? WHERE dep=? AND indep=? AND session_id=?";
    $statement = $dbh->prepare_cached($lrdb_pair_update_query_);
    unless ( $statement->execute($start_time, $end_time, $dep, $indep, $session_id) )
    {
      print "SQL Error: $DBI::errstr\n";
      return;
    }
    $statement->finish;
  }
  return;
}

sub GetPreRow
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $dep_ = shift;
  my $indep_ = shift;
  my $sessionid_ = shift;
  my $date_ = shift;
  my $is_ret_lrdb_ = shift;

  my $dbtable_ = ($is_ret_lrdb_) ? "Ret_LRDB_dated" : "LRDB_dated";
  my $read_query = "select beta,correlation,date_generated from $dbtable_ where dep=? and indep=? and session_id=? and dirty_bit=0 and date_generated<? order by date_generated DESC limit 1;";

  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($dep_,$indep_,$sessionid_,$date_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  my $retarray_ref_ = $statement->fetchrow_arrayref;
  $statement->finish;
  if ( defined $retarray_ref_) { return @{$retarray_ref_}; }
  else { return; }
}

sub FetchLastDateForSession
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $sessionid_ = shift;
  my $curr_date_ = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "Ret_LRDB_dated" : "LRDB_dated";

  my $read_query = "SELECT DISTINCT date_generated FROM $dbtable_ WHERE session_id = ? AND date_generated <= ? order by date_generated DESC limit 1";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($sessionid_, $curr_date_) ) {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  my $retarray_ref_ = $statement->fetchrow_arrayref;
  $statement->finish;
  if ( defined $retarray_ref_ && $#$retarray_ref_ >= 0) { return $$retarray_ref_[0]; }
  else { return; }
}

sub FetchLRDBForPair
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $dep = shift;
  my $indep = shift;
  my $session_id = shift;
  my $date = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "Ret_LRDB_dated" : "LRDB_dated";

  my $read_query = "SELECT beta, correlation FROM $dbtable_ WHERE dep = ? AND indep = ? AND session_id = ? AND date_generated = ?";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($dep, $indep, $session_id, $date) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  my $retarray_ref_ = $statement->fetchrow_arrayref;
  $statement->finish;
  if ( defined $retarray_ref_) { return @{$retarray_ref_}; }
  else { return; }
}

sub FetchLRDBForPairwithoutdirtybit
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $dep = shift;
  my $indep = shift;
  my $session_id = shift;
  my $date = shift;
  my $is_ret_lrdb_ = shift;
  my $dbtable_ = ($is_ret_lrdb_) ? "Ret_LRDB_dated" : "LRDB_dated";

  my $read_query = "SELECT beta, correlation FROM $dbtable_ WHERE dep = ? AND indep = ? AND session_id = ? AND date_generated = ? and dirty_bit = 0";
  my $statement = $dbh->prepare_cached($read_query);
  unless ( $statement->execute($dep, $indep, $session_id, $date) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  my $retarray_ref_ = $statement->fetchrow_arrayref;
  $statement->finish;
  if ( defined $retarray_ref_) { return @{$retarray_ref_}; }
  else { return; }
}
sub InsertLRDBPairValue
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $dep = shift;
  my $indep = shift;
  my $session_id = shift;
  my $date = shift;
  my $beta = shift;
  my $correlation = shift;
  my $dirty_bit_ = shift;
  my $is_ret_lrdb_ = shift;
  my $update_existing_ = shift || 0;
  my $dbtable_ = ($is_ret_lrdb_) ? "Ret_LRDB_dated" : "LRDB_dated";

  my $lrdb_value_insert_query_ = "INSERT  IGNORE INTO $dbtable_ (dep,indep, session_id, date_generated, beta, correlation, dirty_bit)
    VALUES (?,?,?,?,?,?,?)";
  my $statement = $dbh->prepare_cached($lrdb_value_insert_query_);
  unless ( $statement->execute($dep, $indep, $session_id, $date, $beta, $correlation, $dirty_bit_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  $statement->finish;
  
  if ( $update_existing_ == 1 ) {
    my $lrdb_value_update_query_ = "UPDATE $dbtable_ SET beta=?, correlation=?, dirty_bit=? WHERE dep=? AND indep=? AND session_id=? AND date_generated=?";
    $statement = $dbh->prepare_cached($lrdb_value_update_query_);
    unless ( $statement->execute($beta, $correlation, $dirty_bit_, $dep, $indep, $session_id, $date) )
    {
      print "SQL Error: $DBI::errstr\n";
      return;
    }
  }
  return;
}

sub SetDirtyBit
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };
  my $dep = shift;
  my $indep = shift;
  my $session_id = shift;
  my $date = shift;
  my $dirty_bit_ = shift;
  my $is_ret_lrdb_ = shift || 0;
  my $dbtable_ = ($is_ret_lrdb_) ? "Ret_LRDB_dated" : "LRDB_dated";

  my $lrdb_value_update_query_ = "UPDATE $dbtable_ SET dirty_bit=? WHERE dep=? AND indep=? AND session_id=? AND date_generated=?";
  my $statement = $dbh->prepare_cached($lrdb_value_update_query_);
  unless ( $statement->execute($dirty_bit_, $dep, $indep, $session_id, $date) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  $statement->finish;
}

1
