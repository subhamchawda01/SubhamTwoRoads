#!/usr/bin/perl

# \file ModelScripts/get_ind_stats_for_ilist.pl
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
use feature "switch";
use List::Util qw/max min/;


sub ExecuteCommands;
sub RunDatagenForModel;
sub SanityCheckInputArguments;
sub GetDatesVec;
sub ProcessOutput;

#Variables for exec location
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $SPARE_HOME = "/spare/local/".$USER."/";
my $REPO = "basetrade";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $WF_SCRIPTS_DIR = $HOME_DIR."/".$REPO."/walkforward";

require "$GENPERLLIB_DIR/s3_utils.pl"; # S3PutFilePreservePath
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult # IsDateHoliday
require "$GENPERLLIB_DIR/get_files_pending_sim.pl"; # GetFilesPendingSim
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/make_combinable_strat_vecs_from_list.pl"; # MakeCombinableStratVecsFromList
require "$GENPERLLIB_DIR/make_combinable_strat_vecs_from_dir.pl"; # MakeCombinableStratVecsFromDir
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; # GetDatesFromStartDate
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; # MakeStratVecFromDir
require "$GENPERLLIB_DIR/filter_combinable_strategies_for_date.pl"; # FilterCombinableStrategiesForDate 
require "$GENPERLLIB_DIR/make_strat_vec_from_list_matchbase.pl"; #MakeStratVecFromListMatchBase

my $USAGE = "$0 shortcode model_filename trading_start_yyyymmdd trading_end_yyyymmdd start_hhmm end_hhmm work_dir [dates_file]";

if ($#ARGV < 6) {
    print $USAGE."\n";
    exit ( 0 );
}

#Initialize Arguments

my $shortcode_ = $ARGV[0];
my $modelfilename_ = $ARGV[1];

my $num_indicators_ = `cat $modelfilename_ | grep \"INDICATOR \" | grep -v \"^#\" | wc -l`;
chomp($num_indicators_);

my $trading_start_yyyymmdd_ = max ( 20110901, $ARGV[2]);
my $trading_end_yyyymmdd_ = $ARGV[3];

my $start_hhmm_ = $ARGV[4];
my $end_hhmm_ = $ARGV[5];

my $work_dir_ = $ARGV[6]."/IndicatorStats";

my $DATELIST_FILE = "";
if ($#ARGV > 6) {
    $DATELIST_FILE = $ARGV[7];
}

my $USE_DISTRIBUTED = 1;
my $DEBUG = 0;
my $queue_mode_ = "manual";  # default

#Global Variables

#Variables for distributed (celery) version
my $DISTRIBUTED_EXECUTOR = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/run_my_job.py";
my $DISTRIBUTED_STATUS_SCRIPT = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/view_job_status.py";
my $DISTRIBUTED_REVOKE_SCRIPT = "/home/dvctrader/dvccode/scripts/datainfra/celeryFiles/celeryClient/celeryScripts/revoke_my_job.py";

my $SHARED_LOCATION = "/media/shared/ephemeral21";

my $DATAGEN_EXEC = "/home/dvctrader/LiveExec/bin/datagen";

#Vectors for intermediate commands
my @independent_parallel_commands_ = ( );
my $main_log_file_handle_;
my $pnl_stats_result_file_handle_;

my %QUEUES_MAP = ( );
$QUEUES_MAP{"airflow"} = [ "autoscalegroup", "fast" ];
$QUEUES_MAP{"manual"} = [ "autoscalegroupmanual", "slow", "duration" ];

if ($queue_mode_ ne "airflow" && $queue_mode_ ne "manual") {
    print "Error: QMODE has to be either airflow or manual\n";
    exit ( 1 );
}

#Checks global results folder is shared folder incase of distributed version
#SanityCheckInputArguments( );

my $unique_gsm_id_ = `date +%N`;
chomp ( $unique_gsm_id_ );
if (-d $work_dir_) { `rm -rf $work_dir_`; }
`mkdir -p $work_dir_`;

my $main_log_file_ = $work_dir_."/main_log_file.txt";
print "Log File: $main_log_file_ \n";
my $pnl_stats_result_file_ = $work_dir_."/pnl_stats";

$main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

$pnl_stats_result_file_handle_ = FileHandle->new;
$pnl_stats_result_file_handle_->open ( "> $pnl_stats_result_file_ " ) or PrintStacktraceAndDie ( "Could not open $pnl_stats_result_file_ for writing\n" );
$pnl_stats_result_file_handle_->autoflush(1);

RunDatagenForModel ( );

`chmod -R go+w $work_dir_ &> /dev/null`;
ExecuteCommands ( );
ProcessOutput ( );
exit( 0 );

###### subs ######

sub ExecuteCommands
{
    if ($USE_DISTRIBUTED) {
        if ($#independent_parallel_commands_ < 0) { return; }

        my $commands_file_id_ = "commands_".`date +%N`;
        chomp( $commands_file_id_ );
        my $commands_file_ = $work_dir_."/".$commands_file_id_;
        my $commands_file_handle_ = FileHandle->new;
        $commands_file_handle_->open ( "> $commands_file_ " ) or PrintStacktraceAndDie ( "Could not open $commands_file_ for writing\n" );
        print $commands_file_handle_ "$_ \n" for @independent_parallel_commands_;
        close $commands_file_handle_;
        my $dist_exec_cmd_ = "$DISTRIBUTED_EXECUTOR -n dvctrader -m 1 -f $commands_file_ -s 1";

        print "Executing Command: ".$dist_exec_cmd_."\n";
        print $main_log_file_handle_ "Executing Command: ".$dist_exec_cmd_."\n";

        my $groupid_;
        foreach my $queue_ (@{$QUEUES_MAP{$queue_mode_}}) {
            my $this_dist_exec_cmd_ = $dist_exec_cmd_." -q $queue_";
            print $this_dist_exec_cmd_."\n";
            my @output_lines_ = `$this_dist_exec_cmd_ 2>/dev/null | grep -v "Log_group ID"`;
            chomp ( @output_lines_ );
            chomp( @output_lines_ );
            print $main_log_file_handle_ "$_ \n" for @output_lines_;
            print "$_ \n" for @output_lines_;

            my ($groupid_line_) = grep { $_ =~ /Group ID:/ } @output_lines_;

            if (!defined $groupid_line_) {
                my ($queue_full_line_) = grep { $_ =~ /Queue Full!/ } @output_lines_;
            }
            else {
                $groupid_ = (split /\s+/, $groupid_line_)[2];
                last;
            }
        }

        if (!defined $groupid_) {
            if ($queue_mode_ eq "airflow") {
                print "Error: All distributed queues full!!\n Scheduling to Non-distributed!!\n\n";
                print $main_log_file_handle_ "Error: All distributed queues full!!\n Scheduling to Non-distributed!!\n\n";
                $USE_DISTRIBUTED = 0;
            }
            else {
                print STDERR "Error: All distributed queues full!! Could not schedule the tasks!!\n";
                print $main_log_file_handle_ "Error: All distributed queues full!! Could not schedule the tasks!!\n";
                $USE_DISTRIBUTED = 0;
            }
        }
        else {
            my @task_ids_ = `$DISTRIBUTED_STATUS_SCRIPT -g $groupid_ | grep ^ID: | awk '{print \$2}'`;
            chomp ( @task_ids_ );
            print "Task_id: ".$_."\n" foreach @task_ids_;
            my %task_to_status_ = map { $_ => 0 } @task_ids_;

            my $schedule_seconds_ = `date +%s`;
            chomp ( $schedule_seconds_ );
            my $last_update_seconds_ = $schedule_seconds_;
            my $last_pending_count_ = scalar @task_ids_;

            my $POLLING_PERIOD = 20; # repolling after every 120 seconds
            my $REVOKE_TIMEOUT = 2400; 

            while (1) {
                sleep $POLLING_PERIOD;
                foreach my $taskid_ (@task_ids_) {
                    my $task_state_ = `$DISTRIBUTED_STATUS_SCRIPT -i $taskid_ 2>/dev/null | grep STATE: | awk '{print \$2}'`;
                    chomp ( $task_state_ );
                    if ($task_state_ eq "SUCCESS") {
                        my $task_logfile_ = `$DISTRIBUTED_STATUS_SCRIPT -i $taskid_ 2>/dev/null | grep LOGFILE: | awk '{print \$2}'`;
                        chomp ( $task_logfile_ );
                        if (-f $task_logfile_) {
                            my @output_lines_ = `cat $task_logfile_`;
                            chomp ( @output_lines_ );
                        }
                        $task_to_status_{ $taskid_ } = 1;
                    }
                    elsif ($task_state_ eq "FAILURE") {
                        my @t_output_lines_ = `$DISTRIBUTED_STATUS_SCRIPT -i $taskid_ 2>/dev/null`;
                        my @output_lines_ = ( );
                        my $lflag_ = 0;
                        foreach my $line_ (@t_output_lines_) {
                            if ($line_ =~ /STDOUT|STDERR/) {
                                $lflag_ = 1;
                            }
                            elsif ($line_ =~ /None|ID|STATE|STATE|RETURN|TIME/) {
                                $lflag_ = 0;
                            }
                            elsif ($lflag_ == 1) {
                                push ( @output_lines_, $line_ );
                            }
                        }
                        chomp ( @output_lines_ );
                        $task_to_status_{ $taskid_ } = 2;
                    }
                    elsif ($task_state_ eq "REVOKED") {
                        print $main_log_file_handle_ "Task_id: $taskid_  REVOKED\n\n";
                        $task_to_status_{ $taskid_ } = 2;
                    }
                }

                my $update_seconds_ = `date +%s`;
                chomp ( $update_seconds_ );
                my $pending_count_ = grep { $task_to_status_{$_} == 0 } @task_ids_;

                if ($update_seconds_ - $last_update_seconds_ > 600 || abs($last_pending_count_ - $pending_count_) > 0) {
                    my $hhmmss_ = `date`;
                    chomp ( $hhmmss_ );
                    print $hhmmss_." No. of pending tasks: ".$pending_count_."\n";
                    print $main_log_file_handle_ $hhmmss_." No. of pending tasks: ".$pending_count_."\n";
                }
                last if $pending_count_ == 0;

                if ($update_seconds_ - $schedule_seconds_ > $REVOKE_TIMEOUT) {
                    print "Celery Schedule TIMEOUT: ".($update_seconds_ - $schedule_seconds_)." seconds since tasks schedule\n";
                    print $main_log_file_handle_ "Celery Schedule TIMEOUT: ".($update_seconds_ - $schedule_seconds_)." seconds since tasks schedule\n";
                    my $revoke_cmd_ = "$DISTRIBUTED_REVOKE_SCRIPT -g $groupid_ 2>/dev/null";
                    my @output_lines_ = `$revoke_cmd_`;
                    chomp (  @output_lines_ );
                    print $revoke_cmd_."\n".join("\n", @output_lines_)."\n";
                    print $main_log_file_handle_ $revoke_cmd_."\n".join("\n", @output_lines_)."\n";
                    $USE_DISTRIBUTED = 0;
                }
            }
        }
    }
    if (!$USE_DISTRIBUTED) {
        foreach my $command (@independent_parallel_commands_) {
            my @output_lines_ = `$command`;
            chomp ( @output_lines_ );
            my $return_val_ = $?;
        }
    }
}

sub ProcessOutput
{
    my @mean_vec_ = ();
    my @sq_mean_vec_ = ();
    my $sum_instances_ = 0;
    my @stdev_vec_ = ();
    my @dates_vec_ = GetDatesVec();
    my @cov_mat_ = ();
    my @prod_mean_mat_ = ();

    for (my $i = 0; $i < $num_indicators_; $i++)
    {
        push(@mean_vec_, 0);
        push(@sq_mean_vec_, 0);
        push(@stdev_vec_, 0);
        my @temp_cov_mat_ = ();
        push(@temp_cov_mat_, 0) foreach 1..$num_indicators_;
        push(@cov_mat_, \@temp_cov_mat_);
        push(@prod_mean_mat_, \@temp_cov_mat_);
    }

    foreach my $tradingdate_ (@dates_vec_) {
        my $this_day_ind_stats_file_ = $work_dir_."/".$tradingdate_."/ind_stats";
        if (-e $this_day_ind_stats_file_) {
            open IND_STATS,
                "< $this_day_ind_stats_file_" or PrintStacktraceAndDie ( "Could not open indicator_stats_filename_ $this_day_ind_stats_file_ for reading\n" );
            my $line_ = 0;
            while ( my $iline_ = <IND_STATS> )
            {
                my @iwords_ = split ' ', $iline_;
                if ($#iwords_ < 2)
                {
                    print "Malformed Line in $this_day_ind_stats_file_\n";
                    break;
                } elsif ($iwords_[0] eq "PNL_BASED_STATS") {
                    $mean_vec_[$line_] = $mean_vec_[$line_] * ( $sum_instances_ / ( $sum_instances_ + $iwords_[1] ) ) + $iwords_[2] * ( $iwords_[1] / ( $sum_instances_ + $iwords_[1] ) );
                    $sq_mean_vec_[$line_] = $sq_mean_vec_[$line_] * ( $sum_instances_ / ( $sum_instances_ + $iwords_[1] ) ) + $iwords_[3] * ( $iwords_[1] / ( $sum_instances_ + $iwords_[1] ) );

                    for (my $column_ = 0; $column_ < $num_indicators_; $column_++) {
                        my $c = $prod_mean_mat_[$line_][$column_];
                        $prod_mean_mat_[$line_][$column_] = $c * ($sum_instances_ / ( $sum_instances_ + $iwords_[1] ) ) + $iwords_[4 + $column_] * ( $iwords_[1] / ( $sum_instances_ + $iwords_[1] ) )
                    }

                    if ($line_ == ( $num_indicators_ - 1 )) {
                        $sum_instances_ = $sum_instances_ + $iwords_[1];
                    }
                }
                $line_++;
            }
        } else {
            print "File does not exist $this_day_ind_stats_file_\n";
        }
    }

    for (my $i = 0; $i <= $#mean_vec_; $i++)
    {
        if ($sq_mean_vec_[$i] - $mean_vec_[$i] * $mean_vec_[$i] < 0) {
            $stdev_vec_[$i] = 0;
        }
        else {
            $stdev_vec_[$i] = sqrt ( $sq_mean_vec_[$i] - $mean_vec_[$i] * $mean_vec_[$i] );
        }
        for (my $j = 0; $j <= $#mean_vec_; $j++)
        {
            $cov_mat_[$i][$j] = $prod_mean_mat_[$i][$j] - $mean_vec_[$i] * $mean_vec_[$j];
        }
    }
    print $pnl_stats_result_file_handle_ "STDEV @stdev_vec_\n";
    print $pnl_stats_result_file_handle_ "COVARIANCE MATRIX\n";
    print $pnl_stats_result_file_handle_ join(" ", @$_)."\n" foreach @cov_mat_;
}

sub RunDatagenForModel
{
    my @dates_vec_ = GetDatesVec ( );

    foreach my $tradingdate_ (@dates_vec_) {
        my $this_day_work_dir_ = $work_dir_."/".$tradingdate_;
        if (!-d $this_day_work_dir_) { `mkdir -p $this_day_work_dir_`; }
        my $this_day_ind_stats_file_ = $this_day_work_dir_."/ind_stats";
        my $exec_cmd_ = "$DATAGEN_EXEC $modelfilename_ $tradingdate_ $start_hhmm_ $end_hhmm_ 22222 PNL_BASED_STATS 10000 0 0 0 > $this_day_ind_stats_file_";
        push (@independent_parallel_commands_, $exec_cmd_);
    }
}

sub GetDatesVec
{
    my @dates_vec_ = GetDatesFromStartDate ( $shortcode_, $trading_start_yyyymmdd_, $trading_end_yyyymmdd_ );
    if ( defined $DATELIST_FILE && $DATELIST_FILE ne "" ) {
        if ( -f $DATELIST_FILE ) {
            open DATELIST_HANDLE, "< $DATELIST_FILE" or PrintStacktraceAndDie ( "Could not open $DATELIST_FILE for reading" );
            my @file_dates_vec_ = <DATELIST_HANDLE>;
            chomp ( @file_dates_vec_ );
            close DATELIST_HANDLE;

            my %file_dates_map_ = map { $_=>1 } @file_dates_vec_;
            @dates_vec_ = grep { defined $file_dates_map_{ $_ } } @dates_vec_;
        }
    }
    return @dates_vec_;
}

sub SanityCheckInputArguments
{
    if (!( -d $work_dir_ ))
    {
        print STDERR "$work_dir_ isn't a directory";
        exit( 0 );
    }

    if (!( $trading_start_yyyymmdd_ ))
    {
        print STDERR "TRADING_START_YYYYMMDD missing\n";
        exit ( 0 );
    }

    if (!( ValidDate ( $trading_start_yyyymmdd_ ) ))
    {
        print STDERR "TRADING_START_YYYYMMDD not Valid\n";
        exit ( 0 );
    }

    if (!( $trading_end_yyyymmdd_ ))
    {
        print STDERR "TRADING_END_YYYYMMDD missing\n";
        exit ( 0 );
    }

    if (!( ValidDate ( $trading_end_yyyymmdd_ ) ))
    {
        print STDERR "TRADING_END_YYYYMMDD not Valid\n";
        exit ( 0 );
    }
}
