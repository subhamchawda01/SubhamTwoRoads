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


sub CopyFileCreateDir
{
    my ( $orig_file_, $try_dest_file_, $user_, $prod_machine_ip_ ) = @_;

    # Get the directory name - some changes will be made to this for the remote location
    my $t_directory_name_ = dirname ($try_dest_file_); chomp ( $t_directory_name_ );

    # Create directory on remote location
    my $ssh_cmd_ = ("ssh ".$user_."@".$prod_machine_ip_." mkdir -p ".$t_directory_name_);
    system ($ssh_cmd_) == 0 or PrintStacktraceAndDie ( "system $ssh_cmd_ failed : $?\n" );

    my $noneedtocopy_ = 0;

    # search for existing files with same name
    my $ls_output_ = `ssh $prod_machine_ip_ -l $user_ ls $try_dest_file_ 2>/dev/null`; 
    while ( $ls_output_ )
    {
	# get file and checfor similarity
	`mkdir -p $TEMP_MATCH_DIR`;
	my $scp_cmd="scp -q ".$user_."@".$prod_machine_ip_.":".$try_dest_file_." ".$TEMP_MATCH_DIR."/";
	`$scp_cmd`;
	my $local_filename_ = basename ( $try_dest_file_ ); chomp ( $local_filename_ );
	$local_filename_ = $TEMP_MATCH_DIR."/".$local_filename_ ;
	
	if ( ExistsAndSame ( $orig_file_, $local_filename_ ) == 1 )
	{ # it is as if the file is already ocpied
	    $noneedtocopy_ = 1;
	    `rm -f $local_filename_`;
	    last;
	}
	`rm -f $local_filename_`;

	$try_dest_file_ = $try_dest_file_."_".int(rand(9));
	$ls_output_ = `ssh $prod_machine_ip_ -l $user_ ls $try_dest_file_ 2>/dev/null`; 
    }
    if ( $noneedtocopy_ != 1 )
    {
	# scp file over to newly created directory
	my $scp_cmd_ = ("scp -q ".$orig_file_." ".$user_."@".$prod_machine_ip_.":".$try_dest_file_);
	system ($scp_cmd_) == 0 or PrintStacktraceAndDie ( "system $scp_cmd_ failed : $?\n" );
    }
    return $try_dest_file_ ;
}

sub ChangePaths
{
    my $orig_file_path_ = shift;
    my $user_ = shift;
    my $prefix_path_to_remove_ = shift || "";

    # remove the prefix
    $orig_file_path_ =~ s/^$prefix_path_to_remove_//;
    #doing it twice as in case of config, we are making temporary strats twice
    $orig_file_path_ =~ s/^$prefix_path_to_remove_//;

    # Replace 1st occurrence of "modelling" with "LiveModels"
    $orig_file_path_ =~ s/modelling/LiveModels/;

    # Modify destination path correctly
    # Replace 1st occurrence of "gchak" or "dvcinfra" or "dvctrader" with user name provided.
    $orig_file_path_ =~ s/$USER/$user_/;
    $orig_file_path_ =~ s/dvctrader/$user_/;

    return $orig_file_path_;
}

sub ChangePathsModels
{
  my $orig_file_path_ = shift;

  my $dest_file_path_ = "/home/dvctrader/LiveModels/models/".basename($orig_file_path_);
  return $dest_file_path_;
}

sub ChangePathsParams
{
  my $orig_file_path_ = shift;

  my $dest_file_path_ = "/home/dvctrader/LiveModels/params/".basename($orig_file_path_);
  return $dest_file_path_;
}

sub InstallStrategyProduction
{
  my ( $t_strategy_filename_, $prod_machine_ip_, $strat_id_, $user_, $queryid_to_stratbase_ref_, $prefix_path_to_remove_ ) = @_;

  my $strategy_filename_ = File::Spec->rel2abs ($t_strategy_filename_);

# Get the directory name - some changes will be made to this for the remote location
  my $directory_name_ = dirname ($strategy_filename_);
  my $t_strategy_filename_base_ = basename ($strategy_filename_);

  my $tmp_dir_ = $HOME_DIR."/isptemp/"; 
  `mkdir -p $tmp_dir_`;
  my $tmp_strategy_filename_ = $tmp_dir_.$t_strategy_filename_base_;

  my $dest_strat_file_name_ = ChangePaths ( $strategy_filename_, $user_, $prefix_path_to_remove_ )."_".$strat_id_ ; 

# First create a temporary copy of the file with the strat id modified
  open FSTRATF, "> $tmp_strategy_filename_" or PrintStacktraceAndDie ( "Could not open $tmp_strategy_filename_ for writing\n" );

  open ORIGSTRATFILEHANDLE, "< $strategy_filename_ " or PrintStacktraceAndDie ( "Could not open $strategy_filename_\n" );
  while ( my $thisline_ = <ORIGSTRATFILEHANDLE> ) 
  {
    my @t_words_ = split ( ' ', $thisline_ );
    if ( $#t_words_ >= 7 )
    { 
      my $model_file_name_ = $t_words_[3] ;
      if ( substr($t_words_[4],0,5) eq "ROLL:") {
        $t_words_[4] = GetRollParams($t_words_[4], "TODAY");
      }
      my $params_file_name_ = $t_words_[4] ;

      my $dest_model_file_name_ = ChangePathsModels ( $t_words_[3] ) ; 
      $dest_model_file_name_ = CopyFileCreateDir ( $model_file_name_, $dest_model_file_name_, $user_, $prod_machine_ip_ );

      my $dest_param_file_name_ = ChangePathsParams ( $t_words_[4] ) ;
      $dest_param_file_name_ = CopyFileCreateDir ( $params_file_name_, $dest_param_file_name_, $user_, $prod_machine_ip_ );

      $t_words_[3] = $dest_model_file_name_ ;
      $t_words_[4] = $dest_param_file_name_ ;
      $t_words_[7] = $strat_id_ ;
      print FSTRATF join ( ' ', @t_words_ );
      if ( defined $queryid_to_stratbase_ref_ && exists $$queryid_to_stratbase_ref_{ $strat_id_ } ) {
        print FSTRATF " ".$$queryid_to_stratbase_ref_{ $strat_id_ }."\n";
      } else {
        print FSTRATF " ".$t_strategy_filename_base_."\n";
      }
      $strat_id_ ++;
    }
  }
  close ORIGSTRATFILEHANDLE ;

  close FSTRATF ;

  $dest_strat_file_name_ = CopyFileCreateDir ( $tmp_strategy_filename_, $dest_strat_file_name_, $user_, $prod_machine_ip_ );
  `rm -f $tmp_strategy_filename_`;
  return $dest_strat_file_name_;
}

1;
