#!/usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE);

if ( length (args ) < 1 ) {
  stop( "Usage: <config_file> <force_compute = 0[0/1]>\n" ) ;

}

source("~/basetrade/scripts/indicators_tail_score_func.R")
source("~/basetrade/scripts/r_utils.R")

config_file= args[1];
force_compute = 0;
if ( length(args) > 1 ){
force_compute = as.numeric(args[2]);
}

#file = "/media/shared/ephemeral2/Tail_score/config_file"
temp_outfile = tempfile(tmpdir = "/media/ephemeral2" );
freq = 7;
today = Sys.Date() ;

is_date = function(str) {

 if(nchar(str) != 8 ) {
 return (FALSE);
 
 }else {

   year = substr(str,start = 1,stop = 4);
   month = substr(str,start = 5,stop = 6);
   date = substr(str,start = 7,stop = 8);
 
    if( is.na (year) || is.na (month) || is.na (date) ) {
    return(FALSE)
    }
    else {
    return (TRUE );
    }

  }


}


process_ilist = function( product, ilist, begin_time, end_time, begin_date, end_date, pred_dur, sd ) {


get_score = function(  )
{

        system( paste ( "> ", temp_outfile, sep ="") , intern = TRUE  ); 
        get_ilist_score ( ilist, begin_date,end_date,begin_time,end_time, pred_dur, temp_outfile, thres = 0,
                         sd_fac = sd, sampling_time = 1000, dir = "/media/ephemeral2/", delta_y_cutoff = 0.05, use_cor = 0);
      
          write(as.character (get_str_date(today)), out_file, append = FALSE );
          system( paste ("cat ", temp_outfile," >>" ,out_file, sep ="") , intern = TRUE );


}

ilist_name = strsplit(ilist, split = "/")[[1]];
ilist_name = ilist_name[length(ilist_name) ];

dir = paste("/media/shared/ephemeral2/Tail_score/",product,"/",sep="");
dir.create( dir, showWarnings = FALSE) ;

out_file = paste( ilist_name,begin_time,end_time,as.numeric(as.Date(get_r_date(end_date)) - as.Date(get_r_date(begin_date) )+1) ,pred_dur,sd, sep = "_" ) ;
out_file = paste( dir,out_file, sep = "" ) ;

if( force_compute != 0 ) {
get_score();
}
else {
if( !is.na( file.info(out_file)$size ) && ( file.info(out_file)$size > 0 ) ){

    last_updated_date =  system( paste("head -n 1 ", out_file, sep ="" ), intern = TRUE  ) ;  
    print( last_updated_date );
    if ( is_date ( last_updated_date ) ) {
    last_updated_date = as.Date(get_r_date(last_updated_date)  );
    print( last_updated_date );
    if( today - last_updated_date > 7 ) {
     get_score(); 
     }  
     }else {
     get_score(); 
     }   
}else{
     get_score();
   
}
}
}

run_config = function ( config ) {
  conn <- file( config, open="r")
  linn <-readLines(conn);
  close(conn);

  for( i in linn ) {
  if(substr( i,start=1,stop=1 ) !="#" ) {
  tokens = strsplit ( i, split = " " ) [[1]]; 
  end_date = get_str_date( today - 1 );
  begin_date = get_str_date ( today - as.numeric(tokens[5]) );

  process_ilist ( product = tokens[1],ilist = tokens[2], begin_time = tokens[3], end_time = tokens[4], begin_date =  begin_date, end_date = end_date, pred_dur = as.numeric(tokens[6]), sd = as.numeric(tokens[7] ) );
  }
  }


}


run_config(config_file);
file.remove(temp_outfile);
