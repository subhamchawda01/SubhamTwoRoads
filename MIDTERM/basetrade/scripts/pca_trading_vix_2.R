#!/usr/bin/env Rscript
source("~/basetrade/scripts/sim_strategy_vix.R");

args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 5 )
{
  stop ( " Usage: <data_file> <test_set_fraction> <first_maturity> <last_maturity> <index>\n " ) ;
}

data_file <- args[1];
test_set_fraction <- as.numeric(args[2]);
fm <- as.numeric(args[3]);
lm <- as.numeric(args[4]);
index <- as.numeric(args[5]);

load ( data_file );

num_instances <- nrow(trend_900);
training_data_indices <- seq ( 1, floor ( num_instances * ( 1 - test_set_fraction )) , 1 );
test_data_indices <- seq ( ( floor ( num_instances * ( 1 - test_set_fraction )) + 1 ), num_instances, 1 );

colum_indices <- seq( ( fm + 1 ), ( lm + 1), 1 );

# for mid_prices
mid_prices <- as.matrix( ( bid_prices[,colum_indices] + ask_prices[,colum_indices] ) / 2 );
mid_prices_train <- ( bid_prices[training_data_indices,colum_indices] + ask_prices[training_data_indices,colum_indices] ) / 2;
mid_prices_test <- ( bid_prices[test_data_indices,colum_indices] + ask_prices[test_data_indices,colum_indices] ) / 2;

pca <- prcomp ( mid_prices_train, center=TRUE, scale=TRUE );
eigen_vecs <- pca$loadings;

pca_scale <- pca$scale;

port_vec <- pca$loadings[,index];

summary(pca);

#trend_1800
trend_1800_train <- trend_1800[training_data_indices,colum_indices] ;  
trend_1800_test <- trend_1800[test_data_indices,colum_indices] ;

pca <- prcomp ( trend_1800_train, center=TRUE, scale=TRUE );
eigen_vecs <- pca$loadings;

pca_scale <- pca$scale;

#port_vec <- pca$rotation[,index];

summary ( pca );

#trend_3600
trend_3600_train <- trend_3600[training_data_indices,colum_indices] ;  
trend_3600_test <- trend_3600[test_data_indices,colum_indices] ;

pca <- prcomp ( trend_3600_train, center=TRUE, scale=TRUE );
eigen_vecs <- pca$loadings;

pca_scale <- pca$scale;

#port_vec <- pca$rotation[,index];

summary ( pca );

#trend_7200
trend_7200_all <- trend_7200[, colum_indices];
trend_7200_train <- trend_7200[training_data_indices,colum_indices] ;  
trend_7200_test <- trend_7200[test_data_indices,colum_indices] ;

pca <- prcomp ( trend_7200_train, center=TRUE, scale=TRUE );
eigen_vecs <- pca$loadings;

port_vec <- pca$rotation[,index];

pca_scale <- pca$scale;

summary ( pca );

port_vec;

mean_lb <- 150000 #  lookback for computing mean
stdev_lb <- 150000 # lookback for computing stdev

pc2_values <- as.matrix(trend_7200_all) %*% port_vec;

tf_1 <- 0.3 # thresh_factor 1
tf_2 <- 2.0 # thresh_factor 2

buy_pnl <- 0;
sell_pnl <- 0;

total_pnl <- 0;

pos <- 0;

for ( i in test_data_indices)
{
  mean_value <- mean ( pc2_values[(i-mean_lb):i] );
  stdev_value <- sd ( pc2_values[(i-stdev_lb):i] );
  
  vol_factor <- mid_prices[i,1] * mid_prices[i,1] / 225; # volatility factor...currently taking it as a square of 

  signal <- mean_value - pc2_values[i];

  if ( signal > tf_1 * stdev_value * vol_factor && pos < 1 )
  {
    buy_pnl <- buy_pnl - port_vec %*% mid_prices[i,];
    pos = 1;
    cat ( "B ", mid_prices[i,1], " ", mid_prices[i,2], mid_prices[i,3],"\n" );
  }

  if ( signal < 0 & pos > 0 )
  {
    sell_pnl <- sell_pnl + port_vec %*% mid_prices[i,];
    pos <- 0;
    cat ( "S ", mid_prices[i,1], " ", mid_prices[i,2], mid_prices[i,3],"\n" );
    total_pnl <- buy_pnl + sell_pnl;
    cat ( "total_pnl ", total_pnl,"\n");
  }

  if ( signal < - tf_1 * stdev_value * vol_factor && pos > -1 )
  {
    sell_pnl <- sell_pnl + port_vec %*% mid_prices[i,];
    pos <- -1;
    cat ( "S ", mid_prices[i,1], mid_prices[i,2], mid_prices[i,3],"\n");
  }

  if ( signal > 0 && pos < 0 )
  {
    buy_pnl <- buy_pnl - port_vec %*% mid_prices[i,];
    pos <- 0;
    cat ( "B ", mid_prices[i,1], mid_prices[i,2], mid_prices[i,3], "\n" );
    total_pnl <- buy_pnl + sell_pnl;
    cat ("total_pnl ", total_pnl, "\n" );
  }


  if ( pos == 0 )
  {
    total_pnl =  buy_pnl + sell_pnl;
  }
}
