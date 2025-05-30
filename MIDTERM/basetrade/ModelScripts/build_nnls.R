#!/usr/bin/env Rscript


suppressMessages(require(data.table))
suppressMessages(require(VIF))
suppressMessages(require(nnls))
options(warn=-1)


### this script accepts regdata and orders indicator by correlation, takes top N indicators and fits non negative least squares
### using R package nnls

args <- commandArgs(trailingOnly = TRUE);
if ( length ( args ) < 3 )
{
  stop ( " Usage: <script> <regdata> <regout> <max_model_size> \n " ) ;
}





### read data fast using data.table::fread

d <- fread(args[1], header=FALSE);




convertColumnToIndex<-function(columnName)
{
### as per our naming convention, V2 maps to column 1 in indicator space as per place_lasso_coef
## this function does this conversion
### i.e. algo is extract the number after V, then subtract 1 from it
tmp<-strsplit(columnName,"")[[1]]
return(as.numeric(paste0(tmp[2:length(tmp)],collapse=""))-1)
}


### stop if zero dataset
num_predictors_in_data <- length(colnames(d))-1
if(num_predictors_in_data <= 0){stop("No Regdata. Please fix ilist, datagen etc.")}

## extract data.table of predictors
d_x <- d[,2:ncol(d),with=FALSE] ;



## sharpe check
sd <- function(x){sqrt(var(x,na.rm=TRUE))}
abs_mean <- function(x){abs(mean(x,na.rm=TRUE))}

means <- d_x[, lapply(.SD, abs_mean)]
stdevs <- d_x[, lapply(.SD, sd)]

sharpe_check <- means - 0.22 *stdevs; # 0.22 threshold for sharpe of an indicator
inds_excluded_due_to_sharpe_check <- which(sharpe_check > 0);

inds_excluded_due_to_sharpe_check_names <- colnames(d_x)[inds_excluded_due_to_sharpe_check]









## need to add these high sharpe in the worst case that they make the correlation ranking
max_predictors_ <- as.numeric(args[3]) + length(inds_excluded_due_to_sharpe_check_names) + 1  ## need to add 1 because y will be selected
max_predictors_ <- min(num_predictors_in_data , max_predictors_)  ## sanity check

#print('max_predictors_')
#print(max_predictors_)


### now order by correlation, and choose good predictors
c <- cor(d)
C <- c[grep('V1',colnames(d))[1],]
top_predictors <- head(order(-C),max_predictors_)
take_these_predictors <- names(C[top_predictors])[2:length(top_predictors)] ## need to exclude y itself because it has correlation 1
new_select <- take_these_predictors  ## we will choose only these predictors to feed to NNLS



## remove hi sharpe indicators
if(length(inds_excluded_due_to_sharpe_check_names)!=0) { 
    #print('Excluded these high sharpe indicators...')
    #print(inds_excluded_due_to_sharpe_check_names)
    new_select <- setdiff(new_select,inds_excluded_due_to_sharpe_check_names) 
}

#print('Selected these regressors for NNLS...')
#print(new_select)

## subset reg data to new dataset
d1 <- d[,c('V1',new_select),with=FALSE]

##### now fit NNLS
d1 <- d1[complete.cases(d1),]
N <- ncol(d1)
predictors <- colnames(d1)[2:N]

x <- as.matrix(d1[,2:ncol(d1),with=FALSE]) ;
y <- as.numeric(d1[[1]]);


#print("Doing NNLS..")
cc <- coef(nnls(x,y)) ;

## a lot of these will be zero
non_zero_predictor_indices <- which(cc>0)

#print("non_zero_predictor_indices")
#print(non_zero_predictor_indices)




if ( length( non_zero_predictor_indices ) > 0 ) {

cc <- cc[non_zero_predictor_indices] ;
chosen_predictor_names <- predictors[non_zero_predictor_indices]
#print("chosen_predictor_names")
#print(chosen_predictor_names)


chosen_predictor_names_indices <- as.numeric(Map(convertColumnToIndex,chosen_predictor_names))
#print("chosen_predictor_names_indices")
#print(chosen_predictor_names_indices)



ind_n_weights <- cbind (chosen_predictor_names_indices,cc) ;

#print("ind_n_weights")
#print(ind_n_weights)



}else{

#print("NNLS did not converge, doing LM..")
lmodel <- lm ( y ~ x + 0 );


cc <- coef (lmodel) ;
cc[which(is.na(cc))] <- 0;

non_zero_predictor_indices <- which(cc>0)
cc<-cc[non_zero_predictor_indices] ;
chosen_predictor_names <- predictors[non_zero_predictor_indices]
chosen_predictor_names_indices <- as.numeric(Map(convertColumnToIndex,chosen_predictor_names))
ind_n_weights <- cbind (chosen_predictor_names_indices,cc) ;
}






##debug
#print(cc)
signal<-(as.matrix(d_x[,chosen_predictor_names,with=FALSE]))%*%cc
sd<-sqrt(var(signal))
#print("model sd")
#print(sd)



## write out weights
write.table(ind_n_weights, args[2],col.names=F,row.names=F);