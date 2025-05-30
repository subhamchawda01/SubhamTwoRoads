#!/usr/bin/env bash

find /mnt/sdf/ec2_globalresults/ -name results_database.txt | xargs $HOME/basetrade/scripts/remove_duplicates_results_from_file.sh
