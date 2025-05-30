REPOS=(dvccode baseinfra dvctrade basetrade)
REPOSTAGS=$HOME/my_cvquant_etags
REPOSTAGSFILES=$HOME/my_cvquant_etags_files

> $REPOSTAGSFILES
for repo in "${REPOS[@]}"; do cd ~/$repo ; bjam release -an headers | egrep "cp \"" | grep -v "\"g++\"    -o" | awk '{print $2}' | sed 's#"##g' >> $REPOSTAGSFILES ; bjam release -an | egrep "g\+\+" | grep -v "\"g++\"    -o" | grep -v "\"g++\" -L" | grep -v "  -o" | awk '{print $NF}' | grep -v "boost/root" | sed 's#"##g' >> $REPOSTAGSFILES ; done


> $REPOSTAGS
#etags -L $REPOSTAGSFILES -o $REPOSTAGS
for fname in `cat $REPOSTAGSFILES`; do etags -a -o $REPOSTAGS $fname; done

rm -f $REPOSTAGSFILES
