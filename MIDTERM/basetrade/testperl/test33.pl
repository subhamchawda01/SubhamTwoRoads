#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub Getvec;
sub Seevec;

my @refvec_ = GetVec ( );
Seevec ( \@refvec_ );

exit ();

sub GetVec
{
    my @retvec_ = ();
    my @invec_ = ();
    push ( @invec_, 0 );
    push ( @invec_, 1 );
    push ( @retvec_, \@invec_ );
    return @retvec_ ;
}

sub Seevec
{
    my ( $refvecref_ ) = @_;
    my $t_ref_ = $$refvecref_[0];
    printf ( "Values: %d %d\n", $$t_ref_[0], $$t_ref_[1] ) ;
}
