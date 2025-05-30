# \file GenPerlLib/make_strat_vec_from_dir_matchbase.pl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; #GetBasepxStratFirstModel GetOmixFileStratFirstModel
require "$GENPERLLIB_DIR/is_strat_valid.pl"; #IsStratValid

sub MakeCombinableStratVecsFromDir
{
  my $top_directory_ = shift;
  $top_directory_ = File::Spec->rel2abs ( $top_directory_ );
  
  my $basepx_pxtype_ = "ALL";

  if ( @_ )
  {
    $basepx_pxtype_ = shift;
  }

#my $combine_key_to_files_ref_ = \%combine_key_to_files_;
  my $combine_key_to_files_ref_ ;
  if ( @_ )
  {
    $combine_key_to_files_ref_ = shift;
  }
  else
  {
    my %empty_hash_ ;
    $combine_key_to_files_ref_ = \%empty_hash_;
  }


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
	      if ( -f $strat_file_path_ )
	      {
		my @retval = IsStratValid($strat_file_path_);
	      	my $message=$retval[0];
	      	my $exit_code_=$retval[1];
	      	if ($exit_code_ != 0) {
	      		print "Skipping strat_file: $strat_file_path_ Reason: $message\n";	
	      	}
	      	else {

                   my $t_basepx_pxtype_ = GetBasepxStratFirstModel ( $strat_file_path_ );
		   my $t_start_time_ = `awk '{print \$6}' $strat_file_path_`;     #necessary to keep the watch in sync
		   chomp($t_start_time_);
                   if( ( $basepx_pxtype_ eq "ALL" ) || ( $t_basepx_pxtype_ eq $basepx_pxtype_ ) ) 
                   {
                     my $t_omix_file_ = GetOmixFileStratFirstModel ( $strat_file_path_ );
                     my $t_onlinemix_file_=GetOnlineMixFileStratFirstModel($strat_file_path_);
                     my $t_key_ = $t_basepx_pxtype_.$t_omix_file_.$t_onlinemix_file_."_".$t_start_time_;
                     push ( @{$$combine_key_to_files_ref_{$t_key_}}, $strat_file_path_ );
                   }
		}
	      }

	      if ( -d $strat_file_path_ )
	      {
		  MakeCombinableStratVecsFromDir ( $strat_file_path_, $basepx_pxtype_ , $combine_key_to_files_ref_) ;
	      }
	  }
      }
  }
  $combine_key_to_files_ref_;
}

1
