#!/usr/bin/perl

use Date::Parse;
use Time::ParseDate;

my $str_="February 08, 2011 13:00";
my $epoch_time_ = parsedate ( $str_, ZONE => 'America/NewYork' );
print $epoch_time_."\n";
