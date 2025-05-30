#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

#my @prods_ = ("DI1F15","DI1F16","DI1F17");

if ( scalar ( @ARGV ) < 2 )
{
    print "stdev_ratio_factor.pl period dep indep1 indep2 ... \n";
    exit ( 0 ) ;
}

my $period_ = $ARGV[0];
my @shc_ =  ( ) ; push ( @shc_ , $ARGV[1]); # first one is the dep

for ( my $i_ = 2 ; $i_ < scalar ( @ARGV ) ; $i_ ++ )
{
    push ( @shc_ , $ARGV[$i_] );
}

my %stdevs2_ ;
my %stdevs10_ ;
my %stdevs30_ ;
my %stdevs60_ ;
my %stdevs120_ ;
my %stdevs300_ ;
my %stdevs600_ ;

my @dates_ ;
my $file_name_prefix_ = "/spare/local/tradeinfo/datageninfo/stdev_";


for ( my $i_ = 0 ; $i_ < scalar ( @shc_ ) ; $i_ ++ )
{
    my $file_name_ = $file_name_prefix_.$shc_[$i_]."_".$period_;
    open FILE_HANDLE, "< $file_name_" ;
    my @file_lines_ = <FILE_HANDLE>;
    close FILE_HANDLE;

   foreach my $stdev_ ( @file_lines_)
   {
        chomp ( $stdev_);
        my @tkns_ = split ( " ", $stdev_);
	if ( scalar ( @tkns_ ) < 8 ) 
	{
	    next ;
	}
        $stdevs2_{$shc_[$i_]}{$tkns_[0]} = $tkns_[1];
        $stdevs10_{$shc_[$i_]}{$tkns_[0]} = $tkns_[2];
        $stdevs30_{$shc_[$i_]}{$tkns_[0]} = $tkns_[3];
        $stdevs60_{$shc_[$i_]}{$tkns_[0]} = $tkns_[4];
        $stdevs120_{$shc_[$i_]}{$tkns_[0]} = $tkns_[5];
        $stdevs300_{$shc_[$i_]}{$tkns_[0]} = $tkns_[6];
        $stdevs600_{$shc_[$i_]}{$tkns_[0]} = $tkns_[7];
	push ( @dates_ , $tkns_[0] );
   }
}


foreach my $date_ ( sort { $b <=> $a } @dates_ )
{
    my $count_ = 0 ;
    foreach my $shortcode_ ( @shc_ )
    {
	if ( ! exists ($stdevs2_{$shortcode_}{$date_}) )
	{
	    last ;
	}
	else
	{
	    $count_ ++ ;
	}	
    }
    if ( $count_ == scalar ( @shc_ )  )
    {
	print "$date_\nINDEP DURATION_2 DURATION_10 DURATIO_30 DURATION_60 DURATION_120 DURATION_300 DURATION_600 \n";
	my $dep_sd2_ = $stdevs2_{$shc_[0]}{$date_};
	my $dep_sd10_ = $stdevs2_{$shc_[0]}{$date_};
	my $dep_sd30_ = $stdevs2_{$shc_[0]}{$date_};
	my $dep_sd60_ = $stdevs2_{$shc_[0]}{$date_};
	my $dep_sd120_ = $stdevs2_{$shc_[0]}{$date_};
	my $dep_sd300_ = $stdevs2_{$shc_[0]}{$date_};
	my $dep_sd600_ = $stdevs2_{$shc_[0]}{$date_};

	for ( my $i_ = 1 ; $i_ < scalar ( @shc_ ) ; $i_ ++ )
	{
	    print $shc_[$i_]." ";
	    print $stdevs2_{$shc_[$i_]}{$date_}/$dep_sd2_." ".
		$stdevs10_{$shc_[$i_]}{$date_}/$dep_sd10_." ".
		$stdevs30_{$shc_[$i_]}{$date_}/$dep_sd30_." ".
		$stdevs60_{$shc_[$i_]}{$date_}/$dep_sd60_." ".
		$stdevs120_{$shc_[$i_]}{$date_}/$dep_sd120_." ".
		$stdevs300_{$shc_[$i_]}{$date_}/$dep_sd300_." ".
		$stdevs600_{$shc_[$i_]}{$date_}/$dep_sd600_." ";
	    print "\n";
	}
	last ;
    }     
}


#{
#    if ( exists $mkt_vols_{$date} )
#    {
#	my $percent_ = $mkt_vols_{$date}/$o_vols_{$date} * 100 ;
#	print "$date $mkt_vols_{$date} $o_vols_{$date} ".$percent_."\n";
#    }
#}

