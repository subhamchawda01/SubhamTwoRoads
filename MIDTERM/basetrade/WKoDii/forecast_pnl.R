#!/apps/R/root/bin/Rscript
library(rpart);

labeldata <- function ( data, bin_per, start_label = 0 )
{
  out <- array( start_label, length(data) );
  num_per <- length(bin_per);
  if (num_per > 0) 
  {
    bin_bound <- as.numeric(quantile(data, bin_per));
    out[data >= bin_bound[num_per]] <- start_label + num_per;
    if ( num_per > 1 )
    {
      for ( i in seq ( 2, num_per ) )
      {
        out[(data >= bin_bound[i-1]) & (data < bin_bound[i])] <- start_label + i -1;
      }
    }
  }
  return (out);
}

valtolabel <- function ( vals, bin_bound, start_label = 0 )
{
  out <- array( start_label, length(vals) );
  num_per <- length(bin_bound);
  if (num_per > 0) 
  {
    out[vals >= bin_bound[num_per]] <- start_label + num_per;
    if ( num_per > 1 )
    {
      for ( i in seq ( 2, num_per ) )
      {
        out[(vals >= bin_bound[i-1]) & (vals < bin_bound[i])] <- start_label + i -1;
      }
    }
  }
  return (out);
}

args = commandArgs( trailingOnly=TRUE );
if ( length(args) < 3) 
{
  stop("USAGE: <script> <feature_datafile> <pnl_datafile> <forecast_date> <verbose=0>\n");
}

feature_datafile <- args[1];
pnl_datafile <- args[2];
forecast_date <- as.numeric(args[3]);
verbose <- FALSE;
if ( length(args) >= 4 )
{
  verbose <- (args[4] > 0);
}

#read feature data
#date feature1 feature2 ....
feature_data_full <- read.table(feature_datafile); feature_data_full <- as.matrix(feature_data_full); 
#remove data before forecast date
date_idx_ft_data_ <- 1;
feature_data <- feature_data_full[feature_data_full[,date_idx_ft_data_]<forecast_date,];

first_feature_idx_ <- 2;
last_feature_idx_ <- dim(feature_data)[2];
num_features <- last_feature_idx_ - first_feature_idx_ + 1;

#binning features
binned_feature_data <- matrix(0,dim(feature_data)[1],num_features);
feature_bin_per <- c(0.25,0.75);
for( i in 1:num_features )
{
  binned_feature_data[,i] <- labeldata(feature_data[,first_feature_idx_+i-1], feature_bin_per);
}

#read pnl data
#date stratid pnl
pnl_data_full <- read.table(pnl_datafile); pnl_data_full <- as.matrix(pnl_data_full); 
#remove data before forecast date
date_idx_pnl_data_ <- 1;
stratid_idx_ <- 2;
pnl_idx_ <- 3;
pnl_data <- pnl_data_full[pnl_data_full[,date_idx_pnl_data_]<forecast_date,];
pnl_data <- pnl_data [ !is.na(match(pnl_data[,date_idx_pnl_data_],feature_data[,date_idx_ft_data_])) , ];

#binning pnl
binned_pnl_data <- pnl_data[,pnl_idx_];
stratids <- unique(pnl_data[,stratid_idx_]);
pnl_bin_per <- c(0.1,0.35,0.65,0.9);
pnl_start_label_ <- -2;

for ( i in 1:length(stratids) )
{
  binned_pnl_data[pnl_data[,stratid_idx_]==stratids[i]] <- labeldata(pnl_data[pnl_data[,stratid_idx_]==stratids[i],pnl_idx_], pnl_bin_per, pnl_start_label_);
}

#generating training data
num_points <- length(binned_pnl_data);
X <- matrix(0, num_points, num_features);
for ( i in 1:num_points )
{
  X[i,] <- binned_feature_data[feature_data[,date_idx_ft_data_]==pnl_data[i,date_idx_pnl_data_],];
}
train_data <- data.frame( y = binned_pnl_data, x = X );

#training
#train_data$y <- factor(train_data$y);
#pnl_classes <- as.numeric(levels(train_data$y));
#loss_matrix <- matrix(0, length(pnl_classes), length(pnl_classes));
#loss_matrix <- read.table('~/archit/WKoDii/loss.txt'); loss_matrix <- as.matrix(loss_matrix);
#for ( i in 1:length(pnl_classes) ) { loss_matrix[i,] <- abs(pnl_classes - pnl_classes[i]); }
#fit <- rpart( y ~ ., train_data , method="class", parms=list(split="information", loss=loss_matrix), cp=0.001, minsplit=10 );
fit <- rpart( y ~ ., train_data, method="anova", minsplit=10, cp =0.001);
if ( verbose )
{
  print(fit);
  #out <- predict( fit, type = "class" );
  out <- predict( fit, train_data );
  out_class <- round(out);
  print(table ( out_class, train_data$y ));
  mse <- sqrt(mean( (train_data$y - out)^2 ));
  cat("MSE:", mse,"\n");
}

#preparing test/forecast input
if ( is.na(match(forecast_date,feature_data_full[,date_idx_ft_data_])) )
{
  stop("forecast date not found in feature data\n");
}
test_data <- matrix(feature_data_full[feature_data_full[,date_idx_ft_data_]==forecast_date,first_feature_idx_:last_feature_idx_], ncol = num_features);
#write.table(test_data,row.names = FALSE, col.names = FALSE);
for ( i in 1:num_features )
{
  bin_bound <- as.numeric(quantile(feature_data[,first_feature_idx_+i-1], feature_bin_per));
  test_data[,i] <- valtolabel(test_data[,i], bin_bound );
}

#forecasting
pred_pnl_class <- predict( fit, data.frame(x = test_data) );
if ( verbose ) { cat("PNL_CLASS:", forecast_date, pred_pnl_class, "\n") }
pnl_classes <- c(floor(pred_pnl_class), ceiling(pred_pnl_class));
frac <- abs(pred_pnl_class - floor(pred_pnl_class) );
wts <- c(1-frac, frac);
pnl_means <- matrix(0,length(stratids),length(pnl_classes));
for ( j in 1:length(stratids) )
{
  for ( i in 1:length(pnl_classes) )
  {
    pnl_means[j,i] <- mean(pnl_data[pnl_data[,stratid_idx_]==stratids[j] & binned_pnl_data==pnl_classes[i],pnl_idx_]) ;
  }
  t_pnl_ <- sum(wts * pnl_means[j,]);
  cat ( forecast_date, stratids[j], t_pnl_, "\n");
}
