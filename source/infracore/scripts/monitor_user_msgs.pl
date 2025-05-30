#!/usr/bin/perl
use strict;
use warnings;

my $usage_ = "monitor_user_msgs.pl TRADERID1 INSTRUMENT START_UTC_HHMMSS TRADERID2S INSTRUMENT TART_UTC_HHMMSS ... ";

if ($#ARGV < 0) {
    print $usage_."\n";
    exit (0);
}

# Get the trader ids to observe with their respective start times.
my @trader_id_list_ = ();
my %start_utc_to_trader_id_ = ();
my %trader_id_to_is_running_ = ();
my %trader_id_to_instrument_ = ();

for (my $arg_ = 0; $arg_ <= $#ARGV; $arg_++) {
    my $t_trader_id_ = $ARGV[$arg_]; chomp ($t_trader_id_);
    push (@trader_id_list_, $t_trader_id_);

    $arg_++;
    my $t_instrument_ = $ARGV[$arg_]; chomp ($t_instrument_);
    $trader_id_to_instrument_{$t_trader_id_} = $t_instrument_;

    $arg_++;
    my $t_start_utc_ = $ARGV[$arg_]; chomp ($t_start_utc_);
    $start_utc_to_trader_id_{$t_start_utc_} = $t_trader_id_;
}

# Start the user_msg_logger for given trader ids.
my $user_msg_logger_exec_name_ = "user_msg_logger";
my $user_msg_logger_exec_ = "/home/sghosh/LiveExec/bin/".$user_msg_logger_exec_name_;

my @running_exec_ = `pgrep $user_msg_logger_exec_name_`; # Check for only 1 instance

if ($#running_exec_ >= 0 && length ($running_exec_[0]) > 0) {
    print "$user_msg_logger_exec_name_ already running. Not starting another instance\n";
} else {
    my $exec_cmd_ = $user_msg_logger_exec_;
#my $exec_cmd_ = "taskset -c 20,21,22,23 ".$user_msg_logger_exec_;
    for (my $trader_id_ = 0; $trader_id_ <= $#trader_id_list_; $trader_id_++) {
	my $t_trader_id_ = $trader_id_list_[$trader_id_];

	$exec_cmd_ = $exec_cmd_." ".$t_trader_id_;
    }
    $exec_cmd_ = $exec_cmd_." &";
    system ($exec_cmd_);
}

# Keep scanning the log file for user_msgs.
my $yyyymmdd_ = `date +%Y%m%d`; chomp ($yyyymmdd_);
my $log_file_name_ = "/home/sghosh/logs/alllogs/user_msg_logger.".$yyyymmdd_;

while (1) {
    foreach my $utc_time_ (sort {$a <=> $b} keys %start_utc_to_trader_id_) {
	my $trader_id_ = $start_utc_to_trader_id_{$utc_time_};

	my $current_utc_ = `date +%H%M%S`; chomp ($current_utc_);

	if ($current_utc_ < $utc_time_) { # This query should not be running yet.
	    next;
	}

	open USER_MSG_FILE, "< $log_file_name_" or die "Couldnot open $log_file_name_\n";
	my @user_msg_file_lines_ = <USER_MSG_FILE>;
	close USER_MSG_FILE;

	# Move backwards and look for calls to start trading.
	my $is_query_running_ = 0;
	for (my $line_no_ = $#user_msg_file_lines_; $line_no_ >= 0; $line_no_--) {
	    # Date HH:MM:SS [Security] [Trader_Id] [Control_Message_Code ..]
	    my @user_msg_words_ = split (' ', $user_msg_file_lines_[$line_no_]);

	    my $t_trader_id_ = $user_msg_words_[3]; chomp ($t_trader_id_);
	    my $control_message_ = $user_msg_words_[4]; chomp ($control_message_);

	    if (index ($t_trader_id_, $trader_id_) >= 0) {
		if (index ($control_message_, "kControlMessageCodeStartTrading") >= 0 ||
		    index ($control_message_, "kControlMessageCodeUnFreezeTrading") >= 0) {
		    $is_query_running_ = 1;
		    last;
		} elsif (index ($control_message_, "kControlMessageCodeFreezeTrading") >= 0 ||
			 index ($control_message_, "kControlMessageCodeGetflat") >= 0) {
		    $is_query_running_ = 0;
		    last;
		}
	    }
	}

	if ($is_query_running_ == 1) {
	    if (! exists $trader_id_to_is_running_{$trader_id_} || $trader_id_to_is_running_{$trader_id_} == 0) {
		print "$trader_id_to_instrument_{$trader_id_} Query $trader_id_ designated to start at $utc_time_ running at $current_utc_\n";

		$trader_id_to_is_running_{$trader_id_} = 1;
	    }
	} else {
	    if (! exists $trader_id_to_is_running_{$trader_id_} || $trader_id_to_is_running_{$trader_id_} == 1) {
		print "$trader_id_to_instrument_{$trader_id_} Query $trader_id_ designated to start at $utc_time_ not running at $current_utc_\n";

		$trader_id_to_is_running_{$trader_id_} = 0;
	    }
	}
    }

    sleep (60);
}
