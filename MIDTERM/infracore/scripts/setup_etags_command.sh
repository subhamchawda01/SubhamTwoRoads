REPOS=infracore
REPOSTAGS="my_"$REPOS"_etags"
REPOSTAGSFILES="my_"$REPOS"_etags_files"

cd ~/$REPOS

> $REPOSTAGSFILES
bjam debug -an headers | egrep "cp \"" | grep -v "\"g++\"    -o" | awk '{print $2}' | sed 's#"##g' >> $REPOSTAGSFILES
bjam debug -an | egrep "g\+\+" | grep -v "\"g++\"    -o" | grep -v "\"g++\" -L" | grep -v "  -o" | awk '{print $NF}' | grep -v "boost/root" | sed 's#"##g' >> $REPOSTAGSFILES

> $REPOSTAGS
#etags -L $REPOSTAGSFILES -o $REPOSTAGS
for fname in `cat $REPOSTAGSFILES`; do etags -a -o $REPOSTAGS $fname; done

rm -f $REPOSTAGSFILES
