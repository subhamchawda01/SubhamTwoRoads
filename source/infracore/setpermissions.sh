chgrp -R infra ~/infracore
chmod -R g-w ~/infracore
chmod -R o-rwx ~/infracore
find ~/infracore -type d -exec chmod g+s {} \;
