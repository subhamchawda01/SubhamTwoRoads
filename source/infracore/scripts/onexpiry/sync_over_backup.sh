rsync -ravz --timeout=60 -e "ssh -p 22761" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22762" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22763" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22764" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22765" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22781" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22782" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22783" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22784" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz --timeout=60 -e "ssh -p 22769" /spare/local/tradeinfo/NSE_Files 202.189.245.205:/spare/local/tradeinfo --delete-after
rsync -ravz -e "ssh -p 22761" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz -e "ssh -p 22763" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz -e "ssh -p 22764" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz -e "ssh -p 22765" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz -e "ssh -p 22781" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz -e "ssh -p 22782" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz -e "ssh -p 22783" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
rsync -ravz -e "ssh -p 22784" /spare/local/files/NSEFTPFiles 202.189.245.205:/spare/local/files --delete-after
#rsync -avz --progress /spare/local/files/NSEFTPFiles 3.89.148.73:/spare/local/files
rsync -avz --progress /spare/local/files/NSEFTPFiles 52.3.22.99:/spare/local/files
