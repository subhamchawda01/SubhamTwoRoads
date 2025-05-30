
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $GET_UTC_HHMM_STR_EXEC_ = "$BIN_DIR/get_utc_hhmm_str" ;
if ( -e "$LIVE_BIN_DIR/get_utc_hhmm_str" )
{
    $GET_UTC_HHMM_STR_EXEC_ = "$LIVE_BIN_DIR/get_utc_hhmm_str" ;
}


sub GetUTCMinutes
{
    my $num_args_ = @_;
    $num_args_ > 0 or  PrintStacktraceAndDie("GetUTCMinutes called with 0 args\n");
    my $orig_time_ = shift;
    my $date_ = `date +%Y%m%d`;
    if($num_args_ > 1)  {$date_ = shift;}
    my $utc_time_ = `$GET_UTC_HHMM_STR_EXEC_ $orig_time_ $date_`;
    my $utc_min_ = (int($utc_time_/100))*60 + ($utc_time_%100);
    return $utc_min_;
}


1
