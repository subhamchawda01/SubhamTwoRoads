#!/usr/bin/env Rscript
source ( "~/basetrade_install/ModelScripts/regression_utils.R" )
args <- commandArgs(trailingOnly = TRUE);
if ( length ( args ) < 3 )
{
  stop ( " Usage: <script> <regdata> <regout> <max_model_size> \n " ) ;
}
require(data.table)
require(VIF)
require(nnls)


### read data fast using data.table::fread
d <- fread(args[1]);
max_predictors_ <- as.numeric(args[3])
d1<-d[complete.cases(d),]
N<-ncol(d1)
predictors<-colnames(d1)[2:N]



###### have to sub sample the data again as VIF is too slow otherwise
if(nrow(d1)>5000){idx<-sample(1:nrow(d1),5000)}else{idx<-1:nrow(d1)}
sub_sample_size <- min(nrow(d1),300)
#if(nrow(d1)<300){stop('nrow data less than 300, too small. Stopping (R)') }



x <- as.matrix(d1[idx,2:ncol(d1),with=FALSE]) ;
y <- as.numeric(d1[idx,][[1]]);

remove(d);
remove(d1);

weight_sequence <- seq(0.005,0.05,by=0.005)
print(weight_sequence)

best_set <- NULL
next_best <- NULL

##  find dw for the right number of predictors
for (W in weight_sequence) {
print('Variance inflation check...')
vif.sel <- vif(y, x, w0 = 0.0005, dw = W, subsize = sub_sample_size,trace=FALSE);
selected <- length(vif.sel$select)
if(selected<=max_predictors_){best_set <- vif.sel$select}else{next_best<-vif.sel$select;break;}
print('At weight ')
print(W)
print('Length of selected set is...')
print(selected)
}

print("Best ilist...")


if(length(best_set)==0){best_set<-1:dim(x)[2]}


### sometimes the best set is much larger than max_predictors_; then randomly sample predictors from best set to limit overfitting.
if(length(best_set)>max_predictors_ ){best_set<-  sample(best_set,max_predictors_)  }



print(predictors[best_set])
prednames<-predictors[best_set]

print('Here are the candidate predictors...')
print(prednames)

X<-as.matrix(x[,prednames])
Y<-y
print("Doing NNLS..")


cc<-coef(nnls(X,Y))
print(cc)
prednames<-prednames[which(cc>0)]

best_set<-best_set[which(cc>0)]

lm_predictors <- cc

cc<-cc[which(cc>0)]



print("After NNLS")
print(prednames)
print(cc)



if(length(cc)>0){
ind_n_weights <- cbind (best_set,cc) ;
}else{
print("NNLS did not converge, doing LM..")
lmodel <- lm ( Y ~ X + 0 );
coeffs <- coef (lmodel) ;
coeffs[which(is.na(coeffs))] <- 0;
selected_indicators <- which(coeffs!=0) ;
selected_indicators_weights <- coeffs[ which(coeffs!=0 ) ] ;
ind_n_weights <- cbind (selected_indicators,selected_indicators_weights) ;
}
write.table(ind_n_weights, args[2],col.names=F,row.names=F);
