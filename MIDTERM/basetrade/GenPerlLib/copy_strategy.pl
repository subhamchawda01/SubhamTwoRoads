# \file GenPerlLib/install_strategy_production.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use POSIX;
use File::Basename; # For basename & dirname
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $TEMP_MATCH_DIR=$HOME_DIR."/tempfmdir";

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/exists_and_same.pl"; #ExistsAndSame
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/strat_utils.pl"; #GetParam


sub CopyFileCreateDirLocal
{
  my ( $orig_file_, $try_dest_file_ ) = @_;

# Get the directory name - some changes will be made to this for the remote location
  my $t_directory_name_ = dirname ($try_dest_file_);

  if ( ! -d $t_directory_name_ ) {
    my $ssh_cmd_ = ("mkdir -p ".$t_directory_name_);
    system ($ssh_cmd_) == 0 or PrintStacktraceAndDie ( "system $ssh_cmd_ failed : $?\n" );
  }

  my $cp_cmd_ = ("cp $orig_file_ $try_dest_file_");
  system ($cp_cmd_) == 0 or PrintStacktraceAndDie ( "system $cp_cmd_ failed : $?\n" );

  return $try_dest_file_ ;
}

sub ChangePathsLocal
{
  my $orig_file_path_ = shift;
  my $dest_dir_ = shift;

  return $dest_dir_.$orig_file_path_;
}

sub MakeLocalCopyStrategy
{
  my ( $strategy_filename_, $dest_dir_ ) = @_;

  $strategy_filename_ = File::Spec->rel2abs ($strategy_filename_);
  $dest_dir_ = File::Spec->rel2abs ($dest_dir_);

# Get the directory name - some changes will be made to this for the remote location
  my $directory_name_ = dirname ($strategy_filename_);
  my $t_strategy_filename_base_ = basename ($strategy_filename_);

  my $tmp_dir_ = $HOME_DIR."/isptemp/"; 
  `mkdir -p $tmp_dir_`;
  my $tmp_strategy_filename_ = $tmp_dir_.$t_strategy_filename_base_;

  my $dest_strat_file_name_ = ChangePathsLocal ( $strategy_filename_, $dest_dir_ );
  $dest_strat_file_name_ = CopyFileCreateDirLocal( $strategy_filename_, $dest_strat_file_name_ );

  open ORIGSTRATFILEHANDLE, "< $strategy_filename_ " or PrintStacktraceAndDie ( "Could not open $strategy_filename_\n" );
  open FSTRATF, "> $dest_strat_file_name_" or PrintStacktraceAndDie ( "Could not open $dest_strat_file_name_ for writing\n" );
  while ( my $thisline_ = <ORIGSTRATFILEHANDLE> ) 
  {
    my @t_words_ = split ( ' ', $thisline_ );
    if ( $#t_words_ >= 7 )
    { 
      my $model_file_name_ = GetModel($strategy_filename_);
      my $params_file_name_ = GetParam($strategy_filename_, "TODAY");

      my $dest_model_file_name_ = ChangePathsLocal ( $model_file_name_, $dest_dir_ ) ; 
      $dest_model_file_name_ = CopyFileCreateDirLocal ( $model_file_name_, $dest_model_file_name_ );

      my $dest_param_file_name_ = ChangePathsLocal ( $params_file_name_, $dest_dir_ ) ;
      $dest_param_file_name_ = CopyFileCreateDirLocal ( $params_file_name_, $dest_param_file_name_ );

      $t_words_[3] = $dest_model_file_name_ ;
      $t_words_[4] = $dest_param_file_name_ ;
      print FSTRATF join ( ' ', @t_words_ );
      last; # right now no support for multiline strat files
    }
  }
  close ORIGSTRATFILEHANDLE ;

  close FSTRATF ;

  return $dest_strat_file_name_;
}

1;
