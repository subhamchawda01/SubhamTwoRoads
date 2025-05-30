#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;
use File::Find;
use List::Util qw/max min/; # for max
use Data::Dumper;

my $USAGE="$0 PROFILE";
if ( $#ARGV < 0 ) { print "$USAGE \n"; exit ( 0 ); }
my $profile=shift;
my $eod_date_ = `date +"%Y%m%d"` ; chomp ( $eod_date_ ) ;
my $pos_file="/spare/local/ORSlogs/SGX/$profile/position."."$eod_date_";
my $exchange_sym_exec="/home/pengine/prod/live_execs/get_exchange_symbol";
my $shortcode_exec="/home/pengine/prod/live_execs/get_shortcode_for_symbol";

my $shc_0 = "SGX_NK_0";
my $shc_1 = "SGX_NK_1";
my $exch_sym_0= `$exchange_sym_exec $shc_0 $eod_date_`; chomp ($exch_sym_0);
my $exch_sym_1= `$exchange_sym_exec $shc_1 $eod_date_`; chomp ($exch_sym_1);
my $spread_exch_sym=`$exchange_sym_exec "SP_SGX_NK0_NK1"  $eod_date_`; chomp ($spread_exch_sym); #spread symbol
my %symbol_to_pos_map_ = ();
my $num_sec=0; #keeps track of num securities with positions

if (-e $pos_file) {
	open POS_HANDLE, "< $pos_file" or die "could not open position file $pos_file\n";
    	my @pos_file_lines_ = <POS_HANDLE>;
	close POS_HANDLE;
    	for ( my $i = 0 ; $i <= $#pos_file_lines_; $i ++ )
    	{	
		if(index($pos_file_lines_[$i], "Cumulative") != -1) { last;}  #exit the loop
      		if(index($pos_file_lines_[$i], "Security_Id") != -1) {		#store these positions
			my @words_ = split ( ':', $pos_file_lines_[$i] );
			my $symbol=$words_[1]; chomp($symbol);
			my $pos=$words_[3]; chomp($pos);
			$symbol_to_pos_map_{$symbol}=$pos; 
			$num_sec++;
		}
	}
}

if (exists ($symbol_to_pos_map_{$exch_sym_0}) && exists ($symbol_to_pos_map_{$exch_sym_1} )) {
	my $max_pos=0;
	if($symbol_to_pos_map_{$exch_sym_0} *  $symbol_to_pos_map_{$exch_sym_1} <0)
	{
		if(abs($symbol_to_pos_map_{$exch_sym_0}) >= abs($symbol_to_pos_map_{$exch_sym_1})) {	## 16 & -15, transfer -15 to spread | ## -16 & 15, transfer 15 to spread
			$max_pos=$symbol_to_pos_map_{$exch_sym_1};
			$symbol_to_pos_map_{$exch_sym_0}+=$symbol_to_pos_map_{$exch_sym_1};
			$symbol_to_pos_map_{$exch_sym_1}=0;
		}
		else {					## 16 & -18, transfer -16 to spread | ## -16 & 18
			$max_pos=-1*$symbol_to_pos_map_{$exch_sym_0};
			$symbol_to_pos_map_{$exch_sym_1}+=$symbol_to_pos_map_{$exch_sym_0};
			$symbol_to_pos_map_{$exch_sym_0}=0;
		}	
	}
        #transfer this max position to spread contract
	if(exists ($symbol_to_pos_map_{$spread_exch_sym}))
	{
                $symbol_to_pos_map_{$spread_exch_sym}+=$max_pos;
	}
	else {
		$symbol_to_pos_map_{$spread_exch_sym}=$max_pos;
		$num_sec++; #spread contract is added here, increment num_sec
	}
}

#backup position file dumped by ors and overwrite with the squared off 
`cp $pos_file "$pos_file"."_before_NK_sqOff"`;
open (my $SQUARED_POS_HANDLE, '>', "$pos_file") or die "Can't open $pos_file for writing squared off NK positions $!";
select $SQUARED_POS_HANDLE;
$| = 1;
print "No. of securities: $num_sec\n";
print "Security Position Map\n";
foreach my $symbol (sort keys %symbol_to_pos_map_)
{
	print " Security_Id:$symbol".":     Position:$symbol_to_pos_map_{$symbol}".":\n";
}
close $SQUARED_POS_HANDLE;

