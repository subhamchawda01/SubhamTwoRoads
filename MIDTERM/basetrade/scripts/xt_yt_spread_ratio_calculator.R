#!/usr/bin/env Rscript
HOME_DIR <-Sys.getenv("HOME");
USER <-Sys.getenv("USER");
REPO <- "LiveExec"
script_dir <- paste(HOME_DIR,"/",REPO,"/scripts/",sep="");
script <- paste(script_dir,"get_last_price.sh",sep="");
price_file=system(script,intern=TRUE);
price_file=matrix(price_file,ncol=1);
price_file=strsplit(price_file[,1]," ");
price_file=do.call(rbind,price_file);
xt_price_=as.numeric(price_file[which(price_file[,1]=="XT"),2]);
yt_price_=as.numeric(price_file[which(price_file[,1]=="YT"),2]);

GetBondPrice <- function(price_,term_){
  if(price_>0){
    yield_=100-price_;
    j8_=yield_/200;
    j9_=1/(1+j8_);
j10_=1000 * (3 * (1 - (j9_ ^(term_ * 2))) / j8_ + 100 * (j9_^ (term_ * 2)));
    return (j10_);
  }
  else
    return (0);
}
xt_price_diff_=GetBondPrice(xt_price_,10)-GetBondPrice(xt_price_-0.01,10);
yt_price_diff_=GetBondPrice(yt_price_,3)-GetBondPrice(yt_price_-0.01,3);
xt_factor=10;
yt_factor = floor(xt_price_diff_/yt_price_diff_*xt_factor);
price_vec=c(xt_factor,yt_factor);
x=paste(price_vec,collapse="")
y=c("XTYT",x);
z=paste(y,sep=" ");
cat(z);






