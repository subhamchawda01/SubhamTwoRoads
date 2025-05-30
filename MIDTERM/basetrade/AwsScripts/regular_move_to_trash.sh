exceptions_file="/home/dvctrader/basetrade/AwsScripts/exceptions.txt"
trash_folder="/spare/local/trash/"
if [ $# -ge 1 ]; then
	exceptions_file=$1;
fi

function exception {
	myfile=$1;
	present=`grep -x $myfile $exceptions_file`
	if [ "$present" == "" ] && [ ! -L "$myfile" ] && [ -e "$myfile" ]; then
		remove=1;
	else
		remove=0;
	fi
}

#Clean dvctrader user folder
cd /home/dvctrader
mkdir -p $trash_folder
date
ls | while read file; do
	exception $file
	if [ $remove -eq 1 ]; then
		echo $file
		if [ -f "$file" ]; then
			mv "$file" "$trash_folder"
		else
			rsync -ra "$file/" "$trash_folder/$file/"
			rm -rf "$file"
		fi
	fi
done 
