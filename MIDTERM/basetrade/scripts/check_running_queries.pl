#!/usr/bin/perl
use strict;
use warnings;

my $tradeinit_pid_dir_ = "/spare/local/logs/tradelogs/PID_TEXEC_DIR";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ($yyyymmdd_);

use File::Basename;

my %running_tradeinit_pids_ = ();
{ # List running tradeinit pids.
    my @running_tradeinits_ = `ps -e | grep tradeinit | grep -v grep`;

    for (my $tradeinit_no_ = 0; $tradeinit_no_ <= $#running_tradeinits_; $tradeinit_no_++) {
	my @tradeinit_line_fields_ = split (' ', $running_tradeinits_[$tradeinit_no_]);

	my $t_pid_ = $tradeinit_line_fields_[0]; chomp ($t_pid_);
	$running_tradeinit_pids_{$t_pid_} = 1;
    }
}

sleep (30); # Wait, in case query was just killed.

my %query_id_to_pid_ = ();
{ # Populate the query_id_to_pid map.
    my $pid_file_;
    foreach $pid_file_ ( <$tradeinit_pid_dir_/*> ) {
	open PID_FILE_HANDLE, "< $pid_file_" or die "Could not open $pid_file_\n";
	my @pid_file_lines_ = <PID_FILE_HANDLE>;
	close PID_FILE_HANDLE;

	my $t_pid_ = $pid_file_lines_[0]; chomp ($t_pid_);

	my $pid_file_ = basename ($pid_file_); chomp ($pid_file_);

	my @pid_file_name_words_ = split ('_', $pid_file_);
	my $t_query_id_ = $pid_file_name_words_[1]; chomp ($t_query_id_);

	$query_id_to_pid_{$t_query_id_} = $t_pid_;
    }
}

{ # Compare lists.
    my $exception_bool_ = 0;
    my $random_string_ = `date +\"%N\"`; chomp ($random_string_);
    my $tmp_email_file_name_ = "/tmp/tmp_email_file.".$random_string_;
    open TMP_EMAIL_FILE_HANDLE, "> $tmp_email_file_name_" or die "Could not open $tmp_email_file_name_\n";

    my $hostname_ = `hostname`; chomp ($hostname_);

    foreach my $query_id_ (keys %query_id_to_pid_) {
	my $t_pid_ = $query_id_to_pid_{$query_id_};

	if (! (exists $running_tradeinit_pids_{$t_pid_})) {
	    my $alert_string_ = "DEAD QUERY | PID $t_pid_ | QID $query_id_ | HOST $hostname_ | DATE $yyyymmdd_ |\n";

	    print TMP_EMAIL_FILE_HANDLE $alert_string_;

	    { # Send alert as well.
		my $HOME = $ENV{'HOME'};
		my $alert_script_ = $HOME."/"."LiveExec/scripts/sendAlert.sh";

		# Use a slightly smaller alert string for voice alert.
		$alert_string_ = "DEAD QUERY ID $query_id_ HOST $hostname_\n";

		`sh $alert_script_ $alert_string_`;
	    }

	    $exception_bool_ = 1;
	}
    }

    close TMP_EMAIL_FILE_HANDLE;

    if ($exception_bool_ == 1) {
	`/bin/mail -s \"Dead Query on $hostname_\" \"nseall@tworoads.co.in\" < $tmp_email_file_name_`;
    }

    `rm -f $tmp_email_file_name_`;
}
exit (0);
