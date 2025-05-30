#!/bin/bash


rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.66:/spare/local/tradeinfo
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.11:/spare/local/tradeinfo --delete-after
echo "sync indb12"
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.227.71:/spare/local/tradeinfo --delete-after
ssh 10.23.227.71 "rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.11:/spare/local/tradeinfo --delete-after"

ssh 192.168.132.11 "rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.12:/spare/local/tradeinfo --delete-after"
rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.12:/spare/local/tradeinfo --delete-after
ssh 10.23.227.71 "rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 192.168.132.12:/spare/local/tradeinfo --delete-after"

rsync -ravz --timeout=60 /spare/local/tradeinfo/BSE_Files 10.23.5.42:/spare/local/tradeinfo
rsync -avz --progress /spare/local/tradeinfo/BSE_Files 54.90.155.232:/spare/local/tradeinfo 
rsync -avz --progress /spare/local/tradeinfo/BSE_Files 44.202.186.243:/spare/local/tradeinfo
rsync -avz --progress /spare/local/tradeinfo/BSE_Files 10.23.5.62:/spare/local/tradeinfo
rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.66:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.42:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 10.23.5.62:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.11:/spare/local/files --delete-after
rsync -ravz /spare/local/files/BSEFTPFiles 10.23.227.71:/spare/local/files --delete-after
ssh 10.23.227.71 "rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.11:/spare/local/files --delete-after"
ssh 192.168.132.11 "rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.12:/spare/local/files --delete-after"
ssh 10.23.227.71 "rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.12:/spare/local/files --delete-after"

rsync -ravz /spare/local/files/BSEFTPFiles 192.168.132.12:/spare/local/files --delete-after
rsync -ravz /spare/local/files/BSEFTPFiles 54.90.155.232:/spare/local/files
rsync -ravz /spare/local/files/BSEFTPFiles 44.202.186.243:/spare/local/files
