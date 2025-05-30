#!/usr/bin/env Rscript

HOME_DIR <- Sys.getenv("HOME");
USER <- Sys.getenv("USER");

REPO <- "basetrade";

SCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/scripts/", sep="");
MODELSCRIPTS_DIR <- paste(HOME_DIR, "/", REPO, "_install/ModelScripts/", sep="");
GENPERLLIB_DIR <- paste(HOME_DIR, "/", REPO, "_install/GenPerlLib/", sep="");
LIVE_BIN_DIR <- paste(HOME_DIR, "/LiveExec/bin/", sep="");

Search_Linear <- function (Arr,index,val)
{
        len <- length(Arr)
        #print(Arr[2])
        #print(val)
        for(i in index:len)
        {
            #print(Arr[i])
            #print(val)       
            if(Arr[i] >= val){
               #print(i)
               #print(val)
               return(i)
            }
        }
        return(len)
}


args = commandArgs( trailingOnly=TRUE );

if ( length(args) < 2)
{
  stop("USAGE: <script> <filename> col_indices_pairs=(1 2)\nExample <script> datafile 1 2\n");
}


data<-read.table(args[1]); data<-as.matrix(data);
predDurInMsecs<-as.numeric(args[2])
#writeFileName<- "devcheck"
#write.table(data[1,2],writeFileName,row.names=F,col.names=F)
#write(nrow(data),""=[]
corrData <- data.frame(A= numeric(0), B= numeric(0))
write(nrow(data),"")
counter<-1
for (index in 1:nrow(data)){
   if(index%%10000==0){
   print(index)

   }
   curr_time=data[index,1]
   future_time=curr_time+predDurInMsecs
   future_index=Search_Linear(data,index,future_time);
   if(future_index < nrow(data)){
       corrData[counter,1] <-data[future_index,5]-data[index,5]
       corrData[counter,2] <-data[index,4]-data[index,5]
 }


   #print(curr_time)
   #print(future_time)



}

#deltay=as.numeric(deltay)
#PricediffBasePort=as.numeric(PricediffBasePort)
#print(nrow(deltay))
#print(nrow(PricediffBasePort))
#print(corrData)
corrData2 <- subset(corrData, (  abs(corrData[,1])<2*sd(corrData[,1]) & abs(corrData[,2]) <2* sd(corrData[,2])  ) )
corrData3 <- subset(corrData2, (  abs(corrData2[,1])<sd(corrData2[,1]) & abs(corrData2[,2]) < sd(corrData2[,2])  ) )
print (sd(corrData[,1]))
print(sd(corrData[,2]))
print (summary(corrData3))
print(cor(corrData3))

