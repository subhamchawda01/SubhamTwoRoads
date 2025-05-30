#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);
use File::Path qw(mkpath);

my $HOME_DIR=$ENV{'HOME'}; 

if ( scalar ( @ARGV ) < 5 ) 
{
    print "config shc rel_flag mom_flag lv_flag\n" ;
    exit ( 0 ) ;
}

my $config = $ARGV [ 0 ] ;
my $shc = $ARGV [ 1 ] ;
my $rel = $ARGV [ 2 ] ;
my $mom = $ARGV [ 3 ] ;
my $lv = $ARGV [ 4 ] ;


my $depbase_pxtype_ = "MktSizeWPrice" ;
my $deppred_pxtype_ = "MktSizeWPrice" ;
my $indep_pxtype_ = "MktSizeWPrice" ;

my @lines = `grep $shc $config` ;

chomp ( @lines ) ;

my %r_ind = qw(

st DI1CurveAdjustedSimpleTrend
st.me DI1CurveAdjustedSimpleTrendMktEvents
ln.st DI1CurveAdjustedLinearSimpleTrend
ln.st.me DI1CurveAdjustedLinearSimpleTrendMktEvents

);

my %m_ind = qw(

pr.in DIPricingIndicator  
pr.in.me DIPricingIndicatorMktEvents
st.mo DI1CurveAdjustedSimpleTrendMomentum
st.me.mo DI1CurveAdjustedSimpleTrendMktEventsMomentum
ln.st.mo DI1CurveAdjustedLinearSimpleTrendMomentum
ln.st.me.mo DI1CurveAdjustedLinearSimpleTrendMktEventsMomentum

);

my %l_ind = qw( 
le.st  DI1LeveredSimpleTrend
le.st.me  DI1LeveredSimpleTrendMktEvents
);

my %o_ind = qw(
pr DI1CurveAdjustedPrice
);

my $result_dir = "$HOME_DIR/modelling/stratwork/$shc/cbt";
`mkdir -p $result_dir` ;
my $result_file = "$result_dir/cbt.model.$shc";

my @ilist = ( ) ;

my $model_file  = "" ;

my $indexrstr_ = 0 ;
my $indexmstr_ = 0 ;
my $indexlstr_ = 0 ;


foreach my $line ( @lines )
{
    my @tkns = split ( ' ' , $line ) ;
    next if ( $tkns[ 0 ] ne $shc ) ;

    if ( scalar ( @tkns ) == 4 )
    {
	if ( $rel == 1 ) 
	{
	    foreach my $key ( keys % r_ind )
	    {

		$model_file = $result_file.".".$tkns[1].".".$tkns[2].".".$tkns[3].".".$key ;

		open ( MFILE , ">" , $model_file ) ;

		print MFILE "MODELINIT DEPBASE $shc $depbase_pxtype_ $deppred_pxtype_\n" ;
		print MFILE "MODELMATH LINEAR CHANGE\n" ;
		print MFILE "INDICATORSTART\n" ;
		print MFILE "INDICATOR 1 $r_ind{ $key } $shc $tkns[1] $tkns[2] $tkns[3] $indep_pxtype_\n" ;
		print MFILE "INDICATOREND\n" ;

		close ( MFILE ) ;


	    }
	}
	if ( $mom == 1 )
	{
	    foreach my $key ( keys % m_ind )
	    {

		$model_file = $result_file.".".$tkns[1].".".$tkns[2].".".$tkns[3].".".$key ;

		open ( MFILE , ">" , $model_file ) ; 

		print MFILE "MODELINIT DEPBASE $shc $depbase_pxtype_ $deppred_pxtype_\n" ;
		print MFILE "MODELMATH LINEAR CHANGE\n" ;
		print MFILE "INDICATORSTART\n" ;
		print MFILE "INDICATOR 1 $m_ind{ $key } $shc $tkns[1] $tkns[2] $tkns[3] $indep_pxtype_\n" ;
		print MFILE "INDICATOREND\n" ;

		close ( MFILE ) ;
		
	    }
	}

    }    
    elsif ( scalar ( @tkns ) == 3 )
    {	
	if ( $lv == 1 )
	{
	    foreach my $key ( keys % l_ind )
	    {

		$model_file = $result_file.".".$tkns[1].".".$tkns[2].".".$key ;

		open ( MFILE , ">" , $model_file ) ;

		print MFILE "MODELINIT DEPBASE $shc $depbase_pxtype_ $deppred_pxtype_\n" ;
		print MFILE "MODELMATH LINEAR CHANGE\n" ;
		print MFILE "INDICATORSTART\n" ;
		print MFILE "INDICATOR 1 $l_ind{ $key } $shc $tkns[1] $tkns[2] $indep_pxtype_\n" ;
		print MFILE "INDICATOREND\n" ;
		
		close ( MFILE ) ;
	    }
	}	
	
    }
    
}

