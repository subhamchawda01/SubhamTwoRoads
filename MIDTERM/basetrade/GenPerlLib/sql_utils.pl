#!/usr/bin/perl
#
use strict; 
use warnings;
use DBI; 

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl";
require "$GENPERLLIB_DIR/parse_utils.pl";

# Inputs: DB_handler, query_string, query_arg_1, query_arg_2 ... query_arg_n
# Output: results_vec_ref_ - reference to output table(2D array)
# eg.
=cut 
sub GetAllShcVecRef
{
  my $res_vec_ref_ref_ = ExecuteReadSQLQuery( $dbh, "SELECT DISTINCT(shortcode) from strats" );
  @$res_vec_ref_ref_ = map { $$_[0] } @$res_vec_ref_ref_;
  return $res_vec_ref_ref_;
}
=cut
sub ExecuteReadSQLQuery
{
  my $dbh_ = shift;
  my $query_string_ = shift;
  my @args_vec_ ;

  while ( $#_ >= 0 )
  {
    push ( @args_vec_, $_[0] );
    shift;
  }

  my $result_vec_ref_;
  @$result_vec_ref_ = ();

  my $sth_ = $dbh_->prepare_cached($query_string_) or PrintStacktraceAndDie ( $DBI::errstr );
  $sth_->execute(@args_vec_) or PrintStacktraceAndDie ( $DBI::errstr );

  while ( my $row_ref_ = $sth_->fetchrow_arrayref )
  {
    push ( @$result_vec_ref_, [@$row_ref_] );
  }
    
  return $result_vec_ref_ ;
}

# Inputs: DB_handler, query_string, query_arg_1, query_arg_2 ... query_arg_n
# Output: updated_rows_  - number of rows updated
# eg.
=cut 
sub SetStratType
{
  my $stratname_ = shift; $stratname_ = basename($stratname_);
  my $newtype_ = shift;
  my $query_ = "UPDATE strats SET type = ? WHERE sname = ?";
  
  return ExecuteWriteSQLQuery( $dbh, $query_, $newtype_, $stratname_ );
}
=cut
sub ExecuteWriteSQLQuery
{
  my $dbh_ = shift;
  my $query_string_ = shift;
  my @args_vec_ = ();
  while ( $#_ >= 0 )
  {
    push ( @args_vec_, $_[0] );
    shift;
  }

  my $sth_ = $dbh_->prepare_cached($query_string_) or PrintStacktraceAndDie ( $DBI::errstr );
  my $updated_rows_ = $sth_->execute(@args_vec_) or PrintStacktraceAndDie ( $DBI::errstr );

  return int($updated_rows_) ;
}


# Inputs: db_connectionfile
# Outputs: DB_handler, status
sub ConnectToDB
{
  my $file_ = shift;
  my $dbh_;

  #parse config
  my $key_valvec_ref_ = ParseConfig($file_);

  #check sanity of config values
  PrintStacktraceAndDie ("No DBTYPE in Config: $file_") if ( ! exists ($$key_valvec_ref_{"DBTYPE"}) );
  my $dbtype_ = $$key_valvec_ref_{"DBTYPE"}[0];

  PrintStacktraceAndDie ("No DBNAME in Config: $file_") if ( ! exists ($$key_valvec_ref_{"DBNAME"}) );
  my $dbname_ = $$key_valvec_ref_{"DBNAME"}[0];

  PrintStacktraceAndDie ("No HOSTNAME in Config: $file_") if ( ! exists ($$key_valvec_ref_{"HOSTNAME"}) );
  my $host_ = $$key_valvec_ref_{"HOSTNAME"}[0];

  my $hostname_ = `hostname`; chomp($hostname_);
  if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
  { # AWS
    if ( exists ($$key_valvec_ref_{"EC2HOSTNAME"}) )
    {
      $host_ = $$key_valvec_ref_{"EC2HOSTNAME"}[0];
    }
  }
  
  my $user_ = exists ($$key_valvec_ref_{"USERNAME"}) ?  $$key_valvec_ref_{"USERNAME"}[0] : "";
  my $password_ = exists ($$key_valvec_ref_{"PASSWORD"}) ?  $$key_valvec_ref_{"PASSWORD"}[0] : "";

  #connect
  if ($host_ eq "NO") {
    $dbh_ = DBI->connect("DBI:$dbtype_:dbname=$dbname_", $user_, $password_) or PrintStacktraceAndDie("Can't connect to DB $dbtype_:$dbname_;$host_ , $user_");
  }
  else {
    if ( defined $$key_valvec_ref_{"PORT"} ) {
      my $port_ = $$key_valvec_ref_{"PORT"}[0];
      $dbh_ = DBI->connect("DBI:$dbtype_:$dbname_;host=$host_;port=$port_", $user_, $password_) or PrintStacktraceAndDie("Can't connect to DB $dbtype_:$dbname_;$host_;$port_ , $user_");
    }
    else {
      $dbh_ = DBI->connect("DBI:$dbtype_:$dbname_;host=$host_", $user_, $password_) or PrintStacktraceAndDie("Can't connect to DB $dbtype_:$dbname_;$host_ , $user_");
    }
  }
  $dbh_->{mysql_auto_reconnect} = 1;
  return $dbh_;
  
}

1
