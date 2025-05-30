#!/bin/bash
path="/home/dvcinfra/important/gen_html_link/"
DIR_PATH="/var/www/html/"
REPORTING_FILE="/var/www/html/url_list.html"

declare -A Server_ip
  Server_ip=( ["IND11"]="10.23.227.61" \
                ["IND12"]="10.23.227.62" \
                ["IND13"]="10.23.227.63" \
                ["IND14"]="10.23.227.64" \
                ["IND15"]="10.23.227.65" \
                ["IND16"]="10.23.227.81" \
                ["IND17"]="10.23.227.82" \
                ["IND18"]="10.23.227.83" \
                ["IND19"]="10.23.227.69" \
		["LOCAL66"]="10.23.5.66" \
                ["LOCAL67"]="10.23.5.67" \
                ["LOCAL42"]="10.23.5.42" \
                ["IND20"]="10.23.227.84")


'Server_ip=( ["IND11"]="10.23.5.42" )'

>$REPORTING_FILE ;
cat $path"gen_html_link_header.txt" > $REPORTING_FILE ;
echo "<th>SERVER</th><th>PROJECT</th><th>URL</th>" >> $REPORTING_FILE;
echo "</tr></thead><tbody>" >> $REPORTING_FILE ;

for server in "${!Server_ip[@]}"; do
    echo $server
    echo "ssh ${Server_ip[$server]} ls $DIR_PATH"
   for file in `ssh ${Server_ip[$server]} "ls $DIR_PATH"`;do

	if ssh ${Server_ip[$server]} "[ -d $DIR_PATH$file ]"; then
		if ssh ${Server_ip[$server]} stat $DIR_PATH$file"/index.html" \> /dev/null 2\>\&1 
			then
                        echo "index file found"
			echo "<tr><td>$server</td><td>$file</td><td><a target="_blank" href=//${Server_ip[$server]}"/"$file"/index.html">${Server_ip[$server]}"/"$file"/index.html"</a></td></tr>" >> $REPORTING_FILE
		fi 
	elif [[ $file == *"html" ]] ; then
                echo "${Server_ip[$server]}"/"$file"
#			echo "<tr><td>$server</td><td>$file</td><td><a target="_blank" href=//${Server_ip[$server]}"/"$file>${Server_ip[$server]}"/"$file</a></td></tr>" >> $REPORTING_FILE
		
	fi
   done;
done;

#ADD FOOTER
cat $path"gen_html_link_footer.txt" >> $REPORTING_FILE;

