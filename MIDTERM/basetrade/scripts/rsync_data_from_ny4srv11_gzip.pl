#!/usr/bin/perl
use strict;
use warnings;
use FileHandle;
use File::Basename;

my $YYYYMMDD=`date +%Y%m%d`; chomp ($YYYYMMDD);
my $DIR_PREFIX=substr ( $YYYYMMDD, 0, 4 )."/".substr ( $YYYYMMDD, 4, 2 )."/".substr ( $YYYYMMDD, 6, 2 );

my $target_machine_ = "10.1.3.11";

my @dirs_ = `ssh $target_machine_ ls -aldtr /NAS1/data/\\\*Data/\\\*/$DIR_PREFIX | awk \'{print \$9}\'`;
chomp ( @dirs_ );
for ( my $i = 0; $i <= $#dirs_ ; $i ++ )
{
    my $this_dir_ = $dirs_[$i];
    my $this_enclosing_dir_ = dirname ( $this_dir_ );
#    print "$this_dir_ $this_enclosing_dir_\n";
    if ( ! -d $this_dir_ )
    {
        `rsync -avz $target_machine_:$this_dir_ $this_enclosing_dir_`;
        `gzip $this_dir_/\*`;
    }
}
