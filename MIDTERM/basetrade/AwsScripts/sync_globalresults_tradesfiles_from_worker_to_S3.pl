#!/usr/bin/perl

# \file AwsScripts/sync_globalresults_tradesfiles_from_worker_to_S3.pl
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

my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{ # AWS
    $S3_CMD_EXEC = "/mnt/sdf/s3cmd/s3cmd-1.5.0-alpha1/s3cmd";
}

my $S3_BUCKET_PREFIX = "s3://s3dvc";

my $MAX_S3_SYNCS_TO_RUN_IN_PARALLEL = 20;

my $exec_cmd_ = "";
my @exec_cmd_output_ = ( );

$exec_cmd_ = "find /NAS1/ec2_globalresults_trades -type f -amin +1";
print "$exec_cmd_\n";
@exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

for ( my $t_line_ = 0 ; $t_line_ <= $#exec_cmd_output_ ; )
{
    my @pids_to_poll_this_run_ = ( );
    my @files_synced_this_run_ = ( );

    for ( my $command_index_ = 0 ; $command_index_ <= $MAX_S3_SYNCS_TO_RUN_IN_PARALLEL && $t_line_ <= $#exec_cmd_output_ ; $command_index_ ++ , $t_line_ ++ )
    {
	my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ $t_line_ ] ); chomp ( @exec_cmd_output_words_ );

	if ( $#exec_cmd_output_words_ >= 0 )
	{ # /NAS1/ec2_globalresults_trades/UB_0/2012/07/27/trades.20120727.w_lpm.120.3.strategy_ilist_UB_0_US_Sin_Sin_fv.J0_r_8_na_t3_20121119_20130306_EST_830_EST_1600_4000_2_0_fst1_FSVLR_2.0_0.01_0_0_0.7.tt_EST_930_EST_1600.pfi_3.gz/trades.20120727.10015.gz

	    my $trades_filename_ = $exec_cmd_output_words_ [ 0 ];

	    push ( @files_synced_this_run_ , $trades_filename_ );

	    $exec_cmd_ = $S3_CMD_EXEC." put --no-progress ".$trades_filename_." ".$S3_BUCKET_PREFIX.$trades_filename_." >/dev/null 2>&1 & echo \$!";
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

    foreach my $this_file_synced_ ( @files_synced_this_run_ )
    {
	`rm -f $this_file_synced_`;
    }
}

exit ( 0 );
