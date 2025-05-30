#!/usr/bin/perl

########################################################################################
#
# This script is finds the score and count value of a 'Modelling Element' ( i.e. the elemets that 
# affect the generate strategy e.g. paramfiles, ilists, reg_algos, look_aheads etc. 
# available funcitons are -
#     1) getScore( <element_name>, <shortcode(if known - optional)> <elelment_type(optional)> ) - returns one float
#     2) getCount( <element_name>, <shortcode(if known - optional)> <elelment_type(optional)> ) - returns one int
#     3) getScoreAndCount( <element_name>, <shortcode(if known - optional)> <elelment_type(optional)> ) - returns an array with ( score, count );
#  Note - one should specify optional paramaeters for efficient searching. 
#       - It loads the database file at the beginning which makes it pretty faster. Avoid calling it as an executable.
#  available optins for 'types': 
#  a) datagen_param - ( values could be '300_13_1', '150_5_1' etc )
#  b) paramfile     - ( values could be 'param_fgbm_96', 'param_FGBM_0_EU_PBT_std_2_aggs' etc )
#  c) trading_logic - ( values could be 'PriceBasedScalper' )
#  d) reg_la        - ( regression look ahead, 2, 32, 30 etc)
#  e) regress_exec  - ( FSRLM, FSRR, FSLR etc )
#  f) ilist         - ( ilist name )
#  g) filters       - ( f0, fv, fsr1_5 etc )
#  h) reg_algo      - ( na_e3, na_t3, na_e5 )
#  i) stdev         - stdev - still to come.. not there in the data base
#
###########################################################################################

use lib "/spare/local/perllib/perl5/lib/perl5/";    # Local library path for JSON
use JSON;
use Data::Dumper;
use strict;
use warnings;

use constant MODEL_ELEM_FILE_EU => "/spare/local/ModellingElements/modelling_element_db_EU_MORN_DAY.json";
use constant MODEL_ELEM_FILE_US => "/spare/local/ModellingElements/modelling_element_db_US_MORN_DAY.json";

local $/;
open ( FL, MODEL_ELEM_FILE_EU); 
my $JSON_DB_EU = decode_json ( <FL> ); 
close (FL);
open ( FL1, MODEL_ELEM_FILE_US); 
my $JSON_DB_US = decode_json ( <FL1> ); 
close (FL1);

# {"FGBM_0": {"datagen_param": {"300_5_1": [-10986765.532486977, 26],
sub getScoreAndCount 
{
    my ($parameter, $product, $type, $time_p) = @_;
    my $json_db_ref_ = $JSON_DB_US;
    if ( $time_p =~ "EU" ) { $json_db_ref_ = $JSON_DB_EU; }
    
    if ( ! $parameter ) {
	print "ERROR! No parameter supplied whose data retrieve.\n";
	return 0.0;
    }
    # print "$parameter, $product, $type\n";
    
    my @product_ref_ = ( $json_db_ref_ -> {$product} );

    if ( ! @product_ref_ ) {
	print " No product found with : $product\n";
	@product_ref_ = values %{$json_db_ref_};
    }

    foreach my $prd_ ( @product_ref_) {
	my @param_types_ref_ = ( $prd_->{$type} );
	if ( ! @param_types_ref_ ) {
	    print "No ref with type: $prd_\n";
	    @param_types_ref_ = values %{$prd_};
	}
	
	foreach my $prm_typ_ ( @param_types_ref_ ) {
	    my @param_value_ = @{$prm_typ_ -> {$parameter}};
	    if ( $#param_value_ != -1 ) {
		return @param_value_;
	    }
	    print "Could Not find : $parameter for $product, $type\n";
	    return (0.0, 0);
	}
    }
}

sub getScore
{
    my ($score, $count) = getScoreAndCount ( @_ );
    return $score;
}
	  
sub getCount
{
    my ($score, $count) = getScoreAndCount ( @_ );
    return $count;
}

sub getBestNElement 
{
    # returns top N elements for a product i.e returns a hash with 
    # keys are the name of the element  for that product and values are ( score, count );
    # 
    my %retval = ();
    my ($product, $element_type, $count, $time_p ) = @_ ;
    if ( !$product || !$element_type || !$count ) {
	print "One or mroe argument missing. Expecting : <shortcode($product)> <element_type($element_type)> <count($count)>\n";
	return %retval;
    }

    if ( !$time_p ) { $time_p = "US"; }

    my $json_db_ref_ = $JSON_DB_US;
    if ( $time_p =~ "EU" ) { $json_db_ref_ = $JSON_DB_EU; }

    my %element_hash_ = %{$json_db_ref_->{$product} -> { $element_type }};
    if ( ! %element_hash_ ) {
	return %retval;
    }
    
    foreach my $key ( sort { $element_hash_{$a}[0] <=> $element_hash_{$b}[0] } keys %element_hash_ ) {
	if ( $count < 0 ) { last; }
	$retval{$key} = $element_hash_{$key};
	$count -- ;
    }
    return %retval;

}


if ( $0 eq __FILE__ ) {
    my %bestelem = getBestNElement ( 'FESX_0', 'ilist', 5 );
    while ( my ($key, $value) = each %bestelem ) {
	print "$key, @$value\n";
    }
}
