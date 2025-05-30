diff -r -x .git -x bin -x testbed -x testperl ~/basetrade ~/infracore | grep "diff -r -x .git -x bin" | sed -e 's#diff -r -x .git -x bin -x testbed -x testperl ##g' | ~/basetrade/scripts/newer_on_left.pl


#diff -r -x .git -x bin -x scripts -x testbed -x GenPerlLib -x testperl ~/basetrade ~/infracore | grep "diff -r -x .git -x bin" | sed -e 's#diff -r -x .git -x bin -x scripts -x testbed -x GenPerlLib -x testperl ##g' | ~/basetrade/scripts/newer_on_left.pl


