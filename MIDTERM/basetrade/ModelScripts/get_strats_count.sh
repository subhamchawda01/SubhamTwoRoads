#!/bin/bash
`perl ~/basetrade/GenPerlLib/partition_strats_based_on_time.pl`;
`~/infracore/scripts/mail_file_to.sh Strats_Count nseall@tworoads.co.in ~/strats_count.txt`;
