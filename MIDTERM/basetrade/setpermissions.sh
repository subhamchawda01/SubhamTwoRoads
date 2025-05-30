chgrp -R execgrp ~/basetrade
chmod -R g-w ~/basetrade
chmod -R o-rwx ~/basetrade
find ~/basetrade -type d -exec chmod g+s {} \;
