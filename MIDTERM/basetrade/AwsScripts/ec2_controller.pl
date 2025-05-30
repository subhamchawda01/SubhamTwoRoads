#!/usr/bin/perl

# \file ec2_controller.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

#
# Controller 
# USAGE: <IP> <COMMAND> <COMMAND_ARGS>
# eg 1)  <IP> fetch_pid <job_id>
# eg 2)  <IP> do_job <job_id> <command to execute in one line>
#

use IO::Socket::INET;
 
# auto-flush on socket
$| = 1;

#@all_instances = `ec2-describe-instances -O $AWS_ACCESS_KEY -W $AWS_SECRET_KEY --show-empty-fields | grep "^INSTANCE" | awk '{print $2 , $6, $17, $10}'`;

my$job_server_ip=$ARGV[0]; 
my$job_server_port = 7777;

# create a connecting socket
my $socket = new IO::Socket::INET (
    PeerHost => $job_server_ip,
    PeerPort => $job_server_port,
    Proto => 'tcp',
);
die "cannot connect to the server $!\n" unless $socket;
print "connected to the server\n";
 
# data to send to a server
#if ( $ARGV[1] eq "STATUS" || $ARGV[1] eq "DO_JOB" ) 
{
  my $req = join(" ", @ARGV[1..($#ARGV)]);
  my $size = $socket->send($req);

  # notify server that request has been sent
  shutdown($socket, 1);
 
  # receive a response of up to 1024 characters from server
  my $response = "";
  $socket->recv($response, 1024);
  print "received response: $response\n";
}
 
$socket->close();

