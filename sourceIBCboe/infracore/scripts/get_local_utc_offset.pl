#!/usr/bin/perl
#
# this script is meant to be used to get the value to set in all perl files where we need offset_sec
# the reason to make this a separate script is becaseu it uses Time::Zone which does not come with perl installation 
# in RHEL6

use strict ;
use warnings ;
use Time::Zone;
# use DateTime;
# use DateTime::TimeZone;

my $offset_sec = tz_local_offset(); # or tz_offset($tz) if you have the TZ
                                    # in a variable and it is not local
# my $time = time(); # realistically it will be the time value you provide in localtime
# my $utc_time = $time + $offset_sec;

# # cache local timezone because determining it can be slow
# # if your timezone is user specified get it another way
# our $App::LocalTZ = DateTime::TimeZone->new( name => 'local' );
# my $tz = DateTime::TimeZone->new( name => $App::LocalTZ );

# my $dt = DateTime->now(); # again, time will be whatever you are passing in
#                           # formulated as a DateTime
# my $offset_sec = $tz->offset_for_datetime($dt);

print $offset_sec;
