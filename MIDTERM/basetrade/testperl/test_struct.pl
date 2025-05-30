#!/usr/bin/perl

use warnings;
use strict;
use Getopt::Long;
use File::Basename;
use File::Temp qw { tempfile tempdir };
use File::Find;
use File::Path;
use File::Spec;
use IO::File;

use Text::Tabs qw { expand };
use Cwd qw { cwd };


package MyObj;
use Class::Struct;

# declare the struct
struct ( 'MyObj', { count_ => '$', stuff_ => '$' } );

package main;
my $my_obj_ = new MyObj;
$my_obj_->count_ ( 3 );
$my_obj_->stuff_ ( "23.23" );

printf "%d %d\n", $my_obj_->count_, $my_obj_->stuff_;
