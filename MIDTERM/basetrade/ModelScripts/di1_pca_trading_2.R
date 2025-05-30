#!/usr/bin/env Rscript
.libPaths("/apps/R/root/library/")

# DI1 PCA BASED STRATEGY
# The idea here is very simple. In most of our experiments we have found that pc1 explains most of the data
# and other principal components are mean reverting. 
# This is an attempt to capture that mean reversion
# We take online pca on last k ( parameter) days as we move along.
# TODO PCA can be noisy, maybe we should add some regularization

GetListofDates <- function ( shortcode, start_date, end_date )
{
  cat(sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, start_date, num_days_lookback),"\n" );  
  dates <- system ( sprintf("~/basetrade/scripts/get_list_of_dates_for_shortcode.pl %s %s %s", shortcode, start_date, num_days_lookback),intern=TRUE );
  list_of_dates <- unlist(strsplit(dates, "\\ "));
  retVal <- list_of_dates;
}

GoLong <- function ( index, pc2_pos_vec )
{
  position <<- position + 1;
  pnl <<- pnl - (pc2_pos_vec ) %*% di1_data[index,];
  pnl <<- pnl - sum(abs(pc2_pos_vec))*commish;
  pnl_per_product <<- pnl_per_product - pc2_pos_vec * di1_data[index,];
  pnl_per_product <<- pnl_per_product - abs(pc2_pos_vec) * commish;
  trades <<- trades + 1;
  volume <<- volume + sum(abs(pc2_pos_vec));
  cat ("Buy ", index, " pnl : ", pnl + position * ( pc2_pos_vec ) %*% di1_data[index,],"\n");  
  cat ("PCA_POS_VEC ", pc2_pos_vec, "\n" );
  cat ("Prices ", di1_data[index,], "\n");
}

GoShort <- function ( index, pc2_pos_vec )
{
  position <<- position - 1;
  pnl <<- pnl + (pc2_pos_vec ) %*% di1_data[index,];
  pnl <<- pnl - sum(abs(pc2_pos_vec))*commish;
  pnl_per_product <<- pnl_per_product + pc2_pos_vec * di1_data[index,];
  pnl_per_product <<- pnl_per_product - abs(pc2_pos_vec) * commish;
  trades <- trades + 1;
  volume <<- volume + sum(abs(pc2_pos_vec));
  cat ("Sell ", index, " pnl : ", pnl + position * ( pc2_pos_vec ) %*% di1_data[index,],"\n");  
  cat ("PCA_POS_VEC ", pc2_pos_vec, "\n" );
  cat ("Prices ", di1_data[index,], "\n");      
}

AdjustPosition <- function ( index, old_pc2_pos_vec, pc2_pos_vec )
{
  current_position_vec <- old_pc2_pos_vec * position;
  desired_position_vec <- pc2_pos_vec * position;
  diff_position_vec <- desired_position_vec - current_position_vec;
  pnl <<- pnl - ( diff_position_vec ) %*% di1_data[index,];   
  pnl <<- pnl - sum(abs(diff_position_vec)) * commish;
  pnl_per_product <<- pnl_per_product - ( diff_position_vec * di1_data[index,] );
  volume <- volume + sum(abs(diff_position_vec)); 
  cat ("PCA_POS_VEC ", pc2_pos_vec, "\n" );
  cat ("Prices ", di1_data[index,], "\n");  
}

ParseConfig <- function ( config )

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 2 ) {
        stop ("USAGE : <script> <data_dir> <config>\n");
}

data_dir <- args[1];
data_file_prefix <- "di1_l1_data_";
config <- args[2];
start_date <- args[3];
num_days_lookback <- args[4];


pca_refresh_time <- history/5.0;

start_index <- history+1;
end_index <- num_rows;

# to keep count of instances for refreshing online pca
count <- history;

pnl <- 0;
position <- 0;
volume <-0;
trades <-0;
pca_model <- rep(0,num_cols);
pnl_per_product <- rep(0, num_cols);
pc2_vec <- rep(0,num_cols);
pc2_pos_vec <- rep(0,num_cols);
old_pc2_vec <- rep(0,num_cols);
old_pc2_pos_vec <- rep(0,num_cols);
small_threshold <- 0.04;
big_threshold <- 2 * small_threshold;
scale <- rep(0,num_cols);
center <- rep(0,num_cols);
signal <- 0;
last_trade_index <- 0;
cooloff <- 100;
commish <- 0.00;
n2d <- c(2.00, 2.75, 3.50, 4.15, 4,70);

for ( i in c( )
{

  if ( sum(di1_data[i,] < 0.1) > 0 )
  {
    next;
  } 

  if ( count > pca_refresh_time )
  {
    # need to recompute pca
    count <- 0;
    pca_model <- prcomp ( di1_data[(i-history):i,], scale=TRUE );   # window for pca is history 
    old_pc2_vec <- pc2_vec;
    pc2_vec <- pca_model$rotation[,2];
    if ( pc2_vec[1] * old_pc2_vec[1] < 0 )
    {
      pc2_vec <- - pc2_vec;
    }   
    scale <- pca_model$scale; 
    center <- pca_model$center; # simple mean
    old_pc2_pos_vec <- round(old_pc2_vec * 10);
    pc2_pos_vec <- round(pc2_vec * 10);
#    cat ("Need to refresh pca\n");
#    cat ("OLD_PCA ", old_pc2_vec, "\n" );
#    cat ("NEW_PCA ", pc2_vec, "\n" );
    # Adjust position to match with the new pca vector
    AdjustPosition ( i, old_pc2_pos_vec, pc2_pos_vec );
  }

  count <- count + 1;

  signal <- - ( di1_data[i,] - center ) %*% pc2_vec;  # -ve since mean reverting model

  if ( i - last_trade_index < cooloff )
  {
    next;
  }

  if ( signal < - big_threshold )
  {
    # desired position is -2
    if ( position >= -1 )
    {
      GoShort (i, pc2_pos_vec );
      last_trade_index <- i;
    }
  }
  else if ( signal > - big_threshold && signal < - small_threshold )
  {
    # desired position is [-2,-1]
    if ( position > -1 )
    {
      GoShort (i, pc2_pos_vec );
      last_trade_index <- i; 
    }        
  }
  else if ( signal > -small_threshold && signal < 0 )
  {
    # desired position is [-1,0]    
    if ( position > 0 )
    {
      GoShort (i, pc2_pos_vec );
      last_trade_index <- i;
    }
    else if ( position < -1 )
    {
      GoLong (i, pc2_pos_vec );
      last_trade_index <- i;
    }    
  }
  else if ( signal > 0 && signal < small_threshold )
  {
    # desired position is [0,1]    
    if ( position > 1 )
    {
      GoShort (i, pc2_pos_vec );
      last_trade_index <- i;
    }
    else if ( position < 0 )
    {
      GoLong (i, pc2_pos_vec );
      last_trade_index <- i;
    }
  }
  else if ( signal > small_threshold && signal < big_threshold )
  {
    # desired position is [1,2]    
    if ( position < 1 )
    {
      GoLong (i, pc2_pos_vec );
      last_trade_index <- i;
    }
  }
  else if ( signal > big_threshold )
  {
    # desired position is 2    
    if ( position <= 1 )
    {
      GoLong (i, pc2_pos_vec );
      last_trade_index <- i;
    }
  }
}

if ( position != 0 )
{
  pnl <- pnl + position * ( pc2_pos_vec ) %*% di1_data[num_rows,];
  volume <- volume + abs(position) * sum(abs(pc2_pos_vec));
}

cat ( "pnl : ", pnl ," trades : ", trades, " volume : ", volume, "\n" );
cat ( "pnl_per_product : ", pnl_per_product, "\n" );
