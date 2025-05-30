#!/bin/bash

rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.83:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.63:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.64:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.65:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.69:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.81:/spare/local/files --delete-after
rsync -ravz /spare/local/files/NSEFTPFiles 10.23.227.82:/spare/local/files --delete-after
rsync -avz --progress /spare/local/files/NSEFTPFiles 3.89.148.73:/spare/local/files

