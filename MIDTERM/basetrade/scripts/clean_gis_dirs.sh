#!/bin/bash

find /spare/local/$USER/DataGenOut -type d -mtime +2 -exec rm -rf {} \;
find /spare/local/$USER/DataGenOut -type f -atime +2 -exec rm -f {} \;
