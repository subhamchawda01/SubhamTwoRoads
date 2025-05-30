#!/usr/bin/perl

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
#Thus support for different config files for ReadDB and WriteDB
my $read_db_config_file_ = "/spare/local/files/DBCONFIG_indstats";
my $write_db_config_file_ = "/spare/local/files/DBCONFIG_indstats";

my $is_writing_ = 0;
my $initialize_ = 0;
my $db_config_file_ = $read_db_config_file_;

my $dbh ;    
ConnectIndStats(); #should we move this inside fetch functions ?

my @GenInd_table_cols = ( "shc_id", "tp_id", "date", "pred_dur", "predalgo_id", "filter_id", "basepx_id", "futpx_id", "indicator_id", "correlation", "tail_correlation" );

my $num_genind_table_cols = $#GenInd_table_cols + 1;

my $genind_insert_query = "INSERT INTO IndicatorStats ( ".join(',' , @GenInd_table_cols)." ) VALUES ( ".join(',' , ('?') x @GenInd_table_cols)." )";
my $genind_insert_update_query = $genind_insert_query." ON DUPLICATE KEY UPDATE correlation=VALUES(correlation), tail_correlation=VALUES(tail_correlation)";

my $shortcode_insert_query = "INSERT INTO Shortcodes ( shortcode ) VALUES ( ? )";
my $timeperiod_insert_query = "INSERT INTO Timeperiod ( timeperiod ) VALUES ( ? )";
my $preddur_insert_query = "INSERT INTO PredDur ( preddur, shc_id ) VALUES ( ?, ? )";
my $predalgo_insert_query = "INSERT INTO Predalgo ( predalgo ) VALUES ( ? )";
my $filter_insert_query = "INSERT INTO Filter ( filter ) VALUES ( ? )";
my $pricetype_insert_query = "INSERT INTO Pricetype ( pricetype ) VALUES ( ? )";
my $indicator_insert_query = "INSERT INTO Indicator ( indicator ) VALUES ( ? )";

my $genindstats_delete_query = "DELETE FROM IndicatorStats WHERE shc_id = ? AND date = ? AND pred_dur = ? AND predalgo_id = ? AND 
filter_id = ? AND basepx_id = ? AND futpx_id = ? AND indicator_id = ?";

my %pred_algo_cache_ = ();
my %filter_cache_ = ();
my %pricetype_cache_ = ();
my %indicator_cache_ = ();

sub ConnectIndStats
{
  DisConnectIndStats();
  $dbh = ConnectToDB($db_config_file_); 
}

sub DisConnectIndStats
{
  if ( defined $dbh ) { $dbh->disconnect; }
}

sub SetWriteDBIndStats
{
  if ( ! $is_writing_ )
  {
    $is_writing_ = 1;
    $db_config_file_ = $write_db_config_file_;
    ConnectIndStats();
  }
}

sub GetIdFromNameIndStats
{
  if ( ! defined $dbh ) { return -1 };
  my $name = shift;
  my $type = shift;
  my $read_query = "";

  my @value_vec = ();
  push(@value_vec, $name);

  if ( $type eq "shortcode") 
  {
    $read_query = "SELECT shc_id FROM Shortcodes WHERE shortcode = ?";
  }
  elsif ( $type eq "timeperiod") 
  {
    $read_query = "SELECT tp_id FROM Timeperiod WHERE timeperiod = ?";
  }
  elsif ( $type eq "predalgo") 
  {
    $read_query = "SELECT predalgo_id FROM Predalgo WHERE predalgo = ?";
  }
  elsif ( $type eq "filter") 
  {
    $read_query = "SELECT filter_id FROM Filter WHERE filter = ?";
  }
  elsif ( $type eq "pricetype") 
  {
    $read_query = "SELECT pricetype_id FROM Pricetype WHERE pricetype = ?";
  }
  elsif ( $type eq "indicator") 
  {
    $read_query = "SELECT indicator_id FROM Indicator WHERE indicator = ?";
  }
  elsif ( $type eq "preddur")
  {
    my $shc = shift;
    $read_query = "SELECT preddur FROM PredDur WHERE preddur = ? and shc_id = ?";
    push(@value_vec, $shc);
  }
  else
  {
    return -1;
  }

  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute(@value_vec) )
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

sub GetNameFromIdIndStats
{
  if ( ! defined $dbh ) { return -1 };
  my $id = shift;
  my $type = shift;
  my $read_query = "";
  if ( $type eq "shortcode") 
  {
    $read_query = "SELECT shortcode FROM Shortcodes WHERE shc_id = ?";
  }
  elsif ( $type eq "timeperiod") 
  {
    $read_query = "SELECT timeperiod FROM Timeperiod WHERE tp_id = ?";
  }
  elsif ( $type eq "predalgo") 
  {
    if ( defined($pred_algo_cache_{$id}) ) {
      return $pred_algo_cache_{$id};
    }
    $read_query = "SELECT predalgo FROM Predalgo WHERE predalgo_id = ?";
  }
  elsif ( $type eq "filter") 
  {
    if ( defined($filter_cache_{$id}) ) {
      return $filter_cache_{$id};
    }
    $read_query = "SELECT filter FROM Filter WHERE filter_id = ?";
  }
  elsif ( $type eq "pricetype") 
  {
    if ( exists ($pricetype_cache_{$id}) ) {
      return $pricetype_cache_{$id};
    }
    $read_query = "SELECT pricetype FROM Pricetype WHERE pricetype_id = ?";
  }
  elsif ( $type eq "indicator") 
  {
    if ( defined($indicator_cache_{$id}) ) {
      return $indicator_cache_{$id};
    }
    $read_query = "SELECT indicator FROM Indicator WHERE indicator_id = ?";
  }
  else
  {
    return -1;
  }

  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute($id) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
	my $res = "";
  if (my $row_ref_ = $statement->fetchrow_arrayref) 
  {
    $statement->finish;
    $res = $$row_ref_[0];
  }
  if ( $type eq "predalgo")
  {
    $pred_algo_cache_{$id} = $res ;
  }
  elsif ( $type eq "filter") {
    $filter_cache_{$id} = $res ;
  }
  elsif ( $type eq "pricetype") {
    $pricetype_cache_{$id} = $res;
  }
  elsif ( $type eq "indicator") {
    $indicator_cache_{$id} = $res;
  }
  return $res;
  return -1;
}

sub Insert
{
  SetWriteDBIndStats();
  if ( ! defined $dbh ) { return -1 };
  my $arg = shift;
  my $type = shift;
  my $shc_id = 0 ;
  my $arg_id = 0;

  if( $type ne "preddur") {
	  $arg_id = GetIdFromNameIndStats($arg, $type);
	}
	else {
		$shc_id = shift;
		$arg_id = GetIdFromNameIndStats($arg, $type, $shc_id);
	}
  if ( $arg_id >= 0 ) 
  {
    return $arg_id;
  }
  
  my @value_vec = ( $arg );
  
  my $insert_stmt = "";
  if ( $type eq "shortcode") {
  	$insert_stmt = $dbh->prepare_cached($shortcode_insert_query);
  }
  if ( $type eq "timeperiod") {
  	$insert_stmt = $dbh->prepare_cached($timeperiod_insert_query);
  }
  elsif ( $type eq "preddur" ) {
  	$insert_stmt = $dbh->prepare_cached($preddur_insert_query);
  	push( @value_vec, $shc_id);
  }
  elsif ( $type eq "predalgo" ) {
  	$insert_stmt = $dbh->prepare_cached($predalgo_insert_query);
  }
  elsif ( $type eq "filter" ) {
  	$insert_stmt = $dbh->prepare_cached($filter_insert_query);
  }
  elsif ( $type eq "pricetype" ) {
  	$insert_stmt = $dbh->prepare_cached($pricetype_insert_query);
  }
  elsif ( $type eq "indicator" ) {
  	$insert_stmt = $dbh->prepare_cached($indicator_insert_query);
  }
  
  unless ( $insert_stmt->execute(@value_vec) )
  {
    print "SQL Error: $DBI::errstr\n";
    return -1;
  }
 
  return GetIdFromNameIndStats($arg, $type);
}

sub FetchIndStats
{
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $timeperiod = shift;
  my $date = shift;
  my $pred_dur = shift ;
  my $predalgo = shift ;
  my $filter = shift ;
  my $base_px = shift ;
  my $fut_px = shift ;
  my $indicator = shift;
  
  my $shortcode_id = GetIdFromNameIndStats ( $shortcode, "shortcode" );
  my $tp_id = GetIdFromNameIndStats ( $timeperiod, "timeperiod" );
  my $predalgo_id = GetIdFromNameIndStats ( $predalgo, "predalgo" );
  my $filter_id = GetIdFromNameIndStats ( $filter, "filter" );
  my $base_px_id = GetIdFromNameIndStats ( $base_px, "pricetype" );
  my $fut_px_id = GetIdFromNameIndStats ( $fut_px, "pricetype" );
  my $indicator_id = GetIdFromNameIndStats ( $indicator, "indicator" );

  if ( $shortcode_id < 0 || $tp_id < 0 || $predalgo_id < 0 || $filter_id < 0 || 
      $base_px_id < 0 || $fut_px_id < 0 || $indicator_id < 0 ) {
    return;
  }

  my $combined_key = $shortcode_id.",".$tp_id.",".$pred_dur.",".$predalgo_id.",".$filter_id.",".$base_px_id.",".$fut_px_id;
  my $read_query = "SELECT correlation, tail_correlation FROM IndicatorStats 
    WHERE combined_key = ? AND date = ? AND indicator_id = ?";

  my @results_vec_ref_ = ( $combined_key,$date,$indicator_id);
  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute(@results_vec_ref_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_ref_ = $statement->fetchrow_arrayref) 
  {
    my ($corr_, $tailed_corr_) = @$result_ref_[0..1];
    $statement->finish;
    return ($corr_, $tailed_corr_);
  }
  return;
}

sub FetchMultipleIndStats
{
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $timeperiod = shift;
  my $date = shift;
  my $pred_dur = shift ;
  my $predalgo = shift ;
  my $filter = shift ;
  my $base_px = shift ;
  my $fut_px = shift ;
  my $indc_to_corr_ref_ = shift;
  my $indc_to_tailed_corr_ref_ = shift;
  
  my $shortcode_id = GetIdFromNameIndStats ( $shortcode, "shortcode" );
  my $tp_id = GetIdFromNameIndStats ( $timeperiod, "timeperiod" );
  my $predalgo_id = GetIdFromNameIndStats ( $predalgo, "predalgo" );
  my $filter_id = GetIdFromNameIndStats ( $filter, "filter" );
  my $base_px_id = GetIdFromNameIndStats ( $base_px, "pricetype" );
  my $fut_px_id = GetIdFromNameIndStats ( $fut_px, "pricetype" );

  if ( $shortcode_id < 0 || $predalgo_id < 0 || $filter_id < 0 || 
      $base_px_id < 0 || $fut_px_id < 0 ) {
    return;
  }
  

  

  my $combined_key = $shortcode_id.",".$tp_id.",".$pred_dur.",".$predalgo_id.",".$filter_id.",".$base_px_id.",".$fut_px_id;
  my $read_query = "SELECT indicator, correlation, tail_correlation FROM IndicatorStats 
    INNER JOIN Indicator ON IndicatorStats.indicator_id = Indicator.indicator_id 
    WHERE combined_key = ? AND date = ?";

  my @results_vec_ref_ = ( $combined_key,$date);
  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute(@results_vec_ref_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_ref_ = $statement->fetchrow_arrayref) 
  {
    if ( $#$result_ref_ >= 2 ) {
      my ($indicator_, $corr_, $tailed_corr_) = @$result_ref_;
      $$indc_to_corr_ref_{ $indicator_ } = $corr_;
      $$indc_to_tailed_corr_ref_{ $indicator_ } = $tailed_corr_;
    }
  }
  $statement->finish;
}

sub FetchIndCorrsForShc
{
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $timeperiod = shift;
  my $start_date_ = shift;
  my $end_date_ = shift;
  my $predalgo = shift;
  my $preddur_ref_vec_ = shift;
  my $filter_ref_vec_ = shift;
  my $base_px = shift ;
  my $fut_px = shift ;
  my $date_to_indc_to_preddur_filter_to_corr_ref_ = shift;
  my $date_to_indc_to_preddur_filter_to_tailed_corr_ref_ = shift;

  my $shortcode_id = GetIdFromNameIndStats ( $shortcode, "shortcode" );
  my $tp_id = GetIdFromNameIndStats ( $timeperiod, "timeperiod" );
  my $predalgo_id = GetIdFromNameIndStats ( $predalgo, "predalgo" );
  my $base_px_id = GetIdFromNameIndStats ( $base_px, "pricetype" );
  my $fut_px_id = GetIdFromNameIndStats ( $fut_px, "pricetype" );



  my %id_to_filter_ = ( );
  my @filter_vec_ = ();
  my @all_keys = ();
  my @tmp_keys = ();
  foreach my $filter_ ( @$filter_ref_vec_ ) {
    my $filter_id = GetIdFromNameIndStats ( $filter_, "filter" );
    if ( $filter_id >= 0 ) {
      $id_to_filter_ { $filter_id } = $filter_;
      push( @filter_vec_, $filter_id );
    }
  }

  if ( $shortcode_id < 0 || $predalgo_id < 0 || ! %id_to_filter_ || 
      $base_px_id < 0 || $fut_px_id < 0 ) {
    return;
  }
  
  my @results_vec_ref_ = ();

  foreach my $tmp ( @$preddur_ref_vec_ ) { push ( @tmp_keys, $shortcode_id.",".$tp_id.",".$tmp.",".$predalgo_id ); }
  foreach my $tmp ( @tmp_keys ) { push ( @all_keys, map { "$tmp,$_,$base_px_id,$fut_px_id" } @filter_vec_ ); }
  
  my $read_query = "SELECT date, pred_dur, filter_id, indicator, correlation, tail_correlation FROM IndicatorStats  
    INNER JOIN Indicator ON IndicatorStats.indicator_id = Indicator.indicator_id  
    WHERE date >= ? AND date <= ? ";

  $read_query .= " AND combined_key in (".join(',', ('?') x @all_keys).")" ;


  push ( @results_vec_ref_, $start_date_ );
  push ( @results_vec_ref_, $end_date_ );
  push ( @results_vec_ref_, @all_keys );
  
  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute(@results_vec_ref_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_ref_ = $statement->fetchrow_arrayref) 
  {
    if ( $#$result_ref_ >= 5 ) {
      my ($date_, $preddur_, $filter_id_, $indicator_, $corr_, $tailed_corr_) = @$result_ref_;
      if ( defined $id_to_filter_ { $filter_id_ } ) {
        my $preddur_filter_ = $preddur_." ".$id_to_filter_ { $filter_id_ };

        $$date_to_indc_to_preddur_filter_to_corr_ref_{ $date_ }{ $indicator_ }{ $preddur_filter_ } = $corr_;
        $$date_to_indc_to_preddur_filter_to_tailed_corr_ref_{ $date_ }{ $indicator_ }{ $preddur_filter_ } = $tailed_corr_;
      }
    }
  }
  $statement->finish;
}

sub FetchIndCountsForShc
{
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $timeperiod = shift;
  my $start_date_ = shift;
  my $end_date_ = shift;
  my $predalgo = shift;
  my $preddur_ref_vec_ = shift;
  my $filter_ref_vec_ = shift;
  my $base_px = shift ;
  my $fut_px = shift ;
  my $date_to_indc_to_count_ = shift;

  my $shortcode_id = GetIdFromNameIndStats ( $shortcode, "shortcode" );
  my $tp_id = GetIdFromNameIndStats ( $timeperiod, "timeperiod" );
  my $predalgo_id = GetIdFromNameIndStats ( $predalgo, "predalgo" );
  my $base_px_id = GetIdFromNameIndStats ( $base_px, "pricetype" );
  my $fut_px_id = GetIdFromNameIndStats ( $fut_px, "pricetype" );
  
  my %id_to_filter_ = ( );
  my @filter_vec_ = ();
  my @all_keys = ();
  my @tmp_keys = ();
  foreach my $filter_ ( @$filter_ref_vec_ ) {
    my $filter_id = GetIdFromNameIndStats ( $filter_, "filter" );
    if ( $filter_id >= 0 ) {
      $id_to_filter_ { $filter_id } = $filter_;
      push( @filter_vec_, $filter_id );
    }
  }

  if ( $shortcode_id < 0 || $predalgo_id < 0 || ! %id_to_filter_ || 
      $base_px_id < 0 || $fut_px_id < 0 ) {
    return;
  }
  
  my @results_vec_ref_ = ( ) ;

  foreach my $tmp ( @$preddur_ref_vec_ ) { push ( @tmp_keys, $shortcode_id.",".$tp_id.",".$tmp.",".$predalgo_id ); }
  foreach my $tmp ( @tmp_keys ) { push ( @all_keys, map { "$tmp,$_,$base_px_id,$fut_px_id" } @filter_vec_ ); }
  
  my $read_query = "SELECT date, indicator_id, count(*) FROM IndicatorStats  
    WHERE date >= ? AND date <= ? ";

  $read_query .= " AND combined_key in (".join(',', ('?') x @all_keys).")" ;
  $read_query .= " GROUP BY date, IndicatorStats.indicator_id";

  push ( @results_vec_ref_, $start_date_ );
  push ( @results_vec_ref_, $end_date_ );
  push ( @results_vec_ref_, @all_keys );
  
  #print $read_query."\n".join("\n", @results_vec_ref_)."\n";

  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute(@results_vec_ref_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_ref_ = $statement->fetchrow_arrayref) 
  {
    if ( $#$result_ref_ >= 2 ) {
      my ($date_, $indicator_id_, $count_) = @$result_ref_;
      my $indicator_ = GetNameFromIdIndStats($indicator_id_, "indicator");
      $$date_to_indc_to_count_{ $date_ }{ $indicator_ } = $count_;
    }
  }
  $statement->finish;
}

sub FetchIndStatsForShc
{
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $timeperiod = shift;
  my $dates_ref_ = shift;
  my $end_date_ = shift;
  my $predalgo_ref_ = shift;
  my $preddur_ref_ = shift;
  my $filter_ref_ = shift;
  my $basepx_ref_ = shift;
  my $futpx_ref_ = shift;
  my $algodurfilter_to_indc_to_corr_stats_ = shift;
  my $algodurfilter_to_indc_to_tailed_corr_stats_ = shift;

  if ( ! defined $predalgo_ref_ || $#$predalgo_ref_ < 0 || ! defined $preddur_ref_ || $#$preddur_ref_ < 0 || !defined $filter_ref_ || $#$filter_ref_ < 0 
       || ! defined $basepx_ref_ || $#$basepx_ref_ < 0 || ! defined $futpx_ref_ || $#$futpx_ref_ < 0) { return 0};


  my $shortcode_id = GetIdFromNameIndStats ( $shortcode, "shortcode" );
  my $tp_id = GetIdFromNameIndStats ( $timeperiod, "timeperiod" );
	
  my @predalgo_id_ref_ = ( );
  my @filter_id_ref_ = ( );
  my @basepx_id_ref_ = ( );
  my @futpx_id_ref_ = ( );

  @predalgo_id_ref_ =  map { GetIdFromNameIndStats ($_, "predalgo") } @$predalgo_ref_;
  @filter_id_ref_ =  map { GetIdFromNameIndStats ($_, "filter") } @$filter_ref_;
  @basepx_id_ref_ =  map { GetIdFromNameIndStats ($_, "pricetype") } @$basepx_ref_;
  @futpx_id_ref_ = map { GetIdFromNameIndStats ($_, "pricetype") } @$futpx_ref_;

  
  my @tmp_keys   = () ;
  my @tmp_keys_2 = () ;
  my @tmp_keys_3 = () ;
  my @tmp_keys_4 = () ;
  my @all_keys   = () ;

  foreach my $tmp ( @$preddur_ref_ ) { push ( @tmp_keys, $shortcode_id.",".$tp_id.",".$tmp ); }
  foreach my $tmp ( @tmp_keys ) { push ( @tmp_keys_2, map { "$tmp,$_" } @predalgo_id_ref_ ); }
  foreach my $tmp ( @tmp_keys_2 ) { push ( @tmp_keys_3, map { "$tmp,$_" } @filter_id_ref_ ); }
  foreach my $tmp ( @tmp_keys_3 ) { push ( @tmp_keys_4, map { "$tmp,$_" } @basepx_id_ref_ ); }
  foreach my $tmp ( @tmp_keys_4 ) { push ( @all_keys, map { "$tmp,$_" } @futpx_id_ref_ ); }


  my $read_query = "SELECT predalgo_id, pred_dur, filter_id, basepx_id, futpx_id, indicator, date,
    AVG(correlation), STD(correlation), MIN(correlation), MAX(correlation),
    AVG(tail_correlation), STD(tail_correlation), MIN(tail_correlation), MAX(tail_correlation)
    FROM IndicatorStats
    INNER JOIN Indicator ON Indicator.indicator_id = IndicatorStats.indicator_id	
    WHERE combined_key in (".join(',', ('?') x @all_keys).")";
#FORCE INDEX (PRIMARY)

    my $max_date = max @$dates_ref_;
    my $min_date = min @$dates_ref_;
    $read_query .= " AND date >= $min_date";
    $read_query .= " AND date <= $max_date";


  $read_query .= " GROUP BY predalgo_id, pred_dur, filter_id, basepx_id, futpx_id, IndicatorStats.indicator_id";

#  print $read_query."\n".join("\n", @all_keys)."\n";

  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute(@all_keys) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  while (my $result_ref_ = $statement->fetchrow_arrayref) 
  {
    if ( $#$result_ref_ >= 10 && defined $$result_ref_[5] 
        && defined $algodurfilter_to_indc_to_corr_stats_  && $$result_ref_[6] ~~ @$dates_ref_) {

      my $predalgo_ = GetNameFromIdIndStats (@$result_ref_[0], "predalgo");
      my $filter_ = GetNameFromIdIndStats (@$result_ref_[2], "filter");
      my $basepx = GetNameFromIdIndStats (@$result_ref_[3], "pricetype");
      my $futpx = GetNameFromIdIndStats (@$result_ref_[4], "pricetype");
      my $algodurfilter_key_ = $predalgo_.' '.@$result_ref_[1].' '.$filter_.' '.$basepx.' '.$futpx;
      my $indc_ = $$result_ref_[5];
      $$algodurfilter_to_indc_to_corr_stats_{ $algodurfilter_key_ }{ $indc_ }{ "AVG" } = $$result_ref_[7];
      $$algodurfilter_to_indc_to_corr_stats_{ $algodurfilter_key_ }{ $indc_ }{ "STD" } = $$result_ref_[8];
      $$algodurfilter_to_indc_to_corr_stats_{ $algodurfilter_key_ }{ $indc_ }{ "MIN" } = $$result_ref_[9];
      $$algodurfilter_to_indc_to_corr_stats_{ $algodurfilter_key_ }{ $indc_ }{ "MAX" } = $$result_ref_[10];

      if ( $#$result_ref_ >= 14 && defined $algodurfilter_to_indc_to_tailed_corr_stats_ ) {
        $$algodurfilter_to_indc_to_tailed_corr_stats_{ $algodurfilter_key_ }{ $indc_ }{ "AVG" } = $$result_ref_[11];
        $$algodurfilter_to_indc_to_tailed_corr_stats_{ $algodurfilter_key_ }{ $indc_ }{ "STD" } = $$result_ref_[12];
        $$algodurfilter_to_indc_to_tailed_corr_stats_{ $algodurfilter_key_ }{ $indc_ }{ "MIN" } = $$result_ref_[13];
        $$algodurfilter_to_indc_to_tailed_corr_stats_{ $algodurfilter_key_ }{ $indc_ }{ "MAX" } = $$result_ref_[14];
      }
    }
  }
  $statement->finish;
}

sub FetchDetailedIndStatsForShc
{
  if ( ! defined $dbh ) { return 0 };
  my $shortcode = shift;
  my $timeperiod = shift;
  my $dates_ref_ = shift;
  my $end_date_ = shift;
  my $predalgo_ref_ = shift;
  my $preddur_ref_ = shift;
  my $filter_ref_ = shift;
  my $basepx_ref_ = shift;
  my $futpx_ref_ = shift;
  my $algodurfilter_to_indc_to_corr_ = shift;
  my $algodurfilter_to_indc_to_tailed_corr_ = shift;

  if ( ! defined $predalgo_ref_ || $#$predalgo_ref_ < 0 || ! defined $preddur_ref_ || $#$preddur_ref_ < 0 || !defined $filter_ref_ || $#$filter_ref_ < 0 
       || ! defined $basepx_ref_ || $#$basepx_ref_ < 0 || ! defined $futpx_ref_ || $#$futpx_ref_ < 0) { return 0 };

  my $shortcode_id = GetIdFromNameIndStats ( $shortcode, "shortcode" );
  my $tp_id = GetIdFromNameIndStats ( $timeperiod, "timeperiod" );

  my @predalgo_id_ref_ = ( );
  my @filter_id_ref_ = ( );
  my @basepx_id_ref_ = ( );
  my @futpx_id_ref_ = ( );

  @predalgo_id_ref_ =  map { GetIdFromNameIndStats ($_, "predalgo") } @$predalgo_ref_;
  @filter_id_ref_ =  map { GetIdFromNameIndStats ($_, "filter") } @$filter_ref_;
  @basepx_id_ref_ =  map { GetIdFromNameIndStats ($_, "pricetype") } @$basepx_ref_;
  @futpx_id_ref_ = map { GetIdFromNameIndStats ($_, "pricetype") } @$futpx_ref_;


  my @tmp_keys   = () ;
  my @tmp_keys_2 = () ;
  my @tmp_keys_3 = () ;
  my @tmp_keys_4 = () ;
  my @all_keys   = () ;

  foreach my $tmp ( @$preddur_ref_ ) { push ( @tmp_keys, $shortcode_id.",".$tp_id.",".$tmp ); }
  foreach my $tmp ( @tmp_keys ) { push ( @tmp_keys_2, map { "$tmp,$_" } @predalgo_id_ref_ ); }
  foreach my $tmp ( @tmp_keys_2 ) { push ( @tmp_keys_3, map { "$tmp,$_" } @filter_id_ref_ ); }
  foreach my $tmp ( @tmp_keys_3 ) { push ( @tmp_keys_4, map { "$tmp,$_" } @basepx_id_ref_ ); }
  foreach my $tmp ( @tmp_keys_4 ) { push ( @all_keys, map { "$tmp,$_" } @futpx_id_ref_ ); }

  my $read_query = "SELECT predalgo_id, pred_dur, filter_id, basepx_id, futpx_id, indicator, date,
    AVG(correlation), STD(correlation), MIN(correlation), MAX(correlation),
    AVG(tail_correlation), STD(tail_correlation), MIN(tail_correlation), MAX(tail_correlation)
    FROM IndicatorStats
    INNER JOIN Indicator ON Indicator.indicator_id = IndicatorStats.indicator_id  
    WHERE combined_key in (".join(',', ('?') x @all_keys).")";
#FORCE INDEX (PRIMARY)

    my $max_date = max @$dates_ref_;
    my $min_date = min @$dates_ref_;
    $read_query .= " AND date >= $min_date";
    $read_query .= " AND date <= $max_date";



# print $read_query."\n".join("\n", @all_keys)."\n";

  my $statement = $dbh->prepare_cached($read_query); 
  unless ( $statement->execute(@all_keys) )
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  while (my $result_ref_ = $statement->fetchrow_arrayref) 
  {
    if ( $#$result_ref_ >= 7 && $$result_ref_[6] ~~ @$dates_ref_ ) {
      my $predalgo_ = GetNameFromIdIndStats (@$result_ref_[0], "predalgo");
      my $filter_ = GetNameFromIdIndStats (@$result_ref_[2], "filter");
      my $basepx = GetNameFromIdIndStats (@$result_ref_[3], "pricetype");
      my $futpx = GetNameFromIdIndStats (@$result_ref_[4], "pricetype");
      my $algodurfilter_key_ = $predalgo_.' '.@$result_ref_[1].' '.$filter_.' '.$basepx.' '.$futpx;
      my $date_ = $$result_ref_[5];
      my $indc_ = $$result_ref_[6];
      $$algodurfilter_to_indc_to_corr_{ $algodurfilter_key_ }{ $date_ }{ $indc_ } = $$result_ref_[7];
      $$algodurfilter_to_indc_to_tailed_corr_{ $algodurfilter_key_ }{ $date_ }{ $indc_ } = $$result_ref_[8];
    }
  }
  $statement->finish;
}


#Takes three arguments ( stratname, date and a space-separated string of results )
#And inserts the result with the corresponding values, returns 1 if successful and 0 otherwise
sub InsertIndStats
{
  SetWriteDBIndStats();
  if ( ! defined $dbh ) { return 0 };
 
  if ( @_ < 4 ) 
  {
    print "Usage: <exec> shortcode date pred_dur predalgo filter base_px fut_px indicator correlation tail_correlation [overwrite=1]\n";
  }
  
  my $shortcode = shift;
  my $timeperiod = shift;
  my $date = shift;
  my $pred_dur = shift ;
  my $predalgo = shift ;
  my $filter = shift ;
  my $base_px = shift ;
  my $fut_px = shift ;
  my $indicator = shift;
  my $correlation = shift;
  my $tail_correlation = shift;
  my $overwrite_ = 1;
  if ( $#_ >= 0 ) { $overwrite_ = shift; }
  
  my $shortcode_id = Insert ( $shortcode, "shortcode" );
  my $tp_id = Insert ( $timeperiod, "timeperiod" );
  Insert ( $pred_dur, "preddur", $shortcode_id ); #Insert to keepp track of all pred_dur
  my $predalgo_id = Insert ( $predalgo, "predalgo" );
  my $filter_id = Insert ( $filter, "filter" );
  my $base_px_id = Insert ( $base_px, "pricetype" );
  my $fut_px_id = Insert ( $fut_px, "pricetype" );
  my $indicator_id = Insert ( $indicator, "indicator" );

  my @query_vec_ref_ = ();
  push ( @query_vec_ref_, $shortcode_id );
  push ( @query_vec_ref_, $tp_id );
  push ( @query_vec_ref_, $date );
  push ( @query_vec_ref_, $pred_dur );
  push ( @query_vec_ref_, $predalgo_id );
  push ( @query_vec_ref_, $filter_id );
  push ( @query_vec_ref_, $base_px_id );
  push ( @query_vec_ref_, $fut_px_id );
  push ( @query_vec_ref_, $indicator_id );

  push ( @query_vec_ref_, $correlation );
  push ( @query_vec_ref_, $tail_correlation );

  my $t_insert_query_ = ( $overwrite_ ) ? $genind_insert_update_query : $genind_insert_query;
  my $strat_stmt = $dbh->prepare_cached($t_insert_query_); 
  unless ( $strat_stmt->execute(@query_vec_ref_) ) 
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  return 1;
}

#Takes three arguments ( stratname, date and a space-separated string of results )
#And inserts the result with the corresponding values, returns 1 if successful and 0 otherwise
sub InsertMultipleIndStats
{
  SetWriteDBIndStats();
  if ( ! defined $dbh ) { return 0 };
 
  if ( @_ < 4 ) 
  {
    print "Usage: <exec> shortcode date pred_dur predalgo filter base_px fut_px indicator correlation tail_correlation [overwrite=1]\n";
  }
  
  my $shortcode = shift;
  my $timeperiod = shift;
  my $date = shift;
  my $pred_dur = shift ;
  my $predalgo = shift ;
  my $filter = shift ;
  my $base_px = shift ;
  my $fut_px = shift ;
  my $indc_to_corr_ref_ = shift;
  my $indc_to_tailed_corr_ref_ = shift;
  my $overwrite_ = 1;
  if ( $#_ >= 0 ) { $overwrite_ = shift; }

  print "InsertMultipleIndStats: $shortcode $timeperiod $date $pred_dur $predalgo $filter $base_px $fut_px \n";

  #print $base_px." ".$fut_px."\n".join("\n", keys %$indc_to_corr_ref_)."\n"; 
  my $shortcode_id = Insert ( $shortcode, "shortcode" );
  my $tp_id = Insert ( $timeperiod, "timeperiod" );
  Insert ( $pred_dur, "preddur", $shortcode_id ); #Insert to keep track of all pred_dur
  my $predalgo_id = Insert ( $predalgo, "predalgo" );
  my $filter_id = Insert ( $filter, "filter" );
  my $base_px_id = Insert ( $base_px, "pricetype" );
  my $fut_px_id = Insert ( $fut_px, "pricetype" );

  my @query_vec_ref_ = ();
  push ( @query_vec_ref_, $shortcode_id );
  push ( @query_vec_ref_, $tp_id );
  push ( @query_vec_ref_, $date );
  push ( @query_vec_ref_, $pred_dur );
  push ( @query_vec_ref_, $predalgo_id );
  push ( @query_vec_ref_, $filter_id );
  push ( @query_vec_ref_, $base_px_id );
  push ( @query_vec_ref_, $fut_px_id );

  my $t_insert_query_ = ( $overwrite_ ) ? $genind_insert_update_query : $genind_insert_query;
  my $ret_val_ = 1;
  foreach my $indc_ ( keys %$indc_to_corr_ref_ ) {
    my $indicator_id = Insert ( $indc_, "indicator" );
    my $t_corr_ = $$indc_to_corr_ref_{ $indc_ };
    my $t_tailed_corr_ = "-2"; 
    if ( exists $$indc_to_tailed_corr_ref_{ $indc_ } ) {
      $t_tailed_corr_ = $$indc_to_tailed_corr_ref_{ $indc_ };
    }    
    
    my @t_query_vec_ref_ = ( @query_vec_ref_, $indicator_id , $t_corr_, $t_tailed_corr_);
#print $indc_." $t_corr_ $t_tailed_corr_\n";
#print $t_insert_query_."\n".join(" ", @t_query_vec_ref_)."\n";

    my $strat_stmt = $dbh->prepare_cached($t_insert_query_); 
    unless ( $strat_stmt->execute(@t_query_vec_ref_) ) 
    {
      print "SQL Error: $DBI::errstr\n";
      $ret_val_ = 0;
    }
  }
  return $ret_val_;
}

sub END
{
  DisConnectIndStats();
}

#InsertIndStats("HHI_0", 20160530, 200, "predalgo", "filter", "base_px", "fut_px", "indicator", 1.0, 2.25, "true");
1
