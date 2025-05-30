#!/bin/bash

# iterate over all ephemeral mounts
for i in 1 2
do
    cd /media/disk-ephemeral$i/s3_cache/NAS1/data;
    echo "/media/disk-ephemeral$i/s3_cache/NAS1/data";
    CURRENT_USAGE=`du -sc . | head -1 | awk '{print $1}'`;
    echo "current_usage: $CURRENT_USAGE";

    # if the current s3_cache usage is more that 100GB, cleanup
    while [ $CURRENT_USAGE -gt 100000000 ]
    do
        FILES_TO_DELETE=`find . -type f -printf "%T@ %p\n" | sort -n | head -20 | awk '{print $2}'`;
        for FILE in $FILES_TO_DELETE
        do
            echo $FILE;
            rm $FILE;
        done
        CURRENT_USAGE=`du -sc . | head -1 | awk '{print $1}'`;
        echo "current_usage: $CURRENT_USAGE";
    done
done
