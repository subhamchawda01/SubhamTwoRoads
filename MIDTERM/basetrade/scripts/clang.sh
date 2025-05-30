clang-format -style="{BasedOnStyle: google, ColumnLimit: 120}" $1 > $1"_clang"
mv $1"_clang" $1
