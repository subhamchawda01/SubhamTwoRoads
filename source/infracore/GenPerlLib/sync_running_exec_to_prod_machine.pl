# \file GenPerlLib/sync_to_all_machines.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
# my $REPO="infracore";

# my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $HOSTNAMEIP="hostname -i"; chomp($HOSTNAMEIP);

sub SyncRunningExecToProdMachine {

    my $prod_machine_ = shift ;
    my $input_filename_ = shift;
    my $backup_name_ = shift ;
    my $changes_description_ = shift ;

    my $dirname_ = dirname($input_filename_);
    my $execname_ = basename ( $input_filename_ ) ;
    my $execbackupname_ = $input_filename_.".~".$backup_name_."~" ;

    my $tempcopy_ = "/tmp/".$execname_ ;

    my $debug = 0;

    my $remote_machine_ = $prod_machine_ ;

    if ( $remote_machine_ ne $HOSTNAMEIP )
    {
        if ( $debug == 1 )
        {
            print "ssh $remote_machine_ mkdir -p $dirname_"."\n";
            print "ssh $remote_machine_ mv $input_filename_ $execbackupname_"."\n";
            print "scp $input_filename_ $USER\@$remote_machine_:$input_filename_"."\n";
        }

        `ssh $remote_machine_ mkdir -p $dirname_`;
        #Copy the exec to backup location (not mv)
        `ssh $remote_machine_ cp $input_filename_ $execbackupname_`;
        #rsync will handle failure itself
        `rsync -z $input_filename_ $USER\@$remote_machine_:$input_filename_`;
        my $mdsum = `ssh $remote_machine_ md5sum $input_filename_` ;

        print " Location : $remote_machine_ MD5SUM : $mdsum \n" ;

        my $mail_body_ = "Exec Update For $remote_machine_ ,  UpdatedExec :  $input_filename_ PrevStableExec : $execbackupname_ \n";

        open ( MAIL , "|/usr/sbin/sendmail -t" );
        print MAIL "To: ravi.parikh\@tworoads.co.in, nseall@tworoads.co.in \n";
        print MAIL "Subject: Production Update - $changes_description_\n\n";
        print MAIL $mail_body_;
        close(MAIL);

        #Special case for tradeinit exec (update algocode db) and destination FR2
        if ( ( $execname_ eq "tradeinit" ) and
          ( ( $remote_machine_ eq "10.23.102.55" ) or ( $remote_machine_ eq "10.23.102.56" ) or ( $remote_machine_ eq "10.23.102.53" ) or ( $remote_machine_ eq "10.23.102.54" )
          or ( $remote_machine_ eq "10.23.200.55" ) or ( $remote_machine_ eq "10.23.200.56" ) or ( $remote_machine_ eq "10.23.200.53" ) or ( $remote_machine_ eq "10.23.200.54" ) ) ) {
            my $db_file_ = "/spare/local/files/EUREX/eti_md5sum_strategy_algocode_database.db";
            my $newmd5sum = `echo "$mdsum" | awk '{print \$1;}'`;

            #Trimming strings (required for awk)
            $newmd5sum =~ s/^\s+|\s+$//g ;
            $changes_description_ =~ s/^\s+|\s+$//g ;

            `scp $USER\@$remote_machine_:$db_file_ $db_file_.tmp_$remote_machine_`;
            `rm -rf $db_file_.new_$remote_machine_`;
            `grep -m 1 "#MD5SUM" "$db_file_.tmp_$remote_machine_" >> "$db_file_.new_$remote_machine_"`;
            `cat "$db_file_.tmp_$remote_machine_" | awk '{ if( ( substr(\$1,1,1) != "#" ) && ( \$1 != "CONSOLE" ) )print \$2, \$3;}' | sort -nk 2 | uniq | awk '{print "$newmd5sum",\$0,"$changes_description_";}' >> "$db_file_.new_$remote_machine_"`;
            `grep -m 1 "CONSOLE" "$db_file_.tmp_$remote_machine_" >> "$db_file_.new_$remote_machine_"`;
            `scp "$db_file_.new_$remote_machine_" $USER\@$remote_machine_:"$db_file_.new_$remote_machine_"`;
            `ssh $remote_machine_ mv $db_file_.new_$remote_machine_ $db_file_`;
            `rm -rf $db_file_.new_$remote_machine_`;
            `rm -rf $db_file_.tmp_$remote_machine_`;

        }

    }
}

sub SyncRunningExecToProdMachineUser {

    my $prod_user_ = shift ;
    my $prod_machine_ = shift ;
    my $input_filename_ = shift;
    my $dest_filename_ = shift;
    my $backup_name_ = shift ;
    my $changes_description_ = shift ;

    print "$dest_filename_\n";
    my $dirname_ = dirname($dest_filename_);
    my $execname_ = basename ( $dest_filename_ ) ;
    my $execbackupname_ = $dest_filename_.".~".$backup_name_."~" ;

    my $tempcopy_ = "/tmp/".$execname_ ;

    my $remote_machine_ = $prod_machine_ ;

    my $debug = 0;
    if ( $remote_machine_ ne $HOSTNAMEIP )
    {
        if ( $debug == 1 )
        {
            print "ssh $prod_user_\@$remote_machine_ mkdir -p $dirname_"."\n";
            print "ssh $prod_user_\@$remote_machine_ mv $dest_filename_ $execbackupname_"."\n";
            print "scp $input_filename_ $prod_user_\@$remote_machine_:$dest_filename_"."\n";
        }

        `ssh $prod_user_\@$remote_machine_ mkdir -p $dirname_`;
        #Copy the exec to backup location (not mv)
        `ssh $prod_user_\@$remote_machine_ cp $dest_filename_ $execbackupname_`;
        #rsync will handle failure itself
        `rsync -z $input_filename_ $prod_user_\@$remote_machine_:$dest_filename_`;
        my $mdsum = `ssh $prod_user_\@$remote_machine_ md5sum $dest_filename_` ;

        print " Location : $prod_user_\@$remote_machine_ MD5SUM : $mdsum \n" ;

        my $mail_body_ = "Exec Update For $prod_user_\@$remote_machine_ ,  UpdatedExec :  $dest_filename_ PrevStableExec : $execbackupname_ \n";

        open ( MAIL , "|/usr/sbin/sendmail -t" );
        print MAIL "To: ravi.parikh\@tworoads.co.in, nseall@tworoads.co.in \n";
        print MAIL "Subject: Production Update - $changes_description_\n\n";
        print MAIL $mail_body_;
        close(MAIL);

        #Special case for tradeinit exec (update algocode db) and destination FR2
        if ( ( $execname_ eq "tradeinit" ) and
          ( ( $remote_machine_ eq "10.23.102.55" ) or ( $remote_machine_ eq "10.23.102.56" ) or ( $remote_machine_ eq "10.23.102.53" ) or ( $remote_machine_ eq "10.23.102.54" )
          or ( $remote_machine_ eq "10.23.200.55" ) or ( $remote_machine_ eq "10.23.200.56" ) or ( $remote_machine_ eq "10.23.200.53" ) or ( $remote_machine_ eq "10.23.200.54" ) ) ) {
            my $db_file_ = "/spare/local/files/EUREX/eti_md5sum_strategy_algocode_database.db";
            my $newmd5sum = `echo "$mdsum" | awk '{print \$1;}'`;

            #Trimming strings (required for awk)
            $newmd5sum =~ s/^\s+|\s+$//g ;
            $changes_description_ =~ s/^\s+|\s+$//g ;

            `scp $prod_user_\@$remote_machine_:$db_file_ $db_file_.tmp_$remote_machine_`;
            `rm -rf $db_file_.new_$remote_machine_`;
            `grep -m 1 "#MD5SUM" "$db_file_.tmp_$remote_machine_" >> "$db_file_.new_$remote_machine_"`;
            `cat "$db_file_.tmp_$remote_machine_" | awk '{ if( ( substr(\$1,1,1) != "#" ) && ( \$1 != "CONSOLE" ) )print \$2, \$3;}' | sort -nk 2 | uniq | awk '{print "$newmd5sum",\$0,"$changes_description_";}' >> "$db_file_.new_$remote_machine_"`;
            `grep -m 1 "CONSOLE" "$db_file_.tmp_$remote_machine_" >> "$db_file_.new_$remote_machine_"`;
            `scp "$db_file_.new_$remote_machine_" $prod_user_\@$remote_machine_:"$db_file_.new_$remote_machine_"`;
            `ssh $prod_user_\@$remote_machine_ mv $db_file_.new_$remote_machine_ $db_file_`;
            `rm -rf $db_file_.new_$remote_machine_`;
            `rm -rf $db_file_.tmp_$remote_machine_`;

        }

    }
}
1;
