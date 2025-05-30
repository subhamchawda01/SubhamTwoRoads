# \file GenPerlLib/make_strat_vec_from_dir_in_tp_maxpred_excluding_sets.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 353, Evoma, #14, Bhattarhalli,
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
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/make_strat_vec_from_dir_maxpred.pl"; # MakeStratVecFromDirMaxPred
require "$GENPERLLIB_DIR/is_strat_dir_in_timeperiod.pl"; # IsStratDirInTimePeriod
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; # FindItemFromVecWithBase

# Look for directories in $MODELING_STRATS_DIR/$shortcode_
# if IsStratDirInTimePeriod
# then for each of the eligible tp_dirs
sub MakeStratVecFromDirInTpMaxPredExcludingSets
{ # expects an argument like ~/modelling/strats/ZN_0
  my ($top_directory_, $time_period_, $max_preddur_, @exclude_tp_dirs_) = @_;
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
#	      print STDERR "checking $dir_item_\n";
	      if ( ( -d "$top_directory_/$dir_item_" ) &&
		   ( IsStratDirInTimePeriod ( $dir_item_, $time_period_ ) ) &&
		   ( ! ( FindItemFromVecWithBase ( $dir_item_, @exclude_tp_dirs_ ) ) ) )
	      { 
		  push @ret_abs_file_paths_, MakeStratVecFromDirMaxPred ( "$top_directory_/$dir_item_", $max_preddur_ ) ; 
	      }

	  }
      }
  }
  @ret_abs_file_paths_;
}

1
