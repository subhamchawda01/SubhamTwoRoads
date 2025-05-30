#!/usr/bin/Rscript

library(forecast);
library(tseries);

args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 1) 
{
  stop("USAGE: <script> <inputfile> [input_col_idx=1]\n");
}

data <- read.table(args[1], row.names=1); data <- as.matrix(data);
col_idx <- 1;
if ( length(args) >= 2 )
{
  col_idx <- as.integer(args[2]);
}

#reading
x <- as.numeric(data[,col_idx])

#cleaning
x<-x[x!=0 & x!=Inf & x!=-Inf]
kpss.test(x);
xd<- diff(x);
kpss.test(xd);

#
#Acf(xd);
#Pacf(xd);
tsdisplay(xd);

autoarima_model <- auto.arima(x);
autoarima_model

cor(autoarima_model$x, (autoarima_model$x - autoarima_model$residuals) )
Box.test(autoarima_model$residuals,20,fitdf=sum(autoarima_model$arma[1:2]), type="Ljung")

# 
arima_model = Arima(x,order=c(1,1,1) );
arima_model
cor(arima_model$x, (arima_model$x - arima_model$residuals) )
Box.test(arima_model$residuals,20,fitdf=sum(arima_model$arma[1:2]), type="Ljung")

#parition
#partition <- as.integer(0.7*length(x))
#train <- x[1:partition]
#test <- x[(partition+1):length(x)]
#
##train
#arima_model <- arima(train, order=c(1,0,0))
#arima_model
#
##forecast
#out <- array(0,length(test))
#out[1]<-predict(arima_model,1)$pred
#for (i in 1:(length(test)-1) )
#{
#	out[i+1] <- arima_model$coef[1]*(test[i] - arima_model$coef[2]) + arima_model$coef[2]
#}
#cat ( "Correlation on Test Data : ", cor(out,test), "\n")
#
#X11()
#plot(test,type='l',col='blue')
#lines(out,col='red')
#grid()
#
#message("Press Return To Continue")
#invisible(readLines("stdin", n=1))
