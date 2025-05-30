#!/bin/bash

# change permissions for dvctrader access
chmod 444 /spare/local/files/RTS/tickvalues/* ;

# rsync to all ny
rsync -avpz /spare/local/files/RTS/tickvalues 10.23.74.51:/spare/local/files/RTS >/dev/null
rsync -avpz /spare/local/files/RTS/tickvalues 10.23.74.52:/spare/local/files/RTS >/dev/null
rsync -avpz /spare/local/files/RTS/tickvalues 10.23.74.53:/spare/local/files/RTS >/dev/null
rsync -avpz /spare/local/files/RTS/tickvalues 10.23.74.54:/spare/local/files/RTS >/dev/null
rsync -avpz /spare/local/files/RTS/tickvalues 10.23.74.55:/spare/local/files/RTS >/dev/null

chmod 444 /spare/local/files/RTS_P2/tickvalues/* ;

# rsync to all ny
rsync -avpz /spare/local/files/RTS_P2/tickvalues 10.23.74.51:/spare/local/files/RTS_P2 >/dev/null
rsync -avpz /spare/local/files/RTS_P2/tickvalues 10.23.74.52:/spare/local/files/RTS_P2 >/dev/null
rsync -avpz /spare/local/files/RTS_P2/tickvalues 10.23.74.53:/spare/local/files/RTS_P2 >/dev/null
rsync -avpz /spare/local/files/RTS_P2/tickvalues 10.23.74.54:/spare/local/files/RTS_P2 >/dev/null
rsync -avpz /spare/local/files/RTS_P2/tickvalues 10.23.74.55:/spare/local/files/RTS_P2 >/dev/null

