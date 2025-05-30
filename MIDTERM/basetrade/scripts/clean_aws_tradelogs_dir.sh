#!/bin/bash

find /spare/local/logs/tradelogs -type f -atime +2 -exec rm -f {} \;
find /spare/local/logs/tradelogs -type f -name log.\* -mtime +1 -exec rm -f {} \;
find /spare/local/logs/datalogs -type f -atime +1 -exec rm -f {} \;

for i in 0 1 2 3; do
find /spare$i/local/logs/tradelogs -type f -atime +2 -exec rm -f {} \;
find /spare$i/local/logs/tradelogs -type f -name log.\* -mtime +1 -exec rm -f {} \;
find /spare$i/local/logs/datalogs -type f -atime +1 -exec rm -f {} \;
done
