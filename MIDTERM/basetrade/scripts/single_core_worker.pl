#!/usr/bin/perl

use strict;
use warnings;
use IO::Socket::INET;

sub sendReq {

# auto-flush on socket
$| = 1;
# create a connecting socket
my $socket = new IO::Socket::INET (
    PeerHost => '10.0.0.11',
    PeerPort => '33533',
    Proto => 'tcp',
);

die "cannot connect to the server $!\n" unless $socket;
print "connected to the server\n";

# data to send to a server
my $req = join(' ' , @_ );
print "sending ==> ".$req."\n";
my $size = $socket->send($req);
print "sent $size bytes. waitingfor response\n";
# notify server that request has been sent
shutdown($socket, 1);

# receive a response of up to 200k characters from server
my $response = "";
$socket->recv($response, 204800);
print "$response\n";
$socket->close();

return $response;
}

my $host=`hostname -s`;
my $time_id=time; 
chomp ($host);
my $log_dir="/spare/local/logs/aws_job_log/";
`mkdir -p $log_dir`;#ensure log directory exists

while ( 1 ) { 
   my $this_job = sendReq("job_reserve");
   my ($jobId, $job)= split /\s+/ , $this_job, 2 ;
   $jobId=~s/jobId://;
   chomp ($jobId);
   if ( $jobId eq "-1" ) {
        sleep ( 2 );#sleep for 2 seconds, no job available
        next;
   }

   my $log_file = "$log_dir/$jobId.$time_id.log";
   my $signal_running_response = sendReq("job_begin $jobId $host");
   chomp ( $signal_running_response );
   if ( $signal_running_response eq "success" ){
     print "doing job:: $job > $log_file\n";
     `$job > $log_file`;
     `mail -s "LOG_EMAIL: $job" nseall@tworoads.co.in < $log_file`;
     my $signal_running_response = sendReq("job_end $jobId");
     print "done $jobId\n";
   }
   else {
     sendReq("job_unreserve $jobId");
   }
}
