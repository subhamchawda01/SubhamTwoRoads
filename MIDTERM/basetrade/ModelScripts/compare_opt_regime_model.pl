#!/usr/bin/perl
use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $opt_model_ = $ARGV [ 0 ];
my $original_model_ = $ARGV [ 1 ];
my @original_indicators_ = ();

open MODEL_FILE, "< $original_model_";
while ( my $model_line_ = <MODEL_FILE> )
{
    chomp($model_line_);
    my @model_words_ = split ' ', $model_line_;
    my @tmp_words_ = ();
    for ( my $i=0; $i <= $#model_words_; $i ++ )
{
    if ( $model_words_[$i] eq "#" )
{last;}
else
{
push ( @tmp_words_, $model_words_[$i] );
}
}
my $tmp_model_line_ = join(' ',@tmp_words_);
if($model_words_[0] eq "INDICATOR")
{
    push (@original_indicators_, $tmp_model_line_);
}

}
close MODEL_FILE;
my $index_ = 0;
my $model_number_ = 1;
open OPT_MODEL_FILE, "< $opt_model_ ";
while ( my $model_line_ =  <OPT_MODEL_FILE> )
{
    chomp($model_line_);
    my @model_words_ = split ' ', $model_line_;
    my @tmp_words_ = ();
    for ( my $i=0; $i <= $#model_words_; $i ++ )
{
    if ( $model_words_[$i] eq "#" )
{last;}
else
{
push ( @tmp_words_, $model_words_[$i] );
}
}
my $tmp_model_line_ = join(' ',@tmp_words_);

if ($model_words_[0] eq "REGIMEINDICATOR")
{
    print ("INDICATOR for Regime : $model_line_ \n" );
}
if ($model_words_[0] eq "INDICATORSTART" || $model_words_[0] eq "INDICATORINTERMEDIATE")
{
    print ("Model $model_number_ :\n");
    $model_number_ ++;
    $index_ = 0;
}
if ($model_words_[0] eq "INDICATOR")
{
    my $original_model_line_  = $original_indicators_[$index_];
    print ("$original_model_line_ -> $tmp_model_line_\n");
    $index_ ++;
}
}

close OPT_MODEL_FILE;
