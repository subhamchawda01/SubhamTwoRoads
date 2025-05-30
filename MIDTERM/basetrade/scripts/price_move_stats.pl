#!/usr/bin/perl

use strict;
use warnings;
use Math::Complex ; # sqrt
use List::Util qw/max min/;    # for max

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
require "$GENPERLLIB_DIR/array_ops.pl"; 

# start
my $USAGE="$0 PROD DATE STIME ETIME \n";

sub GetNMoreThanEqualValue ;

if ( $#ARGV < 3 )
{ 
    print $USAGE ; exit(0); 
}

my $prod_ = $ARGV[0];
my $date_ = $ARGV[1];
my $stime_ = $ARGV[2];
my $etime_ = $ARGV[3];

#my @lines_ = `~/basetrade_install/bin/mds_log_l1_trade $prod_ $date_ | grep 2>/dev/null` ;
# ~/basetrade_install/bin/mds_log_l1_trade DI1F17 20150123 | grep L1 | awk '{ print ($5+$6)/2}'
# ~/basetrade_install/bin/datagen ~/kputta/kp.ilist 20150123 BRT_901 BRT_1500 2897 /tmp/kp_dout c3 0 0 ADD_SAMPLING_CODES DI1F17 0.5 -1

my $unique_pid_ = `date +%N`; chomp ( $unique_pid_ ); $unique_pid_ = int($unique_pid_) + 0;
my $ilist_file_name_ = "/tmp/".$unique_pid_.".il";
my $dout_file_name_ = "/tmp/".$unique_pid_.".do";

open ( ILIST_FILE, "> $ilist_file_name_ " ) or PrintStacktraceAndDie ( "Could not open file to write $ilist_file_name_\n" );
print ILIST_FILE "MODELINIT DEPBASE NONAME ".$prod_." "."MidPrice MidPrice"."\n";
print ILIST_FILE "MODELMATH LINEAR CHANGE\n";
print ILIST_FILE "INDICATORSTART\n";
print ILIST_FILE "INDICATOR 1 L1Price ".$prod_." MidPrice \n";
print ILIST_FILE "INDICATOREND\n";
close ( ILIST_FILE );

my $min_px_inc_ = `/home/dvctrader/basetrade_install/bin/get_min_price_increment $prod_ $date_` ;
my $exec_cmd_ = "/home/dvctrader/basetrade_install/bin/datagen $ilist_file_name_ $date_ $stime_ $etime_ $unique_pid_ $dout_file_name_ 86400 c3 0 0 ADD_SAMPLING_CODES $prod_ 0.5 -1" ;
print $exec_cmd_."\n" ;
`$exec_cmd_` ;
open ( FH, '<' , $dout_file_name_ ) or PrintStacktraceAndDie ( "Could not open file to read $dout_file_name_ \n" );
my @lines_ = <FH>;
close FH;

my $diff_ = 0 ;
my $tr_ = 0 ;
my %tr_map_ ;
my @tr_vec_ = ( ) ;
my $curr_ = 0 ;
foreach my $line_ ( @lines_ )
{
    chomp ( $line_ );
    my @words_ = split ( ' ', $line_ );
    $words_[2] = int ( ( $words_[2] ) / $min_px_inc_ );

    if ( $curr_ == 0 ) {  $curr_ = $words_[2] ; next ; }

    $diff_ = $words_[2] - $curr_ ; $curr_ = $words_[2];
    #print $diff_."\t".$tr_."\n" ;

    
    if ( $diff_ * $tr_ < 0 ) #reset
    {
	$tr_map_{$tr_} +=1 ;
	push ( @tr_vec_, $tr_ ) ;
	$tr_ = 0 ;
	#$tr_ = $diff_ ;
    }
    else
    {
	$tr_ += $diff_ ;
    }
}


my $avg_distance_ = GetAbsAverage ( \@tr_vec_ ) ;
my $median_distance_ = GetMedianConst ( \@tr_vec_ ) ;
my $max_distance_ = $tr_vec_ [ GetIndexOfMaxValue ( \@tr_vec_ ) ] ;
my $min_distance_ = $tr_vec_ [ GetIndexOfMinValue ( \@tr_vec_ ) ] ;

print $avg_distance_." ".$median_distance_." ".$max_distance_." ".$min_distance_."\n";

my $plot_file_name_ = "/tmp/".$unique_pid_.".pd";
open ( FH , ">" , $plot_file_name_ ) or "could not open to write $plot_file_name_\n";
print FH "$_\n" for @tr_vec_;
close ( FH );

open ( GP, '| gnuplot -persist' ) or die "no gnuplot";
use FileHandle;
GP -> autoflush ( 1 );
#print GP "set xdata time; \n set timefmt \"\%s\"; set grid; \n set title \'$date_(UTC)\'; \n plot \'$plot_file_name_\' using 0:1 with lines title \"$prod_\" \n; ";
print GP "set grid; \n set title \'$date_(UTC)\'; \n plot \'$plot_file_name_\' using 0:1 with lines title \"$prod_\" \n; ";
close GP;

open ( FH , ">" , $plot_file_name_ ) or "could not open to write $plot_file_name_\n";
foreach my $key_ ( sort { $a <=> $b } keys %tr_map_ )
{
    print FH "$key_ $tr_map_{$key_}\n" 
}
close ( FH );

open ( GP, '| gnuplot -persist' ) or die "no gnuplot";
use FileHandle;
GP -> autoflush ( 1 );
#print GP "set xdata time; \n set timefmt \"\%s\"; set grid; \n set title \'$date_(UTC)\'; \n plot \'$plot_file_name_\' using 1:2 with lines title \"$prod_\" \n; ";
print GP "set grid; \n set title \'$date_(UTC)\'; \n plot \'$plot_file_name_\' using 1:2 with lines title \"$prod_\" \n; ";
close GP;
