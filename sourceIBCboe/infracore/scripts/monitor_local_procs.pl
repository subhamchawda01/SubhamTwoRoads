#!/usr/bin/perl

my $local_ips = ` /sbin/ifconfig | grep "inet addr" | sed 's/^.*addr:// ; s/ .*/ /'  | tr -d '\n' ` ;

my @procs_to_check = split(/\n/, `cat /spare/local/files/procs_to_monitor.txt | sed '/^#/d; /^[ \t]*\$/d; s/[ \t][ \t]*/ /g'`);

my $curr_time = `date '+%M'` + 60 * `date '+%k'`; 

for my $proc_to_check (@procs_to_check) {
	
	@tmp_arr = split(" ", $proc_to_check);
	my $trd_server = $tmp_arr[0];
	if ( $local_ips =~ /$trd_server/ ) #is a local process
	{
		my $st_time = (split(":", $tmp_arr[1]))[0]  + 60 * (split(":", $tmp_arr[1]))[1];
		my $e_time = (split(":", $tmp_arr[2]))[0]  + 60 * (split(":", $tmp_arr[2]))[1];
		my $r_exp = $tmp_arr[3];
		
		$ps_cmd = " ps -eaf | grep \"".$r_exp."\" | grep -v grep | wc -l";
		my$ret_val = `$ps_cmd` ;
		chomp($ret_val);
		
		my $expected_to_run = 1;
		if($curr_time < $st_time || $e_time < $curr_time)
	    {
	    	$expected_to_run = 0;
	    }

	    my $grace_period_in_minutes_ = 2;
	    my $allow_grace_period_ = 0;
	    if( abs( $st_time - $curr_time ) < $grace_period_in_minutes_ || abs( $e_time - $curr_time ) < $grace_period_in_minutes_ )
	    {
		$allow_grace_period_ = 1;
	    }
	       
    	if( $ret_val == $expected_to_run ){
          	print "OK running_status:".$ret_val." ".$proc_to_check."\n";
        }
	elsif( $allow_grace_period_ == 1 ){
	    	print "OK_grace_period running status:".$ret_val." ".$proc_to_check_."\n";
	}
        else
        {
          	print "ALERT running_status:".$ret_val." ".$proc_to_check."\n"
        }
	}else {

        }

 
}

