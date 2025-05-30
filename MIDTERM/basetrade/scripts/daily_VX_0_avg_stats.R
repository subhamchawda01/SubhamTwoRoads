#!/usr/bin/env Rscript

source("~/basetrade/scripts/r_utils.R")

end_date = Sys.Date()-1;
end_date = get_str_date(end_date);
begin_date = end_date;


args <- commandArgs(trailingOnly = TRUE);

if(length(args) >= 1 ) {
begin_date = args[1];
}

begin_time = "EST_915";
end_time = "EST_1600";
dep = "VX_0";
indep = "ES_0";
out_file = "/spare/local/tradeinfo/VIX/VX_0_stats";
 
is_date_present = function( out_file, date ){
 
  info = file.info(out_file);
  if( (info$size == 0 ) || (is.na(info$size) ) ){
    return (FALSE);
  }
  
  data = read.table(out_file);
  date = as.numeric(date);
  
  dates = data[,1];
  
  ind = match( date, dates);
  
  flag = FALSE;
  
  if(!is.na(ind)){
    flag = TRUE;
  }
  
  return(flag);
}



 stats <- get_product_stats ( dep, indep, begin_date, end_date, begin_time, end_time, " "  )
 write.table(x = stats,file = out_file,append = TRUE, row.names = FALSE, col.names = FALSE );

