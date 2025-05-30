#!/usr/bin/env Rscript

#source("~/basetrade/scripts/get_kalma_beta_params.R");
source("~/basetrade/scripts/r_utils.R");

tag = "default "

args <- commandArgs(trailingOnly = TRUE);

if( length( args) >= 1 ) {
tag = args[1];
}

allowed_tags = c("AS", "EU", "US");

if( !( tag  %in% allowed_tags ) ) {
 print ( "Invalid Tag using default" ) ; 
 tag = "default";
}

file = "/spare/local/tradeinfo/kalman_beta/config_default";
out_file = "/spare/local/tradeinfo/OnlineInfo/online_beta_kalman.txt";

end_date =  Sys.Date()-1;
begin_date = end_date - 5;

end_date = get_str_date(end_date);
begin_date = get_str_date(begin_date);

begin_time = "UTC_130";
end_time = "UTC_2330";


if( tag == "AS" ) {
  file = "/spare/local/tradeinfo/kalman_beta/config_as";
  out_file = "/spare/local/tradeinfo/OnlineInfo/online_beta_kalman_as.txt";
  begin_time = "UTC_130";
  end_time = "UTC_530";
}

if( tag == "EU" ) {
  file = "/spare/local/tradeinfo/kalman_beta/config_eu";
  out_file = "/spare/local/tradeinfo/OnlineInfo/online_beta_kalman_eu.txt";
  begin_time = "UTC_530";
  end_time = "UTC_1130";
}

if( tag == "US" ) {
  file = "/spare/local/tradeinfo/kalman_beta/config_us";
  out_file = "/spare/local/tradeinfo/OnlineInfo/online_beta_kalman_us.txt";
  begin_time = "UTC_1130";
  end_time = "UTC_2330";
}

duration = 300 ;
price_type = "OfflineMixMMS";
smf = 1;


call_str_1 = "~/basetrade/scripts/get_kalma_beta_params.R";

con=file(file,open="r");

lines = readLines(con);

temp_file = tempfile( tmpdir = "/media/ephemeral2" );

for ( line in lines ) {

if( substr(line,start = 1,1) == "#" ) {next;}

tokens = strsplit( line, split = " " ) [[1]];
dep = tokens[1];
indep =  tokens[2];

call_str_2 = paste( call_str_1, dep, indep, duration, price_type, smf, begin_date, end_date, begin_time, end_time , sep =" " ) ;

out_put_str <- system( call_str_2, intern = TRUE );
print( out_put_str ) ;
#tokens = strsplit( out_put_str, split = " " ) [[1]];
#x = c( dep, indep, tokens[2], tokens[4], tokens[6] );
write(x = out_put_str, file = temp_file,append = TRUE,ncolumns = 1 );
}


system( paste ( "cp" , temp_file , out_file, sep = " " ) ) ; 
file.remove(temp_file);

