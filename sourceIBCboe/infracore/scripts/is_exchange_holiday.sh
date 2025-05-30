#!/bin/bash

is_holiday=`$HOME/infracore_install/bin/holiday_manager "EXCHANGE" "$1" "$2" "F"`
if [ $is_holiday = "1" ]; then
	echo "1" # Holiday
elif [ $is_holiday = "2" ]; then
	echo "2" # Not a Holiday
else
	echo "0" # Error
fi