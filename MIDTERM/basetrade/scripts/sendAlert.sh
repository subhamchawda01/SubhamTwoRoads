all_args=""
for var in "$@"
do
all_args=$all_args" "$var
done
ENCODED=$(echo -n $all_args | \
perl -pe's/([^-_.~A-Za-z0-9])/sprintf("%%%02X", ord($1))/seg');
URL=`head -1 /spare/local/files/alert_server_url `
curl http://$URL:7980/?msg=$ENCODED > /dev/null 2>&1
