# \file GenPerlLib/s3_utils.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use List::Util qw/max min/; # for max
use Math::Complex ; # sqrt
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $HOME_DIR = $ENV { 'HOME' };
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";

my $S3_CMD_EXEC = "/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd";

my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{ # AWS
    $S3_CMD_EXEC = "/mnt/sdf/s3cmd/s3cmd-1.5.0-alpha1/s3cmd";
}

my $S3_BUCKET_PREFIX = "s3://s3dvc";

sub S3GetFile
{
    my ( $full_file_path_ , $local_file_path_ ) = @_;

    if ( index ( $full_file_path_ , "/NAS1/" ) < 0 )
    {
	PrintStacktraceAndDie ( "/NAS1/ should be part of the bucket path name" );
    }

    my $dirname_ = dirname ( $local_file_path_ );
    `mkdir -p $dirname_`; # create local enclosing directory if not exists

    if ( -e $local_file_path_ )
    { # remove local file if it already exists , necessary because 's3cmd get' seems buggy.
	  `rm -f $local_file_path_`;
    }

    my $exec_cmd_ = $S3_CMD_EXEC." get --no-progress ".$S3_BUCKET_PREFIX.$full_file_path_." ".$local_file_path_." 2>&1";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    if ( -e $local_file_path_ )
    { # on success, will return 1
	  return 1;
    }
    else
    {
      return 0;
    }
}

sub S3FetchFileAndReturnLocalPath
{
    my ( $full_file_path_ ) = @_;
    my $local_file_path_ = "";

    if ( index ( $full_file_path_ , "/NAS1/" ) < 0 )
    {
	PrintStacktraceAndDie ( "/NAS1/ should be part of the bucket path name" );
    }

    my $dirname_ = dirname ( $full_file_path_ );
    `mkdir -p $dirname_`; # create local enclosing directory if not exists

    if ( -e $full_file_path_ )
    { # remove local file if it already exists , necessary because 's3cmd get' seems buggy.
	`rm -f $full_file_path_`;
    }

    # exclusively lock before syncing from S3.
    my $lock_file_ = $HOME_DIR."/locks/";
    `mkdir -p $lock_file_`;
    $lock_file_ = $lock_file_.basename ( $full_file_path_ ).".lock";

    open ( my $file_handle_ , ">>" , $lock_file_ ) or PrintStacktraceAndDie ( "Could not create lock file $lock_file_" );
    flock ( $file_handle_ , LOCK_EX ) or PrintStacktraceAndDie ( "LOCK_EX flock failed on $lock_file_" );

    my $exec_cmd_ = $S3_CMD_EXEC." get --no-progress ".$S3_BUCKET_PREFIX.$full_file_path_." ".$full_file_path_." 2>&1";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    close ( $file_handle_ ); # release lock & close lock file.
    `rm -f $lock_file_`;

    if ( -e $full_file_path_ )
    { # on failure , will not create a local file.
	$local_file_path_ = $full_file_path_;
    }

    return $local_file_path_;
}

sub S3FetchDirectoryAndReturnLocalPath
{
    my ( $full_dir_path_ ) = @_;
    my $local_dir_path_ = "";

    if ( index ( $full_dir_path_ , "/NAS1/" ) < 0 )
    {
	PrintStacktraceAndDie ( "/NAS1/ should be part of the bucket path name" );
    }

    # make sure provided directory name does not end with a '/' , s3cmd will fail.
    if ( substr ( $full_dir_path_ , -1 ) eq "/" )
    {
	my $t_full_dir_path_ = substr ( $full_dir_path_ , 0 , -1 );
	$full_dir_path_ = $t_full_dir_path_;
    }

    # Create enclosing directory if does not already exist
    my $exec_cmd_ = "mkdir -p ".dirname ( $full_dir_path_ );
    `$exec_cmd_`;

    # exclusively lock before syncing from S3.
    my $lock_file_ = $HOME_DIR."/locks/";
    `mkdir -p $lock_file_`;
    $lock_file_ = $lock_file_.basename ( $full_dir_path_ ).".lock";

    open ( my $file_handle_ , ">>" , $lock_file_ ) or PrintStacktraceAndDie ( "Could not create lock file $lock_file_" );
    flock ( $file_handle_ , LOCK_EX ) or PrintStacktraceAndDie ( "LOCK_EX flock failed on $lock_file_" );

    $exec_cmd_ = $S3_CMD_EXEC." sync --no-progress --check-md5 ".$S3_BUCKET_PREFIX.$full_dir_path_." ".dirname ( $full_dir_path_ )." 2>&1";
    print $exec_cmd_."\n";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    close ( $file_handle_ ); # release lock & close lock file.
    `rm -f $lock_file_`;

    if ( -d $full_dir_path_ )
    { # on failure , will not create a local file.
	$local_dir_path_ = $full_dir_path_;
    }

    return $local_dir_path_;
}

sub S3PutFile
{
    my ( $local_file_ , $s3_dir_location_ ) = @_;

    if ( index ( $s3_dir_location_ , "/NAS1/" ) < 0 )
    {
	PrintStacktraceAndDie ( "/NAS1/ should be part of the bucket path name" );
    }

    # make sure provided local file name does not end with a '/' , s3cmd will fail.
    if ( substr ( $local_file_ , -1 ) eq "/" )
    {
	my $t_local_file_ = substr ( $local_file_ , 0 , -1 );
	$local_file_ = $t_local_file_;
    }
    # make sure provided s3 directory name ends with a '/' , s3cmd will fail.
    if ( substr ( $s3_dir_location_ , -1 ) ne "/" )
    {
	$s3_dir_location_ = $s3_dir_location_."/";
    }

    my $exec_cmd_ = $S3_CMD_EXEC." put --no-progress ".$local_file_." ".$S3_BUCKET_PREFIX.$s3_dir_location_." 2>&1";
    print $exec_cmd_."\n";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    return;
}

sub S3PutFilePreservePath
{
    my ( $local_file_ , $s3_file_full_path_ ) = @_;

    if ( index ( $s3_file_full_path_ , "/NAS1/" ) < 0 )
    {
	PrintStacktraceAndDie ( "/NAS1/ should be part of the bucket path name" );
    }

    # make sure provided local file name does not end with a '/' , s3cmd will fail.
    if ( substr ( $local_file_ , -1 ) eq "/" )
    {
	my $t_local_file_ = substr ( $local_file_ , 0 , -1 );
	$local_file_ = $t_local_file_;
    }

    # make sure provided s3 full file name does not end with a '/' , s3cmd will fail.
    if ( substr ( $s3_file_full_path_ , -1 ) eq "/" )
    {
	my $t_s3_file_full_path_ = substr ( $s3_file_full_path_ , 0 , -1 );
	$s3_file_full_path_ = $t_s3_file_full_path_;
    }

    my $exec_cmd_ = $S3_CMD_EXEC." put --no-progress ".$local_file_." ".$S3_BUCKET_PREFIX.$s3_file_full_path_." 2>&1";
    print $exec_cmd_."\n";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    return;
}

# S3PutDirectory ( /home/dvctrader/indicatorwork/RI_0_0.5_fsg1_na_e3_US_MORN_OfflineMixMMS_OfflineMixMMS , /NAS1/indicatorwork )
# upload: /home/dvctrader/indicatorwork/RI_0_0.5_fsg1_na_e3_US_MORN_OfflineMixMMS_OfflineMixMMS/indicator_corr_record_file.txt 
# -> s3://s3dvc/NAS1/indicatorwork/RI_0_0.5_fsg1_na_e3_US_MORN_OfflineMixMMS_OfflineMixMMS/indicator_corr_record_file.txt
sub S3PutDirectory
{
    my ( $local_dir_ , $s3_dir_location_ ) = @_;

    if ( index ( $s3_dir_location_ , "/NAS1/" ) < 0 )
    {
	PrintStacktraceAndDie ( "/NAS1/ should be part of the bucket path name" );
    }

    # make sure provided local directory name does not end with a '/' , s3cmd will fail.
    if ( substr ( $local_dir_ , -1 ) eq "/" )
    {
	my $t_local_dir_ = substr ( $local_dir_ , 0 , -1 );
	$local_dir_ = $t_local_dir_;
    }
    # make sure provided s3 directory name ends with a '/' , s3cmd will fail.
    if ( substr ( $s3_dir_location_ , -1 ) ne "/" )
    {
	$s3_dir_location_ = $s3_dir_location_."/";
    }

    my $exec_cmd_ = $S3_CMD_EXEC." put --no-progress --recursive ".$local_dir_." ".$S3_BUCKET_PREFIX.$s3_dir_location_." 2>&1";
#    print $exec_cmd_."\n";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    return;
}

1;
