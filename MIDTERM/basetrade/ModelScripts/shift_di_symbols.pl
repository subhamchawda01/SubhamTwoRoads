#!/usr/bin/perl

# \file ModelScripts/shift_di_symbols.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite 217, Level 2, Prestige Omega,
#	 No 104, EPIP Zone, Whitefield,
#	 Bangalore - 560066, India
#	 +91 80 4060 0717
#
# 
use strict;
use warnings;

# start 
my $USAGE="$0 InputStratFile OutputStratFile";
my $USER=$ENV{'USER'};

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $input_strat_file_ = $ARGV[0];
my $output_strat_file_ = $ARGV[1];

my $csrl_dir_ = "/spare/local/$USER/CRSL/";

my $exec_cmd_ = `mkdir -p $csrl_dir_`;
`$exec_cmd_`;

open INSTRATFILE, "< $input_strat_file_" or PrintStacktraceAndDie ( "Could not open indicator_list_filename_ $input_strat_file_ for reading\n" );

open OUTSTRATFILE, "> $output_strat_file_" or PrintStacktraceAndDie ( "Could not open indicator_list_filename_ $output_strat_file_ for reading\n" );

my $count_ = 1;

my $new_model_file_ = "";

while ( my $strat_line_ = <INSTRATFILE> )
{
     chomp ( $strat_line_ );
     my @strat_line_words_ = split ' ', $strat_line_;  
     $new_model_file_ = $csrl_dir_."tmp_model_".$count_;
     my $exec_cmd_ = "cat $strat_line_words_[3] | sed 's/DI_7/DI_8/g' | sed 's/DI_6/DI_7/g' | sed 's/DI_5/DI_6/g' | sed 's/DI_4/DI_5/g' | sed 's/DI_3/DI_4/g' | sed 's/DI_2/DI_3/g' | sed 's/DI_1/DI_2/g' | sed 's/DI_0/DI_1/g' > $new_model_file_";     
     `$exec_cmd_`;
     my $new_shortcode_ = `echo $strat_line_words_[1] | sed 's/DI_7/DI_8/g' | sed 's/DI_6/DI_7/g' | sed 's/DI_5/DI_6/g' | sed 's/DI_4/DI_5/g' | sed 's/DI_3/DI_4/g' | sed 's/DI_2/DI_3/g' | sed 's/DI_1/DI_2/g' | sed 's/DI_0/DI_1/g'`;
     chomp ( $new_shortcode_ );
     $strat_line_words_[1] = $new_shortcode_;
     $count_ = $count_ + 1;
     $strat_line_words_[3] = $new_model_file_;
     $strat_line_ = join (' ' , @strat_line_words_ );
     print OUTSTRATFILE $strat_line_."\n";
}

close INSTRATFILE;
close OUTSTRATFILE;
