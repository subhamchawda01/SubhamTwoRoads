#!/usr/bin/perl
use strict;
use warnings;

my $USAGE="$0 START_YYYYMMDD END_YYYYMMDD";

if ( $#ARGV < 1 ) { print "$USAGE\n"; exit ( 0 ); }
my $start_date_ = $ARGV[0];
my $end_date_ = $ARGV[1];

my %d2p_;
my %d2s_;
my $sump_ = 0;
my @exch_vec_ = ("CME","EUREX");

for ( my $i = 0 ; $i <= $#exch_vec_ ; $i ++ )
{
    my $t_exch = $exch_vec_[$i];
#    print "~/infracore/scripts/query_pnl_data.pl $t_exch $start_date_ $end_date_\n";
    my @t_exch_out_ = `~/infracore/scripts/query_pnl_data.pl $t_exch $start_date_ $end_date_`;
    chomp ( @t_exch_out_ );
    for ( my $j = 0 ; $j <= $#t_exch_out_; $j ++ )
    {
	my @fields_ = split (',', $t_exch_out_[$j]) ;
	
	if ( $#fields_ >= 2 )
	{
	    $d2p_{$fields_[0]} += $fields_[2];
	    $sump_ += $fields_[2];
	    $d2s_{$fields_[0]} = $sump_ ;
	}
    }
}

foreach my $t_d_ (sort keys %d2p_)
{
    printf "%d %d %d\n", $t_d_, $d2p_{$t_d_}, $d2s_{$t_d_};
}

#printf "SUM: %d\n", $sump_;
