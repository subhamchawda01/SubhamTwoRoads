#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::INET;

my $job_dir="/home/dvctrader/ec2_jobs" ;
my $AWS_SCRIPT_HOME="/apps/aws/" ;
my $num_cores=`nproc` ;
#print "numcores ".$num_cores."\n";
my @running_jobs=() ;
 
# auto-flush on socket
$| = 1;
 
# creating a listening socket
my $socket = new IO::Socket::INET (
    LocalHost => '0.0.0.0',
    LocalPort => '7777',
    Proto => 'tcp',
    Listen => 5,
    Reuse => 1
);
die "cannot create socket $!\n" unless $socket;
print "server waiting for client connection on port 7777\n";
 
while(1)
{
    # waiting for a new client connection
    my $client_socket = $socket->accept();
 
    # get information about a newly connected client
    my $client_address = $client_socket->peerhost();
    my $client_port = $client_socket->peerport();
    print "connection from $client_address:$client_port\n";

    my $num_assigned=`ls -l $job_dir/*running* | wc -l 2>/dev/null `;
    my $num_done=`ls -l $job_dir/*done* | wc -l 2>/dev/null `;
    my $num_running = $num_assigned - $num_done;
 
    # read up to 1024 characters from the connected client
    my $data = "";
    $client_socket->recv($data, 1024);
    print "[SERVERLOG] [RCVD] $data\n";
    my @words = split (' ', $data);
    
    if ( $words[0] eq "DO_JOB" ) {
      if($#running_jobs>=($num_cores-1)){
        $data="-1 not enough cores left, try after some time";
      }
      else {
        my $job_id = $words[1];
        my $my_job_cmd = join ( " ", @words[2..$#words]);
        my $job_file="$job_dir/run_cmd_$job_id";
        my $log_file="$job_dir/log_$job_id";
        my $fh;
        open ( $fh, ">$job_file");
        print $fh "$my_job_cmd > $log_file 2>&1 " ;
        close ( $fh ) ;
        `sh $AWS_SCRIPT_HOME/run_cmd.sh $job_id`;
      }
    }
    elsif ( $words[0] eq "STATUS" ) {
      my $arg = $words[1];
      my $was_assigned  = ` ls -l $job_dir/*running*$arg | wc -l 2>/dev/null `;
      my $has_completed = ` ls -l $job_dir/*done*$arg | wc -l 2>/dev/null `;
     
      if ( $was_assigned == 0 ){
        $data = "invalid job ($arg), was never run";
      }
      else {
        if( $has_completed == 1 ) {
          $data = "job ($arg) compelted";
        }
        else {
          $data = "job ($arg) is still running";
        }
      }
    }
    elsif ( $words[0] eq "NUM_CORES" ) {
      $data="".$num_cores;
      chomp ( $data ) ;
    }
    elsif ( $words[0] eq "FREE_CORES" ) {
      $data="".($num_cores-$num_running);
      chomp ( $data ) ;
    }
    else {}
    print "received data: $data\n";
 
    # write response data to the connected client
    #$data = "ok";
    $client_socket->send($data);
 
    # notify client that response has been sent
    shutdown($client_socket, 1);
}
 
$socket->close();

