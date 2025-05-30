#!/usr/bin/perl
use strict;
use warnings;

my $MASTER_INFRA_LOCATION = "/home/pengine/master/infracore";
my $BACKUP_LOCATION = "/home/pengine/master/fxstreet/backup/";

# Pull most recent master.
my @pull_output_ = `cd $MASTER_INFRA_LOCATION ; git checkout master ; git add dvccode; git commit dvccode -m "_"; git pull && git submodule foreach git pull`;

print "Pull output :\n";
for (my $i = 0; $i <= $#pull_output_; $i++) {
    print $pull_output_[$i];
}
print "\n";

my $year = `date +%Y` ; chomp ($year);

# We need to make updates to the following files:
# (i) Create new fxstreet csv file for today (SysInfo/FXStreetEcoReports/fxstreet_02032016_02032016.csv)
# (ii) Append these envents to this year's csv file (SysInfo/FXStreetEcoReports/fxstreet_eco_2016.csv)
# (iii) Merge events to our format to fxstreet eco file (SysInfo/FXStreetEcoReports/fxstreet_eco_2016_processed.txt)
# (iv) Merge events to our format to bloomberg eco file (SysInfo/BloombergEcoReports/merged_eco_2016_processed.txt)

my $FXSTREET_ECOCALENDAR_URL = "http://www.fxstreet.com/fundamental/economic-calendar";
my $MASTER_FX_PARSE_EXEC = "/home/pengine/master/infracore/scripts/parse_fxstreet_csv.sh";
#my $MASTER_FX_PARSE_EXEC = "/home/pengine/master/infracore_install/bin/parse_fxstreet_economic_events";
my $temp_fx_html_ = "/home/pengine/master/temp/fxstreet.html";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ($yyyymmdd_);
my $yyyy_ = substr ($yyyymmdd_, 0, 4);
my $mm_ = substr ($yyyymmdd_, 4, 2);
my $dd_ = substr ($yyyymmdd_, 6, 2);

my $MASTER_FX_NEW_CSV = "/home/pengine/master/infracore/SysInfo/FXStreetEcoReports/fxstreet_".$mm_.$dd_.$yyyy_."_".$mm_.$dd_.$yyyy_.".csv"; # (i)
my $MASTER_FX_MERGED_CSV = $MASTER_INFRA_LOCATION."/SysInfo/FXStreetEcoReports/fxstreet_eco_".$year.".csv"; # (ii)
my $MASTER_FX_MERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/FXStreetEcoReports/fxstreet_eco_".$year."_processed.txt"; # (iii)
my $MASTER_BB_MERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/merged_eco_".$year."_processed.txt"; # (iv)

`cp $MASTER_BB_MERGED_TXT $MASTER_FX_MERGED_TXT $MASTER_FX_MERGED_CSV $BACKUP_LOCATION` ;


# Fetch the csv file
my $access_token=`curl 'https://authorization.fxstreet.com/token' -H 'Origin: https://www.fxstreet.com' -H 'Accept-Encoding: gzip, deflate, br' -H 'Accept-Language: en-US,en;q=0.8' -H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36' -H 'Content-Type: application/x-www-form-urlencoded' -H 'Accept: application/json, text/javascript, */*; q=0.01' -H 'Referer: https://www.fxstreet.com/economic-calendar' -H 'Connection: keep-alive' --data 'grant_type=domain&client_id=client_id' --compressed  | python -c 'import json,sys;obj=json.load(sys.stdin);print obj["access_token"]'`;

`curl 'https://calendar.fxstreet.com/eventdate/?f=csv&v=2&timezone=UTC&rows=&view=current&countrycode=AU%2CCA%2CCN%2CRU%2CBR%2CMX%2CEMU%2CFR%2CDE%2CGR%2CIN%2CIT%2CJP%2CNZ%2CPT%2CES%2CCH%2CUK%2CUS&volatility=0&culture=en&columns=CountryCurrency%2CCountdown' -H 'Accept-Encoding: gzip, deflate, sdch, br' -H 'Accept-Language: en-US,en;q=0.8' -H 'Upgrade-Insecure-Requests: 1' -H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36' -H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8' -H 'Referer: https://www.fxstreet.com/economic-calendar' -H 'Cookie: ARRAffinity=641ce0822f2461d1e3ac4c2b8261f36e5965ce8ae1dbd1ab6f81977092615db9; _ga=GA1.2.1058546414.1497001793; _gid=GA1.2.1777141227.1507010315; _gat_UA-327849-1=1' -H 'Connection: keep-alive' -H "Authorization: bearer $access_token" --compressed > $temp_fx_html_`;

#`wget -nv -O $temp_fx_html_ \"$FXSTREET_ECOCALENDAR_URL\"`;
# Extract events and dump out to csv (i)
`$MASTER_FX_PARSE_EXEC $temp_fx_html_ $MASTER_FX_NEW_CSV`;
# Merge new events to existing csv events list (ii)
`cat $MASTER_FX_NEW_CSV >> $MASTER_FX_MERGED_CSV`;

# (iii) & (iv) need to be done remotely (perl modules)

# Filter+merge scripts to be run on local m/c need the following files:
# fxstreet_eco_2016.csv, bbg_us_eco_2016_processed.txt, fxstreet_eco_2016_processed.txt, auction_us_eco_2016_processed.txt, wasde_us_eco_2016_processed.txt, merged_eco_2016_processed.txt
# fxstreet_eco_2016.csv
`scp $MASTER_FX_MERGED_CSV pengine\@10.23.74.52:/home/pengine/fxstreet`;
# bbg_us_eco_2016_processed.txt
my $MASTER_BB_UNMERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/bbg_us_eco_".$year."_processed.txt";
`scp $MASTER_BB_UNMERGED_TXT pengine\@10.23.74.52:/home/pengine/fxstreet`;
# fxstreet_eco_2016_processed.txt
`scp $MASTER_FX_MERGED_TXT pengine\@10.23.74.52:/home/pengine/fxstreet`;
# auction_us_eco_2016_processed.txt
my $MASTER_AUCTION_UNMERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/auction_us_eco_".$year."_processed.txt";
`scp $MASTER_AUCTION_UNMERGED_TXT pengine\@10.23.74.52:/home/pengine/fxstreet`;
# wasde_us_eco_2016_processed.txt,
my $MASTER_WASDE_UNMERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/wasde_us_eco_".$year."_processed.txt";
`scp $MASTER_WASDE_UNMERGED_TXT pengine\@10.23.74.52:/home/pengine/fxstreet`;
# merged_eco_2016_processed.txt
`scp $MASTER_BB_MERGED_TXT pengine\@10.23.74.52:/home/pengine/fxstreet`;

# Run filter+merge scripts on local m/c
`sh /home/pengine/prod/live_scripts/generate_fxstreet_eco_file.sh`;
`sh /home/pengine/prod/live_scripts/generate_merged_eco_file.sh`;



my $LOCAL_FX_MERGED_TXT = "/home/pengine/fxstreet/fxstreet_eco_".$year."_processed.txt" ;
my $LOCAL_BB_MERGED_TXT = "/home/pengine/fxstreet/merged_eco_".$year."_processed.txt" ;

# (i) & (ii) are done.
# Sync back (iii) & (iv).
`scp pengine\@10.23.74.52:$LOCAL_FX_MERGED_TXT $MASTER_FX_MERGED_TXT`;
`scp pengine\@10.23.74.52:$LOCAL_BB_MERGED_TXT $MASTER_BB_MERGED_TXT`;

# Remove temporary copies on local desktop
`ssh pengine\@10.23.74.52 'rm -f /home/pengine/fxstreet/*.csv /home/pengine/fxstreet/*.txt'`;

# Push to master branch.
my $auto_commit_message_ = "$yyyymmdd_ : FX-STREET Auto Commit";
my @commit_output_ = `cd $MASTER_INFRA_LOCATION ; git add SysInfo ; git commit -v -m \"$auto_commit_message_\" SysInfo ; git push origin master`;

print "Commit message :\n";
for (my $i = 0; $i <= $#commit_output_; $i++) {
    print $commit_output_[$i];
}
print "\n";

# Mail the changes so we can catch erroneous commits to master.
`cd $MASTER_INFRA_LOCATION ; git show | /bin/mail -s \"$auto_commit_message_\" pengine\@circulumvite.com`;

# Remove local temporary files
`rm -rf $temp_fx_html_`;


