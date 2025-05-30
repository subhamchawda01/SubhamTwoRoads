use strict; 
use warnings;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; #PrintStacktraceAndDie
require "$GENPERLLIB_DIR/string_utils.pl"; #trim

sub ParseConfig
{
  my $file_ = shift;
  my $key_valvec_map_ref_;
  %$key_valvec_map_ref_ = ();

  my $key_set_ = 0;
  my $key_ = "";
  open FILE, "< $file_" or PrintStacktraceAndDie("can't open $file_ for reading\n");
  while ( my $line_ = <FILE> )
  {
    chomp($line_);
    my @words_ = split(' ', $line_);

    if ( $#words_ < 0 )
    {
      $key_set_ = 0;
      next;
    }

    next if ( substr($words_[0],0,1) eq '#' );

    if ( $key_set_ == 0 )
    {
      $key_set_ = 1;
      $key_ = $words_[0];
      next;
    }

    push(@{$$key_valvec_map_ref_{$key_}}, trim($line_));
  }
  close FILE;

  return $key_valvec_map_ref_;
}

sub ParseConfigLines
{
  my $file_ = shift;
  my @value_vec_vec_ = ();

  open FILE, "< $file_" or PrintStacktraceAndDie("can't open $file_ for reading\n");
  while ( my $line_ = <FILE> )
  {
    chomp($line_);
    my @words_ = split(' ', $line_);
    next if ( @words_ <= 0 || substr($words_[0],0,1) eq '#' ); 
    push ( @value_vec_vec_, \@words_ );
  }
  close FILE;
  return \@value_vec_vec_;
}

1
