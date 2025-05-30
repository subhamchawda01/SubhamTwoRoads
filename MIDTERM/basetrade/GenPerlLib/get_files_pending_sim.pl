# \file GenPerlLib/get_files_pending_sim.pl
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
use File::Basename;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";

require "$GENPERLLIB_DIR/global_results_methods.pl"; # 

# Returns the strats for which results are pending
sub GetFilesPendingSimFromShcDateDir
{
  my $shc_ = shift;
  my $yyyymmdd_ = shift;
  my $strategy_filevec_ref_ = shift;
  my $this_day_strategy_filevec_ref_ = shift;
  my $local_res_dir_ = shift;
  
  my $fetch_from_wf_db_ = 0;
  if ( $#_ >= 0 ){
    $fetch_from_wf_db_ = shift;
  }

  @$this_day_strategy_filevec_ref_ = ();

  my @t_strat_with_res_vec_ = ();
  # strats with results with or without pnl_samples
  GetStratsWithGlobalResultsForShortcodeDate($shc_, $yyyymmdd_, \@t_strat_with_res_vec_, $local_res_dir_, "N", $fetch_from_wf_db_);
  foreach my $t_strat_ ( @$strategy_filevec_ref_ )
  {
    my $t_base_ = basename ($t_strat_);
    if ( ! ( grep { $_ eq $t_base_ } @t_strat_with_res_vec_ ) )
    {
      push ( @$this_day_strategy_filevec_ref_ , $t_strat_ );
    }
  }
}

# Returns the strats for which either results or pnl_samples are pending
sub GetFilesPendingSimAndPnlSamplesFromShcDateDir
{
  my $shc_ = shift;
  my $yyyymmdd_ = shift;
  my $strategy_filevec_ref_ = shift;
  my $this_day_strategy_filevec_ref_ = shift;
  my $local_res_dir_ = shift;
  my $local_pnl_samples_dir_ = shift;

  my $fetch_from_wf_db_ = 0;
  if ( $#_ >= 0 ){
    $fetch_from_wf_db_ = shift;
  }

  @$this_day_strategy_filevec_ref_ = ();

  if ( $local_res_dir_ eq "DB" ) 
  {
    my @t_strat_with_res_vec_ = ();
    #new results with pnl_samples from SQLDB
    GetStratsWithGlobalResultsForShortcodeDate($shc_, $yyyymmdd_, \@t_strat_with_res_vec_, "DB", "NP", $fetch_from_wf_db_);

    foreach my $t_strat_ ( @$strategy_filevec_ref_ )
    {
      my $t_base_ = basename ($t_strat_);
      if ( ! ( grep { $_ eq $t_base_ } @t_strat_with_res_vec_ ) )
      {
        push ( @$this_day_strategy_filevec_ref_ , $t_strat_ );
      }
    }
  }
  else
  {
    my @t_strat_with_res_vec_ = ();
    #new results from dir, may or may not have pnl_samples
    GetStratsWithGlobalResultsForShortcodeDate($shc_, $yyyymmdd_, \@t_strat_with_res_vec_, $local_res_dir_);

    my @t_strat_with_pnl_samples_vec_ = ();
    #starts with pnl_samples from dir
    GetStratsWithGlobalResultsForShortcodeDate($shc_, $yyyymmdd_, \@t_strat_with_pnl_samples_vec_, $local_pnl_samples_dir_);

    foreach my $t_strat_ ( @$strategy_filevec_ref_ )
    {
      my $t_base_ = basename ($t_strat_);
      if ( !(grep { $_ eq $t_base_ } @t_strat_with_res_vec_) || !(grep { $_ eq $t_base_ } @t_strat_with_pnl_samples_vec_) )
      {
        push ( @$this_day_strategy_filevec_ref_ , $t_strat_ );
      }
    }
  }
}


1;
