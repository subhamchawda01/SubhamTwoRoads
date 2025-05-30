#!/usr/bin/env Rscript
#suppressPackageStartupMessages( library ( "numDeriv" , lib.loc="/apps/R/root/library/" ) );
.libPaths("/apps/R/root/library/")

#################
#Algorithm
#y = Sum over i { Gi(Xi)  }
#Gi(X)=Bi * ( sigmoid ( Ai  * X ) - 0.5 )


#Reduces the ilist to "max_model_size" using lasso

bar_duration <- 60 ; # in seconds; MarketUpdate called every bar_duration
current_bid_price <- 0;
current_ask_price <- 0;
current_mid_price <- 0;

pnl <- 0;
total_pnl <- 0;
pos <- 0;

max_pos <- 1;

cooloff <- 300;

OnTradePrint <- function ( buysell , size_traded, traded_price ) {
  if ( size_traded > 1000 )
  {
    size_traded <- 1000;
  }
  positioning_1000 <- ( positioning_1000 * ( 1000 - size_traded ) + buysell * size_traded ) / 1000;
  positioning_2000 <- ( positioning_2000 * ( 2000 - size_traded ) + buysell * size_traded ) / 2000;
  positioning_5000 <- ( positioning_5000 * ( 5000 - size_traded ) + buysell * size_traded ) / 5000;
  positioning_10000 <- ( positioning_10000 * ( 10000 - size_traded ) + buysell * size_traded ) / 10000;  
}

OnMarketUpdate <- function ( bid_price, ask_price ) {
  current_bid_price <<- bid_price;
  current_ask_price <<- ask_price;
  current_mid_price <<- 0.5 * bid_price + 0.5 * ask_price;

  ema_5 <<- ema_5 * ( 2 / 3 ) + current_mid_price * ( 1 / 3 );
  ema_10 <<- ema_10 * ( 9 / 11 ) + current_mid_price * ( 2 / 11 );
  ema_15 <<- ema_15 * ( 7 / 8 ) + current_mid_price * ( 1 / 8 );
  ema_20 <<- ema_20 * ( 19 / 21 ) + current_mid_price * ( 2 / 21 );
  ema_30 <<- ema_30 * ( 29 / 31 ) + current_mid_price * ( 2 / 31 );
  ema_50 <<- ema_50 * ( 49 / 51 ) + current_mid_price * ( 2 / 51 );
  ema_60 <<- ema_60 * ( 59 / 61 ) + current_mid_price * ( 2 / 61 );
  ema_100 <<- ema_100 * ( 99 / 101 ) + current_mid_price * ( 2 / 101 );
  ema_120 <<- ema_120 * ( 119 / 121 ) + current_mid_price * ( 2 / 121 );
  
  sq_ema_5 <<- sq_ema_5 * ( 2 / 3 ) + current_mid_price * current_mid_price * ( 1 / 3 );
  sq_ema_10 <<- sq_ema_10 * ( 9 / 11 ) + current_mid_price * current_mid_price * ( 2 / 11 );
  sq_ema_15 <<- sq_ema_15 * ( 7 / 8 ) + current_mid_price * current_mid_price * ( 1 / 8 );
  sq_ema_20 <<- sq_ema_20 * ( 19 / 21 ) + current_mid_price * current_mid_price * ( 2 / 21 );
  sq_ema_30 <<- sq_ema_30 * ( 29 / 31 ) + current_mid_price * current_mid_price * ( 2 / 31 );
  sq_ema_50 <<- sq_ema_50 * ( 49 / 51 ) + current_mid_price * current_mid_price * ( 2 / 51 );
  sq_ema_60 <<- sq_ema_60 * ( 59 / 61 ) + current_mid_price * current_mid_price * ( 2 / 61 );
  sq_ema_100 <<- sq_ema_100 * ( 99 / 101 ) + current_mid_price * current_mid_price * ( 2 / 101 );
  sq_ema_120 <<- sq_ema_120 * ( 119 / 121 ) + current_mid_price * current_mid_price * ( 2 / 121 );
  
  stdev_15 <<- sqrt ( sq_ema_15 - ema_5 * ema_5 );
  stdev_30 <<- sqrt ( sq_ema_30 - ema_30 * ema_30 ); 
  stdev_60 <<- sqrt ( sq_ema_60 - ema_60 * ema_60 );
  stdev_120 <<- sqrt ( sq_ema_120 - ema_120 * ema_120 );

  ShouldBuySell ( );

  cat ( current_mid_price, ema_5, ema_10, ema_20, ema_60, ema_120, "\n", file = "~/dump_inds", sep = " ",  append=TRUE );

}

# Write the signal here
ShouldBuySell <- function ( ) {
  if ( ( ema_20 > ema_100 ) )
  {
    Trade ( 1 );
  }  

  if ( ( ema_20 < ema_100 ) )
  {
    Trade ( -1 );
  }  
}

Trade <- function ( buysell ) {

  if ( current_timestamp - last_trade_timestamp < cooloff )
  {
    return(0);
  }

  if ( pos >= max_pos && buysell > 0 ) 
  {
    return ( 0 );
  }

  if ( pos <= -max_pos && buysell < 0 )
  {
    return ( 0 );
  }

  if ( buysell < 0 )
  { 
    pnl <<- pnl - buysell * current_bid_price;    
  }
  else
  {
    pnl <<- pnl - buysell * current_ask_price;
  }     

  pos <<- pos + buysell;

  last_trade_timestamp <<- current_timestamp; 

  total_pnl <<- pnl + pos * current_mid_price;
  #cat ( "Buysell ", buysell, " " , total_pnl , "\n" );
}

last_trade_timestamp <- -1;

ema_5 <- 0;
ema_10 <- 0;
ema_15 <- 0;
ema_20 <- 0;
ema_30 <- 0;
ema_50 <- 0;
ema_60 <- 0;
ema_100 <- 0;
ema_120 <- 0;

sq_ema_5 <- 0;
sq_ema_10 <- 0;
sq_ema_15 <- 0;
sq_ema_20 <- 0;
sq_ema_30 <- 0;
sq_ema_50 <- 0;
sq_ema_60 <- 0;
sq_ema_100 <- 0;
sq_ema_120 <- 0;

stdev_15 <- 0;
stdev_30 <- 0;
stdev_60 <- 0;
stdev_120 <- 0;

positioning_1000 <- 0;
positioning_2000 <- 0;
positioning_5000 <- 0;
positioning_10000 <- 0;

args = commandArgs( trailingOnly=TRUE )
if ( length(args) < 1 ) {
        stop ("USAGE : <script> <data> \n");
}

datafilename <- args[1];
data <- as.matrix(read.table(datafilename));

last_timeout <- -1;
num_instances <- nrow(data);

current_timestamp <- 0;

for ( i in c(1:num_instances) )
{
  current_timestamp <- as.numeric(data[i,1]);
  event_type <- as.numeric(data[i,2]);
  
  if ( as.numeric(data[i,2]) == 0 )
  { 
    if ( last_timeout < 0 )
    {       
      last_timeout <- current_timestamp;
      OnMarketUpdate ( as.numeric(data[i,3]), as.numeric(data[i,4]) );
    }
 
    if ( current_timestamp - last_timeout > bar_duration )
    {
      last_timeout <- current_timestamp;
      OnMarketUpdate ( as.numeric(data[i,3]), as.numeric(data[i,4]));
    } 
  }

  if ( as.numeric(data[i,2]) < 0 )
  {
    OnTradePrint ( -1, as.numeric(data[i,3]), as.numeric(data[i,4]) );
  }

  if ( data[i,2] > 0 )
  {
    OnTradePrint ( 1, as.numeric(data[i,3]), as.numeric(data[i,4]) );
  }
}

cat ("total_pnl ", total_pnl, "\n" );
