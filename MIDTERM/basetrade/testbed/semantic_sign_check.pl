#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);
use File::Path qw(mkpath);


if ( $#ARGV >= 0 )
{
    my $this_product_ = $ARGV[0];
    my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
    `mkdir -p /tmp/$unique_gsm_id_`;
   ` ~/basetrade_install/bin/datagen $ARGV[0] 20131111 BRT_900 BRT_1540 22897 /tmp/$unique_gsm_id_/semantic__.dout 4000 c2 0 0`; 
    `~/basetrade_install/bin/timed_data_to_reg_data $ARGV[0] /tmp/$unique_gsm_id_/semantic__.dout 32000 na_t3 /tmp/$unique_gsm_id_/semantic__.rout`;
    `~/basetrade_install/bin/callFSLR /tmp/$unique_gsm_id_/semantic__.rout 0.01 0 0 0.65 /tmp/$unique_gsm_id_/semantic__.regout 18 INVALIDFILE $ARGV[0] Y`;
}


