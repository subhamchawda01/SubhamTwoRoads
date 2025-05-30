# \file GenPerlLib/comb_config_files_exist.pl
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

# Look for files in $COMB_CONFIG_DIR/comb_config_$shortcode_
# e.g. $HOME/indicatorwork/prod_configs/comb_config_$SHC*

sub CombConfigFilesExist
{ # expects an argument like ~/modelling/strats/ZN_0
  my ( $top_directory_, $this_product_ ) = @_;
  $top_directory_ = File::Spec->rel2abs ( $top_directory_ );
  my $name_prefix_="comb_config_".$this_product_;

  my $this_prod_files_exist_ = 0;
  if ( -d $top_directory_ )
  {
      if ( opendir my $dh, $top_directory_ )
      {
	  my @t_list_=();
	  while ( my $t_item_ = readdir $dh )
	  {
	      push @t_list_, $t_item_;
	  }
	  closedir $dh;
	  
	  for my $dir_item_ (@t_list_)
	  {
	      # Unix file system considerations.
	      next if $dir_item_ eq '.' || $dir_item_ eq '..';
	      if ( ( -e "$top_directory_/$dir_item_" ) &&
		   ( $dir_item_ =~ /$name_prefix_/ ) )
	      { 
		  $this_prod_files_exist_ = 1;
	      }
	  }
      }
  }
  $this_prod_files_exist_;
}

1
