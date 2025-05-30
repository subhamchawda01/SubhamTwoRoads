#filtering indicators which pass the cutoff and preparing them for mail
#### ========== Reading Parameters
args <- commandArgs(trailingOnly = TRUE);
cv_names = readLines(args[1]);
ind1_names = readLines(args[2]);
ind2_names = readLines(args[3]);
work_dir = toString(args[4]);
min_correlation_threshold = as.numeric(args[5]);
min_increase_threshold = as.numeric(args[6]);
running_date_ = toString(args[7]);
last_trading_date_ = toString(args[8]);
num_prev_days_ = toString(args[9]);
pred_duration_list = toString(args[10]);
pred_algo_list = toString(args[11]);
filter_algo_list = toString(args[12]);
user_correlation_sign = as.numeric(args[13]);

#### ========== Removing Indicator WEIGHT from the name
indicator_name <- function ( indicator_string ) {
  ind_words <- strsplit( indicator_string, " " )[[1]];
  ind_name <- ind_words[3];
  for( i in c( 4 : length( ind_words ) ) ) {
    ind_name <- paste( ind_name, ind_words[i], sep = ' ');
  }
  return( ind_name );
}
trim <- function (x) {
  return (gsub("^\\s+|\\s+$", "", x));
}

#### ========== Filtering files and initializing variables
ind1_names = trim (ind1_names);
ind2_names = trim (ind2_names);
ind1_names = grep("^INDICATOR.*$", ind1_names, value = TRUE);
ind1_names = sub("#.*", "", ind1_names);
ind2_names = grep("^INDICATOR.*$", ind2_names, value = TRUE);
ind2_names = sub("#.*", "", ind2_names);
cv_names = grep("^INDICATOR.*$", cv_names, value = TRUE);
cv_names = sub("#.*", "", cv_names);
combination_filename = paste(work_dir,"combination_indicators", sep='');
file.create(combination_filename);
combination_filename_local = paste(work_dir,"combination_indicators_dir", sep='');
file.create(combination_filename_local);
mail_body = paste( " ======= ", combination_filename_local, " =========\n", sep="");
combination_indicator = paste( " ======= ", combination_filename_local, " =========\n", sep="");
combination_indicator_local =  paste( " ======= ", combination_filename_local, " =========\n", sep="");
pred_duration_list = strsplit(pred_duration_list, ",")[[1]];
pred_algo_list = strsplit(pred_algo_list, ",")[[1]];
filter_algo_list = strsplit(filter_algo_list, ",")[[1]];

cv_names_filter = array();
ind1_names_filter = array();
ind2_names_filter = array();
#### ========== Reading all files and preparing data for mail, local indicator dump, global indicator dump
for ( pred_duration_ in pred_duration_list ) {
	for( pred_algo_ in pred_algo_list ) {
		for( filter_algo_ in filter_algo_list ) {
			file = paste("filtered_regdata_filename_", pred_duration_, "_", pred_algo_, "_", filter_algo_, sep="");
			file_ind1 = paste(work_dir, file, "_corr_ind1", sep ='');
			corr_ind1 = scan(file_ind1, what=double()); ### correlation of indicators in file 1
			file_ind2 = paste(work_dir, file, "_corr_ind2", sep='');
			corr_ind2 = scan(file=file_ind2, what=double()); ## correlation of indicators in file 2

			pattern_string = paste(file,"_cv_corr_*", sep=''); 
			filenames <- list.files( work_dir, pattern=pattern_string, full.names=TRUE); ### for a given pred dur, pred algo, filter algo find all publishable data
			start = paste("\n", "\n", "\n","*****************************************************************", "\n", running_date_, " ", last_trading_date_, " ", num_prev_days_, " ", pred_duration_, " ", pred_algo_, " ", filter_algo_, "\n", "*************************************************************", "\n", sep = ""); ## mail header line
			combination_indicator = paste(combination_indicator, start, sep = '');
			combination_indicator_local = paste(combination_indicator_local, start, sep = '');
			mail_body = paste(mail_body, start, sep = '');
			
			### Constructing mail body, local dump file, global dump file ####
			for( k in c( 1:length( filenames ) ) ){
				data = as.matrix( read.table( filenames[k], header= FALSE ) );
				max_cv = 0;
			 	ind_names = "1,1";	
				cv_names_filter[k] = indicator_name( cv_names[k] ); ### cleaning conditional variables for mail
				len_cv = length( strsplit( cv_names_filter[k], " ")[[1]] ) - 1;
				line1 = paste("Conditional Variable: ", toString(cv_names_filter[k]));
				mail_body = paste(mail_body, "----------------------------------------",  sep="\n");
				mail_body = paste(mail_body, line1, sep = '\n');
				mail_body = paste(mail_body, "----------------------------------------",  sep="\n");
				for(i in c(1:(nrow(data)))){		
					for(j in c((1):ncol(data))){
						attributes = array();
						params = data[i,j];
					 	attributes = strsplit( params,",")[[1]];
						ind1_names_filter[i] = indicator_name( ind1_names[i] );
						ind2_names_filter[j] = indicator_name( ind2_names[j] );
						len_ind1 = length( strsplit( ind1_names_filter[i], " ")[[1]] ) - 1;
						len_ind2 = length( strsplit( ind2_names_filter[j], " ")[[1]] ) - 1;
						ind_names = paste( toString(ind1_names_filter[i]),": ", corr_ind1[i], "\t\t", toString(ind2_names_filter[j]),": ", corr_ind2[j], sep="" );
            ind1_weight = 1; ind2_weight = 1;
            if( user_correlation_sign == 1 ){
              ind1_weight = as.integer(strsplit( ind1_names[i], " ")[[1]][2]);
              ind2_weight = as.integer(strsplit( ind2_names[j], " ")[[1]][2]);
            }
            else{
              if ( corr_ind1[i] >= 0 ){ ind1_weight = 1; } else { ind1_weight = -1 }
              if ( corr_ind2[j] >= 0 ){ ind2_weight = 1; } else { ind2_weight = -1 }
            }
            indicator_expression = paste("INDICATOR 1.00 Expression COMPOSITE ", len_cv, " 1.0 ", cv_names_filter[k], " ", len_ind1, " ", ind1_weight, " ", ind1_names_filter[i], " ", len_ind2 ," ", ind2_weight, " ", ind2_names_filter[j], " ", "PARAMS 6.0 ", toString(attributes[1]), " ", toString(attributes[2]), " ", toString(attributes[3])," ", toString(attributes[4]), " ", toString(attributes[5]), " ", toString(attributes[6]), " # I1C ", corr_ind1[i], " I2C ", corr_ind2[j], " CC ", attributes[8], " RI ", attributes[7], sep=""  ); ## lines for global dump
						indicator_expression_local = paste(indicator_expression, " a10p ", attributes[9], " a50p ", attributes[10], " a90p ", attributes[11], sep=""); ## extra details for local dump
						combination_indicator_local = paste(combination_indicator_local, indicator_expression_local, sep="\n");
						if(  abs(as.numeric(attributes[8])) >= min_correlation_threshold &&  as.numeric(attributes[7]) > min_increase_threshold  ){
							next_line = paste("  ", ind_names,"\tCOMBINATION: ",toString(attributes[8]) ,"\tINCREASE %: ", toString( as.numeric( attributes[7])*100  ), "\t alpha10p ", attributes[9], "\t alpha50p ", attributes[10], "\t alpha90p ", attributes[11] , sep=""); ## lines for mail
							mail_body = paste(mail_body, next_line,sep='\n');
							combination_indicator = paste(combination_indicator, indicator_expression, sep= "\n");
						}	
					}			
				}
			}
		}
	}
}
new_filename = paste(work_dir,"email_contents",sep='');
write.table(mail_body,new_filename, row.names=FALSE, col.names=FALSE);
write.table(combination_indicator, combination_filename, row.names=FALSE, col.names=FALSE);
write.table(combination_indicator_local, combination_filename_local, row.names=FALSE, col.names=FALSE);
