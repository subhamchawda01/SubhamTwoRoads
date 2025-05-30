#!/usr/bin/perl
use strict;
use warnings;

my $usage_ = "monitor_affinity.pl PID1 PID2 PID3 PID4 PID5 .... ";

if ($#ARGV < 0) {
    print $usage_."\n";
    exit (0);
}

my @pid_list_ = ();
for (my $arg_ = 0; $arg_ <= $#ARGV; $arg_++) {
    my $t_pid_ = $ARGV[$arg_]; chomp ($t_pid_);

    push (@pid_list_, $t_pid_);
}

my $affin_exec_ = $ENV{'HOME'}."/LiveExec/bin/cpu_affinity_mgr";

while (1) {
    {
	system ("/usr/bin/clear");
	my $date_ = `date`; chomp ($date_);
	print "$date_\n";
    }

    my %core_to_pid_ = ();
    for (my $pid_no_ = 0; $pid_no_ <= $#pid_list_; $pid_no_++) { 
        # Run the affinity view exec and detect conflicts.
	my @affin_output_ = `taskset -c 20,21,22,23 $affin_exec_ VIEW $pid_list_[$pid_no_]`;

	for (my $output_line_no_ = 0; $output_line_no_ <= $#affin_output_; $output_line_no_++) {
	    if (index ($affin_output_[$output_line_no_], "PID") >=0 && index ($affin_output_[$output_line_no_], "CORE") >=0) {
		# Output : "VIEW : PID : 17081 CORE # 6"
		my @output_words_ = split (' ', $affin_output_[$output_line_no_]);

		if ($#output_words_ < 7) {
		    print "Error obtaining affinity for pid : $pid_list_[$pid_no_]\n";
		    exit (0);
		}

		my $t_pid_ = $output_words_[4]; chomp ($t_pid_);
		my $t_core_ = $output_words_[7]; chomp ($t_core_);

		if ($t_pid_ != $pid_list_[$pid_no_]) {
		    print "Error obtaining affinity for pid : $pid_list_[$pid_no_] . Got $t_pid_\n";
		    exit (0);
		}

		if (exists $core_to_pid_{$t_core_}) {
		    print "Conflict on core : $t_core_. Found pids : $core_to_pid_{$t_core_} & $t_pid_\n";
		    exit (0);
		}

		$core_to_pid_{$t_core_} = $t_pid_;
	    }
	}
    }

    foreach my $core_ (sort {$a <=> $b} keys %core_to_pid_) {
	print "$core_ => $core_to_pid_{$core_}\n";
    }

    sleep (1);
}
