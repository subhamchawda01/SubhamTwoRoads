cat $1 | awk '{print $1 " " $2}' | grep "V" | grep -v "Call:" | sed 's/ \+/ /g' | tr -dc "[:alnum:][:space:][:punct:]" | awk -F"V" '{print "OutCoeff " $2 " " $3}' > $2
