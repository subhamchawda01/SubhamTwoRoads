rm tmp_file_for_path_analysis

for file in `find $1` 
do
	echo $file;
	sed -e '0,/Ready/d' $file | awk -F',\t' '{print $2"\t"$7"\t"$12}' >> tmp_file_for_path_analysis;
done

python3 $HOME/basetrade/hftrap/Scripts/path_analysis.py --path tmp_file_for_path_analysis --bash 1

rm tmp_file_for_path_analysis
