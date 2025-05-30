#!/usr/bin/perl

use strict;
use warnings;
use IO::Socket::INET;
 
# auto-flush on socket
$| = 1;
 
# create a connecting socket
my $socket = new IO::Socket::INET (
#    PeerHost => '10.23.74.55',
    PeerHost => '54.208.92.178',
    PeerPort => '33533',
    Proto => 'tcp',
);
die "cannot connect to the server $!\n" unless $socket;
print "connected to the server\n";
 
# data to send to a server
my $req = join(' ' , @ARGV );
print "trying to send".$req."\n";
my $size = $socket->send($req);
print "sent data of length $size\n";
 
# notify server that request has been sent
shutdown($socket, 1);
 
# receive a response of up to 20k characters from server                                                                                                                                                                                    
my $response = "";                                                                                                                                                                                                                          
$socket->recv($response, 20480);                                                                                                                                                                                                            
print "received response: $response\n";                                                                                                                                                                                                     
                                                                                                                                                                                                                                            
$socket->close();                   