#!/usr/bin/perl

use warnings;
use strict;

if ( $#ARGV < 0 ) {
	printf "usage: position_file ors_dumped_pos_[0/1]\n";
	exit(0);
}

my $ors_dump_file_ = 0;
my $SFILE          = $ARGV[0];
if ( $#ARGV >= 1 ) {
	$ors_dump_file_ = $ARGV[1];
}
my $PFILE_ = "";

my @unique_lines_       = ();
my @instr_              = ( "BAX", "CGB", "CGF", "SXF", "CGZ" );
my %prod_to_position_   = ();
my %prod_to_last_price_ = ();

open( SFILE_HANDLE, "< $SFILE" )
  or die " Could not open file $SFILE for reading \n";
while ( my $sline_ = <SFILE_HANDLE> ) {
	chomp($sline_);
	if ( not $sline_ ) { next; }

	my $found_ = "";
	if ( $ors_dump_file_ == 1 ) {
		if    ( index( $sline_, "No. of securities:" ) >= 0 )    { next; }
		elsif ( index( $sline_, "Security Position Map" ) >= 0 ) { next; }
		else {
			my $str1_ = "Security_Id:";
			my $str2_ = "Position:";
			$sline_ =~ s/$str1_//g;
			$sline_ =~ s/$str2_//g;
			$sline_ =~ s/ //g;
			$sline_ = $sline_ . "0";
		}
	}
	foreach my $prod_ (@instr_) {
		if ( index( $sline_, $prod_ ) >= 0 ) {
			$found_ = "true";

			my @words_ = ();
			if ( $ors_dump_file_ == 1 ) {
				@words_ = split( ':', $sline_ );
			}
			else {
				@words_ = split( ',', $sline_ );
			}
			chomp(@words_);
			if ( $#words_ >= 2 ) {
				chomp( $words_[0] );
				chomp( $words_[1] );
				chomp( $words_[2] );
				my $count_ = () = $sline_ =~ /$prod_/g;
				if ( $count_ == 1 ) {
					if ( exists $prod_to_position_{ $words_[0] } ) {
						$prod_to_position_{ $words_[0] } += $words_[1];
						$prod_to_last_price_{ $words_[0] } = $words_[2];
					}
					else {

						# should not come here
						$prod_to_position_{ $words_[0] } = $words_[1];
					}
				}
				elsif ( $count_ == 2 ) {

					# BAXM15BAXU15 => BAXM5, BAXU5
					my $first_len_ = length($prod_) + 3;
					my $prod1_ =
					    substr( $words_[0], 0, length($prod_) + 1 )
					  . substr( $words_[0], length($prod_) + 2, 1 );
					if ( exists $prod_to_position_{$prod1_} ) {
						$prod_to_position_{$prod1_} += int( $words_[1] );
					}
					else { $prod_to_position_{$prod1_} = int( $words_[1] ); }

					my $prod2_ =
					  substr( $words_[0], $first_len_, length($prod_) + 1 )
					  . substr( $words_[0], $first_len_ + length($prod_) + 2,
						1 );
					if ( exists $prod_to_position_{$prod2_} ) {
						$prod_to_position_{$prod2_} -= int( $words_[1] );
					}
					else { $prod_to_position_{$prod2_} = -int( $words_[1] ); }

#print " $words_[0] $prod1_ ".$prod_to_position_ { $prod1_ }." $prod2_ ".$prod_to_position_ { $prod2_ }."\n";
				}
			}
		}
	}

	if ( not $found_ ) {
		push( @unique_lines_, $sline_ );
	}
}
close(SFILE_HANDLE);

foreach my $line_ (@unique_lines_) {
	print $line_. "\n";
}

if ( $ors_dump_file_ != 0 )
{
	my $num_keys_ = 0;

	foreach my $key_ ( keys %prod_to_position_ ) {
		if ( $prod_to_position_{$key_} != 0 ) {
			$num_keys_++;
		}
	}
	print "No. of securities: $num_keys_\n";
	print "Security Position Map\n";
}

foreach my $key_ ( keys %prod_to_position_ ) {
	if ( $prod_to_position_{$key_} != 0 ) {
		if ( $ors_dump_file_ == 0 ) {
			if ( exists $prod_to_last_price_{$key_} ) {
				print $key_. ","
				  . $prod_to_position_{$key_} . ","
				  . $prod_to_last_price_{$key_} . "\n";
			}
			else {
				print $key_. "," . $prod_to_position_{$key_} . ",0.0\n";
			}
		}
		else {
			print "Security_Id:" 
			  . $key_
			  . ": Position:"
			  . $prod_to_position_{$key_} . ":\n";
		}
	}
}
