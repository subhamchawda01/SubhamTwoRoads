#!/bin/bash

# Make folders for zabbix
echo "Creating folders..."
mkdir -p /var/run/zabbix
mkdir -p /var/log/zabbix

echo "Starting Zabbix at $(date)"
/usr/sbin/zabbix_agentd
