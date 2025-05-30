#!/usr/bin/env Rscript
source("~/basetrade/scripts/sim_strategy_vix.R");

args <- commandArgs(trailingOnly = TRUE);

if ( length ( args ) < 12 )
{
  stop ( " Usage: <begin_date> <end_date> <ilist> <thres_factor> <begin_time> <end_time> <pca_component> <max_pos> <allowed_loss> <beging_date_test> <end_date_test>   \n " ) ;
}

begin_date <- args[1];
end_date <- args[2];
ilist <- args[3];
#out_sample_size <- as.numeric ( args[4] );
thres_factor<-as.numeric (args[4]);
begin_time = args[5];
end_time = args[6];
#in_sample_file = args[8];
#out_sample_file = args[9];
#out_samples = read.table( args[4] )$V1; 
component = as.numeric( args[7] ); 
#thres = as.numeric(args[11]);
max_pos = as.numeric(args[8]);
allowed_loss = as.numeric(args[9]);

begin_date_test = args[10];
end_date_test   = args[11];
max_single_pos = as.numeric( args[12] ) ;

dir = tempdir();
in_sample_file = tempfile(); 
out_sample_file = tempfile();
dir.create( dir, showWarnings = FALSE );

#system(paste("mkdir", dir,sep =" " ));
system(paste(">", in_sample_file,sep =" " ));
system(paste(">", out_sample_file,sep =" " ));


begin_date_r = as.Date(begin_date);
end_date_r = as.Date(end_date);

begin_date_r_test = as.Date(begin_date_test);
end_date_r_test = as.Date(end_date_test);

dates_range = seq( from = begin_date_r, to = end_date_r, by = "days" );
dates_range_test = seq( from = begin_date_r_test, to = end_date_r_test, by = "days" );
dates_range_str = c();
dates_range_str_test = c();

for ( i in 1:length(dates_range) )
{
temp = as.character( dates_range[i] );
temp_1 = strsplit( temp,split = "-"); 
temp_2 = paste ( temp_1[[1]][1], temp_1[[1]][2], temp_1[[1]][3], sep ="" );
dates_range_str[i] = temp_2;

}

for ( i in 1:length(dates_range_test) )
{
temp = as.character( dates_range_test[i] );
temp_1 = strsplit( temp,split = "-");
temp_2 = paste ( temp_1[[1]][1], temp_1[[1]][2], temp_1[[1]][3], sep ="" );
dates_range_str_test[i] = temp_2;

} 



file_names = c();
call_str = "/home/raghuram/basetrade_install/bin/datagen";

for ( i in 1:length(dates_range_str ) )
{
date = dates_range_str[i];
file_name = tempfile();
file_names[i] = file_name;

system(paste(">", file_name,sep =" " ));
call_str_2 =  sprintf ( "%s %s %s %s %s %s %s %s %s %s %s", call_str, ilist, date, begin_time, end_time,"22928" , file_name, "10000", "0", "0","0" );
#print (call_str_2 );
system(call_str_2 ,intern=TRUE);

}


file_names_test = c();
file_names_test_prices = c();
call_str = "/home/raghuram/basetrade_install/bin/datagen";
ilist_prices = "/home/raghuram/ilist_vix_simple_price";
for ( i in 1:length(dates_range_str_test ) )
{
date = dates_range_str_test[i];
file_name = tempfile();
file_names_test[i] = file_name;

file_name_prices = tempfile();
file_names_test_prices[i] = file_name_prices;

system(paste(">", file_name,sep =" " ));
call_str_2 =  sprintf ( "%s %s %s %s %s %s %s %s %s %s %s", call_str, ilist, date, begin_time, end_time,"22928" , file_name, "10000", "0", "0","0" );

system(paste(">", file_name_prices,sep =" " ));
call_str_3 =  sprintf ( "%s %s %s %s %s %s %s %s %s %s %s", call_str, ilist_prices, date, begin_time, end_time,"22928" , file_name_prices, "10000", "0", "0","0" );
#print (call_str_2 );
system(call_str_2 ,intern=TRUE);
system(call_str_3 ,intern=TRUE);
}




#out_samples = sample( 1:length( dates_range_str ), out_sample_size , replace = FALSE, prob = NULL);

for ( i in 1:length(dates_range_str ) )
{


call_str_2 = paste ( "cat", file_names[i], sep = " " );
call_str_2 = paste ( call_str_2 , ">>", in_sample_file, sep = " " ); 
system( call_str_2  ) ;


 

} 

in_sample_data = read.table( in_sample_file );
in_sample_data = in_sample_data [, 5:ncol(in_sample_data) ];
#in_sample_data = cumsum ( in_sample_data* in_sample_data/10000 ) ;
pca = princomp( in_sample_data  );

eigen_vec = pca$loadings;
eigen_vec;
summary(pca);


port_vec = get_intiger_pca_vector( prin_comp = eigen_vec, space_size = component - 1, max_pos = max_single_pos );
print("port_vec");
print(port_vec);
utc = 1000;
slippage = sum( abs(port_vec) )*0.03 * utc;
#slippage = 0; 
print("slippage");
print(slippage);

#port_vec = pca$loadings[,component ];
target_mean =  mean ( as.matrix( in_sample_data ) %*% port_vec );
sd = sd( as.matrix(in_sample_data ) %*% port_vec );
thres = sd * thres_factor ;
 
total_profit = 0;
total_days = 0;
total_drawdown = 0;
total_buys = 0;
all_test_data = data.frame();
all_signal_data = data.frame();
for ( i in 1:length( file_names_test) )
{

if (file.info(file_names_test[i] )$size != 0 )
{
 total_days = total_days + 1;

price_data_series = read.table( file_names_test_prices[i] )[, 5:12];
signal_data_series = read.table( file_names_test[i])[,5:12];
#price_data_series = cumsum ( price_data_series * price_data_series/10000 );
all_test_data = rbind ( all_test_data, price_data_series );
all_signal_data = rbind (all_signal_data, signal_data_series);

pnl = test_pca_strategy(signal_data_frame = signal_data_series, price_data_frame = price_data_series, port_vec = port_vec, target_level = target_mean, thres = thres, max_pos = max_pos, allowed_loss = allowed_loss );
total_profit = total_profit + pnl[[1]][ length( pnl[[1]] ) ] ;
total_drawdown = total_drawdown + pnl[[4]][1] ;
total_buys = total_buys + pnl[[5]][1] ;

print( dates_range_str_test[i]);

print( c ( pnl[[1]][ length( pnl[[1]] ) ], pnl[[4]][1], pnl[[5]][1] ) );
}

}

print("target mean")
print(target_mean);
print(" std dev");
print(sd); 
 
out_sample_comp = predict( pca, all_test_data  );
out_sample_sd = c();
for ( i in 1:ncol( price_data_series ) )
{
out_sample_sd[i] = sd ( out_sample_comp[,i] );
}
print("predicted pca std dev");
print ( out_sample_sd );

print( "predicted pca proportion of var");
print ( out_sample_sd * out_sample_sd / ( sum ( out_sample_sd * out_sample_sd ) ) );

print( "true pca" )
pca_test = princomp(all_test_data);
summary(pca_test);



print( c( "PCA componet mean" , target_mean ) );
print( c( "PCA component sd", sd )); 
print( c( "Total PNL", total_profit ) ) ;
print( c(  "AVG PNL", total_profit/total_days ) );
print( c( "AVG DRAW DOWN", total_drawdown/ total_days ) );
print( c( "AVG Transactions", total_buys/ total_days ) );

system(paste("rm", in_sample_file,sep =" " ));
system(paste("rm", out_sample_file,sep =" " ));
system(paste ("rm -rf, dir, sep = " " )) ;
