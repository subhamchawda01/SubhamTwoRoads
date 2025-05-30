<h1>Sync Service</h1>

We have different machines used for different purposes. This is to ease the sync of namely fetch and other important files.
Plan is to
* Create a common config file, which will have IP and Mapping
* Inclusion of a new server to the setup should be smooth
* Script and cols added should be scalable

<h1> Config File Sample </h1>

```
IND23 192.168.155.14 N
IND12 192.168.155.12 N
IND15 192.168.155.18 N
INDB11 192.168.132.11 B
INDB12 192.168.132.12 B
LOCAL62 10.23.5.62 L
```

Idea is to load 1st and 2nd col in a map based on all other rows

<h1>Sample Code Run to Load</h1>
This map in turn can be used in scritps

Code File: https://github.com/two-roads-trading-pvt-ltd/Infrastructure/blob/main/sync_service/use_config.sh
```
(base) {15:05}~ ➭ /home/gopi/config_maps/use_map.sh LOCAL 
Contents of the map:
LOCAL72 -> 10.23.5.14
LOCAL71 -> 10.23.5.10
LOCAL70 -> 10.23.5.70
LOCAL26 -> 10.23.5.26
LOCAL68 -> 10.23.5.68
LOCAL69 -> 10.23.5.69
LOCAL62 -> 10.23.5.62
LOCAL66 -> 10.23.5.66
LOCAL67 -> 10.23.5.67
```
```
(base) {15:05}~ ➭ /home/gopi/config_maps/use_map.sh NSE
Contents of the map:
IND13 -> 10.23.227.63
IND12 -> 192.168.155.12
IND11 -> 10.23.227.61
IND17 -> 192.168.155.13
IND15 -> 192.168.155.18
IND14 -> 10.23.227.64
IND19 -> 10.23.227.69
IND18 -> 192.168.155.16
IND25 -> 10.23.227.75
IND22 -> 10.23.227.71
IND23 -> 192.168.155.14
IND20 -> 10.23.227.84
IND21 -> 10.23.227.66
IND28 -> 10.23.227.68
IND29 -> 10.23.227.79
IND30 -> 10.23.227.80
(base) {15:05}~ ➭ 
```
```
(base) {15:05}~ ➭ /home/gopi/config_maps/use_map.sh BSE
Contents of the map:
INDB15 -> 192.168.132.15
INDB14 -> 192.168.132.14
INDB11 -> 192.168.132.11
INDB13 -> 192.168.132.13
INDB12 -> 192.168.132.12
(base) {15:05}~ ➭ 
```

<h1>Development and Usage</h1>

We can keep adding new cols based on our req, some ideas...
* Adding rack info to handle connection drop of specific rack.
* Adding T or L based on Logging or Trading server.
* A or I based on active or inactive.
