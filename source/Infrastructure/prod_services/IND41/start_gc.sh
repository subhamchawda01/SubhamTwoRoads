#!/bin/bash

echo "Starting garbage collector at $(date)"

/home/pengine/prod/live_scripts/move_trash_to_trash.sh &
