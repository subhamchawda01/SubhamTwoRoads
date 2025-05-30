chgrp -R execgrp /spare/local/tradeinfo
chmod -R g+rx /spare/local/tradeinfo
chmod g+w /spare/local/tradeinfo
find /spare/local/tradeinfo -type d -exec chmod g+rwxs {} \;
