#this file aims to print all the basic stats of a set of numbers read from input stream
#size, min, man, mode(count), median, mean, stdev

use strict;
use warnings;

use List::Util qw< min max >;
use Math::Complex ; # sqrt
sub by_number {
    if ($a < $b){ -1 } elsif ($a > $b) { 1 } else { 0 }
}


#
my $number_rx = qr{

  # leading sign, positive or negative
    (?: [+-] ? )

  # mantissa
    (?= [0123456789.] )
    (?: 
        # "N" or "N." or "N.N"
        (?:
            (?: [0123456789] +     )
            (?:
                (?: [.] )
                (?: [0123456789] * )
            ) ?
      |
        # ".N", no leading digits
            (?:
                (?: [.] )
                (?: [0123456789] + )
            ) 
        )
    )

  # abscissa
    (?:
        (?: [Ee] )
        (?:
            (?: [+-] ? )
            (?: [0123456789] + )
        )
        |
    )
}x;

my $n = 0;
my $sum = 0;
my @values = ();

my %seen = ();

while (<>) {
    while (/($number_rx)/g) {
        $n++;
        my $num = 0 + $1;  # 0+ is so numbers in alternate form count as same
        $sum += $num;
        push @values, $num;
        $seen{$num}++;
    } 
} 

die "no values" if $n == 0;

my $mean = $sum / $n;

my $sqsum = 0;
for (@values) {
    $sqsum += ( $_ ** 2 );
} 
$sqsum /= $n;
$sqsum -= ( $mean ** 2 );
my $stdev = sqrt($sqsum);

my $max_seen_count = max values %seen;
#my @modes = grep { $seen{$_} == $max_seen_count } keys %seen;

#my $mode = @modes == 1 
#            ? $modes[0] 
#            : "(" . join(", ", @modes) . ")";
#$mode .= ' @ ' . $max_seen_count;

my $median;
my $mid = int @values/2;
my @sorted_values = sort by_number @values;
if (@values % 2) {
    $median = $sorted_values[ $mid ];
} else {
    $median = ($sorted_values[$mid-1] + $sorted_values[$mid])/2;
} 

my $min = min @values;
my $max = max @values;

#printf "%d, %g, %g, %s, %g, %g, %g\n", $n, $min, $max, $mode, $median, $mean, $stdev;
printf "%g %g %g ", $mean, $stdev, $median;

