#!/bin/bash

find /spare/local/$USER/DataGenOut -type d -mtime +10 -exec rm -rf {} \;
find /spare/local/$USER/DataGenOut -type f -atime +5 -exec rm -f {} \;

for i in 0 1 2 3; do
find /spare$i/local/$USER/DataGenOut -type d -mtime +10 -exec rm -rf {} \;
find /spare$i/local/$USER/DataGenOut -type f -atime +5 -exec rm -f {} \;
done
