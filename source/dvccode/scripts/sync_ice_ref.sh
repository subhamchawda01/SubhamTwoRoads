#!/bin/bash

scp /spare/local/files/ICE/ice-ref.txt dvcinfra@10.23.74.51:/spare/local/files/ICE/ice-ref.txt

ssh dvcinfra@10.23.74.51 "~/infracore/scripts/sync_file_to_all_machines.pl /spare/local/files/ICE/ice-ref.txt"
