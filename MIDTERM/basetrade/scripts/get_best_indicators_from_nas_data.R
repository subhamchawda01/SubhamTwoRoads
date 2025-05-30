#!/usr/bin/env Rscript
.libPaths("/apps/R/root/library/")

args <- commandArgs(trailingOnly = TRUE);

if( length(args) < 2 )
{
stop( "Usage <file of data files> ");
}

file_name = args[1];
num_indicators = as.numeric( args[2]);

get_indicators = function( file, num_indicators )
{
  lines = c();
  
  #file = gzfile(file);
  con = file( file );
  lines = readLines(con = con );
  corr_data = c();
  
  for( i in 1:length(lines) )
  {
    corr_data[i] = abs(as.numeric(strsplit(lines[i],split =" " )[[1]][2])) ;
    
  }
  
  ranks = rank(corr_data);
  best_lines = c();
  
  for( i in 1:length(lines) )
  {
    if( ( length(lines) - ranks[i] ) < num_indicators  )
    {
      best_lines = c( best_lines, lines[i]);
    }
    
  }
  close(con);
   return(best_lines);
  
}

get_cut_lines = function( lines )
{
  cut_lines = c();
  
  for( i in 1:length(lines))
  {
    temp_line = substr( lines[i],start = 25, stop =nchar(lines[i]) );
    temp_line_2 = gsub("\\s*\\w*$", "", temp_line);
    temp_line_3 = gsub("\\s*\\w*$", "", temp_line_2);
    
    cut_lines[i] = temp_line_3;
        
  }
  
  return(cut_lines);
}

get_robust_indicators = function( files, num_indicators )
{
  best_lines = c()
  cut_lines = c(); 
  for( i in 1:length(files) )
  {
    if( i == 1 )
    {
      
      best_lines = get_indicators(files[i], num_indicators );
      cut_lines = get_cut_lines(best_lines);
      
    }
    if( i !=1 )
    {
      temp_lines =get_indicators(files[i], num_indicators ) ;
      temp_cut_lines = get_cut_lines(temp_lines);
      
      
      cut_lines = intersect(  cut_lines, temp_cut_lines );
      
    }
    
  }
  
  return(cut_lines);
  
}

conn = file(file_name);

lines = readLines(conn);

files = c();
 
for ( i in 1:length(lines))
{
temp_line = paste("/NAS1/indicatorwork/" ,lines[i], "/summary_indicator_corr_record_file.txt.gz",sep="");
files = c(files, temp_line);


}

get_robust_indicators(files, num_indicators );
