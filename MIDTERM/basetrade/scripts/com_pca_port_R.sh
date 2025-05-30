cmd="x<- read.table(\"$2\") ;  \
x[1:1] <- NULL ; \
x[1:1] <- NULL ; \
std_all <- c() ; \
mean_all <- c() ; \
for (i in 1:ncol(x)){ std_all<- c(std_all, sapply(x[i:i], sd) );  mean_all<- c(mean_all, sapply(x[i:i], mean) ); x[i:i] <- (x[i:i] - mean_all[i])/ std_all[i] } ; \
pr <- prcomp(x) ; \
eig1 <-t( pr\$rotation)[1,] ; \
eig2 <-t( pr\$rotation)[2,] ; \
if( eig1[1] < 0) {eig1 <- 0-eig1 }; \
if( eig2[1] < 0) {eig2 <- 0-eig2 }; \
write ( std_all , \"\", ncol=1000, sep = \" \") ; \
write ( c (  (pr\$sdev[1])^2 /  sum ( pr\$sdev ^2 ) , eig1) , \"\", ncol=1000, sep = \" \"); \
write ( c (  (pr\$sdev[2])^2 /  sum ( pr\$sdev ^2 ) , eig2) , \"\", ncol=1000, sep = \" \"); "

echo $cmd | /home/dvctrader/R/R-2.15.1/bin/R --no-save | grep -v "^>" | tail -3 |  awk '{if (NR==1) \
 { print "PORTFOLIO_STDEV SHORT_CODE "$0; }  \
 else if (NR==2) print "PORTFOLIO_EIGEN SHORT_CODE 1 " $0 ; \
 else if (NR == 3) print "PORTFOLIO_EIGEN SHORT_CODE 2 " $0}' | sed 's/SHORT_CODE/'$1'/' | awk '{ if ( ($2=="BRALL" || $2=="UEBE" || $2=="USALL2" ) && $1=="PORTFOLIO_EIGEN" && $3=="1" && $5>0) \
{ for (i = 5; i <=NF; i+=1) $i = -$i } ; print $0 } '

