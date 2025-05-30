#!/usr/bin/perl

# \file scripts/sort_abs.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 162, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes as input :
# key file_with_columns

use strict;
use warnings;

package KeyLine;
use Class::Struct;
use Data::Dumper ;

# declare the struct
struct ( 'KeyLine', { key_value_ => '$', full_text_ => '$' } );

package main;

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $USAGE="$0 key [ file_with_columns ]";

my $key_index_ = 0;
my $txtfile_ = "";
my $opt_fromfile = 0;

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
$key_index_ = $ARGV[0] - 1; # -1 since array indices in perl start from 0, while user semantics starts from 1
if ( $#ARGV >= 1 ) { $txtfile_ = $ARGV[1]; $opt_fromfile = 1; }

my @key_line_vec_ = ();

if ( $opt_fromfile == 1 )
{
    open TXTFILEHANDLE, "< $txtfile_" or die "sort_abs.pl could not open $txtfile_\n";
    while ( my $txt_line_ = <TXTFILEHANDLE> )
    {
	my @txt_words_ = split ( ' ', $txt_line_ );
	if ( $#txt_words_ >= $key_index_ )
	{
	    my $new_txt_line_with_key_ = new KeyLine;
	    $new_txt_line_with_key_->key_value_ ( $txt_words_[$key_index_] );
	    $new_txt_line_with_key_->full_text_ ( $txt_line_ );
	    push ( @key_line_vec_, $new_txt_line_with_key_ );
	}
    }
    close TXTFILEHANDLE;
}
else
{
    while ( my $txt_line_ = <STDIN> )
    {
	my @txt_words_ = split ( ' ', $txt_line_ );
	if ( $#txt_words_ >= $key_index_ )
	{
	    my $new_txt_line_with_key_ = new KeyLine;
	    $new_txt_line_with_key_->key_value_ ( $txt_words_[$key_index_] );
	    $new_txt_line_with_key_->full_text_ ( $txt_line_ );
	    push ( @key_line_vec_, $new_txt_line_with_key_ );
	}
    }
}

sub KLCmp ($$) 
{
    my($kl1, $kl2) = @_;
    return abs($kl2->key_value_()) <=> abs($kl1->key_value_());
}

my @sorted_key_line_vec_ = sort KLCmp @key_line_vec_ ;

for ( my $i = 0 ; $i <= $#sorted_key_line_vec_; $i++ )
{
    printf "%s", $sorted_key_line_vec_[$i]->full_text_();
}

