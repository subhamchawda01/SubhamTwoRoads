#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;

my $affinity_view_exec="/home/pengine/prod/live_execs/cpu_affinity_mgr" ;
my $affinity_tracking_html="/tmp/.affinity_tracking.html";
my $affinity_monitor_file="/tmp/.affinity_monitoring.dat";

`>$affinity_tracking_html`; #empty the file
`>$affinity_monitor_file`;


open (my $aff_detail, '>', "$affinity_tracking_html") or die "Can't open $affinity_tracking_html $!";
select $aff_detail;
$| = 1; #do not buffer anything

print "<!DOCTYPE html><html><body> \n";

#for tradeinits, see how many processes are running per tags
my %tag_to_num_queries_map=();
my %tag_to_all_queries_map=();
my $tradeinit_exec="/home/pengine/prod/live_execs/tradeinit";
my $date = `date +"%Y%m%d"` ; chomp ( $date) ;
my $risk_client_log="/spare/local/logs/risk_logs/risk_client_log_$date";
my @qid_array=`ps -ef | grep /home/pengine/prod/live_execs/tradeinit | grep -v grep | awk '{print \$NF}'`; chomp(@qid_array);
foreach my $qid (@qid_array) {
    my $tags=`grep "SACI mapping" $risk_client_log | grep $qid | awk '{print \$NF}' | tail -1`; chomp($tags); 
    my @all_tags = split ( ':', $tags );
    if ( $#all_tags >= 0 )
    {
 	for ( my $i = 0 ; $i <= $#all_tags; $i++) 		
	{
	    $tag_to_num_queries_map{$all_tags[$i]}++;
	    $tag_to_all_queries_map{$all_tags[$i]}=$tag_to_all_queries_map{$all_tags[$i]}." ".$qid;	
	}
    }
}

#proceeding to dump running queries per tag
print "<table border='1' style='width:80%' cellpadding='5'><tr><th align=center>TAG</th><th align=center>NUM_QUERIES=>QUERY_IDs</th></tr>  \n";
foreach my $tag (sort keys %tag_to_num_queries_map){
    my $query_print_str=$tag_to_num_queries_map{$tag}."=>".$tag_to_all_queries_map{$tag};
    print "<tr bgcolor='#404040'><td align=center>$tag </td><td align=center>$query_print_str</td></tr> \n";
    `echo "$tag $tag_to_num_queries_map{$tag} $tag_to_all_queries_map{$tag}">> $affinity_monitor_file`;
}
print "</table> <br/> <br/> \n";

#proceeding to dump html for cores usage
print "<table border='1' style='width:80%' cellpadding='5'><tr><th align=center>COLOR</th><th align=center>MEANING</th></tr>  \n";
print "<tr bgcolor='#404040'><td align=center>WHITE</td><td>FREE_CORE</td></tr> \n";
print "<tr bgcolor='#7499FF'><td align=center>GREEN</td><td>CORE_IN_USE</td></tr> \n" ;
print "<tr bgcolor='#97AAB3'><td align=center>GRAY</td><td>INIT_PROCESS_CORES</td></tr> \n" ;
print "<tr bgcolor='#F9999F'><td align=center>LIGHT_RED</td><td>MULTIPLE_PROCESS_ON_SAME_CORE( CAN BE CLUBBED STRATS AS WELL)</td></tr> \n" ;
print "<tr bgcolor='#E45959'><td align=center>RED</td><td>PROCESS_RUNNIG_OFF_INIT_CORES</td></tr><tr></tr><tr></tr><tr></tr><tr></tr> \n";
print "<table border='1' style='width:80%' cellpadding='5'><tr><th align=center>CORE</th><th align=center>PID</th><th align=center>PROCESS_AFFINED</th></tr>  \n";

my $color="#404040" ;
my @init_cores=`$affinity_view_exec VIEW init 1 | awk -F"CORE #" '{print \$2}' | tr ' ' '\n'` ; chomp (@init_cores);
my $total_cores=`cat /proc/cpuinfo | grep -w "processor" | wc -l`;
my %init_cores_hash = map {$_ => 1} @init_cores;

for (my $cores = 0; $cores < $total_cores; $cores+=1) {
    my $is_this_init_core=0 ;
    #check if this is an init core
    if ( defined $init_cores_hash{$cores})  {
    	$is_this_init_core=1;
    	$color="#97AAB3" ;
    }
    #if this core is affined some process
    if ( `grep -w "$cores" /spare/local/files/affinity/affinity_pid_process.txt | wc -l` > 0 ) {

	my $procs_on_a_core=0 ;
	my $running_pid="" ;
	my $running_proc="" ;
	my $pid="";
	my $procs="";
	my @all_lines=`grep -w "$cores" /spare/local/files/affinity/affinity_pid_process.txt | tr ' ' '~'`;
	foreach my $lines (@all_lines) {
	    my @pid_proc = split ( '~', $lines );
	    $pid=$pid_proc[1]; 
	    $procs=$pid_proc[0];
	    if ( `ps -efL | grep -w "$pid" | grep -v grep | wc -l` == 0 ) {
	        next ;
	    }
	    $procs_on_a_core++ ;
	    
	    $running_pid=$running_pid." ".$pid ;
	    $running_proc=$running_proc." ".$procs ;
	}
	my $print_core=$cores;
	if ($procs_on_a_core ==1 ) {	#only 1 process to this core

	    if ( $is_this_init_core == 1) {
		$color="#E45959" ;
		$print_core="INIT" ;
	    }
	    else {		#only 1 process to non init core, normal case
		$color="#7499FF" ;
	    }

	    print "<tr bgcolor=$color> \n "; 
	    print "<td align=center>$print_core</td>  \n";
	    print "<td align=center>$pid</td> \n ";
	    print "<td align=center>$procs</td> \n ";
	}
    	elsif ($procs_on_a_core > 1){
	    if ( $is_this_init_core ==1){	#multiple processes to this core. Alert here!!
		$color="#E45959" ;
	    }
	    else {
		$color="#F9999F" ;
	    }

	    print "<tr bgcolor=$color> \n";
	    print "<td align=center>$print_core</td> \n";
	    print "<td align=center>$running_pid</td> \n";
	    print "<td align=center>$running_proc</td> \n";
    	}
    	else{									#idle cores
	    if ( $is_this_init_core ==1 ) {		#idle init core
		$color="#97AAB3" ;
		print "<tr bgcolor=$color> \n";
		print "<td align=center>INIT</td> \n";
		print "<td align=center>NA</td> \n";
		print "<td align=center>NA</td> \n";
	    }
	    else
	    {									#idle non init core
		$color="#404040" ;
		print "<tr bgcolor=$color> \n";
		print "<td align=center>$cores</td> \n";
		print "<td align=center>NA</td> \n";
		print "<td align=center>NA</td> \n";
		
	    }
    	}
    }
    #no process affined to this core
    else
    {
	if ( $is_this_init_core ==1 ) {		#idle init core

	    $color="#97AAB3" ;
	    print "<tr bgcolor=$color> \n";
	    print "<td align=center>INIT</td> \n";
	    print "<td align=center>NA</td> \n";
	    print "<td align=center>NA</td> \n";
	}
	else
	{									#idle non init core
	    $color="#404040" ;
	    print "<tr bgcolor=$color> \n";
	    print "<td align=center>$cores</td> \n";
	    print "<td align=center>NA</td> \n";
	    print "<td align=center>NA</td> \n";

	}
    }	
}
print "</table></body></html>" ;
close $aff_detail;

#dump tag to num queries in monitoring file
