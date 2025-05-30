#!/bin/bash

>store_circular_data.txt

today_date=`date '+%d-%m-%Y'`
pre_date=`date -d "1 day ago" '+%d-%m-%Y'`
echo "$today_date -  $pre_date"

curl "https://www.nseindia.com/api/circulars?&fromDate=${pre_date}&toDate=${today_date}" \
  -H 'authority: www.nseindia.com' \
  -H 'sec-ch-ua: "Chromium";v="94", "Google Chrome";v="94", ";Not A Brand";v="99"' \
  -H 'sec-ch-ua-mobile: ?0' \
  -H 'user-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36' \
  -H 'sec-ch-ua-platform: "Linux"' \
  -H 'accept: */*' \
  -H 'sec-fetch-site: same-origin' \
  -H 'sec-fetch-mode: cors' \
  -H 'sec-fetch-dest: empty' \
  -H 'referer: https://www.nseindia.com/resources/exchange-communication-circulars' \
  -H 'accept-language: en-US,en;q=0.9' \
  -H 'cookie: _ga=GA1.2.1714124599.1572843446; _gid=GA1.2.668873200.1636982723; AKA_A2=A; nsit=opdhkSECU2kjVgnXFccAqAli; nseappid=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhcGkubnNlIiwiYXVkIjoiYXBpLm5zZSIsImlhdCI6MTYzNjk5NDcxNSwiZXhwIjoxNjM2OTk4MzE1fQ.PznFUYWPu08MvA4Aj1AoM12xUKojv9q7sBFqXhScrVE; bm_mi=2D21085D7C54FF6153E71346A388CF95~tNuJLI36lnnJ4fvSu+7wk9zRCjbatilGkwCuv/0o0DtJo84hlSFfexWq1T1r6g2QxjlmJnqDCI/5K4EiRVU698/BR19crhycTD97aE4U5Tq101UjdFSbdcdDsnwPZ61D2dWxD6Hqwzb5VO9B/tnJKt2HFHZGCFE3j6AiHh81fKI6uHEalHeQUs8XsQPMk+aFMtIsG2S4RdzrLLf7uhzNTFTpcbWUHPqOy3VKoCquB3xOJZ02QRrnXQXLjZbIPkwrxI/REQaitQ6Ut/EEprRvb5f53ztSr4vYWyZFE8OvFczI1ItBKpRMCs4ELcaCzRANkIjsstkqPjNv/2bq+Hw5lw==; ak_bmsc=11692F044DD45A3B59265A953ECBF1B1~000000000000000000000000000000~YAAQzuvfrVRbB+l8AQAAT099JA2hIVllc91hGOwAPgetwaPEtf1r8hYJAh0JeDclp+sR7VeOKbbjuQYL0ysHHqkjksU3FQurHYF6eJvwE7LGZ0p5iX8qaLJobTfXOa7HpEedcfVp8qxhzXsgLbzIlYovntIegzjv162jSfbp1TY5+oRRpCsYvGxFXHlHa1Z7T9DWLSDbGZtbDpNqejaEWmzKo/G+VGsp/CeX/fxZrISkJ5SDat1yo/QlT3yYW9IrX4G8XOIMk9cLigptaaexNHKgBKY/2ly2AgiV5a0id+7r+A3xbROh0nuPVF5vLdCS9gw6bSHtG7FzDdv+/gYjtCDXyNdfelyXNVZyByA2zSE6NHdHwwHGzE6cAjjAqPBA+Z8AeozVxiQCmGT/WqmsPFTgeuyNP3rb9+dhOnQz7z+Dg/CGnFa+YRZ/R3Dt91WZU4zg3a/U//650vSQadTfvkz2Ezff+hl1; bm_sv=D21A7A6EA1E6C716910DE3CB8419439B~DloU2XfVeIq9Mg4C9u8k+md5jm7oefU2trjDcuOfI5M7b6kadIaLJ36RXLnf7M29bwMsfpb9S7VLH/D/aFlQ+vb0gtfvm4ddGTO6vhdJHW7UulCi+eZb7UXtHuh1FMtlYF8Ct3uodP6HoilBShuy3evm4oyDKzrzfwP0SBEn1X4=; RT="z=1&dm=nseindia.com&si=c788af0b-c6a7-4b5c-9abb-439583408c44&ss=kw0wdq1d&sl=2&tt=3kp&bcn=%2F%2F684d0d47.akstat.io%2F&ld=a7i&nu=kpaxjfo&cl=41rq"' \
 --compressed >store_circular_data.txt


python3 read_circular_fields_output.py >store_circular_data_2.txt

retVal=$?
if [ $retVal -ne 0 ]; then
    echo "Error While running python script"
    echo "" | mailx -s "Error While running python script Circular download" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in
    exit    
fi
if [ ! -s store_circular_data_2.txt ]; then
    echo "No output by python script for circular"
    echo "" | mailx -s "No output by python script for circular" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in
    exit
fi

>mail_logs.txt
touch historical_circular_logs.txt

while read line; do
	no_=`echo $line | cut -d',' -f1`
	echo "Circular $no_"
        if  grep -q "$no_" historical_circular_logs.txt ; then
    		continue
	fi
	echo $line
	echo $line >>historical_circular_logs.txt
	echo $line >>mail_logs.txt
done <store_circular_data_2.txt

if [ -s mail_logs.txt ]; then
	cat mail_logs.txt | mailx -s "New Circulars" -r "${HOSTNAME}-${USER}<raghunandan.sharma@tworoads-trading.co.in>" raghunandan.sharma@tworoads-trading.co.in
fi
