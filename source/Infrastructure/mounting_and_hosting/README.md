<h1>Disk Mounting Scripts</h1>

<h2>Steps to mount new driver</h2>
New drive should always be formatted first and assigned a label accordingly.

* format the new empty drive
* assign type as ext4
* assign label

```
sudo mkfs.ext4 -L LABEL /dev/nvmeXXX
```

<h2>On reboot</h2>
We have drives attached(NVMEs & HDDs) to worker and local machines. On reboot these scripts handle the mounting of disks.
Once drives are mounted there are services we host on workers

* Jupyter Notebooks
* Jupyter Hub
* Metabase

On reboot, we have automated scripts on run these processes as well.

<h3>How to run scripts on reboot?</h3>
Add in crontab

```
@reboot /path/to/your/script.sh
```

On each worker you can find these crons in root user
```
##========================== AUTO MOUNT & PROCESSES =======================================#
@reboot /root/automount.sh > /root/automount.$(date +\%s).log
@reboot /root/autoup.sh > /root/autoup.$(date +\%s).log

```

These scripts are different for different workers accordingly to their labels and processes(services).
* First the automount script runs which mounts all the drives.
* `autoup.sh` will sleep for 30 seconds(just to give time for disk mounting)

<h3>Automount script failed</h3>
In case of failure of mounting scripts:

* Check logs
* Open the script to find label(or UUID) mapping with mount-points
* Simply mount these using `mount /dev/nvmeXXX /MOUNT_POINT`

<h3>Process(services) up script failed</h3>
In case of failure or autoup scripts:

* Check logs
* Refer the scripts
* Mostly issue will be in mounting only

<h2>Sym Links</h2>
Check respective folders

* PATH: /home/dvcinfra/trash/symlink.sh
* Run from dvcinfra user
