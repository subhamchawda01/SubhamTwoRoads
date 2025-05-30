#!/usr/bin/perl
use strict;
use warnings;

if ( $#ARGV < 0 )
{
    print "USAGE: exec <fxstreet-html-file> \n";
    exit ( 0 );
}

my $temp_fx_html_ = $ARGV[0]; chomp($temp_fx_html_); # The provided HTML file must be freshly downloaded from FXStreet

# Check if provided file exists with non-zero size
if ( ! ( ( -e $temp_fx_html_ ) &&
         ( -s $temp_fx_html_ > 0 ) ) )
{
    print "Provided file ".$temp_fx_html_." not readable. Exiting\n";
    `/bin/mail -s \"FXStreet revert-reconcile script: Provided file not readable\" nseall@tworoads.co.in < /dev/null`;
    exit ( 0 );
}

my $MASTER_INFRA_LOCATION = "/home/dvcinfra/master/infracore";
my $BACKUP_LOCATION = "/home/dvcinfra/master/fxstreet/backup/";

# Pull most recent master.
my @pull_output_ = `cd $MASTER_INFRA_LOCATION ; git checkout master ; git pull`;

print "Pull output :\n";
for (my $i = 0; $i <= $#pull_output_; $i++) {
    print $pull_output_[$i];
}
print "\n";

# We need to make updates to the following files:
# (i) Create new fxstreet csv file for today (SysInfo/FXStreetEcoReports/fxstreet_02032015_02032015.csv)
# (ii) Append these envents to this year's csv file (SysInfo/FXStreetEcoReports/fxstreet_eco_2015.csv)
# (iii) Merge events to our format to fxstreet eco file (SysInfo/FXStreetEcoReports/fxstreet_eco_2015_processed.txt)
# (iv) Merge events to our format to bloomberg eco file (SysInfo/BloombergEcoReports/merged_eco_2015_processed.txt)

my $FXSTREET_ECOCALENDAR_URL = "http://www.fxstreet.com/economic-calendar";
my $MASTER_FX_PARSE_EXEC = "/home/dvcinfra/master/infracore_install/bin/parse_investingdotcom_economic_events";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ($yyyymmdd_);
my $yyyy_ = substr ($yyyymmdd_, 0, 4);
my $mm_ = substr ($yyyymmdd_, 4, 2);
my $dd_ = substr ($yyyymmdd_, 6, 2);

my $BACKUP_FX_DELETE_CSV = "/home/dvcinfra/master/infracore/SysInfo/FXStreetEcoReports/fxstreet_".$mm_.$dd_.$yyyy_."_".$mm_.$dd_.$yyyy_.".csv"; # (i)
my $BACKUP_FX_MERGED_CSV = $BACKUP_LOCATION."fxstreet_eco_2015.csv"; # (ii)
my $BACKUP_FX_MERGED_TXT = $BACKUP_LOCATION."fxstreet_eco_2015_processed.txt"; # (iii)
my $BACKUP_BB_MERGED_TXT = $BACKUP_LOCATION."merged_eco_2015_processed.txt"; # (iv)

my $MASTER_FX_NEW_CSV = "/home/dvcinfra/master/infracore/SysInfo/FXStreetEcoReports/fxstreet_".$mm_.$dd_.$yyyy_."_".$mm_.$dd_.$yyyy_.".csv"; # (i)
my $MASTER_FX_MERGED_CSV = $MASTER_INFRA_LOCATION."/SysInfo/FXStreetEcoReports/fxstreet_eco_2015.csv"; # (ii)
my $MASTER_FX_MERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/FXStreetEcoReports/fxstreet_eco_2015_processed.txt"; # (iii)
my $MASTER_BB_MERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/merged_eco_2015_processed.txt"; # (iv)

# Check if today's initial events file was pulled from FXStreet or not
my $cur_date=`date +%Y%m%d`; chomp ($cur_date);
my $mod_date=`date +%Y%m%d -r $BACKUP_BB_MERGED_TXT`; chomp($mod_date);
if ( $cur_date ne $mod_date ) {
  print "Events file not pulled yet. Exiting\n";
  exit 0;
}

`rm $BACKUP_FX_DELETE_CSV`;
`cp $BACKUP_FX_MERGED_CSV $MASTER_FX_MERGED_CSV` ;
`cp $BACKUP_FX_MERGED_TXT $MASTER_FX_MERGED_TXT` ;
`cp $BACKUP_BB_MERGED_TXT $MASTER_BB_MERGED_TXT` ;

#cleanup
#`rm $BACKUP_FX_MERGED_CSV $BACKUP_FX_MERGED_TXT $BACKUP_BB_MERGED_TXT` ;

#Disabling reverts for now
#my $auto_revert_message_ = "$yyyymmdd_ : FX-STREET Auto-Revert";
#my @revert_commit_output_ = `cd $MASTER_INFRA_LOCATION ; git add SysInfo ; git commit -v -m \"$auto_revert_message_\" SysInfo ;`;

## Fetch the html file (disabled to use the file provided as argument)
#`wget -nv -O $temp_fx_html_ \"$FXSTREET_ECOCALENDAR_URL\"`;
# Extract events and dump out to csv (i)
`$MASTER_FX_PARSE_EXEC $temp_fx_html_ $MASTER_FX_NEW_CSV`;
# Merge new events to existing csv events list (ii)
`cat $MASTER_FX_NEW_CSV >> $MASTER_FX_MERGED_CSV`;

# (iii) & (iv) need to be done remotely (perl modules)

# Filter+merge scripts to be run on local m/c need the following files:
# fxstreet_eco_2015.csv, bbg_us_eco_2015_processed.txt, fxstreet_eco_2015_processed.txt, auction_us_eco_2015_processed.txt, merged_eco_2015_processed.txt
# fxstreet_eco_2015.csv
`scp $MASTER_FX_MERGED_CSV dvcinfra\@10.23.74.52:/home/dvcinfra/fxstreet`;
# bbg_us_eco_2015_processed.txt
my $MASTER_BB_UNMERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/bbg_us_eco_2015_processed.txt";
`scp $MASTER_BB_UNMERGED_TXT dvcinfra\@10.23.74.52:/home/dvcinfra/fxstreet`;
# fxstreet_eco_2015_processed.txt
`scp $MASTER_FX_MERGED_TXT dvcinfra\@10.23.74.52:/home/dvcinfra/fxstreet`;
# auction_us_eco_2015_processed.txt
my $MASTER_AUCTION_UNMERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/auction_us_eco_2015_processed.txt";
`scp $MASTER_AUCTION_UNMERGED_TXT dvcinfra\@10.23.74.52:/home/dvcinfra/fxstreet`;
# merged_eco_2015_processed.txt
`scp $MASTER_BB_MERGED_TXT dvcinfra\@10.23.74.52:/home/dvcinfra/fxstreet`;

# Run filter+merge scripts on local m/c
`sh /home/dvcinfra/fxstreet/xyz.sh`;
`sh /home/dvcinfra/fxstreet/abc.sh`;

# (i) & (ii) are done.
# Sync back (iii) & (iv).
`scp dvcinfra\@10.23.74.52:fxstreet/fxstreet_eco_2015_processed.txt $MASTER_FX_MERGED_TXT`;
`scp dvcinfra\@10.23.74.52:fxstreet/merged_eco_2015_processed.txt $MASTER_BB_MERGED_TXT`;

# Remove temporary copies on local desktop
`ssh dvcinfra\@10.23.74.52 'rm -f /home/dvcinfra/fxstreet/*.csv /home/dvcinfra/fxstreet/*.txt'`;

# Push to master branch.
my $auto_commit_message_ = "$yyyymmdd_ : FX-STREET Auto Reconcile Update Commit";
my @commit_output_ = `cd $MASTER_INFRA_LOCATION ; git add SysInfo ; git commit -v -m \"$auto_commit_message_\" SysInfo ; git push origin master`;

print "Commit message :\n";
for (my $i = 0; $i <= $#commit_output_; $i++) {
    print $commit_output_[$i];
}
print "\n";

# Mail the changes so we can catch erroneous commits to master.
#`cd $MASTER_INFRA_LOCATION ; git diff HEAD HEAD~2 SysInfo/BloombergEcoReports/merged_eco_2015_processed.txt | /bin/mail -s \"$auto_commit_message_\" nseall@tworoads.co.in`;

# Remove local temporary files
#`rm -rf $temp_fx_html_`;
