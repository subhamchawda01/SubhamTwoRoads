#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::INET;
use Fcntl qw(:flock SEEK_END);

#lock failed return 1 #TODO ADD ERROR HANDLING
sub lock {
  my ($fh) = @_;
  flock($fh, LOCK_EX) or return 1 ;
  # and, in case someone appended while we were waiting...
  seek($fh, 0, SEEK_END) or return 1 ;
  return 0 ;
}
sub unlock {
  my ($fh) = @_;
  flock($fh, LOCK_UN) or die "Cannot unlock mailbox - $!\n";
}

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

    # read up to 1024 characters from the connected client
    my $data = "";
    $client_socket->recv($data, 1024);
    print "[SERVERLOG] [RCVD] $data\n";
    my @words = split (' ', $data);
    
    if ( $words[0] eq "SUBMIT_JOB" ) {
        my $my_job_cmd = join ( " ", @words[1..$#words]);
        my $job_file="/mnt/sdf/JOBS/job_q.txt";
        my $lock_file="/mnt/sdf/JOBS/job_q.lock";
        my $fh;
        my $lock_fh;
        open ($lock_fh, $lock_file);
        open ( $fh, ">>$job_file");
        lock ( $lock_fh );
        chomp ( $my_job_cmd);
        print $fh $my_job_cmd."\n" ;
        unlock ( $lock_fh );
        close ( $fh ) ;
        #close ( $lock_fh);
    }
    elsif ( $words[0] eq "STATUS" ) {
      $data="not implemented now";
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

