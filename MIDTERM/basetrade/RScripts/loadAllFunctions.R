
######################################################################################
# This file set R directory paths and loads all the functions available into workspace
######################################################################################
PWD <- getwd();
user <- system("echo $USER", intern=T);

RD <- paste("/home/",user,"/basetrade/RScripts/",sep="");
SWD <-paste("/home/",user,"/basetrade/SelfRWork/", sep="");
BWD <- paste("/home/",user,"/basetrade/BranchRWork/",sep="");


system(paste("if [ ! -d", SWD, "]; then mkdir -p", SWD, "; fi;"));
system(paste("if [ ! -d", BWD, "]; then mkdir -p", BWD, "; fi;"));
setwd(SWD);

#######################################
#load functions
########################################

source(paste(RD, "regUtils.R",sep=""));
























































