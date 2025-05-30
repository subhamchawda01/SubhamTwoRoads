#!/usr/bin/perl

# \file codegen/fix_code.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 162, Evoma, #14, Bhattarhalli,
#        Old Madras Road, Near Garden City College,
#        KR Puram, Bangalore 560049, India
#        +91 80 4190 3551
#
use strict;
use warnings;

#apply output of fixfast-codegen to this script
#redirect the output to a new file
#purpose of this script is to make old style auto code to conform to new style 
#with all the classes being templatized

my $file = $ARGV[0] ;

my @lines=`cat $file `;

my $start_class=0;
my $end_class=0;

my $curr_line =-1;
my $j=0;
foreach my $line(@lines){
	$curr_line++;
	my $trim = $line ;
	$trim =~ s/\s+/ /g ;
	$trim =~ s/^ //;
	$trim =~ s/ \$//g;
	if ($trim eq ""){
		print $line;
		next;
	}
	my @words= split ' ', $trim ;
	
	if ( $words[0] eq "DeltaField" ){
		my $matched = 0;
		
		for (; $matched ==0  && $j<=$#lines; $j++)
		{
			my $trim = $lines[$j] ;
			chomp($trim);
			next if ($trim eq "");
			$trim =~ s/\s+/ /g ;
			$trim =~ s/^ //;
			$trim =~ s/ \$//g;
			my @words2 = split ' ', $trim ;
			next if ( $trim !~ /new\s+DeltaField/ );
			my $var = $words[$#words];
			chomp($var);
			$var =~ s/^.// ;
			$var =~ s/;// ;
			#print "comparing ..".$words2[0].".. == ..$var..\n" ;
			if ( $words2[0] eq $var) {
				#print "matched\n\n";
				my $to_replace = $words2[8];
				$to_replace =~ s/,// ;
				$lines[$j] =~ s/>/, $to_replace >/ ;
				$lines[$j] =~ s/>.*?,.*?,/> (/ ;
				$line =~ s/>/, $to_replace >/ ;
				$matched = 1;
			}
		}
	    
	}
    elsif ( $words[0] eq "CopyField" ){
		my $matched = 0;
		
		for (; $matched ==0  && $j<=$#lines; $j++)
		{
			my $trim = $lines[$j] ;
			chomp($trim);
			next if ($trim eq "");
			$trim =~ s/\s+/ /g ;
			$trim =~ s/^ //;
			$trim =~ s/ \$//g;
			my @words2 = split ' ', $trim ;
			next if ( $trim !~ /new\s+CopyField/);
			my $var = $words[$#words];
			chomp($var);
			$var =~ s/^.// ;
			$var =~ s/;// ;
			#print "comparing ..".$words2[0].".. == ..$var..\n" ;
			if ( $words2[0] eq $var) {
				#print "matched\n\n";
				my $to_replace = $words2[8]." ".$words2[9];
				$to_replace =~ s/,$// ;
				$lines[$j] =~ s/>/, $to_replace >/ ;
				$lines[$j] =~ s/>.*?,.*?,/> (/ ;
				$line =~ s/>/, $to_replace >/ ;
				$matched = 1;
			}
		}
	    
	}
    elsif ( $words[0] eq "DefaultField" ){
		my $matched = 0;
		
		for (; $matched ==0  && $j<=$#lines; $j++)
		{
			my $trim = $lines[$j] ;
			chomp($trim);
			next if ($trim eq "");
			$trim =~ s/\s+/ /g ;
			$trim =~ s/^ //;
			$trim =~ s/ \$//g;
			my @words2 = split ' ', $trim ;
			next if ( $trim !~ /new\s+DefaultField/ );
			my $var = $words[$#words];
			chomp($var);
			$var =~ s/^.// ;
			$var =~ s/;// ;
			#print "comparing ..".$words2[0].".. == ..$var..\n" ;
			if ( $words2[0] eq $var) {
				#print "matched\n\n";
				my $to_replace = $words2[8]." ".$words2[9];
				$to_replace =~ s/,$// ;
				$lines[$j] =~ s/>/, $to_replace >/ ;
				$lines[$j] =~ s/>.*?,.*?,/> (/ ;
				$line =~ s/>/, $to_replace >/ ;
				$matched = 1;
			}
		}
	    
	}
    elsif ( $words[0] eq "IncrementField" ){
		my $matched = 0;
		
		for (; $matched ==0  && $j<=$#lines; $j++)
		{
			my $trim = $lines[$j] ;
			chomp($trim);
			next if ($trim eq "");
			$trim =~ s/\s+/ /g ;
			$trim =~ s/^ //;
			$trim =~ s/ \$//g;
			my @words2 = split ' ', $trim ;
			next if ( $trim !~ /new\s+IncrementField/ ) ;
			my $var = $words[$#words];
			chomp($var);
			$var =~ s/^.// ;
			$var =~ s/;// ;
			#print "comparing ..".$words2[0].".. == ..$var..\n" ;
			if ( $words2[0] eq $var) {
				#print "matched\n\n";
				my $to_replace = $words2[8]." ".$words2[9];
				$to_replace =~ s/,$// ;
				$lines[$j] =~ s/>/, $to_replace >/ ;
				$lines[$j] =~ s/>.*?,.*?,/> (/ ;
				$line =~ s/>/, $to_replace >/ ;
				$matched = 1;
			}
		}
	    
	}
    elsif ( $words[0] eq "NoOpField" ){
		my $matched = 0;
		
		for (; $matched ==0  && $j<=$#lines; $j++)
		{
			my $trim = $lines[$j] ;
			chomp($trim);
			next if ($trim eq "");
			$trim =~ s/\s+/ /g ;
			$trim =~ s/^ //;
			$trim =~ s/ \$//g;
			my @words2 = split ' ', $trim ;
			next if ( $trim !~ /new\s+NoOpField/ );
			my $var = $words[$#words];
			chomp($var);
			$var =~ s/^.// ;
			$var =~ s/;// ;
			#print "comparing ..".$words2[0].".. == ..$var..\n" ;
			if ( $words2[0] eq $var) {
				#print "matched\n\n";
				my $to_replace = $words2[8]." ".$words2[9];
				$to_replace =~ s/,$// ;
				$lines[$j] =~ s/>/, $to_replace >/ ;
				$lines[$j] =~ s/>.*?,.*?,/> (/ ;
				$line =~ s/>/, $to_replace >/ ;
				$matched = 1;
			}
		}
	    
	}
    elsif ( $words[0] eq "TailField" ){
		my $matched = 0;
		
		for (; $matched ==0  && $j<=$#lines; $j++)
		{
			my $trim = $lines[$j] ;
			chomp($trim);
			next if ($trim eq "");
			$trim =~ s/\s+/ /g ;
			$trim =~ s/^ //;
			$trim =~ s/ \$//g;
			my @words2 = split ' ', $trim ;
			next if ( $trim !~ /new\s+TailField/ );
			my $var = $words[$#words];
			chomp($var);
			$var =~ s/^.// ;
			$var =~ s/;// ;
			#print "comparing ..".$words2[0].".. == ..$var..\n" ;
			if ( $words2[0] eq $var) {
				#print "matched\n\n";
				my $to_replace = $words2[8];
				$to_replace =~ s/,// ;
				$lines[$j] =~ s/>/, $to_replace >/ ;
				$lines[$j] =~ s/>.*?,.*?,/> (/ ;
				$line =~ s/>/, $to_replace >/ ;
				$matched = 1;
			}
		}
	    
	}
    elsif ( $#words >= 2 and $words[2] eq "new" ){
        $line =~ s/ \) ;$/ , \"$words[0]\");/g;
    }
    elsif ( index($line, "previousValue" ) >= 0 ){
        $line =~ s/previousValue.//g;
    }

    if ( index($line, "isAssigned" ) >= 0 ){
        $line =~ s/isAssigned/hasValue/g;
    }
	
	print $line ;
}
