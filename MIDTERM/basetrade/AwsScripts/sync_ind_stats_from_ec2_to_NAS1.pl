#!/usr/bin/perl

# \file AwsScripts/sync_ind_stats_from_ec2_to_NAS1.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib/";

require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # AllPIDSTerminated

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";

my $S3_CMD_EXEC = "/apps/s3cmd/s3cmd-1.5.0-alpha1/s3cmd";

my $NAS1_BUCKET = "s3://s3dvc/NAS1/";

my $MAX_S3_SYNCS_TO_RUN_IN_PARALLEL = 5;

my $s3_work_dir_ = $HOME_DIR."/s3_to_nas1_sync_work_dir";
`mkdir -p $s3_work_dir_`;

my $exec_cmd_ = "";
my @exec_cmd_output_ = ( );

$exec_cmd_ = $S3_CMD_EXEC." ls ".$NAS1_BUCKET."indicatorwork/ | grep DIR";
print $exec_cmd_."\n";

@exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

for ( my $t_line_ = 0 ; $t_line_ <= $#exec_cmd_output_ ; )
{
    my @pids_to_poll_this_run_ = ( );
    my @directories_synced_this_run_ = ( );

    # empty out files/directories from previous run(s)
    $exec_cmd_ = "rm -rf ".$s3_work_dir_."/*";
    print $exec_cmd_."\n";
    `$exec_cmd_`;

    for ( my $command_index_ = 0 ; $command_index_ <= $MAX_S3_SYNCS_TO_RUN_IN_PARALLEL && $t_line_ <= $#exec_cmd_output_ ; $command_index_ ++ , $t_line_ ++ )
    {
	my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ $t_line_ ] ); chomp ( @exec_cmd_output_words_ );

	if ( $#exec_cmd_output_words_ >= 1 && 
	     $exec_cmd_output_words_ [ 0 ] eq "DIR" )
	{ # DIR   s3://s3dvc/NAS1/indicatorwork/ZN_0_128_fst1_na_e3_EU_MORN_DAY_Midprice_OfflineMixMMS/
	    my $directory_name_ = $exec_cmd_output_words_ [ 1 ];

	    if ( substr ( $directory_name_ , -1 ) eq "/" )
	    { # remove trailing "/"
		my $t_directory_name_ = substr ( $directory_name_ , 0 , -1 );
		$directory_name_ = $t_directory_name_;
	    }

	    push ( @directories_synced_this_run_ , $s3_work_dir_."/".basename ( $directory_name_ ) );

	    $exec_cmd_ = $S3_CMD_EXEC." get --no-progress --recursive ".$directory_name_." ".$s3_work_dir_." >/dev/null 2>&1 & echo \$!";
	    print $exec_cmd_."\n";
	    my @sync_cmd_output_ = `$exec_cmd_`; chomp ( @sync_cmd_output_ );

	    if ( $#sync_cmd_output_ >= 0 )
	    {
		my @sync_cmd_output_words_ = split ( ' ' , $sync_cmd_output_ [ 0 ] );
		if ( $#sync_cmd_output_words_ >= 0 )
		{
		    my $t_pid_ = $sync_cmd_output_words_ [ 0 ];
		    $t_pid_ =~ s/^\s+|\s+$//g;

		    push ( @pids_to_poll_this_run_ , $t_pid_ );
		}
	    }
	}
    }

    while ( ! AllPIDSTerminated ( @pids_to_poll_this_run_ ) )
    { # wait till this batch finishes
	sleep ( 1 );
    }

    # sync directories to /NAS1
    foreach my $t_dir_ ( @directories_synced_this_run_ )
    {
	$exec_cmd_ = "rsync -avz ".$t_dir_." dvcinfra\@10.23.74.41:/apps/indicatorwork";
	print $exec_cmd_."\n";
	`$exec_cmd_`;

	#$exec_cmd_ = "ssh dvcinfra\@10.23.74.41 /home/dvcinfra/add_reading_permissions_to_given_directory_since_fs_messed_up.sh /apps/indicatorwork/".basename ( $t_dir_ );
	#print $exec_cmd_."\n";
	#`$exec_cmd_`;
    }
}

exit ( 0 );
