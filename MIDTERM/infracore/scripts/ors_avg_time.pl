#!/usr/bin/perl

require "/home/dvcinfra/infracore/GenPerlLib/array_ops.pl" ;
my $FILENAME = $ARGV[0];

open(INFILE, $FILENAME) or die ("Unable to open");
my @data=<FILE>;
my %seqtime;
my $count = 0;
my $total_conf = 0;
my $total_seqd = 0;

my @vals_ = ();
while (<INFILE>)
{
    $line_ = $_;
    my @words_ = split ( ' ', $line_ );
    if ( $#words_ >= 12 )
    {
	if ( $words_[14] eq "Seqd" )
	{
	    $total_seqd++;
	    $seqtime{$words_[17]}=$words_[11];
	}
	if ( $words_[14] eq "Conf" )
	{
	    $total_conf++;
	    if ( exists $seqtime{$words_[17]} )
	    {
		my $diff_ = $words_[11] - $seqtime{$words_[17]} ;
		push ( @vals_, $diff_ );
		$total += $diff_;
		$count++;
	    }
	    else
	    {
#		print $words_[15]."\n";
	    }
	}
    }
}

print "Total Seq/Conf: ".$count."\n";
print "Total Conf: ".$total_conf."\n";
print "Total Seqd: ".$total_seqd."\n";
printf "Avg.Time(sec): %f\n",GetAverage ( \@vals_ );
printf "Median.Time(sec): %f\n",GetMedianAndSort ( \@vals_ );
#SYM : FGBL201112 Price :138.18 INTPX: 13818 TIMESET : 1315843206.975527 ORRTYPE : Exec SAOS : 4766 GBLPOS: 0
