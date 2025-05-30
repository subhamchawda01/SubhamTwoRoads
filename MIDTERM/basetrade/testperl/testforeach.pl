#!/usr/bin/perl
my $historical_sort_algo_ = "";
foreach $historical_sort_algo_ ( "kCNAPnlConservativeAverage", "kCNAPnlAverage", "kCNAPnlSharpe", "kCNAPnlMedianAverage", "kCNAPnlAdjAverage", "kCNADDAdjPnlAverage", "kCNADDAdjPnlSqrtVolume", "kCNASqDDAdjPnlSqrtVolume" )
{
    print $historical_sort_algo_ ;
}
