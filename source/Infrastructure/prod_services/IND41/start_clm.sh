#!/bin/bash

echo "Starting centralisez_logging_manager at $(date)"

echo "Clear resolve file for ors"
>/etc/resolv.conf
>/etc/resolve.conf

sudo runuser -l dvcinfra -c '/usr/bin/centralized_logging_manager'
