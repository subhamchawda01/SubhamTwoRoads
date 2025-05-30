today=`date +\%Y\%m\%d`

HTML_HOME_DIR='/var/www/html'
HTML_BACKUP_DIR='/var/www/html/backup'
#copy html files
cp "$HTML_HOME_DIR"/indb12.html "$HTML_BACKUP_DIR"/indb12.html.$today.cp

#copy json files
cp "$HTML_HOME_DIR"/oebu_info12.json "$HTML_BACKUP_DIR"/oebu_info12.json.$today

cp "$HTML_HOME_DIR"/market_data12.json "$HTML_BACKUP_DIR"/market_data12.json.$today


#replace the json src
cat "$HTML_BACKUP_DIR"/indb12.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/indb12.html.$today

rm "$HTML_BACKUP_DIR"/indb12.html.$today.cp
