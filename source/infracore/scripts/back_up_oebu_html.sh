today=`date +\%Y\%m\%d`

HTML_HOME_DIR='/var/www/html'
HTML_BACKUP_DIR='/var/www/html/backup'
#copy html files
cp "$HTML_HOME_DIR"/ind15.html "$HTML_BACKUP_DIR"/ind15.html.$today.cp
cp "$HTML_HOME_DIR"/ind20.html "$HTML_BACKUP_DIR"/ind20.html.$today.cp
cp "$HTML_HOME_DIR"/ind20_options.html "$HTML_BACKUP_DIR"/ind20_options.html.$today.cp
cp "$HTML_HOME_DIR"/ind14_options.html "$HTML_BACKUP_DIR"/ind14_options.html.$today.cp
cp "$HTML_HOME_DIR"/ind19.html "$HTML_BACKUP_DIR"/ind19.html.$today.cp
cp "$HTML_HOME_DIR"/ind18.html "$HTML_BACKUP_DIR"/ind18.html.$today.cp
cp "$HTML_HOME_DIR"/ind17.html "$HTML_BACKUP_DIR"/ind17.html.$today.cp
cp "$HTML_HOME_DIR"/ind16.html "$HTML_BACKUP_DIR"/ind16.html.$today.cp

#copy json files
cp "$HTML_HOME_DIR"/oebu_info15.json "$HTML_BACKUP_DIR"/oebu_info15.json.$today
cp "$HTML_HOME_DIR"/oebu_info20.json "$HTML_BACKUP_DIR"/oebu_info20.json.$today
cp "$html_home_dir"/oebu_info20_options.json "$html_backup_dir"/oebu_info20_options.json.$today
cp "$html_home_dir"/oebu_info14_options.json "$html_backup_dir"/oebu_info14_options.json.$today
cp "$HTML_HOME_DIR"/oebu_info19.json "$HTML_BACKUP_DIR"/oebu_info19.json.$today
cp "$HTML_HOME_DIR"/oebu_info18.json "$HTML_BACKUP_DIR"/oebu_info18.json.$today
cp "$HTML_HOME_DIR"/oebu_info17.json "$HTML_BACKUP_DIR"/oebu_info17.json.$today
cp "$HTML_HOME_DIR"/oebu_info16.json "$HTML_BACKUP_DIR"/oebu_info16.json.$today

cp "$HTML_HOME_DIR"/market_data15.json "$HTML_BACKUP_DIR"/market_data15.json.$today
cp "$HTML_HOME_DIR"/market_data20.json "$HTML_BACKUP_DIR"/market_data20.json.$today
cp "$HTML_HOME_DIR"/market_data20_options.json "$HTML_BACKUP_DIR"/market_data20_options.json.$today
cp "$HTML_HOME_DIR"/market_data14_options.json "$HTML_BACKUP_DIR"/market_data14_options.json.$today
cp "$HTML_HOME_DIR"/market_data19.json "$HTML_BACKUP_DIR"/market_data19.json.$today
cp "$HTML_HOME_DIR"/market_data18.json "$HTML_BACKUP_DIR"/market_data18.json.$today
cp "$HTML_HOME_DIR"/market_data17.json "$HTML_BACKUP_DIR"/market_data17.json.$today
cp "$HTML_HOME_DIR"/market_data16.json "$HTML_BACKUP_DIR"/market_data16.json.$today


#replace the json src
cat "$HTML_BACKUP_DIR"/ind15.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/ind15.html.$today
cat "$HTML_BACKUP_DIR"/ind20.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/ind20.html.$today
cat "$HTML_BACKUP_DIR"/ind20_options.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/ind20_options.html.$today
cat "$HTML_BACKUP_DIR"/ind14_options.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/ind14_options.html.$today
cat "$HTML_BACKUP_DIR"/ind19.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/ind19.html.$today
cat "$HTML_BACKUP_DIR"/ind18.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/ind18.html.$today
cat "$HTML_BACKUP_DIR"/ind17.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/ind17.html.$today
cat "$HTML_BACKUP_DIR"/ind16.html.$today.cp | sed -e "s|\.json|.json.${today}|g" > "$HTML_BACKUP_DIR"/ind16.html.$today

rm "$HTML_BACKUP_DIR"/ind15.html.$today.cp "$HTML_BACKUP_DIR"/ind20.html.$today.cp  "$HTML_BACKUP_DIR"/ind19.html.$today.cp "$HTML_BACKUP_DIR"/ind18.html.$today.cp "$HTML_BACKUP_DIR"/ind17.html.$today.cp "$HTML_BACKUP_DIR"/ind16.html.$today.cp "$HTML_BACKUP_DIR"/ind14_options.html.$today.cp "$HTML_BACKUP_DIR"/ind20_options.html.$today.cp
