git config --global core.editor "emacs -nw"
git config --global color.diff auto
git config --global color.status auto
git config --global color.branch auto
#
git config --global alias.st status
git config --global alias.df diff
git config --global alias.co checkout
git config --global alias.br branch
#git config --global alias.up rebase
git config --global alias.ci commit
#
SHORTHOSTNAME=`hostname -s | sed 's#sdv-##g'`
git config --global user.name $USER"_"$SHORTHOSTNAME
git config --global user.email $USER"@circulumvite.com"
