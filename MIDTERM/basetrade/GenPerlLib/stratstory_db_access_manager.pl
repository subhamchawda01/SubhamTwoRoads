#!/usr/bin/perl

# It provides functions for stratstory and pool-correlation related DB queries
# Stratstory: the regime highlights for strategy. It mentions the features that splits the good/bad periods 
# Pool-Correlation: for all the strat-pairs in the pools, we keep the pnlsample-correlations in DB
#

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

require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; #CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; #GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_date.pl"; #CalcPrevDate
require "$GENPERLLIB_DIR/results_db_access_manager.pl";

my @StratStory_table_cols = ( "configid", "pnl_metrics", "feature_metrics" );
my $StratStory_insert_query = "INSERT INTO wf_config_story ( ".join(',' , @StratStory_table_cols)." ) VALUES ( ".join(',' , ('?') x @StratStory_table_cols)." )";
my $StratStory_delete_query = "DELETE FROM wf_config_story WHERE configid = ? ";
my $StratStory_fetch_singlestrat_query = "SELECT pnl_metrics, feature_metrics FROM wf_configs, wf_config_story WHERE wf_configs.configid = wf_config_story.configid AND wf_configs.cname = ? ";
my $StratStory_fetch_all_query = "SELECT cname, pnl_metrics, feature_metrics FROM wf_configs, wf_config_story WHERE  wf_configs.configid = wf_config_story.configid AND shortcode = ? ";
my $StratStory_fetch_recent_query = "SELECT cname FROM wf_configs, wf_config_story WHERE  wf_configs.configid = wf_config_story.configid AND shortcode = ? AND DATEDIFF(date(last_updated),date(CURDATE())) < 30";

my $dbh = ReturnDBH();

sub FormMetricsMap
{
  # input: tuple of pnl_metrics, feature_metrics
  my $row_ref_ = shift;
  my $metrics_ref_ = shift;

  if ( defined $row_ref_ && defined $$row_ref_[0] ) {
    my ($sharpe_, $pnl_avg_, $min_pnl_ ) = map { (split(':',$_))[1] } split(/\s+/, $$row_ref_[0]);

    $$metrics_ref_{ "SHARPE" } = $sharpe_;
    $$metrics_ref_{ "PNL_AVG" } = $pnl_avg_;
    $$metrics_ref_{ "MIN_PNL" } = $min_pnl_;

    if ( $#$row_ref_ > 0 and defined $$row_ref_[1] ) {
      my @feat_vec_ = split(/\s+/, $$row_ref_[1]);

      foreach my $feat_str_ ( @feat_vec_ ) {
        my @tokens_ = split(';', $feat_str_);
        my $featname_ = shift @tokens_;

        my %t_featmap_ = ( );
        foreach my $token_ ( @tokens_ ) {
          my @ttokens_ = split(':', $token_);
          next if $#ttokens_ < 2;

          if ( $ttokens_[0] eq "Split" ) {
            $t_featmap_{ "SplitPercentile" } = $ttokens_[1];
            $t_featmap_{ "SplitValue" } = $ttokens_[2];
          }
          else {
            $t_featmap_{ "split0" }{ $ttokens_[0] } = $ttokens_[1];
            $t_featmap_{ "split1" }{ $ttokens_[0] } = $ttokens_[2];
          }
        }
        $$metrics_ref_{ $featname_ } = \%t_featmap_;
      }
    }
  }
}

# Fetch StratStory for single wfconfig
sub FetchSingleStory
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 1 )
  {
    print "Usage: FetchSingleStory() stratname stratstory_map_ref";
    return;
  }
  my $stratname_ = shift;
  my $metrics_ref_ = shift;

  my $statement = $dbh->prepare_cached($StratStory_fetch_singlestrat_query);
  unless ( $statement->execute($stratname_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;

    FormMetricsMap($row_ref_, $metrics_ref_);
  }
}

# Fetch StratStory for all wfconfigs for a shortcode
sub FetchAllStories
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 2 )
  {
    print "Usage: FetchAllStories() shortcode storyvec_ref_";
    return;
  }
  my $shortcode_ = shift;
  my $storymap_ref_ = shift;

  my $statement = $dbh->prepare_cached($StratStory_fetch_all_query);
  unless ( $statement->execute($shortcode_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( defined $row_ref_ && defined $$row_ref_[0] ) {
      my $cname_ = $$row_ref_[0];
      %{$$storymap_ref_{$cname_}} = ();
       
      FormMetricsMap([$$row_ref_[1], $$row_ref_[2]], $$storymap_ref_{$cname_});
    }
  }
  $statement->finish;
  return;
}

# Fetch StratStory for all wfconfigs for a shortcode
sub FetchRecentStoriesCnames
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 2 )
  {
    print "Usage: FetchRecentStoriesCnames() shortcode storyvec_ref_";
    return;
  }
  my $shortcode_ = shift;
  my $configs_ref_ = shift;

  my $statement = $dbh->prepare_cached($StratStory_fetch_recent_query);
  unless ( $statement->execute($shortcode_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    push (@$configs_ref_, $$row_ref_[0]) if defined $$row_ref_[0];
  }
  $statement->finish;
  return;
}

# Insert StratStory for a strat
sub InsertStory
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 4 ) 
  {
    print "Usage: InsertStory() shortcode stratname pnlsharpe feature";
    return 0;
  }
  my $shc_ = shift;
  my $stratname_ = shift;
  my $pnl_sharpe_ = shift ;
  my $feature_ = shift;

#Find configid from stratname first
  my $config_id_ = GetIdFromName ( $stratname_ , "wf_config" );
  if ( $config_id_ < 0 ) { return 0 ;}

  my $del_stmt = $dbh->prepare_cached($StratStory_delete_query); 
  unless ( $del_stmt->execute($config_id_) ) 
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }

  my @results_vec_ref_ = ();
  push ( @results_vec_ref_, $config_id_ );
  push ( @results_vec_ref_, $pnl_sharpe_ );
  push ( @results_vec_ref_, $feature_ );
  my $strat_stmt = $dbh->prepare_cached($StratStory_insert_query); 
  unless ( $strat_stmt->execute(@results_vec_ref_) ) 
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  return 1;
}

sub InsertCorrelation
{
  SetWriteDB();
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 5 )
  {
    print "Usage: InsertCorrelation() shc strat1name strat2name correlation updateexisting[0/1]";
    return 0;
  }

  my $shc_ = shift;
  my $strat1_ = shift;
  my $strat2_ = shift;
  my $corr_ = shift;
  my $update_existing = shift;

  my $config1_id_ = GetIdFromName($strat1_, "wf_config");
  my $config2_id_ = GetIdFromName($strat2_, "wf_config");
  if ( $config1_id_ > $config2_id_ ) {
    ($config1_id_, $config2_id_) = ($config2_id_, $config1_id_);
  }

  my @query_args_ = ($config1_id_, $config2_id_, $corr_);
  my $insert_corr_query_ = "";
  if( $update_existing == 0 ) {
    $insert_corr_query_ = "INSERT INTO wf_config_correlation (configid1, configid2, correlation) VALUES (?,?,?)";
  } else {
    $insert_corr_query_ = "INSERT INTO wf_config_correlation (configid1, configid2, correlation) VALUES (?,?,?) ON DUPLICATE KEY UPDATE correlation=VALUES(correlation)"
  }
  my $insert_stmt_ = $dbh->prepare_cached($insert_corr_query_);
  unless ( $insert_stmt_->execute(@query_args_) ) 
  {
    print "SQL Error: $DBI::errstr\n";
    return 0;
  }
  return 1;
}

sub FetchCorrelationForPair
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 2 )
  {
    print @_."\n";
    print "Usage: FetchCorrelationForPair() strat1name strat2name";
    return 0;
  }
  my $strat1_ = shift;
  my $strat2_ = shift;

  my $config1_id_ = GetIdFromName($strat1_, "wf_config");
  my $config2_id_ = GetIdFromName($strat2_, "wf_config");
  if ( $config1_id_ > $config2_id_ ) {
    ($config1_id_, $config2_id_) = ($config2_id_, $config1_id_);
  }

  if ( $config1_id_ < 0 || $config2_id_ < 0 ) {
    return;
  }

  my @query_args_ = ($config1_id_, $config2_id_);

  my $fetch_corr_query_ = "SELECT correlation FROM wf_config_correlation WHERE configid1 = ? AND configid2 = ?";
  my $statement = $dbh->prepare_cached($fetch_corr_query_);
  unless ( $statement->execute(@query_args_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  if (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    $statement->finish;
    return $$row_ref_[0];
  }
  return;
}

sub FetchAllCorrelationForStrat
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 2 )
  {
    print "Usage: FetchAllCorrelationForStrat() strat1name strat_to_corr_map";
    return 0;
  }
  my $strat1_ = shift;
  my $strat_to_corr_map_ref_ = shift;

  my $config1_id_ = GetIdFromName($strat1_, "wf_config");
  if ( $config1_id_ < 0 ) { return; }

  my $fetch_corr_query_ = "SELECT wf_configs.cname, correlation FROM wf_config_correlation INNER JOIN wf_configs ON wf_config_correlation.configid2 = wf_configs.configid WHERE configid1 = ?";
  my $statement = $dbh->prepare_cached($fetch_corr_query_);
  unless ( $statement->execute($config1_id_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 1 ) {
      my ($strat_, $corr_) = @$row_ref_;
      $$strat_to_corr_map_ref_{ $strat_ } = $corr_;
    }
  }
  $statement->finish;

  $fetch_corr_query_ = "SELECT wf_configs.cname, correlation FROM wf_config_correlation INNER JOIN wf_configs ON wf_config_correlation.configid1 = wf_configs.configid WHERE configid2 = ?";
  $statement = $dbh->prepare_cached($fetch_corr_query_);
  unless ( $statement->execute($config1_id_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 1 ) {
      my ($strat_, $corr_) = @$row_ref_;
      $$strat_to_corr_map_ref_{ $strat_ } = $corr_;
    }
  }
  $statement->finish;

}

sub FetchAllCorrelationForStrats
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 2 )
  {
    print "Usage: FetchAllCorrelationForStrat() wf_configs_vec_ref strat1_strat2_to_corr_map";
    return 0;
  }
  my $strats_ref_ = shift;
  my $correlations_map_ref_ = shift;

  if ( $#$strats_ref_ < 0 ) { return; }

  my @configids_vec_ = ( );

  my $fetch_configids_query_ = "SELECT configid from wf_configs WHERE cname in ( ".join(',', ('?') x @$strats_ref_)." )";
  my $statement = $dbh->prepare_cached($fetch_configids_query_);
  unless ( $statement->execute(@$strats_ref_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref) {
    push ( @configids_vec_, $$row_ref_[0] ) if ( $#$row_ref_ >= 0 );
  }
  $statement->finish;

  my $fetch_corr_query_ = "SELECT s1.cname, s2.cname, correlation from wf_config_correlation 
    INNER JOIN wf_configs as s1 ON s1.configid = wf_config_correlation.configid1 
    INNER JOIN wf_configs as s2 ON s2.configid = wf_config_correlation.configid2
    WHERE configid1 in ( ".join(',', @configids_vec_)." )
    AND configid2 in ( ".join(',', @configids_vec_)." )";
  $statement = $dbh->prepare_cached($fetch_corr_query_);
  unless ( $statement->execute() )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 2 ) {
      my ($strat1_, $strat2_, $corr_) = @$row_ref_;
      $$correlations_map_ref_{ $strat1_." ".$strat2_ } = $corr_;
    }
  }
  $statement->finish;
}

sub FetchAllCorrelationForStratsUpdatedRecently
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 2 )
  {
    print "Usage: FetchAllCorrelationForStrat() strats_vec_ref strat1_strat2_to_corr_map";
    return 0;
  }
  my $strats_ref_ = shift;
  my $correlations_map_ref_ = shift;

  if ( $#$strats_ref_ < 0 ) { return; }

  my @configids_vec_ = ( );

  my $fetch_configids_query_ = "SELECT configid from wf_configs WHERE cname in ( ".join(',', ('?') x @$strats_ref_)." )";
  my $statement = $dbh->prepare_cached($fetch_configids_query_);
  unless ( $statement->execute(@$strats_ref_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref) {
    push ( @configids_vec_, $$row_ref_[0] ) if ( $#$row_ref_ >= 0 );
  }
  $statement->finish;

  my $fetch_corr_query_ = "SELECT s1.cname, s2.cname, correlation from wf_config_correlation 
    INNER JOIN wf_configs as s1 ON s1.configid = wf_config_correlation.configid1 
    INNER JOIN wf_configs as s2 ON s2.configid = wf_config_correlation.configid2
    WHERE configid1 in ( ".join(',', @configids_vec_)." )
    AND configid2 in ( ".join(',', @configids_vec_)." ) AND DATEDIFF(date(last_updated),date(CURDATE())) < 30";
  $statement = $dbh->prepare_cached($fetch_corr_query_);
  unless ( $statement->execute() )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 2 ) {
      my ($strat1_, $strat2_, $corr_) = @$row_ref_;
      $$correlations_map_ref_{ $strat1_." ".$strat2_ } = $corr_;
    }
  }
  $statement->finish;
}

sub FetchCorrelatedStrats
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 2 )
  {
    print "Usage: FetchCorrelatedStrats() strat1name strat_to_corr_map";
    return 0;
  }
  my $strat_ = shift;
  my $correlated_strat_map_ref_ = shift;
  my $type_ = "N";

  my $config1_id_ = GetIdFromName($strat_, "wf_config");
  if ( $config1_id_ < 0 ) { return; }
  my $fetch_corr_query_ = "SELECT s1.cname, s2.cname, correlation from wf_config_correlation 
    INNER JOIN wf_configs as s1 ON s1.configid = wf_config_correlation.configid1 
    INNER JOIN wf_configs as s2 ON s2.configid = wf_config_correlation.configid2 
    WHERE (configid1=? AND s2.type=?)
    OR (configid2=? AND s1.type=?)
    ORDER BY correlation DESC LIMIT 3";
  my $statement = $dbh->prepare_cached($fetch_corr_query_);
  unless ( $statement->execute($config1_id_, $type_, $config1_id_, $type_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref)
  {
    if ( $#$row_ref_ >= 2 ) {
      my ($strat1_, $strat2_, $corr_) = @$row_ref_;
      if( $strat1_ eq $strat_ ) {
        $$correlated_strat_map_ref_{ $strat2_ } = $corr_;
      } else {
        $$correlated_strat_map_ref_{ $strat1_ } = $corr_;
      }
    }
  }
  $statement->finish;
}

# Fetch list of pruned wfconfigs for a shortcode
sub PrunedStrats
{
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 1 )
  {
    print "Usage: PrunedStrats() pruned_strat_array";
    return 0;
  }
  my $shc_ = shift;
  my $pruned_strat_map_ref_ = shift;

  # @hrishav says in https://github.com/cvquant/basetrade/pull/1028
  # Since, we have moved from strats to configs, 
  # Kaushik and I decided that we don't need to support 
  # the old strats any longer. So, I am just replacing 
  # the functionality, instead of bloating it up with if-else everywhere.
  my $fetch_corr_query_ = "SELECT configid from wf_configs where shortcode = ? AND type='P'"; 
  my $statement = $dbh->prepare_cached($fetch_corr_query_);
  unless ( $statement->execute( $shc_ ) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  while (my $row_ref_ = $statement->fetchrow_arrayref )
  {
    push ( @$pruned_strat_map_ref_, $$row_ref_[0] );
  }
  $statement->finish;
}

sub RemoveStratCorrelations {
  CheckConnection();
  if ( ! defined $dbh ) { return 0 };

  if ( @_ < 1 )
  {
    print "Usage: RemoveStratCorrelations() strat_array_ref";
    return 0;
  }
  my $strats_ref_ = shift;
  return if $#$strats_ref_ < 0;

  my $delete_corr_query_ = "DELETE from wf_config_correlation WHERE configid1 in ( ".join(',', ('?') x @$strats_ref_)." ) OR configid2 in ( ".join(',', ('?') x @$strats_ref_)." ) ";
  my $statement = $dbh->prepare_cached($delete_corr_query_);
  unless ( $statement->execute(@$strats_ref_, @$strats_ref_) )
  {
    print "SQL Error: $DBI::errstr\n";
    return;
  }
  $statement->finish; 
}

1
