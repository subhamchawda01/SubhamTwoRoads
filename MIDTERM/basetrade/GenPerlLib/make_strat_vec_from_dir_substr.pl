# \file GenPerlLib/make_strat_vec_from_dir_substr.pl
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





require "$GENPERLLIB_DIR/is_strat_valid.pl"; #IsStratValid

sub MakeStratVecFromDirSubstr
{
  my ($top_directory_, $substr_search_) = @_;
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
	      
	      if ( ( -f "$top_directory_/$dir_item_" ) && ( $dir_item_ =~ /$substr_search_/ ) )
	      {
		my $strat_file_path_="$top_directory_/$dir_item_";
		my @retval = IsStratValid($strat_file_path_);
                my $message=$retval[0];
                my $exit_code_=$retval[1];
                if ($exit_code_ != 0) {
                        print "Skipping strat_file: $strat_file_path_ Reason: $message\n";      
                }
                else {
			 push @ret_abs_file_paths_, "$top_directory_/$dir_item_" ; 
		}
	      }
	      push @ret_abs_file_paths_, MakeStratVecFromDirSubstr ("$top_directory_/$dir_item_", $substr_search_) if -d "$top_directory_/$dir_item_";
	  }
      }
  }
  @ret_abs_file_paths_;
}

1
