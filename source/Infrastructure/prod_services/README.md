<h1>Production Services</h1>

* Autossh
* Garbage Collector
* Postfix
* Zabbix
* Centralized Logging Manager
* PTPD

<h2>Autossh</h2>

Autossh is a program that can keep alive and restart ssh sessions and tunnels. There are many things that work in it’s favor:

* It is lightweight.
* Uses ssh natively, meaning that all the ssh options and configurations that you have setup over time and have come to love (or hate) will work out of the box.

<h2>Garbage Collector</h2>

Its an in-house script, which keeps `/home/userX` fodlers clean.

* bash script
* On first run it redirects output of `ls` into a file
* /tmp/.dvcinfra_mastercopy_to_check
* /tmp/.dvctrader_mastercopy_to_check
* Anything that is not present in this file is moved into respective user's `~/trash/`

<h2>Postfix</h2>

Postfix is used to send mails.

* Config: `/etc/postfix/main.cf`
* Check relay host in config
* For GRT machines, we dont send mails directly, but rather via local26
* For GRT machines mail check Local26 root cron

<h2>Zabbix</h2>

For alerting.

* Config: `/etc/zabbix/zabbix_agentd.conf`

<h2>Centralized Logging Manager</h2>

Very important in-house executable which is responsible for logging of starts.

* Exec: `/usr/bin/centralized_logging_manager`
* Log: `/spare/local/logs/alllogs/logging_server.log`

<h2>PTPD</h2>

PTP daemon (PTPd) is an implementation the Precision Time Protocol (PTP) version 2 as defined by 'IEEE Std 1588-2008'. PTP provides precise time coordination of Ethernet LAN connected computers.

* Sync machine's clock with exchange clock
* Have to build & install it
* Exec: `/usr/local/sbin/ptpd2`
* Config: `/etc/sysconfig/ptpd2`
* Have to give interface in config, we have to give interface on with we assigned TAP IP
