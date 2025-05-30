
GLOBAL_REPOS=gitolite@10.1.3.11:basetrade
NEW_HEAD=$HOME/basetrade_new_head
PREV_HEAD=$HOME/basetrade_prev_head
WORK_DIR=$HOME/basetrade

cd $HOME

if [ ! -d $PREV_HEAD ] ; then 
    git clone $GLOBAL_REPOS $PREV_HEAD
fi

if [ ! -d $NEW_HEAD ] ; then 
    git clone $GLOBAL_REPOS $NEW_HEAD
fi
