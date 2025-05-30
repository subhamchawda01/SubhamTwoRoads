#!/bin/bash
source ~/.bashrc
cat $1 | while read a ; do s3put $a s3://s3dvc$a ; done
