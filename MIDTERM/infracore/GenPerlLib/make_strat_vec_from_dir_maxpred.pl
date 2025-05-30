# \file GenPerlLib/make_strat_vec_from_dir_maxpred.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 162, Evoma, #14, Bhattarhalli,
#        Old Madras Road, Near Garden City College,
#        KR Puram, Bangalore 560049, India
#        +91 80 4190 3551
#

use strict;
use warnings;
use POSIX;
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/get_pred_dur_strat.pl"; # GetPredDurStrat

sub MakeStratVecFromDirMaxPred
{
  my ($top_directory_, $max_preddur_) = @_;
  $top_directory_ = File::Spec->rel2abs ( $top_directory_ );
  my @ret_abs_file_paths_ = ();
  if ( -d $top_directory_ )
  {
      if (opendir my $dh, $top_directory_)
      {
	  my @t_list_=();
	  while ( my $t_item_ = readdir $dh)
	  {
	      push @t_list_, $t_item_;
	  }
	  closedir $dh;
	  
	  for my $dir_item_ (@t_list_)
	  {
	      # Unix file system considerations.
	      next if $dir_item_ eq '.' || $dir_item_ eq '..';
	      
	      my $strat_file_path_ = $top_directory_."/".$dir_item_;
	      if ( ( -f $strat_file_path_ ) &&
		   ( GetPredDurStrat ( $strat_file_path_ ) <= $max_preddur_ ) )
	      {
		  push @ret_abs_file_paths_, $strat_file_path_ ;
	      }
	      push @ret_abs_file_paths_, MakeStratVecFromDirMaxPred ("$top_directory_/$dir_item_", $max_preddur_) if -d "$top_directory_/$dir_item_";
	  }
      }
  }
  @ret_abs_file_paths_;
}

1
