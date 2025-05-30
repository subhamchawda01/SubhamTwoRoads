#! /usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);
if( length(args) != 5 ){
  stop ("USAGE: <pdt_shortcode> <strategy_name> <start_date> <end_date> <num_days_shift>\n");
}

pdt_shortcode = toString( args[1] );
strategy_name = toString( args[2] );
start_date = as.integer( args[3] );
end_date = as.integer( args[4] );
num_days_shift = as.integer( args[5] );

unique_id = system('date +%N', intern=TRUE);
system( sprintf("mkdir /spare/local/%s ", unique_id ) );
strat_file = sprintf("/spare/local/%s/strat_name", unique_id);
strat_pnl_file = sprintf("/spare/local/%s/strat_pnl", unique_id);
write( strategy_name, file = strat_file, append = FALSE );
system( sprintf( "~/basetrade_install/bin/summarize_strategy_results %s %s DB %s %s INVALIDFILE kCNAPnlSharpe 0 INVALIDFILE 0 | awk {'print $2'} > %s", pdt_shortcode, strat_file, start_date, end_date, strat_pnl_file ) );
strat_pnl = read.table( strat_pnl_file , header=TRUE);
rows = nrow(strat_pnl)-1;
strat_pnl =  strat_pnl[c(1:rows),] ;
moving_avg = array();
for( i in c(1:( rows - num_days_shift ) ) )  {
 moving_avg[i] = 0; 
 for ( j in c( i:( i + num_days_shift - 1 ) ) ) {
   moving_avg[i] = moving_avg[i] +  strat_pnl[j] / num_days_shift ;
 }
}
rows = length(strat_pnl);
strat_pnl = strat_pnl[ c( ( num_days_shift + 1 ) : rows ) ];
print(cor(moving_avg,strat_pnl));
system( sprintf("rm -rf /spare/local/%s", unique_id));
