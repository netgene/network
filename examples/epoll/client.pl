#!/usr/bin/perl -w 
use strict; 
use IO::Socket; 

main:
{
  return -1 if(@ARGV < 2);
    my $host = $ARGV[0]; 
    my $port = $ARGV[1];  
    my $sock = new IO::Socket::INET( PeerAddr => $host, PeerPort => $port, Proto => 'tcp'); 
    $sock or die "no socket :$!"; 
    sleep(1);
    my $msg;
    $sock->send("hello srv");
    sleep(1);
    $sock->recv($msg, 1024);
    print $msg . "\n";
    close $sock;
}
