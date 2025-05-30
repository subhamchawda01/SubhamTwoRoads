NEW_HEAD=$HOME/basetrade_new_head
PREV_HEAD=$HOME/basetrade_prev_head
WORK_DIR=$HOME/basetrade

cd $NEW_HEAD ;
git pull -q ;
diff -x .git -x bin -r -w $PREV_HEAD $NEW_HEAD | egrep "diff|Only";

echo "Enter show to show full changes or manually run the diff commands written above";
read show_val ;
case $show_val in
    show|SHOW)

	diff -x .git -x bin -r -w $PREV_HEAD $NEW_HEAD ;

	;;
    *)
	;;
esac

echo "Enter yes to continue, no to stop";
read tce ;
case $tce in
    yes|YES)
	
	echo "Continuing with pull" ;
	cd $PREV_HEAD ;
	git pull -q ;
	cd $WORK_DIR ;
	git pull ;
	
	;;
    
    *)
	;;
esac
