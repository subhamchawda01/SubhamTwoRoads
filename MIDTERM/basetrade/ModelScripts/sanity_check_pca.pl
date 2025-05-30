#!/usr/bin/perl
# \file GenPerlLib/sanity_check_pca.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

use strict;
use warnings;

#use Data::Dumper;

my $USAGE="$0 old_pca_filename_ new_pca_filename_ new_pca_reconciled_filename";
if ( $#ARGV < 2 ) { die "$USAGE\n"; }

my $port_input_file = "/spare/local/tradeinfo/PCAInfo/portfolio_inputs";
my $old_pca_filename_ = $ARGV[0];
my $new_pca_filename_ = $ARGV[1];
my $new_pca_reconciled_filename_ = $ARGV[2];

my %port_constituent_count = ();
my %old_port_std_count = ();
my %new_port_std_count = ();

my %old_port_2_std = ();
my %old_port_2_eigen = ();
my %old_port_2_eigen2 = ();
my %new_port_2_std = ();
my %new_port_2_eigen = ();
my %new_port_2_eigen2 = ();


my $summary_text = "PCA RUN SUMMARY..\n input: $port_input_file \n oldFile: $old_pca_filename_ \n newFile: $new_pca_filename_\n========================\n";

#print "new info\n===========\n";
##print Dumper %new_port_std_count;
##print Dumper %new_port_2_eigen;
#
##Check for error messages from R
my $r_err_cnt = `grep -c "Type 'q()' to quit R." $new_pca_filename_` ;
chomp($r_err_cnt);
if ( $r_err_cnt > 0 ){
        $summary_text = $summary_text."error messages ($r_err_cnt) from R  ...ERROR\n";
        my @error_lines = `grep Type $new_pca_filename_` ;
        $summary_text = $summary_text."@error_lines \n";
        for ( my $i = 0; $i <= $#error_lines ; $i++ )
        {
                my $this_error_line = $error_lines[$i];
                my @err_words = split ( ' ', $this_error_line ) ;
                $summary_text = $summary_text."Copying $err_words[1] info from old-pca-file to new-pca-file \n";
                `sed -i '/ $err_words[1] /d' $new_pca_filename_` ;
                `grep " $err_words[1] " $old_pca_filename_ >> $new_pca_filename_` ;
        }
        # print $summary_text;
        # exit(0);
        }

#Parse Input file
open(INFO, $port_input_file ) or die("Could not open  file $port_input_file \n");
foreach my $line (<INFO>)  {
	$line =~ s/^\s+// ; #remove leading space
	$line =~ s/\s+$// ; #remove trailing space
	my @toks = split /\s+/, $line ;
	next if ($#toks <= 2 );
	if ($toks[0] eq "PLINE"){
		$port_constituent_count{$toks[1]}= $#toks - 1;
	}
}
close(INFO);
#print "input info\n============\n";
#print Dumper %port_constituent_count;

open(INFO, $old_pca_filename_) or die("Could not open  file $old_pca_filename_ \n");

foreach my $line (<INFO>)  {
	$line =~ s/^\s+// ; #remove leading space
	$line =~ s/\s+$// ; #remove trailing space
	my @toks = split /\s+/, $line ;
	next if ($#toks <= 2 );
	if ($toks[0] eq "PORTFOLIO_STDEV"){
		$old_port_std_count{$toks[1]}= $#toks - 1;
		@{$old_port_2_std{$toks[1]}} = ();
		for(my $i = 2; $i <= $#toks; $i++ ){
			push (@{$old_port_2_std{$toks[1]}}, $toks[$i] );
		}
	}
	elsif ($toks[0] eq "PORTFOLIO_EIGEN" && $toks[2] == 1 ){
		@{$old_port_2_eigen{$toks[1]}} = ();
		for(my $i = 3; $i <= $#toks; $i++){
			push ( @{$old_port_2_eigen{$toks[1]}}, $toks[$i] );
		}
	}
	elsif ( $toks[0] eq "PORTFOLIO_EIGEN" && $toks[2] == 2 ){
		@{$old_port_2_eigen2{$toks[1]}} = ();
		for ( my $i = 3; $i <= $#toks; $i++ ){
			push ( @{$old_port_2_eigen2{$toks[1]}}, $toks[$i] );
		}
	}
}
close(INFO);

#print "old info\n===========\n";
#print Dumper %old_port_std_count;
#print Dumper %old_port_2_eigen;

open(INFO, $new_pca_filename_) or die("Could not open  file $new_pca_filename_ \n");

foreach my $line (<INFO>)  {
	$line =~ s/^\s+// ; #remove leading space
	$line =~ s/\s+$// ; #remove trailing space
	my @toks = split /\s+/, $line ;
	next if ($#toks <= 2 );
	if ($toks[0] eq "PORTFOLIO_STDEV"){
		$new_port_std_count{$toks[1]}= $#toks - 1;
                @{$new_port_2_std{$toks[1]}} = ();
                for(my $i = 2; $i <= $#toks; $i++ ){
                        push (@{$new_port_2_std{$toks[1]}}, $toks[$i] );
                }		
	}
	elsif ($toks[0] eq "PORTFOLIO_EIGEN" && $toks[2] == 1 ){
		@{$new_port_2_eigen{$toks[1]}} = ();
		for(my $i = 3; $i <= $#toks; $i++){
			push ( @{$new_port_2_eigen{$toks[1]}}, $toks[$i] );
		}
	}
        elsif ($toks[0] eq "PORTFOLIO_EIGEN" && $toks[2] == 2 ){
                @{$new_port_2_eigen2{$toks[1]}} = ();
                for(my $i = 3; $i <= $#toks; $i++){
                        push ( @{$new_port_2_eigen2{$toks[1]}}, $toks[$i] );
                }
        }

}
close(INFO);

open RECONCILED_INFO, "> $new_pca_reconciled_filename_" or PrintStacktraceAndDie ( "Could not open $new_pca_reconciled_filename_ for writing\n" );

foreach my $key ( keys %port_constituent_count )
{
	my $num_comp = $port_constituent_count{$key};
	my $exist_old = ( exists $old_port_2_eigen{$key} ) && ( exists $old_port_std_count {$key} ) ;
	my $exist_new = ( exists $new_port_2_eigen{$key} ) && ( exists $new_port_std_count {$key} ) ;
	if ( ! $exist_old  &&  ! $exist_new )
	{
		$summary_text = $summary_text."$key present in input file, but not in eigen value value (old or new)\n";
		next;
	} 
	if ( ! $exist_old  &&  $exist_new )
	{
		$summary_text = $summary_text."$key newly added Porfolio\n";
		my $t_string_ = "PORTFOLIO_STDEV $key ";
		my @t_std_vec_ = @{$new_port_2_std{$key}};
		
		for ( my $i=0; $i <= $#t_std_vec_; $i++ )
		{
			$t_string_ = $t_string_.$t_std_vec_ [$i]." ";			
		}
		$t_string_ = $t_string_."\n";
		$t_string_ = $t_string_."PORTFOLIO_EIGEN $key 1 ";
			
		my @t_eigen_vec_ = @{$new_port_2_eigen{$key}};

		for ( my $i=0; $i <= $#t_eigen_vec_; $i++ )
		{
			$t_string_ = $t_string_.$t_eigen_vec_[$i]." ";		    			
		}

		$t_string_ = $t_string_."\n";
		$t_string_ = $t_string_."PORTFOLIO_EIGEN $key 2 ";
		
		my @t_eigen2_vec_ = @{$new_port_2_eigen2{$key}};

		for ( my $i=0; $i <= $#t_eigen2_vec_; $i++ )
		{
			$t_string_ = $t_string_.$t_eigen2_vec_[$i]." ";
		}

		$t_string_ = $t_string_."\n";
		
		print RECONCILED_INFO $t_string_;
		
		next;
	}
	if ( $exist_old  &&  !$exist_new )
	{
		$summary_text = $summary_text."$key not present in new eigen value file... ERROR\n";
                my $t_string_ = "PORTFOLIO_STDEV $key ";
                my @t_std_vec_ = @{$old_port_2_std{$key}};
                
                for ( my $i=0; $i <= $#t_std_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_std_vec_ [$i]." ";                           
                }
                $t_string_ = $t_string_."\n";
                $t_string_ = $t_string_."PORTFOLIO_EIGEN $key 1 ";

                my @t_eigen_vec_ = @{$old_port_2_eigen{$key}};

                for ( my $i=0; $i <= $#t_eigen_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_eigen_vec_[$i]." ";                                  
                }

                $t_string_ = $t_string_."\n";
                $t_string_ = $t_string_."PORTFOLIO_EIGEN $key 2 ";

                my @t_eigen2_vec_ = @{$old_port_2_eigen2{$key}};

                for ( my $i=0; $i <= $#t_eigen2_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_eigen2_vec_[$i]." ";
                }

                $t_string_ = $t_string_."\n";

                print RECONCILED_INFO $t_string_;
		
		next;
	}  

	#present in both, check if count is correct
	if ( $port_constituent_count{$key} != $old_port_std_count{$key} || 
		$port_constituent_count{$key} != $#{$old_port_2_eigen{$key}} ){
		$summary_text = $summary_text."corrupt old file for entry $key ... ERROR\n";
		print "corrupt old file for entry $key ... ERROR\n";
		print $port_constituent_count{$key}." ".$old_port_std_count{$key}." ".$port_constituent_count{$key}." ".$#{$old_port_2_eigen{$key}}."\n";
		next;
	}
	
	#verify dot product
	my @vec_old =  @{$old_port_2_eigen{$key}};
	my @vec_new =  @{$new_port_2_eigen{$key}};
	my $n1=0;
	my $n2=0;
	my $dot=0;
	my $sign_check_ = 1;
	for (my $i =1; $i <= $#vec_old; $i= $i+1){
		$n1 += $vec_old[$i] * $vec_old[$i];
		$n2 += $vec_new[$i] * $vec_new[$i];
		$dot += $vec_new[$i] * $vec_old[$i];
		if ( $vec_new[$i] * $vec_old[$i] < 0 )
		{
			$sign_check_ = -1;
		}
        }
	my $dp = $dot / sqrt ( $n1 *$n2);
	
	if ($dp < 0.95 && $dp >= 0.90){
		$summary_text = $summary_text."PCA for $key differs by too much\n PCA_OLD @vec_old\n PCA_NEW @vec_new ... DOT_PR=$dp\n";
	}
	elsif ($dp < 0.90 ){
		$summary_text = $summary_text."PCA for $key differs by too much\n PCA_OLD @vec_old\n PCA_NEW @vec_new ... DOT_PR=$dp  ...ERROR\n";
	}


	if ( $sign_check_ < 0 )
	{
		$summary_text = $summary_text."PCA for $key has different signs for constituents\n PCA_OLD @vec_old\n PCA_NEW @vec_new \n";
                my $t_string_ = "PORTFOLIO_STDEV $key ";
                my @t_std_vec_ = @{$old_port_2_std{$key}};

                for ( my $i=0; $i <= $#t_std_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_std_vec_ [$i]." ";
                }
                $t_string_ = $t_string_."\n";
                $t_string_ = $t_string_."PORTFOLIO_EIGEN $key 1 ";

                my @t_eigen_vec_ = @{$old_port_2_eigen{$key}};

                for ( my $i=0; $i <= $#t_eigen_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_eigen_vec_[$i]." ";
                }

                $t_string_ = $t_string_."\n";
                $t_string_ = $t_string_."PORTFOLIO_EIGEN $key 2 ";

                my @t_eigen2_vec_ = @{$old_port_2_eigen2{$key}};

                for ( my $i=0; $i <= $#t_eigen2_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_eigen2_vec_[$i]." ";
                }

                $t_string_ = $t_string_."\n";

                print RECONCILED_INFO $t_string_;
                next;
	
	}


	if ( $dp < 0.95 )
	{
                my $t_string_ = "PORTFOLIO_STDEV $key ";
                my @t_std_vec_ = @{$old_port_2_std{$key}};

                for ( my $i=0; $i <= $#t_std_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_std_vec_ [$i]." ";
                }
                $t_string_ = $t_string_."\n";
                $t_string_ = $t_string_."PORTFOLIO_EIGEN $key 1 ";

                my @t_eigen_vec_ = @{$old_port_2_eigen{$key}};

                for ( my $i=0; $i <= $#t_eigen_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_eigen_vec_[$i]." ";
                }

                $t_string_ = $t_string_."\n";
                $t_string_ = $t_string_."PORTFOLIO_EIGEN $key 2 ";

                my @t_eigen2_vec_ = @{$old_port_2_eigen2{$key}};

                for ( my $i=0; $i <= $#t_eigen2_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_eigen2_vec_[$i]." ";
                }

                $t_string_ = $t_string_."\n";

                print RECONCILED_INFO $t_string_;
		next;	
	}
	else
	{
                my $t_string_ = "PORTFOLIO_STDEV $key ";
                my @t_std_vec_ = @{$new_port_2_std{$key}};

                for ( my $i=0; $i <= $#t_std_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_std_vec_ [$i]." ";
                }
                $t_string_ = $t_string_."\n";
                $t_string_ = $t_string_."PORTFOLIO_EIGEN $key 1 ";

                my @t_eigen_vec_ = @{$new_port_2_eigen{$key}};

                for ( my $i=0; $i <= $#t_eigen_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_eigen_vec_[$i]." ";
                }

                $t_string_ = $t_string_."\n";
                $t_string_ = $t_string_."PORTFOLIO_EIGEN $key 2 ";

                my @t_eigen2_vec_ = @{$new_port_2_eigen2{$key}};

                for ( my $i=0; $i <= $#t_eigen2_vec_; $i++ )
                {
                        $t_string_ = $t_string_.$t_eigen2_vec_[$i]." ";
                }

                $t_string_ = $t_string_."\n";

                print RECONCILED_INFO $t_string_;
                next;
	}
}

close ( RECONCILED_INFO );

print $summary_text;
